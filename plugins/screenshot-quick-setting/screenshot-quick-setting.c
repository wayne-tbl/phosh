/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "screenshot-quick-setting.h"

#include "screenshot-manager.h"
#include "shell.h"

#include <glib/gi18n.h>

/**
 * PhoshScreenshotQuickSetting:
 *
 * Take a screenshot from the drawer, after a delay.
 *
 * Tapping the tile folds the drawer away and starts the countdown. It does not
 * shoot immediately because a screenshot taken from the settings drawer is a
 * picture of the settings drawer, which is never what was wanted; the delay is
 * what buys time for the drawer to leave and for the user to bring up whatever
 * they meant to capture.
 *
 * How long that delay is comes from `screenshot-delay` in Settings, so the tile
 * has no status page and therefore no arrow: it does one thing when tapped.
 *
 * The timer deliberately does not live here. It belongs to the shell's
 * screenshot manager, because this widget is inside the very drawer that has
 * to close for the shot to be worth taking, and a timer owned by a widget that
 * is going away is a timer that may not fire.
 *
 * There is no check for a screenshot backend: the shell takes screenshots
 * itself over wlr-screencopy, so if phosh is running the tile can work.
 */

#define SHELL_SCHEMA_ID "io.furios.phosh.shell"
#define SCREENSHOT_DELAY_KEY "screenshot-delay"

struct _PhoshScreenshotQuickSetting {
  PhoshQuickSetting parent;

  PhoshStatusIcon  *info;

  GSettings        *settings;
};

G_DEFINE_TYPE (PhoshScreenshotQuickSetting,
               phosh_screenshot_quick_setting,
               PHOSH_TYPE_QUICK_SETTING);


static void
on_clicked (PhoshScreenshotQuickSetting *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshScreenshotManager *manager = phosh_shell_get_screenshot_manager (shell);
  int seconds = g_settings_get_int (self->settings, SCREENSHOT_DELAY_KEY);

  g_assert (PHOSH_IS_SCREENSHOT_QUICK_SETTING (self));

  /* Fold first: the delay only buys time for the drawer to get out of the way,
   * so nothing here should wait for the user to close it themselves. */
  phosh_shell_fold_top_panel (shell);

  phosh_screenshot_manager_take_screenshot_delayed (manager, seconds);
}


static void
phosh_screenshot_quick_setting_finalize (GObject *object)
{
  PhoshScreenshotQuickSetting *self = PHOSH_SCREENSHOT_QUICK_SETTING (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_screenshot_quick_setting_parent_class)->finalize (object);
}


static void
phosh_screenshot_quick_setting_class_init (PhoshScreenshotQuickSettingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_screenshot_quick_setting_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/screenshot-quick-setting/qs.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshScreenshotQuickSetting, info);
  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}


static void
phosh_screenshot_quick_setting_init (PhoshScreenshotQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (SHELL_SCHEMA_ID);

  g_object_set (self->info,
                "icon-name", "screenshot-portrait-symbolic",
                "info", _("Screenshot"),
                NULL);
}
