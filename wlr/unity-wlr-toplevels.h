/* unity-wlr-toplevels.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <astal-wl.h>
#include <gio/gio.h>
#include <glib-object.h>

struct wl_surface;

G_BEGIN_DECLS

#define UNITY_WLR_TYPE_TOPLEVEL  (unity_wlr_toplevel_get_type ())
#define UNITY_WLR_TYPE_TOPLEVELS (unity_wlr_toplevels_get_type ())

G_DECLARE_FINAL_TYPE (UnityWlrToplevel, unity_wlr_toplevel, UNITY_WLR, TOPLEVEL, GObject)

const gchar      *unity_wlr_toplevel_get_app_id     (UnityWlrToplevel *self);
const gchar      *unity_wlr_toplevel_get_title      (UnityWlrToplevel *self);
gboolean          unity_wlr_toplevel_get_activated  (UnityWlrToplevel *self);
gboolean          unity_wlr_toplevel_get_minimized  (UnityWlrToplevel *self);
gboolean          unity_wlr_toplevel_get_maximized  (UnityWlrToplevel *self);
gboolean          unity_wlr_toplevel_get_fullscreen (UnityWlrToplevel *self);
UnityWlrToplevel *unity_wlr_toplevel_get_parent     (UnityWlrToplevel *self);
GList            *unity_wlr_toplevel_get_outputs    (UnityWlrToplevel *self);

void unity_wlr_toplevel_activate      (UnityWlrToplevel  *self);
void unity_wlr_toplevel_close         (UnityWlrToplevel  *self);
void unity_wlr_toplevel_minimize      (UnityWlrToplevel  *self,
                                       gboolean           minimize);
void unity_wlr_toplevel_set_rectangle (UnityWlrToplevel  *self,
                                       struct wl_surface *surface,
                                       gint               x,
                                       gint               y,
                                       gint               width,
                                       gint               height);

G_DECLARE_FINAL_TYPE (UnityWlrToplevels, unity_wlr_toplevels, UNITY_WLR, TOPLEVELS, GObject)

UnityWlrToplevels *unity_wlr_toplevels_get_default (void);

G_END_DECLS
