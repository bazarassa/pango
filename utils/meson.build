vogue_view_sources = [
  'vogue-view.c',
  'viewer-main.c',
  'viewer-render.c',
]

vogue_view_deps = [
  vogue_deps,
  libvogue_dep,
]

if build_vogueft2
  vogue_view_sources += 'viewer-vogueft2.c'
  vogue_view_deps += libvogueft2_dep
endif

if xft_dep.found() and build_vogueft2
  vogue_view_sources += [
    'viewer-voguexft.c',
    'viewer-x.c',
  ]
  vogue_view_deps += [ libvogueft2_dep, libvoguexft_dep, ]
endif

if cairo_dep.found()
  vogue_view_sources += [
    'viewer-cairo.c',
    'viewer-voguecairo.c',
  ]
  vogue_view_deps += libvoguecairo_dep
endif

vogue_view = executable('vogue-view', vogue_view_sources,
                        dependencies: vogue_view_deps,
                        include_directories: [ root_inc ],
                        install: true,
                        c_args: [
                          '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
                          '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
                        ])

help2man = find_program('help2man', required: false)
# help2man has to run vogue-view to generate the man page, which it can't do if
# we're cross building. We also can't tell help2man what exe wrapper to use.
if help2man.found() and not meson.is_cross_build()
  help2man_opts = [
    '--no-info',
    '--section=1',
    '--help-option=--help-all',
    '--name=Vogue text viewer',
  ]

  custom_target('vogue-view.1',
                output: 'vogue-view.1',
                command: [
                  help2man, help2man_opts, '--output=@OUTPUT@', vogue_view
                ],
                install: true,
                install_dir: join_paths(vogue_datadir, 'man/man1'))
endif

if cairo_dep.found()
  vogue_list_sources = [
    'vogue-list.c',
  ]

  vogue_list_deps = [
    vogue_deps,
    libvogue_dep,
    libvoguecairo_dep
  ]

  vogue_list = executable('vogue-list', vogue_list_sources,
                          dependencies: vogue_list_deps,
                          include_directories: [ root_inc ],
                          install: true,
                          c_args: [
                           '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
                           '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
                          ])

endif
