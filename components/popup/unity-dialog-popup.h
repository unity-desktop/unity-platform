/* unity-dialog-popup.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unity-base-popup.h>

G_BEGIN_DECLS

#define UNITY_TYPE_DIALOG_POPUP (unity_dialog_popup_get_type ())

G_DECLARE_DERIVABLE_TYPE (UnityDialogPopup,
                          unity_dialog_popup,
                          UNITY,
                          DIALOG_POPUP,
                          UnityBasePopup)

struct _UnityDialogPopupClass
{
  UnityBasePopupClass parent_class;

  gpointer            padding[8];
};

GtkWidget *unity_dialog_popup_get_content (UnityDialogPopup *self);
void       unity_dialog_popup_set_content (UnityDialogPopup *self,
                                           GtkWidget        *content);

G_END_DECLS
