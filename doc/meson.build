dependency('gi-docgen', version: '>= 2021.1',
           fallback: ['gi-docgen', 'dummy_dep'],
           native: true,
           required: get_option('documentation'))

gidocgen = find_program('gi-docgen')

docs = []

if get_option('build-components')
  docs += {'name': 'components', 'namespace': 'UnityPlatformComponents', 'gir': unity_platform_components_gir}
endif

if get_option('build-backgrounds')
  docs += {'name': 'backgrounds', 'namespace': 'UnityPlatformBackgrounds', 'gir': unity_platform_backgrounds_gir}
endif

if get_option('build-translucency')
  docs += {'name': 'translucency', 'namespace': 'UnityPlatformTranslucency', 'gir': unity_platform_translucency_gir}
endif

doc_links = []

foreach doc : docs
  doc_name = 'unity-platform-' + doc['name']

  doc_toml = configure_file(
    input: doc_name + '.toml.in',
    output: doc_name + '.toml',
    configuration: {'VERSION': meson.project_version()},
  )

  custom_target(doc_name + '-doc',
    input: [doc_toml, doc['gir'][0]],
    output: doc_name,
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

  test('doc-check-' + doc['name'],
    gidocgen,
    args: [
      'check',
      '--config=@0@'.format(doc_toml),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '..' / doc['name']),
      doc['gir'][0],
    ],
    depends: doc['gir'][0],
    suite: 'doc',
  )

  doc_links += '    <li><a href="./@0@/">@1@</a></li>'.format(doc_name, doc['namespace'])
endforeach

configure_file(
  input: 'index.html.in',
  output: 'index.html',
  configuration: {
    'PROJECT_VERSION': meson.project_version(),
    'DOC_LINKS': '\n'.join(doc_links),
  },
)
