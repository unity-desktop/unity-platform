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

/**
 * UnityQuit:
 *
 * System power/session integration object backed by logind.
 */
G_DECLARE_FINAL_TYPE (UnityQuit, unity_quit, UNITY, QUIT, GObject)

/**
 * unity_quit_get_default:
 *
 * Gets the shared #UnityQuit instance.
 *
 * Returns: (transfer none): the shared #UnityQuit.
 */
UnityQuit *unity_quit_get_default (void);

/**
 * unity_quit_suspend:
 * @self: a #UnityQuit
 *
 * Requests suspend through logind.
 */
void unity_quit_suspend                    (UnityQuit *self);
/**
 * unity_quit_hibernate:
 * @self: a #UnityQuit
 *
 * Requests hibernation through logind.
 */
void unity_quit_hibernate                  (UnityQuit *self);
/**
 * unity_quit_power_off:
 * @self: a #UnityQuit
 *
 * Requests power off through logind.
 */
void unity_quit_power_off                  (UnityQuit *self);
/**
 * unity_quit_reboot:
 * @self: a #UnityQuit
 *
 * Requests reboot through logind.
 */
void unity_quit_reboot                     (UnityQuit *self);
/**
 * unity_quit_reboot_to_firmware_setup:
 * @self: a #UnityQuit
 *
 * Requests reboot into firmware setup.
 */
void unity_quit_reboot_to_firmware_setup   (UnityQuit *self);
/**
 * unity_quit_reboot_to_boot_loader_menu:
 * @self: a #UnityQuit
 *
 * Requests reboot into the boot loader menu.
 */
void unity_quit_reboot_to_boot_loader_menu (UnityQuit *self);
/**
 * unity_quit_lock:
 * @self: a #UnityQuit
 *
 * Requests locking the current user session.
 */
void unity_quit_lock                       (UnityQuit *self);
/**
 * unity_quit_logout:
 * @self: a #UnityQuit
 *
 * Requests termination of the current user session.
 */
void unity_quit_logout                     (UnityQuit *self);

/**
 * unity_quit_refresh:
 * @self: a #UnityQuit
 *
 * Refreshes cached capability state from logind.
 */
void unity_quit_refresh                    (UnityQuit *self);
/**
 * unity_quit_flush:
 * @self: a #UnityQuit
 *
 * Blocks until pending asynchronous calls complete.
 */
void unity_quit_flush                      (UnityQuit *self);

/**
 * unity_quit_inhibit_async:
 * @self: a #UnityQuit
 * @what: inhibitor target category
 * @who: requester identity
 * @why: reason text
 * @mode: inhibitor mode
 * @cancellable: (nullable): optional cancellable
 * @callback: callback for completion
 * @user_data: user data for @callback
 *
 * Requests a logind inhibitor lock asynchronously.
 */
void unity_quit_inhibit_async              (UnityQuit           *self,
                                            const gchar         *what,
                                            const gchar         *who,
                                            const gchar         *why,
                                            const gchar         *mode,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data);
/**
 * unity_quit_inhibit_finish:
 * @self: a #UnityQuit
 * @result: result from #unity_quit_inhibit_async
 * @error: (nullable): return location for a #GError
 *
 * Finishes #unity_quit_inhibit_async.
 *
 * Returns: the inhibitor lock file descriptor, or -1 on error.
 */
gint unity_quit_inhibit_finish             (UnityQuit    *self,
                                            GAsyncResult *result,
                                            GError      **error);

G_END_DECLS
