/* unity-dialog-popup.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_TYPE_DIALOG_POPUP (unity_dialog_popup_get_type ())

/**
 * UnityDialogPopup:
 *
 * Popup window with dismiss behavior, keyboard-exclusive keymode,
 * and optional per-instance stylesheet.
 */
G_DECLARE_DERIVABLE_TYPE (UnityDialogPopup,
                          unity_dialog_popup,
                          UNITY,
                          DIALOG_POPUP,
                          AstalWindow)

struct _UnityDialogPopupClass
{
  AstalWindowClass parent_class;

  gpointer         padding[8];
};

/**
 * unity_dialog_popup_get_dismissable:
 * @self: a #UnityDialogPopup
 *
 * Returns: %TRUE if dismissable.
 */
gboolean unity_dialog_popup_get_dismissable (UnityDialogPopup *self);
/**
 * unity_dialog_popup_set_dismissable:
 * @self: a #UnityDialogPopup
 * @dismissable: whether dismissal by focus/escape/outside-click is enabled
 */
void     unity_dialog_popup_set_dismissable (UnityDialogPopup *self,
                                             gboolean          dismissable);

G_END_DECLS
