name = 'unity-platform-quit'

deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
  dependency('gobject-2.0'),
  dependency('libsystemd'),
]

sources = files(
  'unity-quit.c',
)
headers = files(
  'unity-quit.h',
)
incs = [
  include_directories('.'),
]

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

gnome.generate_gir(
  lib,
  sources: sources + headers,
  namespace: 'UnityPlatformQuit',
  nsversion: meson.project_version(),
  identifier_prefix: 'Unity',
  symbol_prefix: 'unity',
  includes: ['Gio-2.0'],
  dependencies: deps,
  header: 'unity-quit.h',
  include_directories: incs,
  install: true,
)
