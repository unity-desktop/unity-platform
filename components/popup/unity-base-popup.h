/* unity-base-popup.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_TYPE_BASE_POPUP (unity_base_popup_get_type ())

/**
 * UnityBasePopup:
 *
 * Base popup window type with dismiss behavior and style support.
 */
G_DECLARE_DERIVABLE_TYPE (UnityBasePopup,
                          unity_base_popup,
                          UNITY,
                          BASE_POPUP,
                          AstalWindow)

struct _UnityBasePopupClass
{
  AstalWindowClass parent_class;

  gpointer         padding[8];
};

/**
 * unity_base_popup_get_dismissable:
 * @self: a #UnityBasePopup
 *
 * Gets whether this popup can be dismissed by user interaction.
 *
 * Returns: %TRUE if dismissable.
 */
gboolean unity_base_popup_get_dismissable (UnityBasePopup *self);
/**
 * unity_base_popup_set_dismissable:
 * @self: a #UnityBasePopup
 * @dismissable: whether dismissal by focus/escape/outside-click is enabled
 *
 * Sets dismiss behavior for the popup.
 */
void     unity_base_popup_set_dismissable (UnityBasePopup *self,
                                           gboolean        dismissable);

G_END_DECLS
