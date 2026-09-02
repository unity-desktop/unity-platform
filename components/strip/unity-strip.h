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

gint unity_strip_get_color_scheme (UnityStrip *self);

AstalWindowAnchor unity_strip_anchor_for_edge (GtkPositionType edge);

G_END_DECLS
