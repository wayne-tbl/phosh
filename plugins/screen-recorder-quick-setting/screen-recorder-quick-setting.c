/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "screen-recorder-quick-setting.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>

/**
 * PhoshScreenRecorderQuickSetting:
 *
 * Start and stop a screen recording from the drawer.
 *
 * The recording itself is wf-recorder, driven by a small script rather than
 * spawned directly. That keeps the flags this device needs -- and the reasons
 * for them -- in one readable place, and lets the same recording be started
 * from a terminal or a shortcut without duplicating them here.
 *
 * State lives in the pid file the script writes, not in this widget: a
 * recording begun elsewhere still shows as active here, and one still running
 * when the shell restarts is picked up again rather than orphaned.
 *
 * Without wf-recorder installed there is nothing this tile could do, so it
 * takes itself out of the drawer entirely rather than sitting there greyed
 * out: a control that can never work is worse than no control. Whether the
 * user wants the tile at all is the separate question of whether the plugin is
 * listed in the shell's quick-settings key, which the drawer already watches
 * and reloads on.
 */

#define SCREENREC_TIMEOUT_SECS 1

struct _PhoshScreenRecorderQuickSetting {
  PhoshQuickSetting parent;

  PhoshStatusIcon  *info;
  char             *script;
  char             *pidfile;
  guint             poll_id;
  gint64            started;
  gboolean          recording;
};

G_DEFINE_TYPE (PhoshScreenRecorderQuickSetting,
               phosh_screen_recorder_quick_setting,
               PHOSH_TYPE_QUICK_SETTING);


static gboolean
is_recording (PhoshScreenRecorderQuickSetting *self)
{
  g_autofree char *contents = NULL;
  gint64 pid;

  if (!g_file_get_contents (self->pidfile, &contents, NULL, NULL))
    return FALSE;

  pid = g_ascii_strtoll (contents, NULL, 10);
  if (pid <= 0)
    return FALSE;

  /* Signal 0 asks whether the process exists without touching it, so a pid
   * file left behind by a crash does not wedge the button on. */
  return kill ((pid_t) pid, 0) == 0;
}


static void
update_state (PhoshScreenRecorderQuickSetting *self)
{
  gboolean recording = is_recording (self);

  if (recording && !self->recording)
    self->started = g_get_monotonic_time ();

  self->recording = recording;

  if (recording) {
    int secs = (int) ((g_get_monotonic_time () - self->started) / G_USEC_PER_SEC);
    g_autofree char *info = g_strdup_printf (_("Recording %d:%02d"), secs / 60, secs % 60);

    g_object_set (self->info, "icon-name", "media-record-symbolic", NULL);
    g_object_set (self->info, "info", info, NULL);
  } else {
    g_object_set (self->info, "icon-name", "camera-video-symbolic", NULL);
    g_object_set (self->info, "info", _("Record Screen"), NULL);
  }

  g_object_set (self, "active", recording, NULL);
}


static gboolean
on_poll (gpointer data)
{
  update_state (PHOSH_SCREEN_RECORDER_QUICK_SETTING (data));

  return G_SOURCE_CONTINUE;
}


static void
on_clicked (PhoshScreenRecorderQuickSetting *self)
{
  g_autoptr (GError) err = NULL;
  char *argv[] = { self->script, NULL };

  g_assert (PHOSH_IS_SCREEN_RECORDER_QUICK_SETTING (self));

  /* Hidden because wf-recorder is missing: nothing was set up, and there is
   * nothing to run. */
  if (self->script == NULL)
    return;

  /* The script is a toggle, so the same call starts and stops. Spawned
   * asynchronously: stopping waits for the encoder to flush, and the shell
   * must not block on that. */
  if (!g_spawn_async (NULL, argv, NULL,
                      G_SPAWN_DEFAULT | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                      NULL, NULL, NULL, &err)) {
    g_warning ("Could not run %s: %s", self->script, err->message);
    return;
  }

  /* The pid file appears or disappears a moment later, so let the poll pick
   * the change up rather than guessing at the new state here. */
}


static void
phosh_screen_recorder_quick_setting_finalize (GObject *object)
{
  PhoshScreenRecorderQuickSetting *self = PHOSH_SCREEN_RECORDER_QUICK_SETTING (object);

  g_clear_handle_id (&self->poll_id, g_source_remove);
  g_clear_pointer (&self->script, g_free);
  g_clear_pointer (&self->pidfile, g_free);

  G_OBJECT_CLASS (phosh_screen_recorder_quick_setting_parent_class)->finalize (object);
}


static void
phosh_screen_recorder_quick_setting_class_init (PhoshScreenRecorderQuickSettingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_screen_recorder_quick_setting_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/screen-recorder-quick-setting/qs.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshScreenRecorderQuickSetting, info);
  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}


static void
phosh_screen_recorder_quick_setting_init (PhoshScreenRecorderQuickSetting *self)
{
  const char *runtime = g_get_user_runtime_dir ();
  g_autofree char *recorder = NULL;
  g_autofree char *local = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  /* Checked here rather than at each click so the tile is never offered in the
   * first place. no-show-all is what makes that stick: the drawer only ever
   * parents its children and then shows the lot, so a widget that opts out of
   * show_all is simply never realized.
   *
   * Both halves have to be there. wf-recorder is a hard dependency, so this is
   * the defensive half of the check -- a dependency can still be forced out --
   * while the script, which is what actually knows this device's flags, is the
   * half that can genuinely be missing. A tile that spawns a missing file looks
   * broken in exactly the way an absent tile does not. */
  recorder = g_find_program_in_path ("wf-recorder");
  if (recorder == NULL) {
    g_message ("wf-recorder not installed, hiding the screen recorder quick setting");
    gtk_widget_set_no_show_all (GTK_WIDGET (self), TRUE);
    gtk_widget_hide (GTK_WIDGET (self));
    return;
  }

  /* A copy in ~/.local/bin wins, so the script can be edited and tried without
   * reinstalling the package it ships in. */
  local = g_build_filename (g_get_home_dir (), ".local", "bin", "screenrec", NULL);
  if (g_file_test (local, G_FILE_TEST_IS_EXECUTABLE))
    self->script = g_steal_pointer (&local);
  else if (g_file_test (SCREENREC_PATH, G_FILE_TEST_IS_EXECUTABLE))
    self->script = g_strdup (SCREENREC_PATH);

  if (self->script == NULL) {
    g_message ("No screenrec script, hiding the screen recorder quick setting");
    gtk_widget_set_no_show_all (GTK_WIDGET (self), TRUE);
    gtk_widget_hide (GTK_WIDGET (self));
    return;
  }

  self->pidfile = g_build_filename (runtime ?: "/tmp", "screenrec.pid", NULL);

  update_state (self);
  self->poll_id = g_timeout_add_seconds (SCREENREC_TIMEOUT_SECS, on_poll, self);
}
