/* unity-strip.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-strip.h"

#include <gdesktop-enums.h>

#include "stylesheet-private.h"

#define INTERFACE_SCHEMA "org.gnome.desktop.interface"
#define KEY_COLOR_SCHEME "color-scheme"

typedef struct
{
  gchar *stylesheet;
} UnityStripPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityStrip, unity_strip, ASTAL_TYPE_WINDOW)

#define PRIV(o) ((UnityStripPrivate *) unity_strip_get_instance_private (UNITY_STRIP (o)))

typedef enum
{
  PROP_COLOR_SCHEME = 1,
  PROP_STYLESHEET,
} UnityStripProperty;

static GParamSpec *properties[PROP_STYLESHEET + 1];

static GSettings *
platform_settings (void)
{
  static GSettings *shared;

  if (G_UNLIKELY (shared == NULL))
    shared = g_settings_new (INTERFACE_SCHEMA);

  return shared;
}

AstalWindowAnchor
unity_strip_anchor_for_edge (GtkPositionType edge)
{
  AstalWindowAnchor top    = ASTAL_WINDOW_ANCHOR_TOP;
  AstalWindowAnchor bottom = ASTAL_WINDOW_ANCHOR_BOTTOM;
  AstalWindowAnchor left   = ASTAL_WINDOW_ANCHOR_LEFT;
  AstalWindowAnchor right  = ASTAL_WINDOW_ANCHOR_RIGHT;

  switch (edge)
    {
    case GTK_POS_LEFT:   return top | bottom | left;
    case GTK_POS_RIGHT:  return top | bottom | right;
    case GTK_POS_TOP:    return left | right | top;
    case GTK_POS_BOTTOM: return left | right | bottom;
    default:             return ASTAL_WINDOW_ANCHOR_NONE;
    }
}

static const gchar *
pos_class_for_anchor (AstalWindowAnchor anchor)
{
  AstalWindowAnchor top    = ASTAL_WINDOW_ANCHOR_TOP;
  AstalWindowAnchor bottom = ASTAL_WINDOW_ANCHOR_BOTTOM;
  AstalWindowAnchor left   = ASTAL_WINDOW_ANCHOR_LEFT;
  AstalWindowAnchor right  = ASTAL_WINDOW_ANCHOR_RIGHT;

  if (anchor == (top | bottom | left))
    return "pos-left";
  if (anchor == (top | bottom | right))
    return "pos-right";
  if (anchor == (left | right | top))
    return "pos-top";
  if (anchor == (left | right | bottom))
    return "pos-bottom";

  return NULL;
}

static void
apply_pos_class (UnityStrip *self)
{
  static const gchar *pos_classes[] = {
    "pos-top",
    "pos-bottom",
    "pos-left",
    "pos-right",
  };
  GtkWidget         *widget        = GTK_WIDGET (self);
  AstalWindowAnchor  anchor        = astal_window_get_anchor (ASTAL_WINDOW (self));
  const gchar       *matched_class = pos_class_for_anchor (anchor);

  for (gsize i = 0; i < G_N_ELEMENTS (pos_classes); i++)
    gtk_widget_remove_css_class (widget, pos_classes[i]);

  if (matched_class != NULL)
    gtk_widget_add_css_class (widget, matched_class);
}

static void
on_settings_scheme_changed (UnityStrip *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COLOR_SCHEME]);
}

gint
unity_strip_get_color_scheme (UnityStrip *self)
{
  g_return_val_if_fail (UNITY_IS_STRIP (self), G_DESKTOP_COLOR_SCHEME_DEFAULT);

  return g_settings_get_enum (platform_settings (), KEY_COLOR_SCHEME);
}

static void
unity_strip_realize (GtkWidget *widget)
{
  UnityStrip *self = UNITY_STRIP (widget);

  GTK_WIDGET_CLASS (unity_strip_parent_class)->realize (widget);

  unity_platform_style_ensure (gtk_widget_get_display (widget));
  if (PRIV (self)->stylesheet != NULL)
    unity_platform_load_stylesheet (gtk_widget_get_display (widget), PRIV (self)->stylesheet);

  g_signal_connect_object (platform_settings (), "changed::" KEY_COLOR_SCHEME,
                           G_CALLBACK (on_settings_scheme_changed), self,
                           G_CONNECT_SWAPPED);

  on_settings_scheme_changed (self);
}

static void
unity_strip_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  UnityStrip *self = UNITY_STRIP (object);

  switch ((UnityStripProperty) prop_id)
    {
    case PROP_COLOR_SCHEME:
      g_value_set_int (value, unity_strip_get_color_scheme (self));
      break;
    case PROP_STYLESHEET:
      g_value_set_string (value, PRIV (self)->stylesheet);
      break;
    }
}

static void
unity_strip_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  UnityStrip *self = UNITY_STRIP (object);

  switch ((UnityStripProperty) prop_id)
    {
    case PROP_COLOR_SCHEME:
      break;
    case PROP_STYLESHEET:
      g_free (PRIV (self)->stylesheet);
      PRIV (self)->stylesheet = g_value_dup_string (value);
      break;
    }
}

static void
unity_strip_finalize (GObject *object)
{
  g_free (PRIV (object)->stylesheet);
  G_OBJECT_CLASS (unity_strip_parent_class)->finalize (object);
}

static void
unity_strip_class_init (UnityStripClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = unity_strip_get_property;
  object_class->set_property = unity_strip_set_property;
  object_class->finalize     = unity_strip_finalize;

  widget_class->realize = unity_strip_realize;

  /**
   * UnityStrip:color-scheme:
   *
   * Current desktop color scheme from `org.gnome.desktop.interface`.
   */
  properties[PROP_COLOR_SCHEME] = g_param_spec_int (
    "color-scheme", NULL, NULL,
    G_DESKTOP_COLOR_SCHEME_DEFAULT, G_MAXINT,
    G_DESKTOP_COLOR_SCHEME_DEFAULT,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * UnityStrip:stylesheet:
   *
   * Optional CSS resource path loaded when the strip is realized.
   */
  properties[PROP_STYLESHEET] = g_param_spec_string (
    "stylesheet", NULL, NULL, NULL,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_strip_init (UnityStrip *self)
{
  gtk_widget_add_css_class (GTK_WIDGET (self), "unity-strip");

  g_signal_connect (self, "notify::anchor",
                    G_CALLBACK (apply_pos_class), NULL);
}
