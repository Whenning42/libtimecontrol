cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl', required: true)
latomic_dep = cc.find_library('atomic', required: true)

# Libs
libtime_control = shared_library('time_control', 'time_control.cpp',
                        dependencies: [ldl_dep, latomic_dep],
                        cpp_args: ['-pthread', '-std=c++17', '-fPIC', '-shared', '-march=native', '-O3'],
                        install: true)
libtime_control32 = shared_library('time_control32', 'time_control.cpp',
                        dependencies: [ldl_dep, latomic_dep],
                        cpp_args: ['-pthread', '-std=c++17', '-fPIC', '-shared', '-march=native', '-O3', '-m32'],
                        link_args: ['-m32'],
                        install: true)
libtime_control_dlsym = shared_library('time_control_dlsym', 'time_control.cpp',
                                       dependencies: [ldl_dep, latomic_dep],
                                       cpp_args: ['-pthread', '-std=c++17', '-fPIC', '-shared', '-march=native', '-O3', '-DDLSYM_OVERRIDE'],
                                       install: true)
libtime_control_dlsym32 = shared_library('time_control32_dlsym', 'time_control.cpp',
                                          dependencies: [ldl_dep, latomic_dep],
                                          cpp_args: ['-pthread', '-std=c++17', '-fPIC', '-shared', '-march=native', '-O3', '-m32', '-DDLSYM_OVERRIDE'],
                                          link_args: ['-m32'],
                                          install: true)

# Tests
lgtest_dep = cc.find_library('gtest', required: false)
lgtest_main_dep = cc.find_library('gtest_main', required: false)
time_control_test_exe = executable('time_control_test_exe', ['time_control_test.cpp'],
                       dependencies: [ldl_dep, latomic_dep, lgtest_dep, lgtest_main_dep],
                       cpp_args: ['-pthread', '-std=c++17'])
time_overrides_test_exe = executable('time_overrides_test_exe', ['time_overrides_test.cpp'],
                       dependencies: [ldl_dep, latomic_dep, lgtest_dep, lgtest_main_dep],
                       cpp_args: ['-pthread', '-std=c++17'])
clock_state_test_exe = executable('clock_state_test_exe', ['clock_state_test.cpp'],
                       dependencies: [ldl_dep, latomic_dep, lgtest_dep, lgtest_main_dep],
                       cpp_args: ['-pthread', '-std=c++17'])
test('time_control_test', time_control_test_exe, is_parallel: false)
test('time_overrides_test', time_overrides_test_exe, is_parallel: false)
test('clock_state_test', clock_state_test_exe, is_parallel: false)

subdir('integration_tests')
