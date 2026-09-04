/* unity-wlr-toplevel.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-wlr-toplevel.h"

#include <wayland-client.h>

typedef enum
{
  PROP_APP_ID = 1,
  PROP_TITLE,
  PROP_ACTIVATED,
  PROP_MINIMIZED,
  PROP_MAXIMIZED,
  PROP_FULLSCREEN,
  PROP_PARENT,
} UnityWlrToplevelProperty;

static GParamSpec *properties[PROP_PARENT + 1];

typedef enum
{
  SIGNAL_CLOSED,
} UnityWlrToplevelSignal;

static guint signals[SIGNAL_CLOSED + 1];

struct _UnityWlrToplevel
{
  GObject                                 parent_instance;

  struct zwlr_foreign_toplevel_handle_v1 *handle;
  gchar                                  *app_id;
  gchar                                  *title;
  gboolean                                activated;
  gboolean                                minimized;
  gboolean                                maximized;
  gboolean                                fullscreen;
  GWeakRef                                parent;
  GPtrArray                              *outputs;
};

G_DEFINE_FINAL_TYPE (UnityWlrToplevel, unity_wlr_toplevel, G_TYPE_OBJECT)

const gchar *
unity_wlr_toplevel_get_app_id (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), NULL);
  return self->app_id;
}

const gchar *
unity_wlr_toplevel_get_title (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), NULL);
  return self->title;
}

gboolean
unity_wlr_toplevel_get_activated (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), FALSE);
  return self->activated;
}

gboolean
unity_wlr_toplevel_get_minimized (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), FALSE);
  return self->minimized;
}

gboolean
unity_wlr_toplevel_get_maximized (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), FALSE);
  return self->maximized;
}

gboolean
unity_wlr_toplevel_get_fullscreen (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), FALSE);
  return self->fullscreen;
}

UnityWlrToplevel *
unity_wlr_toplevel_get_parent (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), NULL);
  return g_weak_ref_get (&self->parent);
}

GList *
unity_wlr_toplevel_get_outputs (UnityWlrToplevel *self)
{
  g_return_val_if_fail (UNITY_WLR_IS_TOPLEVEL (self), NULL);

  GList *list = NULL;
  for (guint i = 0; i < self->outputs->len; i++)
    list = g_list_prepend (list, g_ptr_array_index (self->outputs, i));
  return g_list_reverse (list);
}

static struct wl_seat *
default_seat (void)
{
  static struct wl_seat *cached;
  if (cached == NULL)
    {
      GList *seats = astal_wl_registry_get_seats (astal_wl_registry_get_default ());
      if (seats != NULL && seats->data != NULL)
        cached = astal_wl_seat_get_wl_seat (seats->data);
      g_list_free (seats);
    }
  return cached;
}

void
unity_wlr_toplevel_activate (UnityWlrToplevel *self)
{
  g_return_if_fail (UNITY_WLR_IS_TOPLEVEL (self));
  if (self->handle == NULL)
    return;

  struct wl_seat *seat = default_seat ();
  if (seat == NULL)
    {
      g_warning ("unity-wlr: cannot activate without a wayland seat");
      return;
    }

  zwlr_foreign_toplevel_handle_v1_activate (self->handle, seat);
}

void
unity_wlr_toplevel_close (UnityWlrToplevel *self)
{
  g_return_if_fail (UNITY_WLR_IS_TOPLEVEL (self));
  if (self->handle != NULL)
    zwlr_foreign_toplevel_handle_v1_close (self->handle);
}

void
unity_wlr_toplevel_minimize (UnityWlrToplevel *self, gboolean minimize)
{
  g_return_if_fail (UNITY_WLR_IS_TOPLEVEL (self));
  if (self->handle == NULL)
    return;

  if (minimize)
    zwlr_foreign_toplevel_handle_v1_set_minimized (self->handle);
  else
    zwlr_foreign_toplevel_handle_v1_unset_minimized (self->handle);
}

void
unity_wlr_toplevel_set_rectangle (UnityWlrToplevel  *self,
                                  struct wl_surface *surface,
                                  gint               x,
                                  gint               y,
                                  gint               width,
                                  gint               height)
{
  g_return_if_fail (UNITY_WLR_IS_TOPLEVEL (self));
  if (self->handle != NULL)
    zwlr_foreign_toplevel_handle_v1_set_rectangle (self->handle, surface, x, y, width, height);
}

static void
handle_title (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h, const gchar *title)
{
  UnityWlrToplevel *self = user_data;
  if (g_set_str (&self->title, title))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

static void
handle_app_id (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h, const gchar *app_id)
{
  UnityWlrToplevel *self = user_data;
  if (g_set_str (&self->app_id, app_id))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_APP_ID]);
}

static void
handle_output_enter (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h,
                     struct wl_output *output)
{
  UnityWlrToplevel *self = user_data;
  AstalWlOutput    *aout = astal_wl_registry_get_output_by_wl_output (
    astal_wl_registry_get_default (), output);
  if (aout != NULL && !g_ptr_array_find (self->outputs, aout, NULL))
    g_ptr_array_add (self->outputs, aout);
}

static void
handle_output_leave (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h,
                     struct wl_output *output)
{
  UnityWlrToplevel *self = user_data;
  AstalWlOutput    *aout = astal_wl_registry_get_output_by_wl_output (
    astal_wl_registry_get_default (), output);
  if (aout != NULL)
    g_ptr_array_remove (self->outputs, aout);
}

static void
on_output_removed (AstalWlRegistry *registry, AstalWlOutput *output, gpointer user_data)
{
  UnityWlrToplevel *self = user_data;
  g_ptr_array_remove (self->outputs, output);
}

static void
handle_state (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h,
              struct wl_array *state)
{
  UnityWlrToplevel *self = user_data;
  gboolean activated = FALSE, minimized = FALSE, maximized = FALSE, fullscreen = FALSE;

  guint32 *entry;
  wl_array_for_each (entry, state)
    {
      switch (*entry)
        {
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:  activated  = TRUE; break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:  minimized  = TRUE; break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED:  maximized  = TRUE; break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN: fullscreen = TRUE; break;
        default: break;
        }
    }

  g_object_freeze_notify (G_OBJECT (self));
  if (self->activated != activated)
    {
      self->activated = activated;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVATED]);
    }
  if (self->minimized != minimized)
    {
      self->minimized = minimized;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MINIMIZED]);
    }
  if (self->maximized != maximized)
    {
      self->maximized = maximized;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAXIMIZED]);
    }
  if (self->fullscreen != fullscreen)
    {
      self->fullscreen = fullscreen;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FULLSCREEN]);
    }
  g_object_thaw_notify (G_OBJECT (self));
}

static void
handle_done (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h)
{
}

static void
handle_closed (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h)
{
  UnityWlrToplevel *self = user_data;

  g_clear_pointer (&self->handle, zwlr_foreign_toplevel_handle_v1_destroy);
  g_signal_emit (self, signals[SIGNAL_CLOSED], 0);
}

static void
handle_parent (gpointer user_data, struct zwlr_foreign_toplevel_handle_v1 *h,
               struct zwlr_foreign_toplevel_handle_v1 *parent)
{
  UnityWlrToplevel *self = user_data;
  UnityWlrToplevel *pobj = parent != NULL
    ? zwlr_foreign_toplevel_handle_v1_get_user_data (parent)
    : NULL;

  g_weak_ref_set (&self->parent, pobj);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PARENT]);
}

static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
  handle_title,
  handle_app_id,
  handle_output_enter,
  handle_output_leave,
  handle_state,
  handle_done,
  handle_closed,
  handle_parent,
};

UnityWlrToplevel *
unity_wlr_toplevel_new (struct zwlr_foreign_toplevel_handle_v1 *handle)
{
  UnityWlrToplevel *self = g_object_new (UNITY_WLR_TYPE_TOPLEVEL, NULL);

  self->handle = handle;
  zwlr_foreign_toplevel_handle_v1_set_user_data (handle, self);
  zwlr_foreign_toplevel_handle_v1_add_listener (handle, &toplevel_handle_listener, self);
  return self;
}

static void
unity_wlr_toplevel_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityWlrToplevel *self = UNITY_WLR_TOPLEVEL (object);

  switch ((UnityWlrToplevelProperty) prop_id)
    {
    case PROP_APP_ID:     g_value_set_string  (value, self->app_id);     break;
    case PROP_TITLE:      g_value_set_string  (value, self->title);      break;
    case PROP_ACTIVATED:  g_value_set_boolean (value, self->activated);  break;
    case PROP_MINIMIZED:  g_value_set_boolean (value, self->minimized);  break;
    case PROP_MAXIMIZED:  g_value_set_boolean (value, self->maximized);  break;
    case PROP_FULLSCREEN: g_value_set_boolean (value, self->fullscreen); break;
    case PROP_PARENT:     g_value_take_object (value, g_weak_ref_get (&self->parent)); break;
    }
}

static void
unity_wlr_toplevel_finalize (GObject *object)
{
  UnityWlrToplevel *self = UNITY_WLR_TOPLEVEL (object);

  g_clear_pointer (&self->handle, zwlr_foreign_toplevel_handle_v1_destroy);
  g_free (self->app_id);
  g_free (self->title);
  g_weak_ref_clear (&self->parent);
  g_clear_pointer (&self->outputs, g_ptr_array_unref);

  G_OBJECT_CLASS (unity_wlr_toplevel_parent_class)->finalize (object);
}

static void
unity_wlr_toplevel_class_init (UnityWlrToplevelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = unity_wlr_toplevel_get_property;
  object_class->finalize     = unity_wlr_toplevel_finalize;

  properties[PROP_APP_ID] = g_param_spec_string (
    "app-id", NULL, NULL, NULL,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_TITLE] = g_param_spec_string (
    "title", NULL, NULL, NULL,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_ACTIVATED] = g_param_spec_boolean (
    "activated", NULL, NULL, FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_MINIMIZED] = g_param_spec_boolean (
    "minimized", NULL, NULL, FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_MAXIMIZED] = g_param_spec_boolean (
    "maximized", NULL, NULL, FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_FULLSCREEN] = g_param_spec_boolean (
    "fullscreen", NULL, NULL, FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_PARENT] = g_param_spec_object (
    "parent", NULL, NULL, UNITY_WLR_TYPE_TOPLEVEL,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  signals[SIGNAL_CLOSED] = g_signal_new (
    "closed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
    0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
unity_wlr_toplevel_init (UnityWlrToplevel *self)
{
  self->outputs = g_ptr_array_new ();
  g_weak_ref_init (&self->parent, NULL);

  g_signal_connect_object (astal_wl_registry_get_default (), "output-removed",
                           G_CALLBACK (on_output_removed), self, G_CONNECT_DEFAULT);
}
