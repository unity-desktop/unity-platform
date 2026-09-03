/* platform-style-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

void unity_platform_style_ensure     (GdkDisplay  *display);
void unity_platform_load_stylesheet  (GdkDisplay  *display,
                                      const gchar *resource_path);

G_END_DECLS
