testenv = environment()
testenv.set('BUNSAN_SOURCE_DIR', meson.current_source_dir() + '/..')
testenv.set('BUNSAN_BINARY_DIR', meson.current_build_dir() + '/..')

test_deps = [
    bacs_system_single_dep,
    dependency('bunsan_test'),
    dependency('boost', modules : [
        'unit_test_framework',
    ]),
]

prefix = 'bacs_system_single_test_'
tests = [
    'check',
    'test',
    'worker',
]

foreach t : tests
  e = executable(prefix + t,
                 t + '.cpp',
                 dependencies : test_deps)
  test(prefix + t, e, env : testenv)
endforeach
