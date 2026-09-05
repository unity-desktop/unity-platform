name = 'unity-platform-translucency'

deps = [
  dependency('gtk4', version: '>= 4.22'),
  dependency('gio-2.0'),
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
]

sources = files(
  'unity-translucency.c',
)

headers = files(
  'unity-translucency.h',
)

incs = [
  include_directories('.'),
]

sources += gnome.compile_resources(
  name + '-resources',
  'translucency.gresource.xml',
  c_name: 'unity_translucency',
)

unity_platform_translucency_lib = library(
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
  unity_platform_translucency_lib,
  name: name,
  subdirs: name,
  requires: deps,
)

install_data(
  'org.unity.stylesheets.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

gnome.compile_schemas(
  depend_files: files('org.unity.stylesheets.gschema.xml'),
)

gnome.post_install(glib_compile_schemas: true)

gnome.generate_gir(
  unity_platform_translucency_lib,
  sources: sources + headers,
  namespace: 'UnityPlatformTranslucency',
  nsversion: '0',
  identifier_prefix: 'Unity',
  symbol_prefix: 'unity',
  includes: ['Gtk-4.0'],
  dependencies: deps,
  header: 'unity-translucency.h',
  include_directories: incs,
  install: true,
)
