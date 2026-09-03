/* unity-quit.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define UNITY_TYPE_QUIT (unity_quit_get_type ())

G_DECLARE_FINAL_TYPE (UnityQuit, unity_quit, UNITY, QUIT, GObject)

UnityQuit *unity_quit_get_default (void);

void unity_quit_suspend                    (UnityQuit *self);
void unity_quit_hibernate                  (UnityQuit *self);
void unity_quit_power_off                  (UnityQuit *self);
void unity_quit_reboot                     (UnityQuit *self);
void unity_quit_reboot_to_firmware_setup   (UnityQuit *self);
void unity_quit_reboot_to_boot_loader_menu (UnityQuit *self);
void unity_quit_lock                       (UnityQuit *self);
void unity_quit_logout                     (UnityQuit *self);

void unity_quit_refresh                    (UnityQuit *self);
void unity_quit_flush                      (UnityQuit *self);

void unity_quit_inhibit_async              (UnityQuit           *self,
                                            const gchar         *what,
                                            const gchar         *who,
                                            const gchar         *why,
                                            const gchar         *mode,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data);
gint unity_quit_inhibit_finish             (UnityQuit    *self,
                                            GAsyncResult *result,
                                            GError      **error);

G_END_DECLS
