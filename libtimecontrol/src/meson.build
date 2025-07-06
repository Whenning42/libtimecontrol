cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl')
latomic_dep = cc.find_library('atomic')
lrt_dep = cc.find_library('rt')
thread_dep = dependency('threads')
lgtest_dep = cc.find_library('gtest')
lgtest_main_dep = cc.find_library('gtest_main')


lib_deps = [ldl_dep, latomic_dep, thread_dep, lrt_dep]
common_cpp = ['-march=native', '-O3']

inc = include_directories('../')

# Static libraries
ipc = static_library('ipc', ['ipc.cpp'], include_directories: inc)
ipc_server = static_library('ipc_server', ['ipc_server.cpp'], include_directories: inc)
time_operators = static_library('time_operators', ['time_operators.cpp'], include_directories: inc)
synced_fake_clock = static_library('synced_fake_clock', ['synced_fake_clock.cpp'], include_directories: inc)
libc_overrides = static_library('libc_overrides', ['libc_overrides.cpp'], include_directories: inc)
time_control = static_library('time_control', ['time_control.cpp'], include_directories: inc)

# Tests
test_deps = lib_deps + [lgtest_dep, lgtest_main_dep]
ipc_test = executable('ipc_test', ['ipc_test.cpp'], dependencies: test_deps, include_directories: inc, link_with: [ipc, ipc_server])
synced_fake_clock_test = executable('synced_fake_clock_test', ['synced_fake_clock_test.cpp'], dependencies: test_deps, include_directories: inc, 
                                    link_with: [time_control, synced_fake_clock, time_operators, ipc, ipc_server, libc_overrides])
time_control_test = executable('time_control_test', ['time_control_test.cpp'], dependencies: test_deps, include_directories: inc, 
                               link_with: [time_control, synced_fake_clock, time_operators, ipc, ipc_server, libc_overrides])


test('ipc_test', ipc_test, is_parallel: false)
test('synced_fake_clock', synced_fake_clock_test, is_parallel: false)
test('time_control_test', time_control_test, is_parallel: false)
