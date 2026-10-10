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
UnityBackgroundsSource *unity_backgrounds_source_new         (void);

/**
 * unity_backgrounds_source_get_texture:
 * @self: a #UnityBackgroundsSource
 *
 * Gets the latest drawn wallpaper. [property@Source:texture] changes only when
 * a new wallpaper is drawn.
 *
 * Returns: (transfer none) (nullable): the wallpaper, or %NULL before the
 *   first draw
 */
GdkTexture             *unity_backgrounds_source_get_texture (UnityBackgroundsSource *self);

G_END_DECLS
