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

/**
 * UnityWlrOutputPowerMode:
 * @UNITY_WLR_OUTPUT_POWER_MODE_OFF: output should be powered off
 * @UNITY_WLR_OUTPUT_POWER_MODE_ON: output should be powered on
 *
 * Requested output power state for wlroots output-power management.
 */
GType unity_wlr_output_power_mode_get_type (void);

/**
 * UnityWlrOutputPower:
 *
 * wlroots output-power controller bound to a specific #AstalWlOutput.
 */
G_DECLARE_FINAL_TYPE (UnityWlrOutputPower, unity_wlr_output_power,
                      UNITY_WLR, OUTPUT_POWER, GObject)

/**
 * unity_wlr_output_power_get_output:
 * @self: a #UnityWlrOutputPower
 *
 * Gets the output associated with this power controller.
 *
 * Returns: (transfer none): the related #AstalWlOutput.
 */
AstalWlOutput           *unity_wlr_output_power_get_output   (UnityWlrOutputPower *self);
/**
 * unity_wlr_output_power_get_mode:
 * @self: a #UnityWlrOutputPower
 *
 * Gets the last known output power mode.
 *
 * Returns: the current #UnityWlrOutputPowerMode.
 */
UnityWlrOutputPowerMode  unity_wlr_output_power_get_mode     (UnityWlrOutputPower *self);
/**
 * unity_wlr_output_power_request_mode:
 * @self: a #UnityWlrOutputPower
 * @mode: requested output power mode
 *
 * Requests changing the output power state through wlroots protocol.
 */
void                     unity_wlr_output_power_request_mode (UnityWlrOutputPower     *self,
                                                              UnityWlrOutputPowerMode  mode);

/**
 * UnityWlrOutputPowerManager:
 *
 * Singleton manager for wlroots output-power controllers.
 */
G_DECLARE_FINAL_TYPE (UnityWlrOutputPowerManager, unity_wlr_output_power_manager,
                      UNITY_WLR, OUTPUT_POWER_MANAGER, GObject)

/**
 * unity_wlr_output_power_manager_get_default:
 *
 * Gets the shared #UnityWlrOutputPowerManager instance.
 *
 * Returns: (transfer none): the shared manager.
 */
UnityWlrOutputPowerManager *unity_wlr_output_power_manager_get_default (void);

/**
 * unity_wlr_output_power_manager_get_output_power:
 * @self: a #UnityWlrOutputPowerManager
 * @output: an #AstalWlOutput
 *
 * Gets the controller object for @output.
 *
 * Returns: (transfer none): the #UnityWlrOutputPower for @output.
 */
UnityWlrOutputPower *unity_wlr_output_power_manager_get_output_power (
  UnityWlrOutputPowerManager *self,
  AstalWlOutput              *output);

G_END_DECLS
