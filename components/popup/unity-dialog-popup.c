/* unity-dialog-popup.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-dialog-popup.h"

typedef struct
{
  GtkLabel  *title_label;
  AdwBin    *content_bin;
  GtkWidget *content;
} UnityDialogPopupPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityDialogPopup, unity_dialog_popup, UNITY_TYPE_BASE_POPUP)

#define PRIV(o) ((UnityDialogPopupPrivate *) unity_dialog_popup_get_instance_private (UNITY_DIALOG_POPUP (o)))

typedef enum
{
  PROP_CONTENT = 1,
} UnityDialogPopupProperty;

static GParamSpec *properties[PROP_CONTENT + 1];

GtkWidget *
unity_dialog_popup_get_content (UnityDialogPopup *self)
{
  g_return_val_if_fail (UNITY_IS_DIALOG_POPUP (self), NULL);
  return PRIV (self)->content;
}

void
unity_dialog_popup_set_content (UnityDialogPopup *self, GtkWidget *content)
{
  g_return_if_fail (UNITY_IS_DIALOG_POPUP (self));
  g_return_if_fail (content == NULL || GTK_IS_WIDGET (content));

  if (PRIV (self)->content == content)
    return;

  PRIV (self)->content = content;
  adw_bin_set_child (PRIV (self)->content_bin, content);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONTENT]);
}

static void
unity_dialog_popup_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch ((UnityDialogPopupProperty) prop_id)
    {
    case PROP_CONTENT:
      g_value_set_object (value, unity_dialog_popup_get_content (UNITY_DIALOG_POPUP (object)));
      break;
    }
}

static void
unity_dialog_popup_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch ((UnityDialogPopupProperty) prop_id)
    {
    case PROP_CONTENT:
      unity_dialog_popup_set_content (UNITY_DIALOG_POPUP (object), g_value_get_object (value));
      break;
    }
}

static void
unity_dialog_popup_class_init (UnityDialogPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = unity_dialog_popup_get_property;
  object_class->set_property = unity_dialog_popup_set_property;

  properties[PROP_CONTENT] = g_param_spec_object (
    "content", NULL, NULL, GTK_TYPE_WIDGET,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/unity/platform/components/popup/unity-dialog-popup.ui");
  gtk_widget_class_bind_template_child_private (widget_class, UnityDialogPopup, title_label);
  gtk_widget_class_bind_template_child_private (widget_class, UnityDialogPopup, content_bin);
}

static void
unity_dialog_popup_init (UnityDialogPopup *self)
{
  gtk_widget_add_css_class (GTK_WIDGET (self), "unity-dialog-popup-overlay");

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self, "title", PRIV (self)->title_label, "label",
                          G_BINDING_SYNC_CREATE);
}
