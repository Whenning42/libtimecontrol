cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl')
latomic_dep = cc.find_library('atomic')
lrt_dep = cc.find_library('rt')
thread_dep = dependency('threads')
lgtest_dep = cc.find_library('gtest')
lgtest_main_dep = cc.find_library('gtest_main')


lib_deps = [ldl_dep, latomic_dep, thread_dep, lrt_dep]
common_cpp = ['-march=native', '-O3']

# Tests
test_deps = lib_deps + [lgtest_dep, lgtest_main_dep]
ipc_test = executable('ipc_test', ['ipc_test.cpp'], dependencies: test_deps)
test('ipc_test', ipc_test, is_parallel: false)
