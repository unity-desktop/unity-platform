/* unity-translucency.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-translucency.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

#define SCHEMA_ID      "org.unity.stylesheets"
#define KEY_ON         "translucency"
#define SHEET_RESOURCE "/org/unity/platform/translucency/translucency.css"
#define PRIORITY       (GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1)

static GSettings      *settings;
static GtkCssProvider *sheet;
static gboolean        attached;
static gsize           init_guard;

static void
sync_provider (void)
{
  GdkDisplay *display = gdk_display_get_default ();
  if (display == NULL)
    return;

  gboolean enabled = g_settings_get_boolean (settings, KEY_ON);

  if (enabled && !attached)
    {
      if (sheet == NULL)
        {
          sheet = gtk_css_provider_new ();
          gtk_css_provider_load_from_resource (sheet, SHEET_RESOURCE);
        }
      gtk_style_context_add_provider_for_display (
        display, GTK_STYLE_PROVIDER (sheet), PRIORITY);
      attached = TRUE;
    }
  else if (!enabled && attached)
    {
      gtk_style_context_remove_provider_for_display (
        display, GTK_STYLE_PROVIDER (sheet));
      attached = FALSE;
    }
}

void
unity_translucency_init (void)
{
  if (g_once_init_enter (&init_guard))
    {
      settings = g_settings_new (SCHEMA_ID);
      g_signal_connect (settings, "changed::" KEY_ON,
                        G_CALLBACK (sync_provider), NULL);
      sync_provider ();
      g_once_init_leave (&init_guard, 1);
    }
}
