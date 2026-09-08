/* unity-window.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-window.h"

#include <math.h>

#include "stylesheet-private.h"

#define UI_SCALE_MOBILE_TARGET_DPI     135
#define UI_SCALE_LARGE_TARGET_DPI      110
#define UI_SCALE_LARGE_MIN_SIZE_INCHES 20
#define SCALE_MAX_DENOMINATOR          4
#define SCALE_MIN_INTEGER              1
#define SCALE_MAX_INTEGER              4
#define MINIMUM_LOGICAL_AREA           (600 * 600)
#define MM_PER_INCH                    25.4

typedef struct
{
  gchar *stylesheet;
} UnityWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (UnityWindow, unity_window, ASTAL_TYPE_WINDOW)

#define PRIV(o) ((UnityWindowPrivate *) unity_window_get_instance_private (UNITY_WINDOW (o)))

typedef enum
{
  PROP_STYLESHEET = 1,
} UnityWindowProperty;

static GParamSpec *properties[PROP_STYLESHEET + 1];

static guint
highest_common_factor (guint a, guint b)
{
  while (b != 0)
    {
      guint t = b;
      b = a % b;
      a = t;
    }
  return a;
}

static gboolean
scale_is_valid_for_size (gint width, gint height, gdouble scale)
{
  if (scale < SCALE_MIN_INTEGER || scale > SCALE_MAX_INTEGER)
    return FALSE;

  gint64 logical_width  = (gint64) floor (width  / scale);
  gint64 logical_height = (gint64) floor (height / scale);

  return logical_width * logical_height >= MINIMUM_LOGICAL_AREA;
}

static gboolean
physical_size_is_aspect_ratio (gint width_mm, gint height_mm)
{
  return (width_mm == 16 && height_mm == 9)  ||
         (width_mm == 16 && height_mm == 10) ||
         (width_mm == 4  && height_mm == 3);
}

gdouble
unity_window_compute_optimal_scale (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), 1.0);

  gint physical_width  = gdk_monitor_get_width_mm  (monitor);
  gint physical_height = gdk_monitor_get_height_mm (monitor);

  if (physical_width <= 0 || physical_height <= 0)
    return 1.0;
  if (physical_size_is_aspect_ratio (physical_width, physical_height))
    return 1.0;

  GdkRectangle geometry;
  gdk_monitor_get_geometry (monitor, &geometry);

  gint width  = geometry.width;
  gint height = geometry.height;
  if (width <= 0 || height <= 0)
    return 1.0;

  gdouble diagonal_mm     = sqrt ((gdouble) physical_width  * physical_width +
                                  (gdouble) physical_height * physical_height);
  gdouble diagonal_inches = diagonal_mm / MM_PER_INCH;
  if (diagonal_inches <= 0.0)
    return 1.0;

  gint target_dpi = diagonal_inches < UI_SCALE_LARGE_MIN_SIZE_INCHES
                      ? UI_SCALE_MOBILE_TARGET_DPI
                      : UI_SCALE_LARGE_TARGET_DPI;

  gdouble physical_dpi  = sqrt ((gdouble) width * width + (gdouble) height * height) /
                          diagonal_inches;
  gdouble perfect_scale = physical_dpi / target_dpi;

  gdouble  best_scale = 1.0;
  gdouble  best_error = 0.0;
  gboolean have_best  = FALSE;

  for (guint denominator = 1; denominator <= SCALE_MAX_DENOMINATOR; denominator++)
    {
      for (guint numerator = SCALE_MIN_INTEGER * denominator;
           numerator <= SCALE_MAX_INTEGER * denominator;
           numerator++)
        {
          if (((guint) width  * denominator) % numerator != 0 ||
              ((guint) height * denominator) % numerator != 0)
            continue;

          if (highest_common_factor (numerator, denominator) > 1)
            continue;

          gdouble scale = (gdouble) numerator / denominator;

          if (!scale_is_valid_for_size (width, height, scale))
            continue;

          gdouble error = fabs (scale - perfect_scale);
          if (!have_best || error < best_error)
            {
              best_scale = scale;
              best_error = error;
              have_best  = TRUE;
            }
        }
    }

  return best_scale;
}

static void
unity_window_constructed (GObject *object)
{
  UnityWindow *self = UNITY_WINDOW (object);

  G_OBJECT_CLASS (unity_window_parent_class)->constructed (object);

  astal_window_set_anchor (ASTAL_WINDOW (self),
                           ASTAL_WINDOW_ANCHOR_TOP |
                           ASTAL_WINDOW_ANCHOR_BOTTOM |
                           ASTAL_WINDOW_ANCHOR_LEFT |
                           ASTAL_WINDOW_ANCHOR_RIGHT);
}

static void
unity_window_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (unity_window_parent_class)->realize (widget);

  unity_platform_style_ensure (gtk_widget_get_display (widget));
  if (PRIV (widget)->stylesheet != NULL)
    unity_platform_load_stylesheet (gtk_widget_get_display (widget),
                                    PRIV (widget)->stylesheet);
}

static void
unity_window_get_property (GObject *object, guint prop_id,
                           GValue *value, GParamSpec *pspec)
{
  switch ((UnityWindowProperty) prop_id)
    {
    case PROP_STYLESHEET:
      g_value_set_string (value, PRIV (object)->stylesheet);
      break;
    }
}

static void
unity_window_set_property (GObject *object, guint prop_id,
                           const GValue *value, GParamSpec *pspec)
{
  switch ((UnityWindowProperty) prop_id)
    {
    case PROP_STYLESHEET:
      g_free (PRIV (object)->stylesheet);
      PRIV (object)->stylesheet = g_value_dup_string (value);
      break;
    }
}

static void
unity_window_finalize (GObject *object)
{
  g_free (PRIV (object)->stylesheet);

  G_OBJECT_CLASS (unity_window_parent_class)->finalize (object);
}

static void
unity_window_class_init (UnityWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = unity_window_constructed;
  object_class->get_property = unity_window_get_property;
  object_class->set_property = unity_window_set_property;
  object_class->finalize     = unity_window_finalize;

  widget_class->realize = unity_window_realize;

  properties[PROP_STYLESHEET] = g_param_spec_string (
    "stylesheet", NULL, NULL, NULL,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_window_init (UnityWindow *self)
{
}

typedef struct
{
  GtkApplication         *application;
  UnityWindowMonitorFunc  factory;
  gpointer                user_data;
  GDestroyNotify          user_data_free;
  GListModel             *monitors;
  gulong                  monitors_changed_id;
  GHashTable             *windows;   /* GdkMonitor* → UnityWindow* */
} UnityWindowMonitorGroup;

