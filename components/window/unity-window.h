/* unity-window.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_TYPE_WINDOW (unity_window_get_type ())

/**
 * UnityWindow:
 *
 * Fullscreen layer-shell window with the platform stylesheet loaded.
 */
G_DECLARE_DERIVABLE_TYPE (UnityWindow,
                          unity_window,
                          UNITY,
                          WINDOW,
                          AstalWindow)

struct _UnityWindowClass
{
  AstalWindowClass parent_class;

  gpointer padding[8];
};

/**
 * unity_window_compute_optimal_scale:
 * @monitor: (not nullable): the monitor to measure
 *
 * Returns Mutter's optimal fractional UI scale for @monitor.
 *
 * Returns: the scale, or 1.0 when the monitor has no usable size.
 */
gdouble unity_window_compute_optimal_scale (GdkMonitor *monitor);

/**
 * UnityWindowMonitorFunc:
 * @monitor: the monitor the window will bind to
 * @user_data: user data
 *
 * Builds a #UnityWindow for @monitor.
 *
 * Returns: (transfer full): the new window.
 */
typedef UnityWindow *(*UnityWindowMonitorFunc) (GdkMonitor *monitor,
                                                gpointer    user_data);

/**
 * unity_window_present_for_each_monitor:
 * @application: the owning application
 * @factory: window factory, called per monitor
 * @user_data: passed to @factory
 * @user_data_free: (nullable): frees @user_data on teardown
 *
 * Spawns one window per monitor via @factory and presents it. Adds
 * windows on monitor hotplug, destroys them on unplug.
 */
void unity_window_present_for_each_monitor (GtkApplication         *application,
                                            UnityWindowMonitorFunc  factory,
                                            gpointer                user_data,
                                            GDestroyNotify          user_data_free);

G_END_DECLS
