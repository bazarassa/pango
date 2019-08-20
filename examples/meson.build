examples = []

if vogue_cairo_backends.contains('png')
  examples += [
    'cairoshape',
    'cairosimple',
    'cairotwisted',
  ]
endif

examples_deps = [ libvogue_dep, libvoguecairo_dep, ]

if vogue_font_backends.contains('freetype')
  examples_deps += libvogueft2_dep
endif

# somebody will have to fix this
# if host_system == 'windows'
#   # This hasn't been built in years, most likely, but try now
#   examples += [ 'voguewin32tobmp' ]
#   examples_deps += [ libvoguewin32_dep ]
# endif

foreach e: examples
  executable(e, '@0@.c'.format(e),
             dependencies: examples_deps + vogue_deps,
             include_directories: root_inc,
             install: false)
endforeach
