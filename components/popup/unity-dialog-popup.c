/* unity-dialog-popup.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-dialog-popup.h"

#include <gsound.h>

#include "stylesheet-private.h"

#define FADE_SPRING_DAMPING   1.0
#define FADE_SPRING_STIFFNESS 500
#define FADE_SPRING_MASS      1
#define FADE_SPRING_EPSILON   0.01

typedef struct
{
  gboolean                 dismissable;
  gchar                   *stylesheet;

  UnityDialogPopupUrgency  urgency;
  GSoundContext           *sound;

  AdwAnimation            *anim;
  gdouble                  progress;
  gboolean                 close_pending;
} UnityDialogPopupPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityDialogPopup, unity_dialog_popup, ASTAL_TYPE_WINDOW)

G_DEFINE_ENUM_TYPE (UnityDialogPopupUrgency, unity_dialog_popup_urgency,
                    G_DEFINE_ENUM_VALUE (UNITY_DIALOG_POPUP_URGENCY_NONE,     "none"),
                    G_DEFINE_ENUM_VALUE (UNITY_DIALOG_POPUP_URGENCY_INFO,     "info"),
                    G_DEFINE_ENUM_VALUE (UNITY_DIALOG_POPUP_URGENCY_QUESTION, "question"),
                    G_DEFINE_ENUM_VALUE (UNITY_DIALOG_POPUP_URGENCY_WARNING,  "warning"),
                    G_DEFINE_ENUM_VALUE (UNITY_DIALOG_POPUP_URGENCY_CRITICAL, "critical"))

#define PRIV(o) ((UnityDialogPopupPrivate *) unity_dialog_popup_get_instance_private (UNITY_DIALOG_POPUP (o)))

typedef enum
{
  PROP_DISMISSABLE = 1,
  PROP_STYLESHEET,
  PROP_URGENCY,
} UnityDialogPopupProperty;

static GParamSpec *properties[PROP_URGENCY + 1];

typedef enum
{
  SIGNAL_CLOSED,
} UnityDialogPopupSignal;

static guint signals[SIGNAL_CLOSED + 1];

static const gchar *urgency_event_id[] = {
  [UNITY_DIALOG_POPUP_URGENCY_NONE]     = NULL,
  [UNITY_DIALOG_POPUP_URGENCY_INFO]     = "dialog-information",
  [UNITY_DIALOG_POPUP_URGENCY_QUESTION] = "window-question",
  [UNITY_DIALOG_POPUP_URGENCY_WARNING]  = "dialog-warning",
  [UNITY_DIALOG_POPUP_URGENCY_CRITICAL] = "dialog-error",
};

gboolean
unity_dialog_popup_get_dismissable (UnityDialogPopup *self)
{
  g_return_val_if_fail (UNITY_IS_DIALOG_POPUP (self), TRUE);
  return PRIV (self)->dismissable;
}

void
unity_dialog_popup_set_dismissable (UnityDialogPopup *self, gboolean dismissable)
{
  g_return_if_fail (UNITY_IS_DIALOG_POPUP (self));
  dismissable = !!dismissable;
  if (PRIV (self)->dismissable == dismissable)
    return;

  PRIV (self)->dismissable = dismissable;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DISMISSABLE]);
}

UnityDialogPopupUrgency
unity_dialog_popup_get_urgency (UnityDialogPopup *self)
{
  g_return_val_if_fail (UNITY_IS_DIALOG_POPUP (self), UNITY_DIALOG_POPUP_URGENCY_NONE);
  return PRIV (self)->urgency;
}

void
unity_dialog_popup_set_urgency (UnityDialogPopup       *self,
                                UnityDialogPopupUrgency urgency)
{
  g_return_if_fail (UNITY_IS_DIALOG_POPUP (self));
  if (PRIV (self)->urgency == urgency)
    return;

  PRIV (self)->urgency = urgency;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URGENCY]);
}

static gboolean
maybe_dismiss (UnityDialogPopup *self)
{
  if (!PRIV (self)->dismissable)
    return FALSE;

  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
  return TRUE;
}

static void
close_action (GtkWidget *widget, const gchar *action_name, GVariant *parameter)
{
  gtk_window_close (GTK_WINDOW (widget));
}

static gboolean
on_escape_pressed (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  return maybe_dismiss (UNITY_DIALOG_POPUP (widget));
}

static void
on_focus_leave (GtkEventControllerFocus *focus, gpointer user_data)
{
  maybe_dismiss (UNITY_DIALOG_POPUP (user_data));
}

static void
on_outside_pressed (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y,
                    gpointer user_data)
{
  UnityDialogPopup *self  = user_data;
  GtkWidget        *area  = gtk_event_controller_get_widget (
    GTK_EVENT_CONTROLLER (gesture));
  GtkWidget        *child = gtk_window_get_child (GTK_WINDOW (self));

  if (child == NULL)
    return;

  graphene_rect_t bounds;
  if (gtk_widget_compute_bounds (child, area, &bounds) &&
      graphene_rect_contains_point (&bounds, &GRAPHENE_POINT_INIT ((gfloat) x, (gfloat) y)))
    return;

  if (maybe_dismiss (self))
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
on_sound_played (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GError) error = NULL;

  if (!gsound_context_play_full_finish (GSOUND_CONTEXT (source), result, &error) &&
      !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    g_warning ("unity-dialog-popup: play failed: %s", error->message);
}

static void
maybe_play_urgency (UnityDialogPopup *self)
{
  UnityDialogPopupPrivate *priv  = PRIV (self);
  GtkApplication          *app;
  const gchar             *event;
  const gchar             *app_id = NULL;

  if (priv->urgency == UNITY_DIALOG_POPUP_URGENCY_NONE || priv->sound == NULL)
    return;

  event = urgency_event_id[priv->urgency];
  if (event == NULL)
    return;

  app = gtk_window_get_application (GTK_WINDOW (self));
  if (app != NULL)
    app_id = g_application_get_application_id (G_APPLICATION (app));

  gsound_context_play_full (priv->sound, NULL, on_sound_played, NULL,
                            GSOUND_ATTR_EVENT_ID,       event,
                            GSOUND_ATTR_MEDIA_ROLE,     "event",
                            GSOUND_ATTR_APPLICATION_ID, app_id != NULL ? app_id : "org.unity.DialogPopup",
                            NULL);
}

static void
on_fade_progress (gdouble value, gpointer user_data)
{
  UnityDialogPopup *self  = user_data;
  GtkWidget        *child = gtk_window_get_child (GTK_WINDOW (self));

  PRIV (self)->progress = value;
  if (child != NULL)
    gtk_widget_set_opacity (child, CLAMP (value, 0.0, 1.0));
}

static void
on_close_animation_done (UnityDialogPopup *self)
{
  UnityDialogPopupPrivate *priv = PRIV (self);

  if (!priv->close_pending)
    return;

  priv->close_pending = FALSE;
  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
  g_signal_emit (self, signals[SIGNAL_CLOSED], 0);
}

static gboolean
unity_dialog_popup_close_request (GtkWindow *window)
{
  UnityDialogPopupPrivate *priv = PRIV (window);

  if (priv->close_pending)
    return GDK_EVENT_STOP;

  priv->close_pending = TRUE;
  adw_spring_animation_set_value_from (ADW_SPRING_ANIMATION (priv->anim), priv->progress);
  adw_spring_animation_set_value_to   (ADW_SPRING_ANIMATION (priv->anim), 0.0);
  adw_animation_play (priv->anim);
  return GDK_EVENT_STOP;
}

static void
unity_dialog_popup_map (GtkWidget *widget)
{
  UnityDialogPopupPrivate *priv = PRIV (widget);

  GTK_WIDGET_CLASS (unity_dialog_popup_parent_class)->map (widget);

  adw_spring_animation_set_value_from (ADW_SPRING_ANIMATION (priv->anim), priv->progress);
  adw_spring_animation_set_value_to   (ADW_SPRING_ANIMATION (priv->anim), 1.0);
  adw_animation_play (priv->anim);

  maybe_play_urgency (UNITY_DIALOG_POPUP (widget));
}

static void
unity_dialog_popup_constructed (GObject *object)
{
  UnityDialogPopup *self = UNITY_DIALOG_POPUP (object);

  G_OBJECT_CLASS (unity_dialog_popup_parent_class)->constructed (object);

  astal_window_set_anchor (ASTAL_WINDOW (self),
                           ASTAL_WINDOW_ANCHOR_TOP | ASTAL_WINDOW_ANCHOR_BOTTOM |
                           ASTAL_WINDOW_ANCHOR_LEFT | ASTAL_WINDOW_ANCHOR_RIGHT);
  astal_window_set_exclusivity (ASTAL_WINDOW (self), ASTAL_EXCLUSIVITY_NORMAL);
  astal_window_set_keymode     (ASTAL_WINDOW (self), ASTAL_KEYMODE_EXCLUSIVE);
}

static void
unity_dialog_popup_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (unity_dialog_popup_parent_class)->realize (widget);

  unity_platform_style_ensure (gtk_widget_get_display (widget));
  if (PRIV (widget)->stylesheet != NULL)
    unity_platform_load_stylesheet (gtk_widget_get_display (widget), PRIV (widget)->stylesheet);
}

static void
unity_dialog_popup_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityDialogPopup *self = UNITY_DIALOG_POPUP (object);

  switch ((UnityDialogPopupProperty) prop_id)
    {
    case PROP_DISMISSABLE:
      g_value_set_boolean (value, unity_dialog_popup_get_dismissable (self));
      break;
    case PROP_STYLESHEET:
      g_value_set_string (value, PRIV (self)->stylesheet);
      break;
    case PROP_URGENCY:
      g_value_set_enum (value, unity_dialog_popup_get_urgency (self));
      break;
    }
}

static void
unity_dialog_popup_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  UnityDialogPopup *self = UNITY_DIALOG_POPUP (object);

  switch ((UnityDialogPopupProperty) prop_id)
    {
    case PROP_DISMISSABLE:
      unity_dialog_popup_set_dismissable (self, g_value_get_boolean (value));
      break;
    case PROP_STYLESHEET:
      g_free (PRIV (self)->stylesheet);
      PRIV (self)->stylesheet = g_value_dup_string (value);
      break;
    case PROP_URGENCY:
      unity_dialog_popup_set_urgency (self, g_value_get_enum (value));
      break;
    }
}

static void
unity_dialog_popup_dispose (GObject *object)
{
  g_clear_object (&PRIV (object)->anim);
  G_OBJECT_CLASS (unity_dialog_popup_parent_class)->dispose (object);
}

static void
unity_dialog_popup_finalize (GObject *object)
{
  UnityDialogPopupPrivate *priv = PRIV (object);

  g_clear_pointer (&priv->stylesheet, g_free);
  g_clear_object  (&priv->sound);

  G_OBJECT_CLASS (unity_dialog_popup_parent_class)->finalize (object);
}

static void
unity_dialog_popup_class_init (UnityDialogPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkWindowClass *window_class = GTK_WINDOW_CLASS (klass);

  object_class->constructed  = unity_dialog_popup_constructed;
  object_class->get_property = unity_dialog_popup_get_property;
  object_class->set_property = unity_dialog_popup_set_property;
  object_class->dispose      = unity_dialog_popup_dispose;
  object_class->finalize     = unity_dialog_popup_finalize;

  widget_class->realize = unity_dialog_popup_realize;
  widget_class->map     = unity_dialog_popup_map;
  window_class->close_request = unity_dialog_popup_close_request;

  /**
   * UnityDialogPopup:dismissable:
   *
   * Whether Escape, focus-out, and outside-click can dismiss the popup.
   */
  properties[PROP_DISMISSABLE] = g_param_spec_boolean (
    "dismissable", NULL, NULL, TRUE,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * UnityDialogPopup:stylesheet:
   *
   * Optional CSS resource path loaded for the popup.
   */
  properties[PROP_STYLESHEET] = g_param_spec_string (
    "stylesheet", NULL, NULL, NULL,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * UnityDialogPopup:urgency:
   *
   * Sound cue tone played when the popup becomes visible. Defaults to
   * %UNITY_DIALOG_POPUP_URGENCY_NONE (silent). Failed sound-server
   * initialisation is non-fatal — the popup stays silent.
   */
  properties[PROP_URGENCY] = g_param_spec_enum (
    "urgency", NULL, NULL,
    UNITY_TYPE_DIALOG_POPUP_URGENCY, UNITY_DIALOG_POPUP_URGENCY_NONE,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  /**
   * UnityDialogPopup::closed:
   * @self: the #UnityDialogPopup instance
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
unity_dialog_popup_init (UnityDialogPopup *self)
{
  UnityDialogPopupPrivate *priv = PRIV (self);
  GtkGesture              *click;
  GtkEventController      *focus;

  AdwAnimationTarget *target = adw_callback_animation_target_new (
    on_fade_progress, self, NULL);
  AdwSpringParams    *params = adw_spring_params_new (
    FADE_SPRING_DAMPING, FADE_SPRING_MASS, FADE_SPRING_STIFFNESS);

  priv->dismissable = TRUE;
  priv->urgency     = UNITY_DIALOG_POPUP_URGENCY_NONE;
  priv->sound       = gsound_context_new (NULL, NULL);
  priv->progress    = 0.0;

  priv->anim = adw_spring_animation_new (GTK_WIDGET (self), 0.0, 1.0, params, target);
  adw_spring_animation_set_epsilon (ADW_SPRING_ANIMATION (priv->anim), FADE_SPRING_EPSILON);
  adw_spring_animation_set_clamp   (ADW_SPRING_ANIMATION (priv->anim), TRUE);
  g_signal_connect_swapped (priv->anim, "done",
                            G_CALLBACK (on_close_animation_done), self);

  gtk_widget_add_css_class (GTK_WIDGET (self), "unity-popup");

  click = gtk_gesture_click_new ();
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (click),
                                              GTK_PHASE_CAPTURE);
  g_signal_connect (click, "pressed", G_CALLBACK (on_outside_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click));

  focus = gtk_event_controller_focus_new ();
  g_signal_connect (focus, "leave", G_CALLBACK (on_focus_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), focus);
}
