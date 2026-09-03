/* unity-strip.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_TYPE_STRIP (unity_strip_get_type ())

/**
 * UnityStrip:
 *
 * Layer-shell strip window widget used by Unity shell surfaces.
 */
G_DECLARE_DERIVABLE_TYPE (UnityStrip,
                          unity_strip,
                          UNITY,
                          STRIP,
                          AstalWindow)

struct _UnityStripClass
{
  AstalWindowClass parent_class;

  gpointer padding[8];
};

/**
 * unity_strip_get_color_scheme:
 * @self: a #UnityStrip
 *
 * Gets the current desktop color scheme from interface settings.
 *
 * Returns: the current #GDesktopColorScheme value as an integer.
 */
gint unity_strip_get_color_scheme (UnityStrip *self);

/**
 * unity_strip_anchor_for_edge:
 * @edge: a #GtkPositionType edge
 *
 * Converts a strip edge into an #AstalWindowAnchor mask.
 *
 * Returns: anchor flags for @edge.
 */
AstalWindowAnchor unity_strip_anchor_for_edge (GtkPositionType edge);

G_END_DECLS
