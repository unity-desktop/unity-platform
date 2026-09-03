name = 'unity-platform-components'

deps = [
  dependency('gtk4-layer-shell-0', version: '>= 1.0'),
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gtk4', version: '>= 4.20'),
  dependency('libadwaita-1', version: '>= 1.9'),
  dependency('astal-4-4.0'),
  dependency('gsettings-desktop-schemas'),
]

sources = files(
  'stylesheet.c',
  'strip/unity-strip.c',
  'popup/unity-base-popup.c',
  'popup/unity-dialog-popup.c',
  'popup/unity-window-popup.c',
)
headers = files(
  'strip/unity-strip.h',
  'popup/unity-base-popup.h',
  'popup/unity-dialog-popup.h',
  'popup/unity-window-popup.h',
)
incs = [
  include_directories('.'),
  include_directories('strip'),
  include_directories('popup'),
]

sources += gnome.compile_resources(
  name + '-resources',
  'components.gresource.xml',
  c_name: 'unity_platform_components',
)

lib = library(
  name,
  sources,
  dependencies: deps,
  include_directories: incs,
  version: meson.project_version(),
  soversion: '0',
  install: true,
)

install_headers(headers, subdir: name)

pkgconfig.generate(
  lib,
  name: name,
  subdirs: name,
  requires: deps,
)

unity_platform_components_gir = gnome.generate_gir(
  lib,
  sources: sources + headers,
  namespace: 'UnityPlatformComponents',
  nsversion: meson.project_version(),
  identifier_prefix: 'Unity',
  symbol_prefix: 'unity',
  includes: ['Gtk-4.0', 'Adw-1', 'Astal-4.0'],
  dependencies: deps,
  header: 'unity-strip.h',
  include_directories: incs,
  install: true,
)
