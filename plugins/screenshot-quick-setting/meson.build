name = 'screenshot-quick-setting'

screenshot_quick_setting_resources = gnome.compile_resources(
  'phosh-plugin-screenshot-quick-setting-resources',
  'phosh-plugin-screenshot-quick-setting.gresources.xml',
  c_name: 'phosh_plugin_screenshot_quick_setting',
)

screenshot_quick_setting_plugin_sources = files(
  'screenshot-quick-setting.c',
  'phosh-plugin-screenshot-quick-setting.c',
)

phosh_screenshot_quick_setting_plugin = shared_module(
  'phosh-plugin-screenshot-quick-setting',
  screenshot_quick_setting_plugin_sources,
  screenshot_quick_setting_resources,
  c_args: [
    '-DG_LOG_DOMAIN="phosh-plugin-@0@"'.format(name),
    '-DPLUGIN_NAME="@0@"'.format(name),
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
