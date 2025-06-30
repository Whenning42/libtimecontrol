cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl', required: true)
thread_dep = dependency('threads')
deps = [ldl_dep, thread_dep]

test_prog = executable('test_prog', ['test_prog.cpp'],
                       dependencies: deps,
                       install: true)
test_prog32 = executable('test_prog32', ['test_prog.cpp'],
                         dependencies: deps,
                         cpp_args: ['-m32', '-pthread'], link_args: ['-m32'],
                         install: true)
test_prog_dlsym = executable('test_prog_dlsym', ['test_prog_dlsym.cpp'],
                             dependencies: deps,
                             cpp_args: ['-pthread'],
                             install: true)

python = find_program('python3')
test('integration test',
     python,
     args: [meson.project_source_root() / 'libtimecontrol/integration_tests/integration_test.py'],
     env: ['PYTHONPATH=' + meson.project_source_root() / 'libtimecontrol'],
     is_parallel: false)
