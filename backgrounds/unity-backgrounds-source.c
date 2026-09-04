/* unity-backgrounds-source.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-backgrounds-source.h"

#include <math.h>

#include <gdesktop-enums.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <gnome-bg/gnome-bg.h>

#define BACKGROUND_SCHEMA_ID "org.gnome.desktop.background"
#define INTERFACE_SCHEMA_ID  "org.gnome.desktop.interface"
#define PICTURE_URI_KEY      "picture-uri"
#define PICTURE_URI_DARK_KEY "picture-uri-dark"
#define COLOR_SCHEME_KEY     "color-scheme"
#define DIM_ALPHA            0.45f

struct _UnityBackgroundsSource
{
  GObject      parent_instance;

  GnomeBG     *bg;
  GSettings   *background_settings;
  GSettings   *interface_settings;

  GdkTexture  *cache;
  gint         cache_width;
  gint         cache_height;

  GskRenderer *renderer;
};

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
  gnome_bg_draw (self->bg, pixbuf);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  g_autoptr (GBytes) bytes = g_bytes_new_with_free_func (
    gdk_pixbuf_get_pixels (pixbuf),
    (gsize) rowstride * height,
    g_object_unref, pixbuf);

  return gdk_memory_texture_new (width, height, GDK_MEMORY_R8G8B8, bytes, rowstride);
}

static GdkTexture *
ensure_cache (UnityBackgroundsSource *self, gint width, gint height)
{
  if (self->cache != NULL
      && self->cache_width  >= width
      && self->cache_height >= height)
    return self->cache;

  gint target_width  = MAX (width,  self->cache_width);
  gint target_height = MAX (height, self->cache_height);

  g_clear_object (&self->cache);
  self->cache        = render_texture (self, target_width, target_height);
  self->cache_width  = target_width;
  self->cache_height = target_height;
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
render_processed (UnityBackgroundsSource *self, gint width, gint height,
                  gdouble blur_radius, gboolean dim)
{
  gint overshoot     = blur_radius > 0 ? (gint) ceil (blur_radius) : 0;
  gint render_width  = width  + 2 * overshoot;
  gint render_height = height + 2 * overshoot;

  g_autoptr (GdkTexture) base = render_texture (self, render_width, render_height);
  if (base == NULL)
    return NULL;

  GtkSnapshot *snapshot = gtk_snapshot_new ();

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (snapshot,
                          &GRAPHENE_POINT_INIT ((gfloat) -overshoot, (gfloat) -overshoot));
  if (blur_radius > 0)
    gtk_snapshot_push_blur (snapshot, blur_radius);
  gtk_snapshot_append_texture (snapshot, base,
                               &GRAPHENE_RECT_INIT (0, 0, render_width, render_height));
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

gboolean
unity_backgrounds_source_save_png (UnityBackgroundsSource *self,
                                   const gchar            *dest,
                                   gint                    width,
                                   gint                    height,
                                   gdouble                 blur_radius,
                                   gboolean                dim,
                                   GError                **error)
{
  g_return_val_if_fail (UNITY_BACKGROUNDS_IS_SOURCE (self), FALSE);
  g_return_val_if_fail (dest != NULL, FALSE);
  g_return_val_if_fail (width > 0 && height > 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_autoptr (GdkTexture) texture = render_processed (self, width, height, blur_radius, dim);
  if (texture == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           "Failed to render wallpaper");
      return FALSE;
    }

  g_autoptr (GBytes) png = gdk_texture_save_to_png_bytes (texture);
  gsize len = 0;
  const gchar *data = g_bytes_get_data (png, &len);
  return g_file_set_contents_full (dest, data, len,
                                   G_FILE_SET_CONTENTS_CONSISTENT
                                     | G_FILE_SET_CONTENTS_DURABLE,
                                   0644, error);
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
    return;

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

  if (self->cache != NULL)
    return GDK_PAINTABLE (g_object_ref (self->cache));

  GdkTexture *texture = ensure_cache (self, 1920, 1080);
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
  g_clear_object (&self->cache);
  self->cache_width  = 0;
  self->cache_height = 0;
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

  if (self->renderer != NULL)
    {
      gsk_renderer_unrealize (self->renderer);
      g_clear_object (&self->renderer);
    }
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
