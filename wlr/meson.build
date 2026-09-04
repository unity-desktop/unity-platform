name = 'unity-platform-wlr'

deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gobject-2.0'),
  dependency('wayland-client'),
  dependency('astal-wl-0.1'),
]

fs = import('fs')

wayland_scanner = find_program('wayland-scanner')
wlr_protocol_dir = dependency('wlr-protocols').get_variable(pkgconfig: 'pkgdatadir')

protocols = [
  wlr_protocol_dir / 'unstable/wlr-foreign-toplevel-management-unstable-v1.xml',
  wlr_protocol_dir / 'unstable/wlr-output-power-management-unstable-v1.xml',
]

protocol_sources = []
foreach protocol : protocols
  header = custom_target(
    fs.name(protocol) + '_client_header',
    input: protocol,
    output: '@BASENAME@.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
  code = custom_target(
    fs.name(protocol) + '_private_code',
    input: protocol,
    output: '@BASENAME@.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
  protocol_sources += [header, code]
endforeach

sources = files(
  'unity-wlr-toplevel.c',
  'unity-wlr-toplevels.c',
  'unity-wlr-output-power.c',
)
headers = files(
  'unity-wlr-toplevels.h',
  'unity-wlr-output-power.h',
)
incs = [
  include_directories('.'),
]

lib = library(
  name,
  sources + protocol_sources,
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

unity_platform_wlr_gir = gnome.generate_gir(
  lib,
  sources: sources + headers,
  namespace: 'UnityPlatformWlr',
  nsversion: meson.project_version(),
  identifier_prefix: 'UnityWlr',
  symbol_prefix: 'unity_wlr',
  includes: ['Gio-2.0', 'AstalWl-0.1'],
  dependencies: deps,
  header: 'unity-wlr-toplevels.h',
  include_directories: incs,
  install: true,
)
