/* platform-style.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stylesheet-private.h"

#include <gdesktop-enums.h>

#define INTERFACE_SCHEMA         "org.gnome.desktop.interface"
#define KEY_COLOR_SCHEME         "color-scheme"
#define STYLE_RESOURCE_PATH      "/org/unity/platform/components/components.css"

static GSettings *
platform_settings (void)
{
  static GSettings *shared;

  if (G_UNLIKELY (shared == NULL))
    shared = g_settings_new (INTERFACE_SCHEMA);

  return shared;
}

static void
apply_scheme_to_provider (GtkCssProvider *provider)
{
  GDesktopColorScheme     current_scheme  = g_settings_get_enum (platform_settings (), KEY_COLOR_SCHEME);
  GtkInterfaceColorScheme provider_scheme =
    current_scheme == G_DESKTOP_COLOR_SCHEME_PREFER_LIGHT
      ? GTK_INTERFACE_COLOR_SCHEME_LIGHT
      : GTK_INTERFACE_COLOR_SCHEME_DARK;

  g_object_set (provider, "prefers-color-scheme", provider_scheme, NULL);
}

void
unity_platform_style_ensure (GdkDisplay *display)
{
  static GQuark   provider_quark;
  GtkCssProvider *provider;

  g_return_if_fail (GDK_IS_DISPLAY (display));

  if (G_UNLIKELY (provider_quark == 0))
    provider_quark = g_quark_from_static_string ("unity-platform-provider");

  if (g_object_get_qdata (G_OBJECT (display), provider_quark) != NULL)
    return;

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, STYLE_RESOURCE_PATH);
  gtk_style_context_add_provider_for_display (
    display, GTK_STYLE_PROVIDER (provider),
    GTK_STYLE_PROVIDER_PRIORITY_THEME + 1);
  apply_scheme_to_provider (provider);

  g_signal_connect_object (platform_settings (), "changed::" KEY_COLOR_SCHEME,
                           G_CALLBACK (apply_scheme_to_provider), provider,
                           G_CONNECT_SWAPPED);

  g_object_set_qdata_full (G_OBJECT (display), provider_quark, provider, g_object_unref);
}

void
unity_platform_load_stylesheet (GdkDisplay *display, const gchar *resource_path)
{
  g_return_if_fail (GDK_IS_DISPLAY (display));
  g_return_if_fail (resource_path != NULL);

  if (!g_resources_get_info (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
    return;

  g_autoptr (GtkCssProvider) provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, resource_path);
  gtk_style_context_add_provider_for_display (
    display, GTK_STYLE_PROVIDER (provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
