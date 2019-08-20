vogue_sources = [
  'break.c',
  'ellipsize.c',
  'fonts.c',
  'glyphstring.c',
  'modules.c',
  'vogue-attributes.c',
  'vogue-bidi-type.c',
  'vogue-color.c',
  'vogue-context.c',
  'vogue-coverage.c',
  'vogue-emoji.c',
  'vogue-engine.c',
  'vogue-fontmap.c',
  'vogue-fontset.c',
  'vogue-glyph-item.c',
  'vogue-gravity.c',
  'vogue-item.c',
  'vogue-language.c',
  'vogue-layout.c',
  'vogue-markup.c',
  'vogue-matrix.c',
  'vogue-renderer.c',
  'vogue-script.c',
  'vogue-tabs.c',
  'vogue-utils.c',
  'reorder-items.c',
  'shape.c',
  'voguefc-shape.c',
]

vogue_headers = [
  'vogue.h',
  'vogue-attributes.h',
  'vogue-bidi-type.h',
  'vogue-break.h',
  'vogue-context.h',
  'vogue-coverage.h',
  'vogue-direction.h',
  'vogue-engine.h',
  'vogue-font.h',
  'vogue-fontmap.h',
  'vogue-fontset.h',
  'vogue-glyph.h',
  'vogue-glyph-item.h',
  'vogue-gravity.h',
  'vogue-item.h',
  'vogue-language.h',
  'vogue-layout.h',
  'vogue-matrix.h',
  'vogue-modules.h',
  'vogue-renderer.h',
  'vogue-script.h',
  'vogue-tabs.h',
  'vogue-types.h',
  'vogue-utils.h',
  'vogue-version-macros.h',
]

install_headers(vogue_headers, subdir: vogue_api_path)

# Features header
vogue_features_conf = configuration_data()
vogue_features_conf.set('PANGO_VERSION_MAJOR', vogue_major_version)
vogue_features_conf.set('PANGO_VERSION_MINOR', vogue_minor_version)
vogue_features_conf.set('PANGO_VERSION_MICRO', vogue_micro_version)
vogue_features_conf.set('PANGO_API_VERSION', vogue_api_version)
vogue_features_conf.set('PANGO_CURRENT_MINUS_AGE', '0')

configure_file(
  input: 'vogue-features.h.meson',
  output: 'vogue-features.h',
  configuration: vogue_features_conf,
  install: true,
  install_dir: join_paths(vogue_includedir, vogue_api_path),
)

# Enumerations for GType
vogue_enums = gnome.mkenums(
  'vogue-enum-types',
  sources: vogue_headers,
  c_template: 'vogue-enum-types.c.template',
  h_template: 'vogue-enum-types.h.template',
  install_dir: join_paths(vogue_includedir, vogue_api_path),
  install_header: true,
)
vogue_enum_h = vogue_enums[1]

if host_system == 'windows'
  vogue_rc = configure_file(
    input: 'vogue.rc.in',
    output: 'vogue.rc',
    configuration: vogue_features_conf,
  )
  vogue_res = import('windows').compile_resources(vogue_rc)
  vogue_sources += vogue_res
endif

vogue_cflags = [
  '-DG_LOG_DOMAIN="Vogue"',
  '-DG_LOG_USE_STRUCTURED=1',
  '-DPANGO_COMPILATION',
  '-DSYSCONFDIR="@0@"'.format(vogue_sysconfdir),
  '-DLIBDIR="@0@"'.format(vogue_libdir),
]

vogue_inc = include_directories('.')

libvogue = library(
  vogue_api_name,
  sources: vogue_sources + vogue_enums,
  version: vogue_libversion,
  soversion: vogue_soversion,
  darwin_versions : vogue_osxversion,
  install: true,
  dependencies: vogue_deps,
  include_directories: [ root_inc, vogue_inc ],
  c_args: common_cflags + vogue_debug_cflags + vogue_cflags,
  link_args: common_ldflags,
)

vogue_dep_sources = [vogue_enum_h]

build_gir = get_option('introspection')

if build_gir
  gir_args = [
    '--quiet',
  ]

  vogue_gir = gnome.generate_gir(
    libvogue,
    sources: vogue_sources + vogue_headers + [ vogue_enum_h ],
    namespace: 'Vogue',
    nsversion: vogue_api_version,
    identifier_prefix: 'Vogue',
    symbol_prefix: 'vogue',
    export_packages: 'vogue',
    includes: [ 'GObject-2.0', 'cairo-1.0', ],
    header: 'vogue/vogue.h',
    install: true,
    extra_args: gir_args,
  )
  vogue_gir_dep = declare_dependency(sources: vogue_gir)
  vogue_dep_sources += vogue_gir
