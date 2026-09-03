/* unity-window-popup.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-window-popup.h"

typedef struct
{
  gdouble  size_ratio;
  gboolean maximized;
} UnityWindowPopupPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityWindowPopup, unity_window_popup, UNITY_TYPE_BASE_POPUP)

#define PRIV(o) ((UnityWindowPopupPrivate *) unity_window_popup_get_instance_private (UNITY_WINDOW_POPUP (o)))

typedef enum
{
  PROP_SIZE_RATIO = 1,
  PROP_MAXIMIZED,
} UnityWindowPopupProperty;

static GParamSpec *properties[PROP_MAXIMIZED + 1];

static void
apply_layout (UnityWindowPopup *self)
{
  UnityWindowPopupPrivate *priv = PRIV (self);
  GtkWidget               *child;
  GdkRectangle             geo  = { 0, 0, 0, 0 };
  g_autoptr (GdkMonitor) monitor = NULL;

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return;

  child = gtk_window_get_child (GTK_WINDOW (self));
  if (child == NULL)
    return;

  if (priv->maximized)
    {
      gtk_widget_add_css_class (child, "maximized");
      gtk_widget_set_halign  (child, GTK_ALIGN_FILL);
      gtk_widget_set_valign  (child, GTK_ALIGN_FILL);
      gtk_widget_set_hexpand (child, TRUE);
      gtk_widget_set_vexpand (child, TRUE);
      gtk_widget_set_size_request (child, -1, -1);
      return;
    }

  gtk_widget_remove_css_class (child, "maximized");
  gtk_widget_set_halign  (child, GTK_ALIGN_START);
  gtk_widget_set_valign  (child, GTK_ALIGN_START);
  gtk_widget_set_hexpand (child, FALSE);
  gtk_widget_set_vexpand (child, FALSE);

  monitor = astal_window_get_current_monitor (ASTAL_WINDOW (self));
  if (monitor != NULL)
    gdk_monitor_get_geometry (monitor, &geo);
  if (geo.width > 0 && geo.height > 0)
    gtk_widget_set_size_request (child,
                                 (gint) (geo.width  * priv->size_ratio),
                                 (gint) (geo.height * priv->size_ratio));
}

static void
minimize_action (GtkWidget *widget, const gchar *action_name, GVariant *parameter)
{
  (void) action_name; (void) parameter;
  gtk_widget_set_visible (widget, FALSE);
}

static void
toggle_maximized_action (GtkWidget *widget, const gchar *action_name, GVariant *parameter)
{
  (void) action_name; (void) parameter;
  UnityWindowPopup *self = UNITY_WINDOW_POPUP (widget);
  unity_window_popup_set_maximized (self, !PRIV (self)->maximized);
}

static void
on_map (GtkWidget *widget, gpointer user_data)
{
  (void) user_data;
  apply_layout (UNITY_WINDOW_POPUP (widget));
}

gdouble
unity_window_popup_get_size_ratio (UnityWindowPopup *self)
{
  g_return_val_if_fail (UNITY_IS_WINDOW_POPUP (self), 2.0 / 3.0);
  return PRIV (self)->size_ratio;
}

void
unity_window_popup_set_size_ratio (UnityWindowPopup *self, gdouble ratio)
{
  g_return_if_fail (UNITY_IS_WINDOW_POPUP (self));
  ratio = CLAMP (ratio, 0.0, 1.0);
  if (PRIV (self)->size_ratio == ratio)
    return;

  PRIV (self)->size_ratio = ratio;
  apply_layout (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SIZE_RATIO]);
}

gboolean
unity_window_popup_get_maximized (UnityWindowPopup *self)
{
  g_return_val_if_fail (UNITY_IS_WINDOW_POPUP (self), FALSE);
  return PRIV (self)->maximized;
}

void
unity_window_popup_set_maximized (UnityWindowPopup *self, gboolean maximized)
{
  g_return_if_fail (UNITY_IS_WINDOW_POPUP (self));
  maximized = !!maximized;
  if (PRIV (self)->maximized == maximized)
    return;

  PRIV (self)->maximized = maximized;

  if (maximized)
    gtk_window_maximize (GTK_WINDOW (self));
  else
    gtk_window_unmaximize (GTK_WINDOW (self));

  apply_layout (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAXIMIZED]);
}

static void
unity_window_popup_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (unity_window_popup_parent_class)->realize (widget);

  apply_layout (UNITY_WINDOW_POPUP (widget));
}

static void
unity_window_popup_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityWindowPopup *self = UNITY_WINDOW_POPUP (object);

  switch ((UnityWindowPopupProperty) prop_id)
    {
    case PROP_SIZE_RATIO:
      g_value_set_double (value, unity_window_popup_get_size_ratio (self));
      break;
    case PROP_MAXIMIZED:
      g_value_set_boolean (value, unity_window_popup_get_maximized (self));
      break;
    }
}

static void
unity_window_popup_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  UnityWindowPopup *self = UNITY_WINDOW_POPUP (object);

  switch ((UnityWindowPopupProperty) prop_id)
    {
    case PROP_SIZE_RATIO:
      unity_window_popup_set_size_ratio (self, g_value_get_double (value));
      break;
    case PROP_MAXIMIZED:
      unity_window_popup_set_maximized (self, g_value_get_boolean (value));
      break;
    }
}

static void
unity_window_popup_class_init (UnityWindowPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = unity_window_popup_get_property;
  object_class->set_property = unity_window_popup_set_property;

  widget_class->realize = unity_window_popup_realize;

  properties[PROP_SIZE_RATIO] = g_param_spec_double (
    "size-ratio", NULL, NULL, 0.0, 1.0, 2.0 / 3.0,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  properties[PROP_MAXIMIZED] = g_param_spec_boolean (
    "maximized", NULL, NULL, FALSE,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_install_action (widget_class, "window.minimize",         NULL, minimize_action);
  gtk_widget_class_install_action (widget_class, "window.toggle-maximized", NULL, toggle_maximized_action);
}

static void
unity_window_popup_init (UnityWindowPopup *self)
{
  UnityWindowPopupPrivate *priv = PRIV (self);

  priv->size_ratio = 2.0 / 3.0;
  priv->maximized  = FALSE;

  gtk_widget_add_css_class (GTK_WIDGET (self), "unity-window-popup-overlay");

  g_signal_connect (self, "map", G_CALLBACK (on_map), NULL);
}
