/* unity-backgrounds-source.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-backgrounds-source.h"

#include <math.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API

#include <gdesktop-enums.h>
#include <gnome-bg/gnome-bg.h>
#include <png.h>

#define BACKGROUND_SCHEMA_ID "org.gnome.desktop.background"
#define INTERFACE_SCHEMA_ID  "org.gnome.desktop.interface"
#define PICTURE_URI_KEY      "picture-uri"
#define PICTURE_URI_DARK_KEY "picture-uri-dark"
#define PRIMARY_COLOR_KEY    "primary-color"
#define COLOR_SCHEME_KEY     "color-scheme"
#define DIM_ALPHA            0.45f
#define PNG_LEVEL            1

struct _UnityBackgroundsSource
{
  GObject      parent_instance;

  GnomeBG     *bg;
  GSettings   *background_settings;
  GSettings   *interface_settings;

  GdkTexture  *cache;
  gint         cache_width;
  gint         cache_height;

  GPtrArray   *pending_saves;

  GMutex       draw_lock;
  GCancellable *draw_cancellable;
  gboolean     drawing;
  gboolean     stale;

  GskRenderer *renderer;
};

typedef struct
{
  gint width;
  gint height;
} DrawOp;

typedef struct
{
  GStrv    dests;
  gint     width;
  gint     height;
  gdouble  blur_radius;
  gboolean dim;
  GBytes  *pixels;
  gsize    stride;
} SaveOp;

static void
save_op_free (SaveOp *op)
{
  g_clear_pointer (&op->dests, g_strfreev);
  g_clear_pointer (&op->pixels, g_bytes_unref);
  g_free (op);
}

static void paintable_iface_init (GdkPaintableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (UnityBackgroundsSource, unity_backgrounds_source, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE, paintable_iface_init))

static void
reload (UnityBackgroundsSource *self)
{
  gnome_bg_load_from_preferences (self->bg, self->background_settings);

  if (gnome_bg_get_placement (self->bg) == G_DESKTOP_BACKGROUND_STYLE_NONE)
    return;

  gboolean     dark = g_settings_get_enum (self->interface_settings, COLOR_SCHEME_KEY)
                      == G_DESKTOP_COLOR_SCHEME_PREFER_DARK;
  const gchar *key  = dark ? PICTURE_URI_DARK_KEY : PICTURE_URI_KEY;

  g_autofree gchar *uri  = g_settings_get_string (self->background_settings, key);
  g_autofree gchar *path = (uri != NULL && *uri != '\0')
                             ? g_filename_from_uri (uri, NULL, NULL) : NULL;
  if (path != NULL)
    gnome_bg_set_filename (self->bg, path);
}

static GdkTexture *
render_texture (UnityBackgroundsSource *self, gint width, gint height)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);

  g_mutex_lock (&self->draw_lock);
  gnome_bg_draw (self->bg, pixbuf);
  g_mutex_unlock (&self->draw_lock);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  g_autoptr (GBytes) bytes = g_bytes_new_with_free_func (
    gdk_pixbuf_get_pixels (pixbuf),
    (gsize) rowstride * height,
    g_object_unref, pixbuf);

  return gdk_memory_texture_new (width, height, GDK_MEMORY_R8G8B8, bytes, rowstride);
}

static void queue_draw (UnityBackgroundsSource *self, gint width, gint height);
static void drain_pending_saves (UnityBackgroundsSource *self);

static void
draw_worker (GTask        *task,
             gpointer      source_object,
             gpointer      task_data,
             GCancellable *cancellable)
{
  UnityBackgroundsSource *self = source_object;
  DrawOp                 *op   = task_data;

  g_task_return_pointer (task, render_texture (self, op->width, op->height), g_object_unref);
}

static void
on_drawn (GObject      *source_object,
          GAsyncResult *result,
          gpointer      user_data)
{
  UnityBackgroundsSource *self = UNITY_BACKGROUNDS_SOURCE (source_object);
  DrawOp *op = g_task_get_task_data (G_TASK (result));
  gint    width  = op->width;
  gint    height = op->height;

  g_autoptr (GError) error = NULL;
  g_autoptr (GdkTexture) texture = g_task_propagate_pointer (G_TASK (result), &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  self->drawing = FALSE;

  if (texture != NULL)
    {
      g_set_object (&self->cache, texture);
      self->cache_width  = width;
      self->cache_height = height;
      gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
    }

  if (self->stale)
    {
      queue_draw (self, width, height);
      return;
    }

  drain_pending_saves (self);
}

static void
queue_draw (UnityBackgroundsSource *self, gint width, gint height)
{
  if (self->drawing)
    {
      self->stale = TRUE;
      return;
    }

  self->drawing = TRUE;
  self->stale   = FALSE;

  DrawOp *op = g_new0 (DrawOp, 1);

  op->width  = width;
  op->height = height;

  g_autoptr (GTask) task = g_task_new (self, self->draw_cancellable, on_drawn, NULL);
  g_task_set_source_tag (task, queue_draw);
  g_task_set_task_data (task, op, g_free);
  g_task_run_in_thread (task, draw_worker);
}

static GdkTexture *
ensure_cache (UnityBackgroundsSource *self, gint width, gint height)
{
  if (self->stale
      || self->cache == NULL
      || self->cache_width  < width
      || self->cache_height < height)
    queue_draw (self, MAX (width, self->cache_width), MAX (height, self->cache_height));

  return self->cache;
}

gboolean
unity_backgrounds_source_has_wallpaper (UnityBackgroundsSource *self)
{
  g_return_val_if_fail (UNITY_BACKGROUNDS_IS_SOURCE (self), FALSE);
  return gnome_bg_get_placement (self->bg) != G_DESKTOP_BACKGROUND_STYLE_NONE;
}

static GskRenderer *
ensure_renderer (UnityBackgroundsSource *self)
{
  if (self->renderer != NULL)
    return self->renderer;

  GdkDisplay *display = gdk_display_get_default ();
  if (display != NULL)
    {
      GskRenderer *renderer = gsk_gl_renderer_new ();
      if (gsk_renderer_realize_for_display (renderer, display, NULL))
        {
          self->renderer = renderer;
          return renderer;
        }
      g_object_unref (renderer);
    }

  GskRenderer *renderer = gsk_cairo_renderer_new ();
  if (gsk_renderer_realize (renderer, NULL, NULL))
    {
      self->renderer = renderer;
      return renderer;
    }
  g_object_unref (renderer);
  return NULL;
}

static GdkTexture *
render_node_to_texture (UnityBackgroundsSource *self, GskRenderNode *node,
                        gint width, gint height)
{
  GskRenderer *renderer = ensure_renderer (self);
  if (renderer == NULL)
    return NULL;

  return gsk_renderer_render_texture (renderer, node,
                                      &GRAPHENE_RECT_INIT (0, 0, width, height));
}

static GdkTexture *
render_processed (UnityBackgroundsSource *self, GdkTexture *base, gint width, gint height,
                  gdouble blur_radius, gboolean dim)
{
  GskRenderer *renderer = ensure_renderer (self);
  if (renderer == NULL)
    return NULL;

  gint overshoot     = blur_radius > 0 ? (gint) ceil (blur_radius) : 0;
  gint render_width  = width + 2 * overshoot;
  gint render_height = height + 2 * overshoot;

  gfloat  base_width  = gdk_texture_get_width (base);
  gfloat  base_height = gdk_texture_get_height (base);
  gdouble scale       = MAX (render_width / base_width, render_height / base_height);
  gfloat  drawn_width  = base_width * scale;
  gfloat  drawn_height = base_height * scale;

  GtkSnapshot *snapshot = gtk_snapshot_new ();

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (snapshot,
                          &GRAPHENE_POINT_INIT ((gfloat) -overshoot, (gfloat) -overshoot));

  if (blur_radius > 0)
    gtk_snapshot_push_blur (snapshot, blur_radius);

  gtk_snapshot_append_scaled_texture (snapshot, base, GSK_SCALING_FILTER_LINEAR,
                                      &GRAPHENE_RECT_INIT ((render_width - drawn_width) / 2.0f,
                                                           (render_height - drawn_height) / 2.0f,
                                                           drawn_width, drawn_height));

  if (blur_radius > 0)
    gtk_snapshot_pop (snapshot);

  gtk_snapshot_restore (snapshot);

  if (dim)
    gtk_snapshot_append_color (snapshot,
                               &(GdkRGBA){ 0.0f, 0.0f, 0.0f, DIM_ALPHA },
                               &GRAPHENE_RECT_INIT (0, 0, width, height));

  g_autoptr (GskRenderNode) node = gtk_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return NULL;

  return render_node_to_texture (self, node, width, height);
}

static void
png_append (png_structp png, png_bytep data, png_size_t length)
{
  g_byte_array_append (png_get_io_ptr (png), data, length);
}

static GBytes *
encode_png (const guchar *pixels, gint width, gint height, gsize stride)
{
  g_autoptr (GByteArray) out = g_byte_array_new ();
  png_structp png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct (png);

  if (setjmp (png_jmpbuf (png)))
    {
      png_destroy_write_struct (&png, &info);
      return NULL;
    }

  png_set_write_fn (png, out, png_append, NULL);
  png_set_compression_level (png, PNG_LEVEL);
  png_set_filter (png, 0, PNG_FILTER_SUB);
  png_set_IHDR (png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info (png, info);

  for (gint y = 0; y < height; y++)
    png_write_row (png, (png_bytep) (pixels + (gsize) y * stride));

  png_write_end (png, NULL);
  png_destroy_write_struct (&png, &info);

  return g_byte_array_free_to_bytes (g_steal_pointer (&out));
}

static void
encode_worker (GTask        *task,
               gpointer      source_object,
               gpointer      task_data,
               GCancellable *cancellable)
{
  SaveOp *op = task_data;

  g_autoptr (GBytes) png = encode_png (g_bytes_get_data (op->pixels, NULL),
                                       op->width, op->height, op->stride);

  if (png == NULL)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to encode PNG");
      return;
    }

  gsize        length = 0;
  const gchar *data   = g_bytes_get_data (png, &length);
  g_autoptr (GError) first = NULL;

  for (gsize i = 0; op->dests[i] != NULL; i++)
    {
      g_autoptr (GError) local = NULL;

      if (g_file_set_contents_full (op->dests[i], data, length,
                                    G_FILE_SET_CONTENTS_CONSISTENT
                                      | G_FILE_SET_CONTENTS_DURABLE,
                                    0644, &local))
        continue;

      if (first == NULL)
        first = g_steal_pointer (&local);
    }

  if (first != NULL)
    g_task_return_error (task, g_steal_pointer (&first));
  else
    g_task_return_boolean (task, TRUE);
}

static void
process_save (UnityBackgroundsSource *self, GTask *task)
{
  SaveOp *op = g_task_get_task_data (task);

  g_autoptr (GdkTexture) texture =
    render_processed (self, self->cache, op->width, op->height, op->blur_radius, op->dim);

  if (texture == NULL)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to render wallpaper");
      return;
    }

  g_autoptr (GdkTextureDownloader) downloader = gdk_texture_downloader_new (texture);
  gdk_texture_downloader_set_format (downloader, GDK_MEMORY_R8G8B8);
  op->pixels = gdk_texture_downloader_download_bytes (downloader, &op->stride);

  g_task_run_in_thread (task, encode_worker);
}

static gboolean
cache_ready (UnityBackgroundsSource *self, gint width, gint height)
{
  return !self->stale
         && !self->drawing
         && self->cache != NULL
         && self->cache_width >= width
         && self->cache_height >= height;
}

static void
drain_pending_saves (UnityBackgroundsSource *self)
{
  if (self->pending_saves->len == 0)
    return;

  g_autoptr (GPtrArray) ready = g_ptr_array_new_with_free_func (g_object_unref);

  for (guint i = 0; i < self->pending_saves->len; i++)
    g_ptr_array_add (ready, g_object_ref (g_ptr_array_index (self->pending_saves, i)));

  g_ptr_array_set_size (self->pending_saves, 0);

  for (guint i = 0; i < ready->len; i++)
    process_save (self, g_ptr_array_index (ready, i));
}

void
unity_backgrounds_source_save_png_async (UnityBackgroundsSource *self,
                                         const gchar * const    *dests,
                                         gint                    width,
                                         gint                    height,
                                         gdouble                 blur_radius,
                                         gboolean                dim,
                                         GCancellable           *cancellable,
                                         GAsyncReadyCallback     callback,
                                         gpointer                user_data)
{
  g_return_if_fail (UNITY_BACKGROUNDS_IS_SOURCE (self));
  g_return_if_fail (dests != NULL && dests[0] != NULL);
  g_return_if_fail (width > 0 && height > 0);

  SaveOp *op = g_new0 (SaveOp, 1);

  op->dests       = g_strdupv ((gchar **) dests);
  op->width       = width;
  op->height      = height;
  op->blur_radius = blur_radius;
  op->dim         = dim;

  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, unity_backgrounds_source_save_png_async);
  g_task_set_task_data (task, op, (GDestroyNotify) save_op_free);

  ensure_cache (self, width, height);

  if (cache_ready (self, width, height))
    {
      process_save (self, task);
      return;
    }

  g_ptr_array_add (self->pending_saves, g_steal_pointer (&task));
}

gboolean
unity_backgrounds_source_save_png_finish (UnityBackgroundsSource *self,
                                          GAsyncResult           *result,
                                          GError                **error)
{
  g_return_val_if_fail (UNITY_BACKGROUNDS_IS_SOURCE (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
paintable_snapshot (GdkPaintable *paintable, GdkSnapshot *snapshot,
                    gdouble width, gdouble height)
{
  UnityBackgroundsSource *self = UNITY_BACKGROUNDS_SOURCE (paintable);

  gint w = (gint) width;
  gint h = (gint) height;
  if (w <= 0 || h <= 0)
    return;

  GdkTexture *texture = ensure_cache (self, w, h);

  if (texture == NULL)
    {
      g_autofree gchar *spec = g_settings_get_string (self->background_settings,
                                                      PRIMARY_COLOR_KEY);
      GdkRGBA colour;

      if (!gdk_rgba_parse (&colour, spec))
        colour = (GdkRGBA) { 0.16f, 0.16f, 0.16f, 1.0f };

      gtk_snapshot_append_color (GTK_SNAPSHOT (snapshot), &colour,
                                 &GRAPHENE_RECT_INIT (0, 0, (gfloat) width, (gfloat) height));
      return;
    }

  gtk_snapshot_append_scaled_texture (GTK_SNAPSHOT (snapshot), texture,
                                      GSK_SCALING_FILTER_LINEAR,
                                      &GRAPHENE_RECT_INIT (0, 0, (gfloat) width, (gfloat) height));
}

static GdkPaintableFlags
paintable_get_flags (GdkPaintable *paintable)
{
  return GDK_PAINTABLE_STATIC_SIZE;
}

static GdkPaintable *
paintable_get_current_image (GdkPaintable *paintable)
{
  UnityBackgroundsSource *self = UNITY_BACKGROUNDS_SOURCE (paintable);

  GdkTexture *texture = ensure_cache (self, self->cache_width, self->cache_height);

  if (texture == NULL)
    return gdk_paintable_new_empty (0, 0);

  return GDK_PAINTABLE (g_object_ref (texture));
}

static void
paintable_iface_init (GdkPaintableInterface *iface)
{
  iface->snapshot          = paintable_snapshot;
  iface->get_flags         = paintable_get_flags;
  iface->get_current_image = paintable_get_current_image;
}

static void
invalidate (UnityBackgroundsSource *self)
{
  self->stale = TRUE;

  if (self->cache_width > 0)
    queue_draw (self, self->cache_width, self->cache_height);

  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
}

static void
on_settings_changed (GSettings *settings, gchar *key, gpointer user_data)
{
  reload (user_data);
}

static void
on_bg_changed (GnomeBG *bg, gpointer user_data)
{
  invalidate (user_data);
}

static void
unity_backgrounds_source_dispose (GObject *object)
{
  UnityBackgroundsSource *self = UNITY_BACKGROUNDS_SOURCE (object);

  g_cancellable_cancel (self->draw_cancellable);
  g_clear_object (&self->draw_cancellable);

  if (self->renderer != NULL)
    {
      gsk_renderer_unrealize (self->renderer);
      g_clear_object (&self->renderer);
    }
  g_clear_object (&self->bg);
  g_clear_object (&self->background_settings);
  g_clear_object (&self->interface_settings);
  g_clear_object (&self->cache);
  g_clear_pointer (&self->pending_saves, g_ptr_array_unref);
  g_mutex_clear (&self->draw_lock);

  G_OBJECT_CLASS (unity_backgrounds_source_parent_class)->dispose (object);
}

static void
unity_backgrounds_source_class_init (UnityBackgroundsSourceClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = unity_backgrounds_source_dispose;
}

static void
unity_backgrounds_source_init (UnityBackgroundsSource *self)
{
  self->bg                  = gnome_bg_new ();
  self->background_settings = g_settings_new (BACKGROUND_SCHEMA_ID);
  self->interface_settings  = g_settings_new (INTERFACE_SCHEMA_ID);
  self->draw_cancellable    = g_cancellable_new ();
  self->pending_saves       = g_ptr_array_new_with_free_func (g_object_unref);

  g_mutex_init (&self->draw_lock);

  g_signal_connect (self->background_settings, "changed",
                    G_CALLBACK (on_settings_changed), self);
  g_signal_connect (self->interface_settings, "changed::" COLOR_SCHEME_KEY,
                    G_CALLBACK (on_settings_changed), self);
  g_signal_connect (self->bg, "changed",      G_CALLBACK (on_bg_changed), self);
  g_signal_connect (self->bg, "transitioned", G_CALLBACK (on_bg_changed), self);

  reload (self);
}

UnityBackgroundsSource *
unity_backgrounds_source_new (void)
{
  return g_object_new (UNITY_BACKGROUNDS_TYPE_SOURCE, NULL);
}
