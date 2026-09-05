/* unity-window-popup.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unity-dialog-popup.h>

G_BEGIN_DECLS

#define UNITY_TYPE_WINDOW_POPUP (unity_window_popup_get_type ())

/**
 * UnityWindowPopup:
 *
 * Popup variant that supports monitor-relative sizing and maximize state.
 */
G_DECLARE_DERIVABLE_TYPE (UnityWindowPopup,
                          unity_window_popup,
                          UNITY,
                          WINDOW_POPUP,
                          UnityDialogPopup)

struct _UnityWindowPopupClass
{
  UnityDialogPopupClass parent_class;

  gpointer            padding[8];
};

/**
 * unity_window_popup_get_size_ratio:
 * @self: a #UnityWindowPopup
 *
 * Gets the monitor-relative size ratio used for content sizing.
 *
 * Returns: ratio in the range [0, 1].
 */
gdouble  unity_window_popup_get_size_ratio (UnityWindowPopup *self);
/**
 * unity_window_popup_set_size_ratio:
 * @self: a #UnityWindowPopup
 * @ratio: size ratio in the range [0, 1]
 *
 * Sets monitor-relative content sizing ratio.
 */
void     unity_window_popup_set_size_ratio (UnityWindowPopup *self,
                                            gdouble           ratio);

/**
 * unity_window_popup_get_maximized:
 * @self: a #UnityWindowPopup
 *
 * Gets whether the popup is in maximized mode.
 *
 * Returns: %TRUE if maximized mode is enabled.
 */
gboolean unity_window_popup_get_maximized  (UnityWindowPopup *self);
/**
 * unity_window_popup_set_maximized:
 * @self: a #UnityWindowPopup
 * @maximized: whether maximized mode is enabled
 *
 * Sets maximized mode for the popup.
 */
void     unity_window_popup_set_maximized  (UnityWindowPopup *self,
                                            gboolean          maximized);

G_END_DECLS
