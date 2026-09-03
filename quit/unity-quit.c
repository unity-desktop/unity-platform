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
  guint            pending;
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

static const struct
{
  const gchar       *method;
  UnityQuitProperty  prop;
  gsize              offset;
} caps[] = {
  { "CanSuspend",                    PROP_CAN_SUSPEND,                    G_STRUCT_OFFSET (struct _UnityQuit, can_suspend)                    },
  { "CanHibernate",                  PROP_CAN_HIBERNATE,                  G_STRUCT_OFFSET (struct _UnityQuit, can_hibernate)                  },
  { "CanPowerOff",                   PROP_CAN_POWER_OFF,                  G_STRUCT_OFFSET (struct _UnityQuit, can_power_off)                  },
  { "CanReboot",                     PROP_CAN_REBOOT,                     G_STRUCT_OFFSET (struct _UnityQuit, can_reboot)                     },
  { "CanRebootToFirmwareSetup",      PROP_CAN_REBOOT_TO_FIRMWARE_SETUP,   G_STRUCT_OFFSET (struct _UnityQuit, can_reboot_to_firmware_setup)   },
  { "CanRebootToBootLoaderMenu",     PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU, G_STRUCT_OFFSET (struct _UnityQuit, can_reboot_to_boot_loader_menu) },
};

static gboolean
permitted (const gchar *result)
{
  return g_strcmp0 (result, "yes") == 0 || g_strcmp0 (result, "challenge") == 0;
}

typedef struct
{
  UnityQuit   *self;
  const gchar *method;
} CallCtx;

static gboolean
try_apply_cap_reply (UnityQuit *self, const gchar *method, GVariant *reply)
{
  for (gsize i = 0; i < G_N_ELEMENTS (caps); i++)
    {
      if (g_strcmp0 (caps[i].method, method) != 0)
        continue;

      if (reply != NULL)
        {
          const gchar *result = NULL;
          g_variant_get (reply, "(&s)", &result);
          gboolean  value = permitted (result);
          gboolean *field = G_STRUCT_MEMBER_P (self, caps[i].offset);
          if (*field != value)
            {
              *field = value;
              g_object_notify_by_pspec (G_OBJECT (self), properties[caps[i].prop]);
            }
        }
      return TRUE;
    }
  return FALSE;
}

static void
on_call_reply (GObject *source, GAsyncResult *res, gpointer user_data)
{
  CallCtx             *ctx   = user_data;
  g_autoptr (GError)   error = NULL;
  g_autoptr (GVariant) reply = g_dbus_connection_call_finish (
    G_DBUS_CONNECTION (source), res, &error);

  gboolean was_cap = try_apply_cap_reply (ctx->self, ctx->method, reply);

  if (reply == NULL && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    g_warning ("unity-quit: %s failed: %s", ctx->method, error->message);
  (void) was_cap;

  g_assert (ctx->self->pending > 0);
  ctx->self->pending--;
  g_object_unref (ctx->self);
  g_free (ctx);
}

static void
call_manager (UnityQuit *self, const gchar *method,
              GVariant *params, const GVariantType *reply_type)
{
  if (self->bus == NULL)
    {
      if (params != NULL)
        g_variant_unref (g_variant_ref_sink (params));
      return;
    }

  CallCtx *ctx = g_new0 (CallCtx, 1);
  ctx->self   = g_object_ref (self);
  ctx->method = method;

  self->pending++;
  g_dbus_connection_call (
    self->bus, LOGIND_NAME, LOGIND_PATH, LOGIND_IFACE, method, params,
    reply_type, G_DBUS_CALL_FLAGS_NONE, -1,
    NULL, on_call_reply, ctx);
}

void
unity_quit_refresh (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  if (self->bus == NULL)
    return;

  for (gsize i = 0; i < G_N_ELEMENTS (caps); i++)
    call_manager (self, caps[i].method, NULL, G_VARIANT_TYPE ("(s)"));
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
  gint   rc  = sd_pid_get_session (0, &sid);
  if (rc < 0 || sid == NULL)
    {
      g_warning ("unity-quit: sd_pid_get_session: %s", g_strerror (-rc));
      free (sid);
      return NULL;
    }

  self->session_id = g_strdup (sid);
  free (sid);
  return self->session_id;
}

void
unity_quit_flush (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));

  GMainContext *ctx = g_main_context_default ();
  while (self->pending > 0)
    g_main_context_iteration (ctx, TRUE);
}

void
unity_quit_suspend (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  call_manager (self, "Suspend", g_variant_new ("(b)", TRUE), NULL);
}

void
unity_quit_hibernate (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  call_manager (self, "Hibernate", g_variant_new ("(b)", TRUE), NULL);
}

void
unity_quit_power_off (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  call_manager (self, "PowerOff", g_variant_new ("(b)", TRUE), NULL);
}

void
unity_quit_reboot (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  call_manager (self, "Reboot", g_variant_new ("(b)", TRUE), NULL);
}

void
unity_quit_reboot_to_firmware_setup (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  call_manager (self, "SetRebootToFirmwareSetup", g_variant_new ("(b)", TRUE), NULL);
  call_manager (self, "Reboot",                   g_variant_new ("(b)", TRUE), NULL);
}

