/* unity-quit.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-quit.h"

#include <stdlib.h>
#include <unistd.h>

#include <gio/gunixfdlist.h>
#include <systemd/sd-login.h>

#define LOGIND_NAME  "org.freedesktop.login1"
#define LOGIND_PATH  "/org/freedesktop/login1"
#define LOGIND_IFACE "org.freedesktop.login1.Manager"

struct _UnityQuit
{
  GObject          parent_instance;

  GDBusConnection *bus;
  gchar           *session_id;

  gboolean         can_suspend;
  gboolean         can_hibernate;
  gboolean         can_power_off;
  gboolean         can_reboot;
  gboolean         can_reboot_to_firmware_setup;
  gboolean         can_reboot_to_boot_loader_menu;

  guint            prepare_for_sleep_sub;
};

G_DEFINE_FINAL_TYPE (UnityQuit, unity_quit, G_TYPE_OBJECT)

typedef enum
{
  PROP_CAN_SUSPEND = 1,
  PROP_CAN_HIBERNATE,
  PROP_CAN_POWER_OFF,
  PROP_CAN_REBOOT,
  PROP_CAN_REBOOT_TO_FIRMWARE_SETUP,
  PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU,
} UnityQuitProperty;

static GParamSpec *properties[PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU + 1];

typedef enum
{
  SIGNAL_PREPARE_FOR_SLEEP,
} UnityQuitSignal;

static guint signals[SIGNAL_PREPARE_FOR_SLEEP + 1];

typedef struct
{
  const gchar       *method;
  const gchar       *property_name;
  UnityQuitProperty  prop;
} Capability;

static const Capability caps[] = {
  { "CanSuspend",                "can-suspend",                    PROP_CAN_SUSPEND                    },
  { "CanHibernate",              "can-hibernate",                  PROP_CAN_HIBERNATE                  },
  { "CanPowerOff",               "can-power-off",                  PROP_CAN_POWER_OFF                  },
  { "CanReboot",                 "can-reboot",                     PROP_CAN_REBOOT                     },
  { "CanRebootToFirmwareSetup",  "can-reboot-to-firmware-setup",   PROP_CAN_REBOOT_TO_FIRMWARE_SETUP   },
  { "CanRebootToBootLoaderMenu", "can-reboot-to-boot-loader-menu", PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU },
};

static gboolean *
cap_field (UnityQuit *self, UnityQuitProperty prop)
{
  switch (prop)
    {
    case PROP_CAN_SUSPEND:                    return &self->can_suspend;
    case PROP_CAN_HIBERNATE:                  return &self->can_hibernate;
    case PROP_CAN_POWER_OFF:                  return &self->can_power_off;
    case PROP_CAN_REBOOT:                     return &self->can_reboot;
    case PROP_CAN_REBOOT_TO_FIRMWARE_SETUP:   return &self->can_reboot_to_firmware_setup;
    case PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU: return &self->can_reboot_to_boot_loader_menu;
    }
  g_return_val_if_reached (NULL);
}

static void
set_cap (UnityQuit *self, UnityQuitProperty prop, gboolean value)
{
  gboolean *field = cap_field (self, prop);

  if (*field == value)
    return;

  *field = value;
  g_object_notify_by_pspec (G_OBJECT (self), properties[prop]);
}

static gboolean
permitted (const gchar *result)
{
  return g_strcmp0 (result, "yes") == 0 || g_strcmp0 (result, "challenge") == 0;
}

static const gchar *
resolve_session_id (UnityQuit *self)
{
  if (self->session_id != NULL)
    return self->session_id;

  const gchar *env = g_getenv ("XDG_SESSION_ID");
  if (env != NULL && *env != '\0')
    {
      self->session_id = g_strdup (env);
      return self->session_id;
    }

  gchar *sid = NULL;
  gint   rc  = sd_uid_get_display (getuid (), &sid);
  if (rc < 0 || sid == NULL)
    {
      g_warning ("unity-quit: sd_uid_get_display: %s", g_strerror (-rc));
      free (sid);
      return NULL;
    }

  self->session_id = g_strdup (sid);
  free (sid);
  return self->session_id;
}

static DexFuture *
call_manager (UnityQuit          *self,
              const gchar        *method,
              GVariant           *params,
              const GVariantType *reply_type)
{
  if (self->bus == NULL)
    {
      if (params != NULL)
        g_variant_unref (g_variant_ref_sink (params));
      return dex_future_new_reject (G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
                                    "unity-quit: no system bus");
    }

  return dex_dbus_connection_call (
    self->bus, LOGIND_NAME, LOGIND_PATH, LOGIND_IFACE, method, params,
    reply_type, G_DBUS_CALL_FLAGS_NONE, -1);
}

static DexFuture *
then_reboot (DexFuture *previous, gpointer user_data)
{
  return call_manager (user_data, "Reboot", g_variant_new ("(b)", TRUE), NULL);
}

DexFuture *
unity_quit_suspend (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);
  return call_manager (self, "Suspend", g_variant_new ("(b)", TRUE), NULL);
}

DexFuture *
unity_quit_hibernate (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);
  return call_manager (self, "Hibernate", g_variant_new ("(b)", TRUE), NULL);
}

DexFuture *
unity_quit_power_off (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);
  return call_manager (self, "PowerOff", g_variant_new ("(b)", TRUE), NULL);
}

DexFuture *
unity_quit_reboot (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);
  return call_manager (self, "Reboot", g_variant_new ("(b)", TRUE), NULL);
}

DexFuture *
unity_quit_reboot_to_firmware_setup (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);
  DexFuture *set = call_manager (self, "SetRebootToFirmwareSetup",
                                 g_variant_new ("(b)", TRUE), NULL);
  return dex_future_then (set, then_reboot, g_object_ref (self), g_object_unref);
}

DexFuture *
unity_quit_reboot_to_boot_loader_menu (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);
  DexFuture *set = call_manager (self, "SetRebootToBootLoaderMenu",
                                 g_variant_new ("(t)", (guint64) 0), NULL);
  return dex_future_then (set, then_reboot, g_object_ref (self), g_object_unref);
}

DexFuture *
unity_quit_lock (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);

  const gchar *sid = resolve_session_id (self);
  if (sid == NULL)
    return dex_future_new_reject (G_IO_ERROR, G_IO_ERROR_FAILED,
                                  "unity-quit: could not resolve session id");
  return call_manager (self, "LockSession", g_variant_new ("(s)", sid), NULL);
}

DexFuture *
unity_quit_logout (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);

  const gchar *sid = resolve_session_id (self);
  if (sid == NULL)
    return dex_future_new_reject (G_IO_ERROR, G_IO_ERROR_FAILED,
                                  "unity-quit: could not resolve session id");
  return call_manager (self, "TerminateSession", g_variant_new ("(s)", sid), NULL);
}

static DexFuture *
apply_refresh (DexFuture *set_future, gpointer user_data)
{
  UnityQuit    *self = user_data;
  DexFutureSet *set  = DEX_FUTURE_SET (set_future);

  for (gsize i = 0; i < G_N_ELEMENTS (caps); i++)
    {
      g_autoptr (GError) error = NULL;
      const GValue *value = dex_future_set_get_value_at (set, i, &error);
      if (value == NULL)
        {
          g_warning ("unity-quit: %s failed: %s", caps[i].method, error->message);
          continue;
        }

      GVariant    *reply  = g_value_get_variant (value);
      const gchar *result = NULL;
      g_variant_get (reply, "(&s)", &result);

      set_cap (self, caps[i].prop, permitted (result));
    }

  return dex_future_new_true ();
}

DexFuture *
unity_quit_refresh (UnityQuit *self)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);

  DexFuture *futures[G_N_ELEMENTS (caps)];
  for (gsize i = 0; i < G_N_ELEMENTS (caps); i++)
    futures[i] = call_manager (self, caps[i].method, NULL, G_VARIANT_TYPE ("(s)"));

  DexFuture *set = dex_future_allv (futures, G_N_ELEMENTS (caps));
  return dex_future_then (set, apply_refresh, g_object_ref (self), g_object_unref);
}

static DexFuture *
extract_inhibit_fd (DexFuture *set_future, gpointer user_data)
{
  DexFutureSet *set = DEX_FUTURE_SET (set_future);

  g_autoptr (GError) error = NULL;
  const GValue *reply_value = dex_future_set_get_value_at (set, 0, &error);
  if (reply_value == NULL)
    return dex_future_new_for_error (g_steal_pointer (&error));

  const GValue *fd_list_value = dex_future_set_get_value_at (set, 1, &error);
  if (fd_list_value == NULL)
    return dex_future_new_for_error (g_steal_pointer (&error));

  GVariant    *reply   = g_value_get_variant (reply_value);
  GUnixFDList *fd_list = g_value_get_object (fd_list_value);

  gint32 index = -1;
  g_variant_get (reply, "(h)", &index);

  gint fd = g_unix_fd_list_get (fd_list, index, &error);
  if (fd < 0)
    return dex_future_new_for_error (g_steal_pointer (&error));

  return dex_future_new_for_fd (fd);
}

DexFuture *
unity_quit_inhibit (UnityQuit   *self,
                    const gchar *what,
                    const gchar *who,
                    const gchar *why,
                    const gchar *mode)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), NULL);

  if (self->bus == NULL)
    return dex_future_new_reject (G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
                                  "unity-quit: no system bus");

  DexFuture *set = dex_dbus_connection_call_with_unix_fd_list (
    self->bus, LOGIND_NAME, LOGIND_PATH, LOGIND_IFACE, "Inhibit",
    g_variant_new ("(ssss)", what, who, why, mode),
    G_VARIANT_TYPE ("(h)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL);

  return dex_future_then (set, extract_inhibit_fd, NULL, NULL);
}

static void
on_prepare_for_sleep (GDBusConnection *conn,
                      const gchar     *sender,
                      const gchar     *path,
                      const gchar     *iface,
                      const gchar     *signal_name,
                      GVariant        *params,
                      gpointer         user_data)
{
  UnityQuit *self  = user_data;
  gboolean   start = FALSE;

  g_variant_get (params, "(b)", &start);
  g_signal_emit (self, signals[SIGNAL_PREPARE_FOR_SLEEP], 0, start);
}

static void
unity_quit_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityQuit *self = UNITY_QUIT (object);

  g_value_set_boolean (value, *cap_field (self, (UnityQuitProperty) prop_id));
}

static void
unity_quit_dispose (GObject *object)
{
  UnityQuit *self = UNITY_QUIT (object);

  if (self->prepare_for_sleep_sub != 0 && self->bus != NULL)
    {
      g_dbus_connection_signal_unsubscribe (self->bus, self->prepare_for_sleep_sub);
      self->prepare_for_sleep_sub = 0;
    }

  g_clear_object (&self->bus);
  g_clear_pointer (&self->session_id, g_free);

  G_OBJECT_CLASS (unity_quit_parent_class)->dispose (object);
}

static void
unity_quit_class_init (UnityQuitClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = unity_quit_get_property;
  object_class->dispose      = unity_quit_dispose;

  /**
   * UnityQuit:can-suspend:
   *
   * Whether logind currently allows suspend.
   */
  /**
   * UnityQuit:can-hibernate:
   *
   * Whether logind currently allows hibernation.
   */
  /**
   * UnityQuit:can-power-off:
   *
   * Whether logind currently allows power off.
   */
  /**
   * UnityQuit:can-reboot:
   *
   * Whether logind currently allows reboot.
   */
  /**
   * UnityQuit:can-reboot-to-firmware-setup:
   *
   * Whether reboot-to-firmware-setup is currently available.
   */
  /**
   * UnityQuit:can-reboot-to-boot-loader-menu:
   *
   * Whether reboot-to-boot-loader-menu is currently available.
   */
  for (gsize i = 0; i < G_N_ELEMENTS (caps); i++)
    properties[caps[i].prop] = g_param_spec_boolean (
      caps[i].property_name, NULL, NULL, FALSE,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  /**
   * UnityQuit::prepare-for-sleep:
   * @self: the #UnityQuit instance
   * @start: %TRUE before suspend/hibernate, %FALSE after resume
   *
   * Emitted when logind announces a sleep transition.
   */
  signals[SIGNAL_PREPARE_FOR_SLEEP] = g_signal_new (
    "prepare-for-sleep", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
    0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
unity_quit_init (UnityQuit *self)
{
  g_autoptr (GError) error = NULL;

  dex_init ();

  self->bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (self->bus == NULL)
    {
      g_critical ("unity-quit: could not connect to system bus: %s", error->message);
      return;
    }

  self->prepare_for_sleep_sub = g_dbus_connection_signal_subscribe (
    self->bus, LOGIND_NAME, LOGIND_IFACE, "PrepareForSleep", LOGIND_PATH, NULL,
    G_DBUS_SIGNAL_FLAGS_NONE, on_prepare_for_sleep, self, NULL);

  dex_future_disown (unity_quit_refresh (self));
}

UnityQuit *
unity_quit_get_default (void)
{
  static UnityQuit *instance;

  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, g_object_new (UNITY_TYPE_QUIT, NULL));
  return instance;
}
