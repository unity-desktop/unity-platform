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

/**
 * UnityWlrToplevel:
 *
 * Representation of a remote wlroots toplevel window.
 */
G_DECLARE_FINAL_TYPE (UnityWlrToplevel, unity_wlr_toplevel, UNITY_WLR, TOPLEVEL, GObject)

/**
 * unity_wlr_toplevel_get_app_id:
 * @self: a #UnityWlrToplevel
 *
 * Gets the application ID reported by the toplevel.
 *
 * Returns: (nullable): the current app id string.
 */
const gchar      *unity_wlr_toplevel_get_app_id     (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_title:
 * @self: a #UnityWlrToplevel
 *
 * Gets the title reported by the toplevel.
 *
 * Returns: (nullable): the current title string.
 */
const gchar      *unity_wlr_toplevel_get_title      (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_activated:
 * @self: a #UnityWlrToplevel
 *
 * Gets whether the toplevel is currently activated.
 *
 * Returns: %TRUE if activated.
 */
gboolean          unity_wlr_toplevel_get_activated  (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_minimized:
 * @self: a #UnityWlrToplevel
 *
 * Gets whether the toplevel is currently minimized.
 *
 * Returns: %TRUE if minimized.
 */
gboolean          unity_wlr_toplevel_get_minimized  (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_maximized:
 * @self: a #UnityWlrToplevel
 *
 * Gets whether the toplevel is currently maximized.
 *
 * Returns: %TRUE if maximized.
 */
gboolean          unity_wlr_toplevel_get_maximized  (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_fullscreen:
 * @self: a #UnityWlrToplevel
 *
 * Gets whether the toplevel is currently fullscreen.
 *
 * Returns: %TRUE if fullscreen.
 */
gboolean          unity_wlr_toplevel_get_fullscreen (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_parent:
 * @self: a #UnityWlrToplevel
 *
 * Gets the parent toplevel, if one is set.
 *
 * Returns: (transfer none) (nullable): the parent toplevel.
 */
UnityWlrToplevel *unity_wlr_toplevel_get_parent     (UnityWlrToplevel *self);
/**
 * unity_wlr_toplevel_get_outputs:
 * @self: a #UnityWlrToplevel
 *
 * Gets outputs currently associated with the toplevel.
 *
 * Returns: (transfer none) (element-type AstalWl.Output): list of outputs.
 */
GList            *unity_wlr_toplevel_get_outputs    (UnityWlrToplevel *self);

/**
 * unity_wlr_toplevel_activate:
 * @self: a #UnityWlrToplevel
 *
 * Requests activation of this toplevel.
 */
void unity_wlr_toplevel_activate      (UnityWlrToplevel  *self);
/**
 * unity_wlr_toplevel_close:
 * @self: a #UnityWlrToplevel
 *
 * Requests that this toplevel be closed.
 */
void unity_wlr_toplevel_close         (UnityWlrToplevel  *self);
/**
 * unity_wlr_toplevel_minimize:
 * @self: a #UnityWlrToplevel
 * @minimize: %TRUE to minimize, %FALSE to unminimize
 *
 * Requests a minimize state change for this toplevel.
 */
void unity_wlr_toplevel_minimize      (UnityWlrToplevel  *self,
                                       gboolean           minimize);
/**
 * unity_wlr_toplevel_set_rectangle:
 * @self: a #UnityWlrToplevel
 * @surface: anchor #wl_surface
 * @x: x offset in surface coordinates
 * @y: y offset in surface coordinates
 * @width: rectangle width
 * @height: rectangle height
 *
 * Sets the rectangle used by the compositor for this toplevel.
 */
void unity_wlr_toplevel_set_rectangle (UnityWlrToplevel  *self,
                                       struct wl_surface *surface,
                                       gint               x,
                                       gint               y,
                                       gint               width,
                                       gint               height);

/**
 * UnityWlrToplevels:
 *
 * Singleton manager exposing wlroots toplevel handles.
 */
G_DECLARE_FINAL_TYPE (UnityWlrToplevels, unity_wlr_toplevels, UNITY_WLR, TOPLEVELS, GObject)

/**
 * unity_wlr_toplevels_get_default:
 *
 * Gets the shared #UnityWlrToplevels instance.
 *
 * Returns: (transfer none): the shared manager.
 */
UnityWlrToplevels *unity_wlr_toplevels_get_default (void);

G_END_DECLS
