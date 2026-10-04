/* unity-backgrounds-source.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_BACKGROUNDS_TYPE_SOURCE (unity_backgrounds_source_get_type ())

/**
 * UnityBackgroundsSource:
 *
 * A #GdkPaintable that draws the current desktop wallpaper.
 */
G_DECLARE_FINAL_TYPE (UnityBackgroundsSource, unity_backgrounds_source, UNITY_BACKGROUNDS, SOURCE, GObject)

/**
 * unity_backgrounds_source_new:
 *
 * Creates a source that follows the desktop wallpaper settings.
 *
 * Returns: (transfer full): the new source
 */
UnityBackgroundsSource *unity_backgrounds_source_new           (void);

/**
 * unity_backgrounds_source_has_wallpaper:
 * @self: a #UnityBackgroundsSource
 *
 * Checks whether the desktop shows a wallpaper.
 *
 * Returns: %TRUE unless the placement is `G_DESKTOP_BACKGROUND_STYLE_NONE`
 */
gboolean                unity_backgrounds_source_has_wallpaper (UnityBackgroundsSource *self);

/**
 * unity_backgrounds_source_save_png_async:
 * @self: a #UnityBackgroundsSource
 * @dests: (array zero-terminated=1) (element-type filename): output paths
 * @width: pixels
 * @height: pixels
 * @blur_radius: GSK blur radius, or 0 for no blur
 * @dim: overlay a 0.45-alpha black over the result
 * @cancellable: (nullable): a #GCancellable
 * @callback: (scope async): called when the write completes
 * @user_data: data for @callback
 *
 * Derives the PNG from the shared wallpaper render, then encodes and writes it
 * on a worker thread. Several calls share one render.
 */
void     unity_backgrounds_source_save_png_async  (UnityBackgroundsSource *self,
                                                   const gchar * const    *dests,
                                                   gint                    width,
                                                   gint                    height,
                                                   gdouble                 blur_radius,
                                                   gboolean                dim,
                                                   GCancellable           *cancellable,
                                                   GAsyncReadyCallback     callback,
                                                   gpointer                user_data);

/**
 * unity_backgrounds_source_save_png_finish:
 * @self: a #UnityBackgroundsSource
 * @result: a #GAsyncResult
 * @error: (nullable): return location for a #GError
 *
 * Finishes [method@BackgroundsSource.save_png_async].
 * Every path is tried, so %FALSE means one failed, not that none was written.
 *
 * Returns: %TRUE when every path was written.
 */
gboolean unity_backgrounds_source_save_png_finish (UnityBackgroundsSource *self,
                                                   GAsyncResult           *result,
                                                   GError                **error);

G_END_DECLS
