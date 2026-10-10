/* unity-backgrounds-source.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-backgrounds-source.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API

#include <gdesktop-enums.h>
#include <gnome-bg/gnome-bg.h>

#define BACKGROUND_SCHEMA_ID "org.gnome.desktop.background"
#define INTERFACE_SCHEMA_ID  "org.gnome.desktop.interface"
#define PICTURE_URI_KEY      "picture-uri"
#define PICTURE_URI_DARK_KEY "picture-uri-dark"
#define PRIMARY_COLOR_KEY    "primary-color"
#define COLOR_SCHEME_KEY     "color-scheme"

struct _UnityBackgroundsSource
{
  GObject      parent_instance;

  GnomeBG     *bg;
  GSettings   *background_settings;
  GSettings   *interface_settings;

  GdkTexture  *cache;
  gint         cache_width;
  gint         cache_height;

  GCancellable *draw_cancellable;
  gboolean     drawing;
  gboolean     stale;
};

typedef struct
{
  gint                      width;
  gint                      height;
  gchar                    *filename;
  GDesktopBackgroundStyle   placement;
  GDesktopBackgroundShading shading;
  GdkRGBA                   primary;
  GdkRGBA                   secondary;
} DrawOp;

static void
draw_op_free (DrawOp *op)
{
  g_free (op->filename);
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
render_texture (DrawOp *op)
{
  gint       width  = op->width;
  gint       height = op->height;
  GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);

  g_autoptr (GMainContext) context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  g_autoptr (GnomeBG) bg = gnome_bg_new ();
  gnome_bg_set_rgba (bg, op->shading, &op->primary, &op->secondary);
  gnome_bg_set_placement (bg, op->placement);
  gnome_bg_set_filename (bg, op->filename);
  gnome_bg_draw (bg, pixbuf);

  g_main_context_pop_thread_default (context);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  g_autoptr (GBytes) bytes = g_bytes_new_with_free_func (
    gdk_pixbuf_get_pixels (pixbuf),
    (gsize) rowstride * height,
    g_object_unref, pixbuf);

  return gdk_memory_texture_new (width, height, GDK_MEMORY_R8G8B8, bytes, rowstride);
}

static void queue_draw (UnityBackgroundsSource *self, gint width, gint height);

static void
draw_worker (GTask        *task,
             gpointer      source_object,
             gpointer      task_data,
             GCancellable *cancellable)
{
  g_task_return_pointer (task, render_texture (task_data), g_object_unref);
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

  self->drawing = FALSE;

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  if (texture != NULL)
    {
      g_set_object (&self->cache, texture);
      self->cache_width  = width;
      self->cache_height = height;
      gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
    }

  if (self->stale)
    queue_draw (self, width, height);
}

static void
queue_draw (UnityBackgroundsSource *self, gint width, gint height)
{
  if (width <= 0 || height <= 0)
    return;

  if (self->drawing)
    {
      self->stale = TRUE;
      return;
    }

  self->drawing = TRUE;
  self->stale   = FALSE;

  DrawOp *op = g_new0 (DrawOp, 1);

  op->width     = width;
  op->height    = height;
  op->filename  = g_strdup (gnome_bg_get_filename (self->bg));
  op->placement = gnome_bg_get_placement (self->bg);
  gnome_bg_get_rgba (self->bg, &op->shading, &op->primary, &op->secondary);

  g_autoptr (GTask) task = g_task_new (self, self->draw_cancellable, on_drawn, NULL);
  g_task_set_source_tag (task, queue_draw);
  g_task_set_task_data (task, op, (GDestroyNotify) draw_op_free);
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

  g_clear_object (&self->bg);
  g_clear_object (&self->background_settings);
  g_clear_object (&self->interface_settings);
  g_clear_object (&self->cache);

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