void
unity_quit_reboot_to_boot_loader_menu (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));
  call_manager (self, "SetRebootToBootLoaderMenu", g_variant_new ("(t)", (guint64) 0), NULL);
  call_manager (self, "Reboot",                    g_variant_new ("(b)", TRUE),        NULL);
}

void
unity_quit_lock (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));

  const gchar *sid = resolve_session_id (self);
  if (sid != NULL)
    call_manager (self, "LockSession", g_variant_new ("(s)", sid), NULL);
}

void
unity_quit_logout (UnityQuit *self)
{
  g_return_if_fail (UNITY_IS_QUIT (self));

  const gchar *sid = resolve_session_id (self);
  if (sid != NULL)
    call_manager (self, "TerminateSession", g_variant_new ("(s)", sid), NULL);
}

static void
on_inhibit_reply (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GTask)       task    = user_data;
  g_autoptr (GError)      error   = NULL;
  g_autoptr (GUnixFDList) fd_list = NULL;
  g_autoptr (GVariant)    reply   = g_dbus_connection_call_with_unix_fd_list_finish (
    G_DBUS_CONNECTION (source), &fd_list, res, &error);

  if (reply == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  gint32 index = -1;
  g_variant_get (reply, "(h)", &index);

  gint fd = g_unix_fd_list_get (fd_list, index, &error);
  if (fd < 0)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  g_task_return_int (task, fd);
}

void
unity_quit_inhibit_async (UnityQuit           *self,
                          const gchar         *what,
                          const gchar         *who,
                          const gchar         *why,
                          const gchar         *mode,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  g_return_if_fail (UNITY_IS_QUIT (self));

  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, unity_quit_inhibit_async);

  if (self->bus == NULL)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
                               "unity-quit: no system bus");
      return;
    }

  g_dbus_connection_call_with_unix_fd_list (
    self->bus, LOGIND_NAME, LOGIND_PATH, LOGIND_IFACE, "Inhibit",
    g_variant_new ("(ssss)", what, who, why, mode),
    G_VARIANT_TYPE ("(h)"), G_DBUS_CALL_FLAGS_NONE, -1,
    NULL, cancellable, on_inhibit_reply, g_steal_pointer (&task));
}

gint
unity_quit_inhibit_finish (UnityQuit    *self,
                           GAsyncResult *result,
                           GError      **error)
{
  g_return_val_if_fail (UNITY_IS_QUIT (self), -1);
  g_return_val_if_fail (g_task_is_valid (result, self), -1);

  return g_task_propagate_int (G_TASK (result), error);
}

static void
on_prepare_for_sleep (GDBusConnection *conn, const gchar *sender,
                      const gchar *path, const gchar *iface,
                      const gchar *signal_name, GVariant *params, gpointer user_data)
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

  switch ((UnityQuitProperty) prop_id)
    {
    case PROP_CAN_SUSPEND:                    g_value_set_boolean (value, self->can_suspend);                    break;
    case PROP_CAN_HIBERNATE:                  g_value_set_boolean (value, self->can_hibernate);                  break;
    case PROP_CAN_POWER_OFF:                  g_value_set_boolean (value, self->can_power_off);                  break;
    case PROP_CAN_REBOOT:                     g_value_set_boolean (value, self->can_reboot);                     break;
    case PROP_CAN_REBOOT_TO_FIRMWARE_SETUP:   g_value_set_boolean (value, self->can_reboot_to_firmware_setup);   break;
    case PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU: g_value_set_boolean (value, self->can_reboot_to_boot_loader_menu); break;
    }
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

  static const struct {
    UnityQuitProperty  prop;
    const gchar       *name;
  } pspecs[] = {
    { PROP_CAN_SUSPEND,                    "can-suspend"                    },
    { PROP_CAN_HIBERNATE,                  "can-hibernate"                  },
    { PROP_CAN_POWER_OFF,                  "can-power-off"                  },
    { PROP_CAN_REBOOT,                     "can-reboot"                     },
    { PROP_CAN_REBOOT_TO_FIRMWARE_SETUP,   "can-reboot-to-firmware-setup"   },
    { PROP_CAN_REBOOT_TO_BOOT_LOADER_MENU, "can-reboot-to-boot-loader-menu" },
  };
  for (gsize i = 0; i < G_N_ELEMENTS (pspecs); i++)
    properties[pspecs[i].prop] = g_param_spec_boolean (
      pspecs[i].name, NULL, NULL, FALSE,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  signals[SIGNAL_PREPARE_FOR_SLEEP] = g_signal_new (
    "prepare-for-sleep", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
    0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
unity_quit_init (UnityQuit *self)
{
  g_autoptr (GError) error = NULL;

  self->bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (self->bus == NULL)
    {
      g_critical ("unity-quit: could not connect to system bus: %s", error->message);
      return;
    }

  self->prepare_for_sleep_sub = g_dbus_connection_signal_subscribe (
    self->bus, LOGIND_NAME, LOGIND_IFACE, "PrepareForSleep", LOGIND_PATH, NULL,
    G_DBUS_SIGNAL_FLAGS_NONE, on_prepare_for_sleep, self, NULL);

  unity_quit_refresh (self);
}

UnityQuit *
unity_quit_get_default (void)
{
  static UnityQuit *instance;
  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, g_object_new (UNITY_TYPE_QUIT, NULL));
  return instance;
}
