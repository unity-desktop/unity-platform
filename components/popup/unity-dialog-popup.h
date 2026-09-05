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
 * UnityDialogPopupUrgency:
 * @UNITY_DIALOG_POPUP_URGENCY_NONE: no sound cue on show (default).
 * @UNITY_DIALOG_POPUP_URGENCY_INFO: play the `dialog-information` event.
 * @UNITY_DIALOG_POPUP_URGENCY_QUESTION: play the `window-question` event.
 * @UNITY_DIALOG_POPUP_URGENCY_WARNING: play the `dialog-warning` event.
 * @UNITY_DIALOG_POPUP_URGENCY_CRITICAL: play the `dialog-error` event.
 *
 * Semantic urgency of the popup. Base plays the mapped XDG sound event
 * once each time the popup becomes visible.
 */
typedef enum
{
  UNITY_DIALOG_POPUP_URGENCY_NONE = 0,
  UNITY_DIALOG_POPUP_URGENCY_INFO,
  UNITY_DIALOG_POPUP_URGENCY_QUESTION,
  UNITY_DIALOG_POPUP_URGENCY_WARNING,
  UNITY_DIALOG_POPUP_URGENCY_CRITICAL,
} UnityDialogPopupUrgency;

#define UNITY_TYPE_DIALOG_POPUP_URGENCY (unity_dialog_popup_urgency_get_type ())
GType unity_dialog_popup_urgency_get_type (void);

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

/**
 * unity_dialog_popup_get_urgency:
 * @self: a #UnityDialogPopup
 *
 * Returns: current urgency.
 */
UnityDialogPopupUrgency unity_dialog_popup_get_urgency (UnityDialogPopup *self);
/**
 * unity_dialog_popup_set_urgency:
 * @self: a #UnityDialogPopup
 * @urgency: new urgency; use %UNITY_DIALOG_POPUP_URGENCY_NONE to silence.
 */
void                    unity_dialog_popup_set_urgency (UnityDialogPopup       *self,
                                                        UnityDialogPopupUrgency urgency);

G_END_DECLS
