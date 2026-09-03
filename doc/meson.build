dependency('gi-docgen', version: '>= 2021.1',
           fallback: ['gi-docgen', 'dummy_dep'],
           native: true,
           required: get_option('documentation'))

gidocgen = find_program('gi-docgen')

doc_index = configure_file(
  input: 'index.html.in',
  output: 'index.html',
  configuration: {
    'PROJECT_VERSION': meson.project_version(),
  },
)

if get_option('build-components')
  components_toml = configure_file(
    input: 'unity-platform-components.toml.in',
    output: 'unity-platform-components.toml',
    configuration: {'VERSION': meson.project_version()},
  )

  custom_target('unity-platform-components-doc',
    input: [components_toml, unity_platform_components_gir[0]],
    output: 'unity-platform-components',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--no-namespace-dir',
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    depend_files: files('urlmap.js'),
    build_by_default: true,
    install: true,
    install_dir: get_option('datadir') / 'doc' / meson.project_name(),
  )

  test('doc-check-components',
    gidocgen,
    args: [
      'check',
      '--config=@0@'.format(components_toml),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../components'),
      unity_platform_components_gir[0],
    ],
    depends: unity_platform_components_gir[0],
    suite: 'doc',
  )
endif

if get_option('build-quit')
  quit_toml = configure_file(
    input: 'unity-platform-quit.toml.in',
    output: 'unity-platform-quit.toml',
    configuration: {'VERSION': meson.project_version()},
  )

  custom_target('unity-platform-quit-doc',
    input: [quit_toml, unity_platform_quit_gir[0]],
    output: 'unity-platform-quit',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--no-namespace-dir',
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    depend_files: files('urlmap.js'),
    build_by_default: true,
    install: true,
    install_dir: get_option('datadir') / 'doc' / meson.project_name(),
  )

  test('doc-check-quit',
    gidocgen,
    args: [
      'check',
      '--config=@0@'.format(quit_toml),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../quit'),
      unity_platform_quit_gir[0],
    ],
    depends: unity_platform_quit_gir[0],
    suite: 'doc',
  )
endif
