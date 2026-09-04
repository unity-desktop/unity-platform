/* unity-wlr-output-power.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <astal-wl.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
  UNITY_WLR_OUTPUT_POWER_MODE_OFF = 0,
  UNITY_WLR_OUTPUT_POWER_MODE_ON  = 1,
} UnityWlrOutputPowerMode;

#define UNITY_WLR_TYPE_OUTPUT_POWER_MODE    (unity_wlr_output_power_mode_get_type ())
#define UNITY_WLR_TYPE_OUTPUT_POWER         (unity_wlr_output_power_get_type ())
#define UNITY_WLR_TYPE_OUTPUT_POWER_MANAGER (unity_wlr_output_power_manager_get_type ())

GType unity_wlr_output_power_mode_get_type (void);

G_DECLARE_FINAL_TYPE (UnityWlrOutputPower, unity_wlr_output_power,
                      UNITY_WLR, OUTPUT_POWER, GObject)

AstalWlOutput           *unity_wlr_output_power_get_output   (UnityWlrOutputPower *self);
UnityWlrOutputPowerMode  unity_wlr_output_power_get_mode     (UnityWlrOutputPower *self);
void                     unity_wlr_output_power_request_mode (UnityWlrOutputPower     *self,
                                                              UnityWlrOutputPowerMode  mode);

G_DECLARE_FINAL_TYPE (UnityWlrOutputPowerManager, unity_wlr_output_power_manager,
                      UNITY_WLR, OUTPUT_POWER_MANAGER, GObject)

UnityWlrOutputPowerManager *unity_wlr_output_power_manager_get_default (void);

UnityWlrOutputPower *unity_wlr_output_power_manager_get_output_power (
  UnityWlrOutputPowerManager *self,
  AstalWlOutput              *output);

G_END_DECLS
