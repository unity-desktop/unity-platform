/* unity-quit-cli.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <glib.h>

#include "unity-quit.h"

typedef void (*ActionFn) (UnityQuit *self, gint argc, gchar **argv);

static void
logout (UnityQuit *self, gint argc, gchar **argv)
{
  unity_quit_logout (self);
}

static void
power_off (UnityQuit *self, gint argc, gchar **argv)
{
  unity_quit_power_off (self);
}

static void
reboot (UnityQuit *self, gint argc, gchar **argv)
{
  if (argc > 0 && g_strcmp0 (argv[0], "--firmware-setup") == 0)
    unity_quit_reboot_to_firmware_setup (self);
  else if (argc > 0 && g_strcmp0 (argv[0], "--bootloader-menu") == 0)
    unity_quit_reboot_to_boot_loader_menu (self);
  else
    unity_quit_reboot (self);
}

static void
suspend (UnityQuit *self, gint argc, gchar **argv)
{
  unity_quit_suspend (self);
}

static void
hibernate (UnityQuit *self, gint argc, gchar **argv)
{
  unity_quit_hibernate (self);
}

static void
lock (UnityQuit *self, gint argc, gchar **argv)
{
  unity_quit_lock (self);
}

static const struct {
  const gchar *name;
  ActionFn     fn;
} commands[] = {
  { "logout",    logout    },
  { "power-off", power_off },
  { "reboot",    reboot    },
  { "suspend",   suspend   },
  { "hibernate", hibernate },
  { "lock",      lock      },
};

static void
usage (void)
{
  g_printerr ("Usage: unity-quit COMMAND [OPTIONS]\n"
              "\n"
              "Commands:\n"
              "  logout      Log out (default)\n"
              "  power-off   Power off\n"
              "  reboot      Reboot [--firmware-setup | --bootloader-menu]\n"
              "  suspend     Suspend\n"
              "  hibernate   Hibernate\n"
              "  lock        Lock the session\n");
}

gint
main (gint argc, gchar **argv)
{
  for (gint i = 1; i < argc; i++)
    if (g_strcmp0 (argv[i], "--help") == 0 || g_strcmp0 (argv[i], "-h") == 0)
      {
        usage ();
        return 0;
      }

  const gchar *cmd = argc > 1 ? argv[1] : "logout";

  for (gsize i = 0; i < G_N_ELEMENTS (commands); i++)
    {
      if (g_strcmp0 (commands[i].name, cmd) == 0)
        {
          UnityQuit *quit = unity_quit_get_default ();
          commands[i].fn (quit, argc - 2, argv + 2);
          unity_quit_flush (quit);
          return 0;
        }
    }

  g_printerr ("unity-quit: unknown command '%s'\n\n", cmd);
  usage ();
  return 1;
}
