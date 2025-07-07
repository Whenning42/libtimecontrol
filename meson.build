project('libtimecontrol', ['c', 'cpp'],
        license: ['MIT'],
        license_files: ['LICENSE'],
        version: '0.0.0',
        default_options: [
                'warning_level=3',
                'werror=true',
                'cpp_std=c++17',
                'b_lto=true',
                'b_ndebug=if-release'])

if get_option('buildtype') == 'release'
    add_project_arguments('-DNDEBUG', language : ['cpp'])
else
    add_project_arguments('-DDEBUG', language : ['cpp'])
endif

subdir('libtimecontrol')
