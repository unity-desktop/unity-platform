/* unity-window-popup.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unity-base-popup.h>

G_BEGIN_DECLS

#define UNITY_TYPE_WINDOW_POPUP (unity_window_popup_get_type ())

G_DECLARE_DERIVABLE_TYPE (UnityWindowPopup,
                          unity_window_popup,
                          UNITY,
                          WINDOW_POPUP,
                          UnityBasePopup)

struct _UnityWindowPopupClass
{
  UnityBasePopupClass parent_class;

  gpointer            padding[8];
};

gdouble  unity_window_popup_get_size_ratio (UnityWindowPopup *self);
void     unity_window_popup_set_size_ratio (UnityWindowPopup *self,
                                            gdouble           ratio);

gboolean unity_window_popup_get_maximized  (UnityWindowPopup *self);
void     unity_window_popup_set_maximized  (UnityWindowPopup *self,
                                            gboolean          maximized);

G_END_DECLS
