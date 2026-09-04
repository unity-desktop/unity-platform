/* unity-wlr-toplevel.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-wlr-toplevels.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL
UnityWlrToplevel *unity_wlr_toplevel_new (struct zwlr_foreign_toplevel_handle_v1 *handle);

G_END_DECLS
