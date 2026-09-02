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

gboolean unity_base_popup_get_dismissable (UnityBasePopup *self);
void     unity_base_popup_set_dismissable (UnityBasePopup *self,
                                           gboolean        dismissable);

G_END_DECLS
