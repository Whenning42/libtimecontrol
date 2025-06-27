cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl', required: true)

test_prog = executable('test_prog', ['test_prog.cpp'],
                       dependencies: [ldl_dep],
                       cpp_args: ['-std=c++17'])
test_prog32 = executable('test_prog32', ['test_prog.cpp'],
                         dependencies: [ldl_dep],
                         cpp_args: ['-std=c++17', '-m32'], link_args: ['-m32'])
test_prog_dlsym = executable('test_prog_dlsym', ['test_prog_dlsym.cpp'],
                             dependencies: [ldl_dep],
                             cpp_args: ['-std=c++17'])
