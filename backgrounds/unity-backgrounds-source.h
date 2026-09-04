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
 * Returns: (transfer full): a new source following the desktop wallpaper settings.
 */
UnityBackgroundsSource *unity_backgrounds_source_new           (void);

/**
 * unity_backgrounds_source_has_wallpaper:
 * @self: a #UnityBackgroundsSource
 *
 * Returns: %TRUE unless the placement is `G_DESKTOP_BACKGROUND_STYLE_NONE`.
 */
gboolean                unity_backgrounds_source_has_wallpaper (UnityBackgroundsSource *self);

/**
 * unity_backgrounds_source_save_png:
 * @self: a #UnityBackgroundsSource
 * @dest: absolute output path
 * @width: pixels
 * @height: pixels
 * @blur_radius: GSK blur radius, or 0 for no blur
 * @dim: overlay a 0.45-alpha black over the result
 * @error: (nullable): return location for a #GError
 *
 * Renders the wallpaper and writes it atomically as a PNG.
 *
 * Returns: %TRUE on success.
 */
gboolean                unity_backgrounds_source_save_png      (UnityBackgroundsSource *self,
                                                                const gchar            *dest,
                                                                gint                    width,
                                                                gint                    height,
                                                                gdouble                 blur_radius,
                                                                gboolean                dim,
                                                                GError                **error);

G_END_DECLS
