project('bacs_system_single', 'cpp',
        version : '0.1.0',
        license : 'apache2',
        meson_version : '>=0.45',
        default_options : [
            'cpp_std=c++17',
            'warning_level=3',
        ])

add_global_arguments('-DBOOST_ALL_DYN_LINK', language : 'cpp')

include = include_directories('include')

srcs = [
    'src/check.cpp',
    'src/file.cpp',
    'src/main.cpp',
    'src/test/storage.cpp',
    'src/tester_util.cpp',
    'src/worker.cpp',
]

deps = [
    dependency('threads'),
    dependency('bunsan_protobuf'),
    dependency('bunsan_broker'),
    dependency('bacs_system'),
]


lib = shared_library('bacs_system_single', srcs,
    include_directories : include,
    dependencies : deps,
    install : true,
    version : meson.project_version(),
    soversion : meson.project_version(),
)

bacs_system_single_dep = declare_dependency(
    link_with : lib,
    dependencies : deps,
    include_directories : include,
)

pkg = import('pkgconfig')

pkg.generate(
    name : 'bacs_system_single',
    description : 'bacs::system::single helpers',
    libraries : [lib] + deps,
    version : meson.project_version(),
)

install_subdir('include',
               install_dir : get_option('includedir'),
               strip_directory : true)

subdir('tests')
