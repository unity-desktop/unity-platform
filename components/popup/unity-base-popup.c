/* unity-base-popup.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-base-popup.h"

#include "stylesheet-private.h"

typedef struct
{
  gboolean  dismissable;
  gchar    *stylesheet;
} UnityBasePopupPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityBasePopup, unity_base_popup, ASTAL_TYPE_WINDOW)

#define PRIV(o) ((UnityBasePopupPrivate *) unity_base_popup_get_instance_private (UNITY_BASE_POPUP (o)))

typedef enum
{
  PROP_DISMISSABLE = 1,
  PROP_STYLESHEET,
} UnityBasePopupProperty;

static GParamSpec *properties[PROP_STYLESHEET + 1];

typedef enum
{
  SIGNAL_CLOSED,
} UnityBasePopupSignal;

static guint signals[SIGNAL_CLOSED + 1];

gboolean
unity_base_popup_get_dismissable (UnityBasePopup *self)
{
  g_return_val_if_fail (UNITY_IS_BASE_POPUP (self), TRUE);
  return PRIV (self)->dismissable;
}

void
unity_base_popup_set_dismissable (UnityBasePopup *self, gboolean dismissable)
{
  g_return_if_fail (UNITY_IS_BASE_POPUP (self));
  dismissable = !!dismissable;
  if (PRIV (self)->dismissable == dismissable)
    return;

  PRIV (self)->dismissable = dismissable;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DISMISSABLE]);
}

static gboolean
maybe_dismiss (UnityBasePopup *self)
{
  if (!PRIV (self)->dismissable)
    return FALSE;

  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
  return TRUE;
}

static void
close_action (GtkWidget *widget, const gchar *action_name, GVariant *parameter)
{
  (void) action_name; (void) parameter;
  gtk_window_close (GTK_WINDOW (widget));
}

static gboolean
on_escape_pressed (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void) args; (void) user_data;
  return maybe_dismiss (UNITY_BASE_POPUP (widget));
}

static void
on_focus_leave (GtkEventControllerFocus *focus, gpointer user_data)
{
  (void) focus;
  maybe_dismiss (UNITY_BASE_POPUP (user_data));
}

static void
on_outside_pressed (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y,
                    gpointer user_data)
{
  UnityBasePopup *self  = user_data;
  GtkWidget      *area  = gtk_event_controller_get_widget (
    GTK_EVENT_CONTROLLER (gesture));
  GtkWidget      *child = gtk_window_get_child (GTK_WINDOW (self));

  (void) n_press;

  if (child == NULL)
    return;

  graphene_rect_t bounds;
  if (gtk_widget_compute_bounds (child, area, &bounds) &&
      graphene_rect_contains_point (&bounds, &GRAPHENE_POINT_INIT ((gfloat) x, (gfloat) y)))
    return;

  if (maybe_dismiss (self))
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static gboolean
unity_base_popup_close_request (GtkWindow *window)
{
  gtk_widget_set_visible (GTK_WIDGET (window), FALSE);
  g_signal_emit (window, signals[SIGNAL_CLOSED], 0);
  return GDK_EVENT_STOP;
}

static void
unity_base_popup_constructed (GObject *object)
{
  UnityBasePopup *self = UNITY_BASE_POPUP (object);

  G_OBJECT_CLASS (unity_base_popup_parent_class)->constructed (object);

  astal_window_set_anchor (ASTAL_WINDOW (self),
                           ASTAL_WINDOW_ANCHOR_TOP | ASTAL_WINDOW_ANCHOR_BOTTOM |
                           ASTAL_WINDOW_ANCHOR_LEFT | ASTAL_WINDOW_ANCHOR_RIGHT);
  astal_window_set_exclusivity (ASTAL_WINDOW (self), ASTAL_EXCLUSIVITY_NORMAL);
}

static void
unity_base_popup_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (unity_base_popup_parent_class)->realize (widget);

  unity_platform_style_ensure (gtk_widget_get_display (widget));
  if (PRIV (widget)->stylesheet != NULL)
    unity_platform_load_stylesheet (gtk_widget_get_display (widget), PRIV (widget)->stylesheet);
}

static void
unity_base_popup_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityBasePopup *self = UNITY_BASE_POPUP (object);

  switch ((UnityBasePopupProperty) prop_id)
    {
    case PROP_DISMISSABLE:
      g_value_set_boolean (value, unity_base_popup_get_dismissable (self));
      break;
    case PROP_STYLESHEET:
      g_value_set_string (value, PRIV (self)->stylesheet);
      break;
    }
}

static void
unity_base_popup_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  UnityBasePopup *self = UNITY_BASE_POPUP (object);

  switch ((UnityBasePopupProperty) prop_id)
    {
    case PROP_DISMISSABLE:
      unity_base_popup_set_dismissable (self, g_value_get_boolean (value));
      break;
    case PROP_STYLESHEET:
      g_free (PRIV (self)->stylesheet);
      PRIV (self)->stylesheet = g_value_dup_string (value);
      break;
    }
}

static void
unity_base_popup_finalize (GObject *object)
{
  g_free (PRIV (object)->stylesheet);
  G_OBJECT_CLASS (unity_base_popup_parent_class)->finalize (object);
}

static void
unity_base_popup_class_init (UnityBasePopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkWindowClass *window_class = GTK_WINDOW_CLASS (klass);

  object_class->constructed  = unity_base_popup_constructed;
  object_class->get_property = unity_base_popup_get_property;
  object_class->set_property = unity_base_popup_set_property;
  object_class->finalize     = unity_base_popup_finalize;

  widget_class->realize = unity_base_popup_realize;
  window_class->close_request = unity_base_popup_close_request;

  /**
   * UnityBasePopup:dismissable:
   *
   * Whether Escape, focus-out, and outside-click can dismiss the popup.
   */
  properties[PROP_DISMISSABLE] = g_param_spec_boolean (
    "dismissable", NULL, NULL, TRUE,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * UnityBasePopup:stylesheet:
   *
   * Optional CSS resource path loaded for the popup.
   */
  properties[PROP_STYLESHEET] = g_param_spec_string (
    "stylesheet", NULL, NULL, NULL,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  /**
   * UnityBasePopup::closed:
   * @self: the #UnityBasePopup instance
   *
   * Emitted after the popup close request is handled and the window is hidden.
   */
  signals[SIGNAL_CLOSED] = g_signal_new (
    "closed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
    0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  gtk_widget_class_install_action (widget_class, "window.close", NULL, close_action);

  gtk_widget_class_add_binding (widget_class, GDK_KEY_Escape, GDK_NO_MODIFIER_MASK,
                                on_escape_pressed, NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_w, GDK_CONTROL_MASK,
                                       "window.close", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F4, GDK_ALT_MASK,
                                       "window.close", NULL);
}

static void
unity_base_popup_init (UnityBasePopup *self)
{
  GtkGesture         *click;
  GtkEventController *focus;

  PRIV (self)->dismissable = TRUE;

  click = gtk_gesture_click_new ();
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (click),
                                              GTK_PHASE_CAPTURE);
  g_signal_connect (click, "pressed", G_CALLBACK (on_outside_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click));

  focus = gtk_event_controller_focus_new ();
  g_signal_connect (focus, "leave", G_CALLBACK (on_focus_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), focus);
}
