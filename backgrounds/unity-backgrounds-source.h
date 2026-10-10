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
UnityBackgroundsSource *unity_backgrounds_source_new (void);

G_END_DECLS
