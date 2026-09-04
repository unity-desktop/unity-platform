/* unity-wlr-toplevels.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-wlr-toplevels.h"

#include <wayland-client.h>

#include "unity-wlr-toplevel.h"

#define MANAGER_INTERFACE "zwlr_foreign_toplevel_manager_v1"
#define MANAGER_VERSION   3

typedef enum
{
  PROP_N_ITEMS = 1,
} UnityWlrToplevelsProperty;

static GParamSpec *properties[PROP_N_ITEMS + 1];

struct _UnityWlrToplevels
{
  GObject                                  parent_instance;

  struct zwlr_foreign_toplevel_manager_v1 *manager;
  GPtrArray                               *items;
};

static void
list_model_iface_init (GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (UnityWlrToplevels, unity_wlr_toplevels, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static GType
list_get_item_type (GListModel *model)
{
  return UNITY_WLR_TYPE_TOPLEVEL;
}

static guint
list_get_n_items (GListModel *model)
{
  return UNITY_WLR_TOPLEVELS (model)->items->len;
}

static gpointer
list_get_item (GListModel *model, guint position)
{
  UnityWlrToplevels *self = UNITY_WLR_TOPLEVELS (model);
  if (position >= self->items->len)
    return NULL;
  return g_object_ref (g_ptr_array_index (self->items, position));
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_get_item_type;
  iface->get_n_items   = list_get_n_items;
  iface->get_item      = list_get_item;
}

static void
remove_toplevel (UnityWlrToplevels *self, UnityWlrToplevel *tl)
{
  guint index;
  if (!g_ptr_array_find (self->items, tl, &index))
    return;

  g_ptr_array_remove_index (self->items, index);
  g_list_model_items_changed (G_LIST_MODEL (self), index, 1, 0);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

static void
on_toplevel_closed (UnityWlrToplevel *tl, gpointer user_data)
{
  remove_toplevel (UNITY_WLR_TOPLEVELS (user_data), tl);
}

static void
manager_handle_toplevel (gpointer user_data,
                         struct zwlr_foreign_toplevel_manager_v1 *manager,
                         struct zwlr_foreign_toplevel_handle_v1  *handle)
{
  UnityWlrToplevels *self = user_data;
  UnityWlrToplevel  *tl   = unity_wlr_toplevel_new (handle);
  guint              at   = self->items->len;

  g_ptr_array_add (self->items, tl);
  g_signal_connect_object (tl, "closed", G_CALLBACK (on_toplevel_closed), self, G_CONNECT_DEFAULT);
  g_list_model_items_changed (G_LIST_MODEL (self), at, 0, 1);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

static void
manager_handle_finished (gpointer user_data,
                         struct zwlr_foreign_toplevel_manager_v1 *manager)
{
  UnityWlrToplevels *self = user_data;
  g_clear_pointer (&self->manager, zwlr_foreign_toplevel_manager_v1_destroy);
}

static const struct zwlr_foreign_toplevel_manager_v1_listener manager_listener = {
  manager_handle_toplevel,
  manager_handle_finished,
};

static void
unity_wlr_toplevels_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityWlrToplevels *self = UNITY_WLR_TOPLEVELS (object);

  switch ((UnityWlrToplevelsProperty) prop_id)
    {
    case PROP_N_ITEMS:
      g_value_set_uint (value, self->items->len);
      break;
    }
}

static void
unity_wlr_toplevels_finalize (GObject *object)
{
  UnityWlrToplevels *self = UNITY_WLR_TOPLEVELS (object);

  g_clear_pointer (&self->manager, zwlr_foreign_toplevel_manager_v1_destroy);
  g_clear_pointer (&self->items, g_ptr_array_unref);

  G_OBJECT_CLASS (unity_wlr_toplevels_parent_class)->finalize (object);
}

static void
unity_wlr_toplevels_class_init (UnityWlrToplevelsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = unity_wlr_toplevels_get_property;
  object_class->finalize     = unity_wlr_toplevels_finalize;

  properties[PROP_N_ITEMS] = g_param_spec_uint (
    "n-items", NULL, NULL, 0, G_MAXUINT, 0,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_wlr_toplevels_init (UnityWlrToplevels *self)
{
  self->items = g_ptr_array_new_with_free_func (g_object_unref);

  AstalWlRegistry *registry = astal_wl_registry_get_default ();
  GList           *globals  = astal_wl_registry_find_globals (registry, MANAGER_INTERFACE);
  AstalWlGlobal   *global   = globals != NULL ? globals->data : NULL;
  if (global == NULL)
    {
      g_critical ("unity-wlr: compositor does not support %s", MANAGER_INTERFACE);
      g_list_free (globals);
      return;
    }

  guint32 version = MIN (global->version, (guint32) MANAGER_VERSION);
  self->manager = wl_registry_bind (astal_wl_registry_get_registry (registry),
                                    global->name,
                                    &zwlr_foreign_toplevel_manager_v1_interface,
                                    version);
  zwlr_foreign_toplevel_manager_v1_add_listener (self->manager, &manager_listener, self);

  astal_wl_registry_roundtrip (registry);
  astal_wl_registry_roundtrip (registry);

  g_list_free (globals);
}

UnityWlrToplevels *
unity_wlr_toplevels_get_default (void)
{
  static UnityWlrToplevels *instance;
  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, g_object_new (UNITY_WLR_TYPE_TOPLEVELS, NULL));
  return instance;
}
