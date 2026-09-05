/* unity-translucency.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * unity_translucency_init:
 *
 * Watches the `org.unity.stylesheets::translucency` GSettings key and attaches
 * or detaches a translucent stylesheet on the default #GdkDisplay accordingly.
 * Idempotent; call once at startup after the display is open.
 */
void unity_translucency_init (void);

G_END_DECLS
