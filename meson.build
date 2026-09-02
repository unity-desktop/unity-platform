project(
  'unity-platform',
  'c',
  version: '26',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 1.3.0',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'werror=false',
    'b_ndebug=if-release',
  ],
)

cc = meson.get_compiler('c')

add_project_arguments(
  cc.get_supported_arguments(['-Wno-unused-parameter']),
  language: 'c',
)

gnome = import('gnome')
pkgconfig = import('pkgconfig')

if get_option('build-components')
  subdir('components')
endif
