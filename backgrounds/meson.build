name = 'unity-platform-backgrounds'

cc = meson.get_compiler('c')

deps = [
  dependency('gtk4', version: '>= 4.22'),
  dependency('gio-2.0'),
  dependency('glib-2.0', version: '>= 2.80'),
  dependency('gobject-2.0'),
  dependency('gnome-bg-4'),
  dependency('gsettings-desktop-schemas'),
]

link_deps = deps + [ cc.find_library('m', required: false) ]

sources = files(
  'unity-backgrounds-source.c',
)

headers = files(
  'unity-backgrounds-source.h',
)

incs = [
  include_directories('.'),
]

unity_platform_backgrounds_lib = library(
  name,
  sources,
  dependencies: link_deps,
  include_directories: incs,
  version: meson.project_version(),
  soversion: '0',
  install: true,
)

install_headers(headers, subdir: name)

pkgconfig.generate(
  unity_platform_backgrounds_lib,
  name: name,
  subdirs: name,
  requires: deps,
)

unity_platform_backgrounds_dep = declare_dependency(
  link_with: unity_platform_backgrounds_lib,
  include_directories: incs,
  dependencies: deps,
)

gnome.generate_gir(
  unity_platform_backgrounds_lib,
  sources: sources + headers,
  namespace: 'UnityPlatformBackgrounds',
  nsversion: '0',
  identifier_prefix: 'UnityBackgrounds',
  symbol_prefix: 'unity_backgrounds',
  includes: ['Gtk-4.0'],
  dependencies: deps,
  header: 'unity-backgrounds-source.h',
  include_directories: incs,
  install: true,
)