endif

libvogue_dep = declare_dependency(
  link_with: libvogue,
  include_directories: vogue_inc,
  dependencies: vogue_deps,
  sources: vogue_dep_sources,
)

pkgconfig.generate(libvogue,
  name: 'Vogue',
  description: 'Internationalized text handling',
  version: meson.project_version(),
  requires: ['gobject-2.0', 'harfbuzz'],
  filebase: 'vogue',
  subdirs: vogue_api_name,
  install_dir: join_paths(vogue_libdir, 'pkgconfig'),
)

# FreeType
if build_vogueft2
  voguefc_headers = [
    'voguefc-font.h',
    'voguefc-fontmap.h',
    'voguefc-decoder.h',
  ]

  voguefc_public_sources = [
    'voguefc-font.c',
    'voguefc-fontmap.c',
    'voguefc-decoder.c',
  ]

  vogueot_headers = [
    'vogue-ot.h',
  ]

  vogueot_public_sources = [
    'vogue-ot-buffer.c',
    'vogue-ot-info.c',
    'vogue-ot-ruleset.c',
    'vogue-ot-tag.c',
  ]

  vogueft2_headers = [
    'vogueft2.h',
  ]

  vogueft2_public_sources = [
    'vogueft2-fontmap.c',
    'vogueft2-render.c',
    'vogueft2.c',
  ]

  vogueft2_sources = voguefc_public_sources + vogueot_public_sources + vogueft2_public_sources

  if host_system == 'windows'
    vogueft2_rc = configure_file(
      input: 'vogueft2.rc.in',
      output: 'vogueft2.rc',
      configuration: vogue_features_conf,
    )
    vogueft2_res = import('windows').compile_resources(vogueft2_rc)
    vogueft2_sources += vogueft2_res
  endif

  install_headers(vogueft2_headers + voguefc_headers + vogueot_headers, subdir: vogue_api_path)

  libvogueft2 = library(
    'vogueft2-@0@'.format(vogue_api_version),
    sources: vogueft2_sources,
    version: vogue_libversion,
    soversion: vogue_soversion,
    darwin_versions : vogue_osxversion,
    install: true,
    dependencies: vogue_deps + [ libvogue_dep ],
    include_directories: [ root_inc, vogue_inc ],
    c_args: common_cflags + vogue_debug_cflags + vogue_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags,
  )

  vogueft2_dep_sources = []

  if build_gir
    voguefc_gir = gnome.generate_gir(
      libvogueft2,
      sources: voguefc_headers + voguefc_public_sources,
      dependencies: [ libvogue_dep, vogue_gir_dep ],
      namespace: 'VogueFc',
      nsversion: vogue_api_version,
      identifier_prefix: 'VogueFc',
      symbol_prefix: 'vogue_fc',
      export_packages: 'vogueft2',
      includes: [ vogue_gir[0], 'fontconfig-2.0', ],
      header: 'vogue/vogueft2.h',
      install: true,
      extra_args: gir_args,
    )
    voguefc_gir_dep = declare_dependency(sources: voguefc_gir)
    vogueft2_dep_sources += voguefc_gir

    # Deprecated API, still needed for VogueXft
    vogueot_gir = gnome.generate_gir(
      libvogueft2,
      sources: vogueot_headers + vogueot_public_sources,
      dependencies: [ libvogue_dep, vogue_gir_dep, voguefc_gir_dep ],
      namespace: 'VogueOT',
      nsversion: vogue_api_version,
      identifier_prefix: 'VogueOT',
      symbol_prefix: 'vogue_ot',
      export_packages: 'vogueft2',
      includes: [ voguefc_gir[0], 'freetype2-2.0', ],
      header: 'vogue/vogueft2.h',
      install: true,
      extra_args: gir_args,
    )
    vogueot_gir_dep = declare_dependency(sources: vogueot_gir)
    vogueft2_dep_sources += vogueot_gir

    vogueft2_gir = gnome.generate_gir(
      libvogueft2,
      sources: vogueft2_headers + vogueft2_public_sources,
      dependencies: [ libvogue_dep, vogue_gir_dep, voguefc_gir_dep, vogueot_gir_dep ],
      namespace: 'VogueFT2',
      nsversion: vogue_api_version,
      identifier_prefix: 'VogueFT2',
      symbol_prefix: 'vogue_ft2',
      export_packages: 'vogueft2',
      includes: [ voguefc_gir[0], 'freetype2-2.0', ],
      header: 'vogue/vogueft2.h',
      install: true,
      extra_args: gir_args,
    )
    vogueft2_gir_dep = declare_dependency(sources: vogueft2_gir)
    vogueft2_dep_sources += vogueft2_gir
  endif

  libvogueft2_dep = declare_dependency(
    link_with: libvogueft2,
    include_directories: vogue_inc,
    dependencies: vogue_deps + [ libvogue_dep ],
    sources: vogueft2_dep_sources,
  )

  pkgconfig.generate(libvogueft2,
    name: 'Vogue FT2 and Vogue Fc',
    description: 'Freetype 2.0 and fontconfig font support for Vogue',
    version: meson.project_version(),
    filebase: 'vogueft2',
    subdirs: vogue_api_name,
    requires: [ 'vogue', freetype2_pc, fontconfig_pc ],
    install_dir: join_paths(vogue_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libvogueft2_dep = dependency('', required : false)
  # Needed for making ft2 no-op while generating voguexft_gir below
  vogueft2_gir_dep = dependency('', required : false)
  vogueft2_gir = [[]]
endif

# Xft
if xft_dep.found() and fontconfig_dep.found()
  voguexft_headers = [
    'voguexft.h',
    'voguexft-render.h',
  ]

  voguexft_sources = [
    'voguexft-font.c',
    'voguexft-fontmap.c',
    'voguexft-render.c',
  ]

  install_headers(voguexft_headers, subdir: vogue_api_path)

  libvoguexft = library(
    'voguexft-@0@'.format(vogue_api_version),
    sources: voguexft_sources,
    version: vogue_libversion,
    soversion: vogue_soversion,
    darwin_versions : vogue_osxversion,
    install: true,
    dependencies: vogue_deps + [ libvogue_dep, libvogueft2_dep ],
    include_directories: [ root_inc, vogue_inc ],
    c_args: common_cflags + vogue_debug_cflags + vogue_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags,
  )
  voguexft_dep_sources = []

  if build_gir
    voguexft_gir = gnome.generate_gir(
      libvoguexft,
      sources: voguexft_headers + voguexft_sources,
      dependencies: [ libvogue_dep, libvogueft2_dep, vogue_gir_dep, vogueot_gir_dep, vogueft2_gir_dep ],
      namespace: 'VogueXft',
      nsversion: vogue_api_version,
      identifier_prefix: 'VogueXft',
      symbol_prefix: 'vogue_xft',
      export_packages: 'voguexft',
      includes: [ vogue_gir[0], vogueot_gir[0], vogueft2_gir[0], 'GObject-2.0', 'xft-2.0', 'xlib-2.0' ],
      header: 'vogue/voguexft.h',
      install: true,
      extra_args: gir_args,
    )
    voguexft_dep_sources += voguexft_gir
  endif

  libvoguexft_dep = declare_dependency(
    link_with: libvoguexft,
    include_directories: root_inc,
    dependencies: vogue_deps + [ libvogue_dep, libvogueft2_dep ],
    sources: voguexft_dep_sources,
  )

  pkgconfig.generate(libvoguexft,
    name: 'Vogue Xft',
    description: 'Xft font support for Vogue',
    version: meson.project_version(),
    filebase: 'voguexft',
    subdirs: vogue_api_name,
    requires: [ 'vogueft2', 'xft' ],
    install_dir: join_paths(vogue_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libvoguexft_dep = dependency('', required: false)
endif

# Win32
if host_system == 'windows'
  voguewin32_headers = [
    'voguewin32.h',
  ]

  voguewin32_sources = [
    'voguewin32.c',
    'voguewin32-fontcache.c',
    'voguewin32-fontmap.c',
  ]

  voguewin32_deps = vogue_deps + [
    libvogue_dep,
    cc.find_library('gdi32'),
  ]

  voguewin32_rc = configure_file(
    input: 'voguewin32.rc.in',
    output: 'voguewin32.rc',
    configuration: vogue_features_conf,
  )
  voguewin32_res = import('windows').compile_resources(voguewin32_rc)
  voguewin32_sources += voguewin32_res

  install_headers(voguewin32_headers, subdir: vogue_api_path)

  libvoguewin32 = library(
    'voguewin32-@0@'.format(vogue_api_version),
    sources: voguewin32_sources,
    version: vogue_libversion,
    soversion: vogue_soversion,
    install: true,
    dependencies: voguewin32_deps,
    include_directories: [ root_inc, vogue_inc ],
    c_args: common_cflags + vogue_debug_cflags + vogue_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags,
  )

  libvoguewin32_dep = declare_dependency(
    link_with: libvoguewin32,
    include_directories: root_inc,
    dependencies: vogue_deps + [ libvogue_dep ],
  )

  pkgconfig.generate(libvoguewin32,
    name: 'Vogue Win32',
    description: 'Win32 GDI font support for Vogue',
    version: meson.project_version(),
    filebase: 'voguewin32',
    subdirs: vogue_api_name,
    requires: 'vogue',
    install_dir: join_paths(vogue_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libvoguewin32_dep = dependency('', required: false)
endif

# Cairo
if cairo_dep.found()
  voguecairo_headers = [
    'voguecairo.h',
  ]

  voguecairo_sources = [
    'voguecairo-context.c',
    'voguecairo-font.c',
    'voguecairo-fontmap.c',
    'voguecairo-render.c',
  ]

  voguecairo_deps = vogue_deps + [ libvogue_dep ]

  if vogue_font_backends.contains('freetype')
    voguecairo_sources += [
      'voguecairo-fcfont.c',
      'voguecairo-fcfontmap.c',
    ]

    voguecairo_deps += libvogueft2_dep
  endif

  if host_system == 'windows' and vogue_font_backends.contains('win32')
    voguecairo_sources += [
      'voguecairo-win32font.c',
      'voguecairo-win32fontmap.c',
    ]

    voguecairo_rc = configure_file(
      input: 'voguecairo.rc.in',
      output: 'voguecairo.rc',
      configuration: vogue_features_conf,
    )
    voguecairo_res = import('windows').compile_resources(voguecairo_rc)
    voguecairo_sources += voguecairo_res

    voguecairo_deps += cc.find_library('gdi32')
    voguecairo_deps += libvoguewin32_dep
  endif

  if host_system == 'darwin' and vogue_font_backends.contains('quartz')
    voguecairo_sources += [
      'voguecoretext.c',
      'voguecoretext-fontmap.c',
      'voguecairo-coretextfont.c',
      'voguecairo-coretextfontmap.c',
    ]
  endif

  install_headers(voguecairo_headers, subdir: vogue_api_path)

  libvoguecairo = library(
    'voguecairo-@0@'.format(vogue_api_version),
    sources: voguecairo_sources,
    version: vogue_libversion,
    soversion: vogue_soversion,
    darwin_versions : vogue_osxversion,
    install: true,
    dependencies: vogue_deps + voguecairo_deps,
    include_directories: [ root_inc, vogue_inc ],
    c_args: common_cflags + vogue_debug_cflags + vogue_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags
  )
  voguecairo_dep_sources = []

  if build_gir
    voguecairo_gir = gnome.generate_gir(
      libvoguecairo,
      sources: voguecairo_headers + voguecairo_sources,
      dependencies: [ voguecairo_deps, vogue_gir_dep ],
      namespace: 'VogueCairo',
      nsversion: vogue_api_version,
      identifier_prefix: 'VogueCairo',
      symbol_prefix: 'vogue_cairo',
      export_packages: 'voguecairo',
      includes: [ vogue_gir[0], 'GObject-2.0', 'cairo-1.0' ],
      header: 'vogue/voguecairo.h',
      install: true,
      extra_args: gir_args,
    )
    voguecairo_dep_sources += voguecairo_gir
  endif

  libvoguecairo_dep = declare_dependency(
    link_with: libvoguecairo,
    include_directories: root_inc,
    dependencies: vogue_deps + [ libvogue_dep ],
    sources: voguecairo_dep_sources,
  )

  # Create voguecairo.pc according to whether we found Cairo
  # manually
  if ['pkgconfig', 'internal'].contains(cairo_found_type)
    vogue_cairo_requires = [ 'vogue', cairo_pc ]
  else
    vogue_cairo_requires = [ 'vogue' ]
  endif

  pkgconfig.generate(libvoguecairo,
    name: 'Vogue Cairo',
    description: 'Cairo rendering support for Vogue',
    version: meson.project_version(),
    filebase: 'voguecairo',
    subdirs: vogue_api_name,
    requires: vogue_cairo_requires,
    install_dir: join_paths(vogue_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libvoguecairo_dep = dependency('', required: false)
endif
