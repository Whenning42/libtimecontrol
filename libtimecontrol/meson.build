cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl')
latomic_dep = cc.find_library('atomic')
lrt_dep = cc.find_library('rt')
thread_dep = dependency('threads')
lgtest_dep = cc.find_library('gtest')
lgtest_main_dep = cc.find_library('gtest_main')


lib_deps = [ldl_dep, latomic_dep, thread_dep, lrt_dep]
common_cpp = ['-march=native', '-O3']

# Libs
variants = [
  {'name':'time_controller', 'source': ['time_writer.cpp'], 'cpp': common_cpp},
  {'name':'time_control', 'source': ['time_reader.cpp'], 'cpp': common_cpp},
  {'name':'time_control32', 'source': ['time_reader.cpp'], 'cpp': common_cpp + ['-m32'], 'link': ['-m32']},
  {'name':'time_control_dlsym', 'source': ['time_reader.cpp'], 'cpp': common_cpp + ['-DDLSYM_OVERRIDE']},
  {'name':'time_control_dlsym32', 'source': ['time_reader.cpp'], 'cpp': common_cpp + ['-m32', '-DDLSYM_OVERRIDE'], 'link': ['-m32']},
]

foreach v : variants
  shared_library(
    v['name'], v['source'],
    dependencies: lib_deps,
    cpp_args: v.get('cpp', []),
    link_args: v.get('link', []),
    install: true
  )
endforeach


# Tests
test_deps = lib_deps + [lgtest_dep, lgtest_main_dep]

time_control_test_exe = executable('time_control_test_exe', ['time_control_test.cpp'],
                                   dependencies: test_deps)
time_overrides_test_exe = executable('time_overrides_test_exe', ['time_overrides_test.cpp'],
                                  dependencies: test_deps)
clock_state_test_exe = executable('clock_state_test_exe', ['clock_state_test.cpp'],
                                  dependencies: test_deps)

test('time_control_test', time_control_test_exe, is_parallel: false)
test('time_overrides_test', time_overrides_test_exe, is_parallel: false)
test('clock_state_test', clock_state_test_exe, is_parallel: false)

subdir('integration_tests')
