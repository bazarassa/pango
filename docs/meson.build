subdir('xml')

version_xml_conf = configuration_data()
version_xml_conf.set('PANGO_VERSION', meson.project_version())
configure_file(input: 'version.xml.in',
               output: 'version.xml',
               configuration: version_xml_conf)

private_headers = [
  'voguecairo-private.h',
  'voguecairo-fc-private.h',
  'voguecairo-fc.h',
  'voguecairo-win32.h',
  'voguecairo-atsui.h',
  'vogue-coverage-private.h',
  'vogue-color-table.h',
  'vogue-emoji-private.h',
  'vogue-font-private.h',
  'vogue-fontmap-private.h',
  'vogue-fontset-private.h',
  'vogue-impl-utils.h',
  'vogue-glyph-item-private.h',
  'vogue-layout-private.h',
  'vogue-script-table.h',
  'vogue-script-private.h',
  'vogue-utils-private.h',
  'voguefc-private.h',
  'voguefc-fontmap-private.h',
  'vogueft2-private.h',
  'voguewin32-private.h',
  'vogueatsui-private.h',
  'voguexft-private.h',
  'vogue-ot-private.h',
  'vogue-script-lang-table.h',
  'vogue-language-sample-table.h',
  'voguecoretext-private.h',
]

content_files = [
  'vogue_markup.sgml',
  'version.xml',
]

html_images = [
  'layout.png',
  'pipeline.png',
  'rotated-text.png',
  'rects1.png',
  'rects2.png',
]

docdeps = [ libvogue_dep ]

if build_vogueft2
  docdeps += libvogueft2_dep
endif

if xft_dep.found()
  docdeps += libvoguexft_dep
endif

if cairo_dep.found()
  docdeps += libvoguecairo_dep
endif

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
cairo_prefix = dependency('cairo').get_pkgconfig_variable('prefix')
cairo_docpath = join_paths(cairo_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(vogue_datadir, 'gtk-doc', 'html')

gnome.gtkdoc('vogue',
             main_xml: 'vogue-docs.sgml',
             src_dir: [
               join_paths(meson.source_root(), 'vogue'),
               join_paths(meson.build_root(), 'vogue'),
             ],
             dependencies: docdeps,
             gobject_typesfile: configure_file(input: 'vogue.types.in',
                                               output: 'vogue.types',
                                               configuration: vogue_conf),
             scan_args: [
               '--deprecated-guards=PANGO_DISABLE_DEPRECATED',
               '--ignore-decorators=_PANGO_EXTERN',
               '--ignore-headers=' + ' '.join(private_headers),
             ],
             mkdb_args: [
               '--sgml-mode',
               '--output-format=xml',
             ],
             fixxref_args: [
               '--html-dir=@0@'.format(docpath),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
               '--extra-dir=@0@'.format(join_paths(cairo_docpath, 'cairo')),
             ],
             html_assets: html_images,
             install: true)
