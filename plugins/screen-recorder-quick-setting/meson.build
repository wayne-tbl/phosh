name = 'screen-recorder-quick-setting'

screen_recorder_quick_setting_resources = gnome.compile_resources(
  'phosh-plugin-screen-recorder-quick-setting-resources',
  'phosh-plugin-screen-recorder-quick-setting.gresources.xml',
  c_name: 'phosh_plugin_screen_recorder_quick_setting',
)

screen_recorder_quick_setting_plugin_sources = files(
  'screen-recorder-quick-setting.c',
  'phosh-plugin-screen-recorder-quick-setting.c',
)

# The plugin only starts and stops it, so the two ship together.
# Installed as a file directly in libexecdir, not into a phosh/ subdirectory:
# /usr/libexec/phosh is the shell binary itself, so a directory of that name
# cannot exist beside it.
install_data(
  'screenrec',
  install_dir: libexecdir,
  rename: 'phosh-screenrec',
  install_mode: 'rwxr-xr-x',
)

# Identity for the recorder's notifications; NoDisplay, so not in the app grid.
install_data(
  'mobi.phosh.ScreenRecorder.desktop',
  install_dir: desktopdir,
)

phosh_screen_recorder_quick_setting_plugin = shared_module(
  'phosh-plugin-screen-recorder-quick-setting',
  screen_recorder_quick_setting_plugin_sources,
  screen_recorder_quick_setting_resources,
  c_args: [
    '-DG_LOG_DOMAIN="phosh-plugin-@0@"'.format(name),
    '-DPLUGIN_NAME="@0@"'.format(name),
    '-DSCREENREC_PATH="@0@"'.format(join_paths(libexecdir, 'phosh-screenrec')),
  ],
  dependencies: plugin_dep,
  include_directories: phosh_inc,
  install: true,
  install_dir: plugins_dir,
)

pluginconf = configuration_data()
pluginconf.set('name', name)
pluginconf.set('plugins_dir', plugins_dir)
pluginconf.set('plugin_prefs_dir', plugin_prefs_dir)

i18n.merge_file(
  input: configure_file(
    input: name + '.desktop.in.in',
    output: name + '.desktop.in',
    configuration: pluginconf,
  ),
  output: name + '.plugin',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: plugins_dir,
  type: 'desktop',
)