static void
monitor_group_attach (UnityWindowMonitorGroup *group, GdkMonitor *monitor)
{
  if (g_hash_table_contains (group->windows, monitor))
    return;

  UnityWindow *window = group->factory (monitor, group->user_data);
  if (window == NULL)
    return;

  gtk_window_set_application  (GTK_WINDOW (window), group->application);
  astal_window_set_gdkmonitor (ASTAL_WINDOW (window), monitor);
  g_hash_table_insert (group->windows, monitor, window);
  gtk_window_present (GTK_WINDOW (window));
}

static void
monitor_group_detach (UnityWindowMonitorGroup *group, GdkMonitor *monitor)
{
  UnityWindow *window = g_hash_table_lookup (group->windows, monitor);

  if (window == NULL)
    return;

  gtk_window_destroy  (GTK_WINDOW (window));
  g_hash_table_remove (group->windows, monitor);
}

static void
monitor_group_sync (UnityWindowMonitorGroup *group)
{
  g_autoptr (GHashTable) present = g_hash_table_new (NULL, NULL);
  guint n = g_list_model_get_n_items (group->monitors);

  for (guint i = 0; i < n; i++)
    {
      g_autoptr (GdkMonitor) monitor = g_list_model_get_item (group->monitors, i);
      g_hash_table_add (present, monitor);
      monitor_group_attach (group, monitor);
    }

  g_autoptr (GPtrArray) stale = g_ptr_array_new ();
  GHashTableIter iter;
  gpointer       key, value;

  g_hash_table_iter_init (&iter, group->windows);
  while (g_hash_table_iter_next (&iter, &key, &value))
    if (!g_hash_table_contains (present, key))
      g_ptr_array_add (stale, key);

  for (guint i = 0; i < stale->len; i++)
    monitor_group_detach (group, g_ptr_array_index (stale, i));
}

static void
on_display_monitors_changed (GListModel *model,
                             guint       position,
                             guint       removed,
                             guint       added,
                             gpointer    user_data)
{
  monitor_group_sync (user_data);
}

static void
monitor_group_free (gpointer data)
{
  UnityWindowMonitorGroup *group = data;

  if (group->monitors != NULL && group->monitors_changed_id != 0)
    g_signal_handler_disconnect (group->monitors, group->monitors_changed_id);
  g_clear_object (&group->monitors);

  if (group->windows != NULL)
    {
      GHashTableIter iter;
      gpointer       key, value;

      g_hash_table_iter_init (&iter, group->windows);
      while (g_hash_table_iter_next (&iter, &key, &value))
        gtk_window_destroy (GTK_WINDOW (value));
      g_hash_table_destroy (group->windows);
    }

  if (group->user_data_free != NULL)
    group->user_data_free (group->user_data);
  g_free (group);
}

void
unity_window_present_for_each_monitor (GtkApplication         *application,
                                       UnityWindowMonitorFunc  factory,
                                       gpointer                user_data,
                                       GDestroyNotify          user_data_free)
{
  g_return_if_fail (GTK_IS_APPLICATION (application));
  g_return_if_fail (factory != NULL);

  UnityWindowMonitorGroup *group = g_new0 (UnityWindowMonitorGroup, 1);
  group->application    = application;
  group->factory        = factory;
  group->user_data      = user_data;
  group->user_data_free = user_data_free;
  group->windows        = g_hash_table_new (NULL, NULL);
  group->monitors       = g_object_ref (
    gdk_display_get_monitors (gdk_display_get_default ()));

  group->monitors_changed_id = g_signal_connect (
    group->monitors, "items-changed", G_CALLBACK (on_display_monitors_changed), group);

  g_object_set_data_full (G_OBJECT (application),
                          "unity-window-monitor-group",
                          group, monitor_group_free);

  monitor_group_sync (group);
}
