cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl', required: true)

test_prog = executable('test_prog', ['test_prog.cpp'],
                       dependencies: [ldl_dep],
                       cpp_args: ['-std=c++17'],
                       install: true)
test_prog32 = executable('test_prog32', ['test_prog.cpp'],
                         dependencies: [ldl_dep],
                         cpp_args: ['-std=c++17', '-m32'], link_args: ['-m32'],
                         install: true)
test_prog_dlsym = executable('test_prog_dlsym', ['test_prog_dlsym.cpp'],
                             dependencies: [ldl_dep],
                             cpp_args: ['-std=c++17'],
                             install: true)

python = find_program('python3')
test('integration test',
     python,
     args: [meson.project_source_root() / 'libtimecontrol/integration_tests/integration_test.py'],
     env: ['PYTHONPATH=' + meson.project_source_root() / 'libtimecontrol'],
     is_parallel: false)
