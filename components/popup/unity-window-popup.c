/* unity-window-popup.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-window-popup.h"

#include <astal-4.h>

typedef struct
{
  gdouble  size_ratio;
  gboolean maximized;
  GtkAlign content_halign;
  GtkAlign content_valign;
} UnityWindowPopupPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityWindowPopup, unity_window_popup, UNITY_TYPE_DIALOG_POPUP)

#define PRIV(o) ((UnityWindowPopupPrivate *) unity_window_popup_get_instance_private (UNITY_WINDOW_POPUP (o)))

typedef enum
{
  PROP_SIZE_RATIO = 1,
  PROP_MAXIMIZED,
  PROP_CONTENT_HALIGN,
  PROP_CONTENT_VALIGN,
} UnityWindowPopupProperty;

static GParamSpec *properties[PROP_CONTENT_VALIGN + 1];

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
  gtk_widget_set_halign  (child, priv->content_halign);
  gtk_widget_set_valign  (child, priv->content_valign);
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

GtkAlign
unity_window_popup_get_content_halign (UnityWindowPopup *self)
{
  g_return_val_if_fail (UNITY_IS_WINDOW_POPUP (self), GTK_ALIGN_START);
  return PRIV (self)->content_halign;
}

void
unity_window_popup_set_content_halign (UnityWindowPopup *self, GtkAlign halign)
{
  g_return_if_fail (UNITY_IS_WINDOW_POPUP (self));
  if (PRIV (self)->content_halign == halign)
    return;
  PRIV (self)->content_halign = halign;
  apply_layout (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONTENT_HALIGN]);
}

GtkAlign
unity_window_popup_get_content_valign (UnityWindowPopup *self)
{
  g_return_val_if_fail (UNITY_IS_WINDOW_POPUP (self), GTK_ALIGN_START);
  return PRIV (self)->content_valign;
}

void
unity_window_popup_set_content_valign (UnityWindowPopup *self, GtkAlign valign)
{
  g_return_if_fail (UNITY_IS_WINDOW_POPUP (self));
  if (PRIV (self)->content_valign == valign)
    return;
  PRIV (self)->content_valign = valign;
  apply_layout (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONTENT_VALIGN]);
}

static void
unity_window_popup_constructed (GObject *object)
{
  G_OBJECT_CLASS (unity_window_popup_parent_class)->constructed (object);
  astal_window_set_keymode (ASTAL_WINDOW (object), ASTAL_KEYMODE_ON_DEMAND);
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
    case PROP_CONTENT_HALIGN:
      g_value_set_enum (value, unity_window_popup_get_content_halign (self));
      break;
    case PROP_CONTENT_VALIGN:
      g_value_set_enum (value, unity_window_popup_get_content_valign (self));
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
    case PROP_CONTENT_HALIGN:
      unity_window_popup_set_content_halign (self, g_value_get_enum (value));
      break;
    case PROP_CONTENT_VALIGN:
      unity_window_popup_set_content_valign (self, g_value_get_enum (value));
      break;
    }
}

static void
unity_window_popup_class_init (UnityWindowPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = unity_window_popup_constructed;
  object_class->get_property = unity_window_popup_get_property;
  object_class->set_property = unity_window_popup_set_property;

  widget_class->realize = unity_window_popup_realize;

  /**
   * UnityWindowPopup:size-ratio:
   *
   * Fraction of monitor geometry used to size popup content when not maximized.
   */
  properties[PROP_SIZE_RATIO] = g_param_spec_double (
    "size-ratio", NULL, NULL, 0.0, 1.0, 2.0 / 3.0,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * UnityWindowPopup:maximized:
   *
   * Whether maximized layout mode is enabled.
   */
  properties[PROP_MAXIMIZED] = g_param_spec_boolean (
    "maximized", NULL, NULL, FALSE,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * UnityWindowPopup:content-halign:
   *
   * Horizontal alignment applied to the popup's content when not maximized.
   * Lets callers pin the ratio-sized content to an edge of the layer-shell
   * surface. Maximized mode always fills.
   */
  properties[PROP_CONTENT_HALIGN] = g_param_spec_enum (
    "content-halign", NULL, NULL, GTK_TYPE_ALIGN, GTK_ALIGN_START,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * UnityWindowPopup:content-valign:
   *
   * Vertical alignment applied to the popup's content when not maximized.
   */
  properties[PROP_CONTENT_VALIGN] = g_param_spec_enum (
    "content-valign", NULL, NULL, GTK_TYPE_ALIGN, GTK_ALIGN_START,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_install_action (widget_class, "window.minimize",         NULL, minimize_action);
  gtk_widget_class_install_action (widget_class, "window.toggle-maximized", NULL, toggle_maximized_action);
}

static void
unity_window_popup_init (UnityWindowPopup *self)
{
  UnityWindowPopupPrivate *priv = PRIV (self);

  priv->size_ratio     = 2.0 / 3.0;
  priv->maximized      = FALSE;
  priv->content_halign = GTK_ALIGN_START;
  priv->content_valign = GTK_ALIGN_START;

  g_signal_connect (self, "map", G_CALLBACK (on_map), NULL);
}
