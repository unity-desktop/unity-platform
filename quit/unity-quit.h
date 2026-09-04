/* unity-quit.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>
#include <glib-object.h>
#include <libdex.h>

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
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the call.
 */
DexFuture *unity_quit_suspend                    (UnityQuit *self);
/**
 * unity_quit_hibernate:
 * @self: a #UnityQuit
 *
 * Requests hibernation through logind.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the call.
 */
DexFuture *unity_quit_hibernate                  (UnityQuit *self);
/**
 * unity_quit_power_off:
 * @self: a #UnityQuit
 *
 * Requests power off through logind.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the call.
 */
DexFuture *unity_quit_power_off                  (UnityQuit *self);
/**
 * unity_quit_reboot:
 * @self: a #UnityQuit
 *
 * Requests reboot through logind.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the call.
 */
DexFuture *unity_quit_reboot                     (UnityQuit *self);
/**
 * unity_quit_reboot_to_firmware_setup:
 * @self: a #UnityQuit
 *
 * Requests reboot into firmware setup.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the reboot.
 */
DexFuture *unity_quit_reboot_to_firmware_setup   (UnityQuit *self);
/**
 * unity_quit_reboot_to_boot_loader_menu:
 * @self: a #UnityQuit
 *
 * Requests reboot into the boot loader menu.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the reboot.
 */
DexFuture *unity_quit_reboot_to_boot_loader_menu (UnityQuit *self);
/**
 * unity_quit_lock:
 * @self: a #UnityQuit
 *
 * Requests locking the current user session.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the call.
 */
DexFuture *unity_quit_lock                       (UnityQuit *self);
/**
 * unity_quit_logout:
 * @self: a #UnityQuit
 *
 * Requests termination of the current user session.
 *
 * Returns: (transfer full): a #DexFuture that resolves when logind acknowledges the call.
 */
DexFuture *unity_quit_logout                     (UnityQuit *self);

/**
 * unity_quit_refresh:
 * @self: a #UnityQuit
 *
 * Refreshes cached capability state from logind.
 *
 * Returns: (transfer full): a #DexFuture that resolves once every capability query completes.
 */
DexFuture *unity_quit_refresh                    (UnityQuit *self);

/**
 * unity_quit_inhibit:
 * @self: a #UnityQuit
 * @what: inhibitor target category
 * @who: requester identity
 * @why: reason text
 * @mode: inhibitor mode
 *
 * Requests a logind inhibitor lock.
 *
 * Returns: (transfer full): a #DexFuture that resolves to the inhibitor lock file descriptor.
 */
DexFuture *unity_quit_inhibit                    (UnityQuit   *self,
                                                  const gchar *what,
                                                  const gchar *who,
                                                  const gchar *why,
                                                  const gchar *mode);

G_END_DECLS
