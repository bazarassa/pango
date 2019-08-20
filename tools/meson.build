if fontconfig_dep.found()
  executable('gen-script-for-lang', 'gen-script-for-lang.c',
             dependencies: vogue_deps + [ libvogue_dep ],
             c_args: common_cflags + vogue_debug_cflags + [ '-DPANGO_DISABLE_DEPRECATED' ],
             include_directories: root_inc,
             install: false)
endif
