/* unity-wlr-output-power.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-wlr-output-power.h"

#include <wayland-client.h>

#include "wlr-output-power-management-unstable-v1.h"

#define MANAGER_INTERFACE "zwlr_output_power_manager_v1"

G_DEFINE_ENUM_TYPE (UnityWlrOutputPowerMode, unity_wlr_output_power_mode,
                    G_DEFINE_ENUM_VALUE (UNITY_WLR_OUTPUT_POWER_MODE_OFF, "off"),
                    G_DEFINE_ENUM_VALUE (UNITY_WLR_OUTPUT_POWER_MODE_ON,  "on"))

typedef enum
{
  OP_PROP_OUTPUT = 1,
  OP_PROP_MODE,
} UnityWlrOutputPowerProperty;

static GParamSpec *op_properties[OP_PROP_MODE + 1];

typedef enum
{
  OP_SIGNAL_FAILED,
} UnityWlrOutputPowerSignal;

static guint op_signals[OP_SIGNAL_FAILED + 1];

struct _UnityWlrOutputPower
{
  GObject                       parent_instance;

  struct zwlr_output_power_v1  *handle;
  GWeakRef                      output;
  UnityWlrOutputPowerMode       mode;
};

G_DEFINE_FINAL_TYPE (UnityWlrOutputPower, unity_wlr_output_power, G_TYPE_OBJECT)

AstalWlOutput *
unity_wlr_output_power_get_output (UnityWlrOutputPower *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_OUTPUT_POWER (self), NULL);
  return g_weak_ref_get (&self->output);
}

UnityWlrOutputPowerMode
unity_wlr_output_power_get_mode (UnityWlrOutputPower *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_OUTPUT_POWER (self), UNITY_WLR_OUTPUT_POWER_MODE_OFF);
  return self->mode;
}

void
unity_wlr_output_power_request_mode (UnityWlrOutputPower     *self,
                                     UnityWlrOutputPowerMode  mode)
{
  g_return_if_fail (UNITY_WLR_IS_OUTPUT_POWER (self));
  if (self->handle == NULL)
    return;
  zwlr_output_power_v1_set_mode (self->handle, (enum zwlr_output_power_v1_mode) mode);
}

static void
handle_mode (gpointer user_data, struct zwlr_output_power_v1 *h, guint32 mode)
{
  UnityWlrOutputPower *self = user_data;
  UnityWlrOutputPowerMode value = (UnityWlrOutputPowerMode) mode;

  if (self->mode == value)
    return;

  self->mode = value;
  g_object_notify_by_pspec (G_OBJECT (self), op_properties[OP_PROP_MODE]);
}

static void
handle_failed (gpointer user_data, struct zwlr_output_power_v1 *h)
{
  UnityWlrOutputPower *self = user_data;

  g_clear_pointer (&self->handle, zwlr_output_power_v1_destroy);
  g_signal_emit (self, op_signals[OP_SIGNAL_FAILED], 0);
}

static const struct zwlr_output_power_v1_listener output_power_listener = {
  handle_mode,
  handle_failed,
};

static UnityWlrOutputPower *
unity_wlr_output_power_new (struct zwlr_output_power_v1 *handle, AstalWlOutput *output)
{
  UnityWlrOutputPower *self = g_object_new (UNITY_WLR_TYPE_OUTPUT_POWER, NULL);

  self->handle = handle;
  g_weak_ref_set (&self->output, output);
  zwlr_output_power_v1_add_listener (handle, &output_power_listener, self);
  return self;
}

static void
unity_wlr_output_power_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityWlrOutputPower *self = UNITY_WLR_OUTPUT_POWER (object);

  switch ((UnityWlrOutputPowerProperty) prop_id)
    {
    case OP_PROP_OUTPUT:
      g_value_take_object (value, g_weak_ref_get (&self->output));
      break;
    case OP_PROP_MODE:
      g_value_set_enum (value, self->mode);
      break;
    }
}

static void
unity_wlr_output_power_finalize (GObject *object)
{
  UnityWlrOutputPower *self = UNITY_WLR_OUTPUT_POWER (object);

  g_clear_pointer (&self->handle, zwlr_output_power_v1_destroy);
  g_weak_ref_clear (&self->output);

  G_OBJECT_CLASS (unity_wlr_output_power_parent_class)->finalize (object);
}

static void
unity_wlr_output_power_class_init (UnityWlrOutputPowerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = unity_wlr_output_power_get_property;
  object_class->finalize     = unity_wlr_output_power_finalize;

  op_properties[OP_PROP_OUTPUT] = g_param_spec_object (
    "output", NULL, NULL, ASTAL_WL_TYPE_OUTPUT,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  op_properties[OP_PROP_MODE] = g_param_spec_enum (
    "mode", NULL, NULL,
    UNITY_WLR_TYPE_OUTPUT_POWER_MODE,
    UNITY_WLR_OUTPUT_POWER_MODE_OFF,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (op_properties), op_properties);

  op_signals[OP_SIGNAL_FAILED] = g_signal_new (
    "failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
    0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
unity_wlr_output_power_init (UnityWlrOutputPower *self)
{
  g_weak_ref_init (&self->output, NULL);
}

struct _UnityWlrOutputPowerManager
{
  GObject                              parent_instance;

  struct zwlr_output_power_manager_v1 *manager;
};

G_DEFINE_FINAL_TYPE (UnityWlrOutputPowerManager, unity_wlr_output_power_manager, G_TYPE_OBJECT)

UnityWlrOutputPower *
unity_wlr_output_power_manager_get_output_power (
  UnityWlrOutputPowerManager *self,
  AstalWlOutput              *output)
{
  g_return_val_if_fail (UNITY_WLR_IS_OUTPUT_POWER_MANAGER (self), NULL);
  g_return_val_if_fail (ASTAL_WL_IS_OUTPUT (output), NULL);

  if (self->manager == NULL)
    return NULL;

  struct wl_output           *wl_output = astal_wl_output_get_wl_output (output);
  struct zwlr_output_power_v1 *handle    =
    zwlr_output_power_manager_v1_get_output_power (self->manager, wl_output);
  return unity_wlr_output_power_new (handle, output);
}

static void
unity_wlr_output_power_manager_finalize (GObject *object)
{
  UnityWlrOutputPowerManager *self = UNITY_WLR_OUTPUT_POWER_MANAGER (object);

  g_clear_pointer (&self->manager, zwlr_output_power_manager_v1_destroy);

  G_OBJECT_CLASS (unity_wlr_output_power_manager_parent_class)->finalize (object);
}

static void
unity_wlr_output_power_manager_class_init (UnityWlrOutputPowerManagerClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = unity_wlr_output_power_manager_finalize;
}

static void
unity_wlr_output_power_manager_init (UnityWlrOutputPowerManager *self)
{
  AstalWlRegistry *registry = astal_wl_registry_get_default ();
  GList           *globals  = astal_wl_registry_find_globals (registry, MANAGER_INTERFACE);
  AstalWlGlobal   *global   = globals != NULL ? globals->data : NULL;
  if (global == NULL)
    {
      g_critical ("unity-wlr: compositor does not support %s", MANAGER_INTERFACE);
      g_list_free (globals);
      return;
    }

  guint32 version = MIN (global->version, (guint32) 1);
  self->manager = wl_registry_bind (astal_wl_registry_get_registry (registry),
                                    global->name,
                                    &zwlr_output_power_manager_v1_interface,
                                    version);

  g_list_free (globals);
}

UnityWlrOutputPowerManager *
unity_wlr_output_power_manager_get_default (void)
{
  static UnityWlrOutputPowerManager *instance;
  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, g_object_new (UNITY_WLR_TYPE_OUTPUT_POWER_MANAGER, NULL));
  return instance;
}
