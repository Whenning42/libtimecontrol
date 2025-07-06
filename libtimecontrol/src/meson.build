cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl')
latomic_dep = cc.find_library('atomic')
lrt_dep = cc.find_library('rt')
thread_dep = dependency('threads')
lgtest_dep = cc.find_library('gtest')
lgtest_main_dep = cc.find_library('gtest_main')


lib_deps = [ldl_dep, latomic_dep, thread_dep, lrt_dep]
common_cpp = ['-march=native', '-O2']

inc = include_directories('../')

# Static libraries
ipc = static_library('ipc', ['ipc.cpp'], include_directories: inc)
ipc_server = static_library('ipc_server', ['ipc_server.cpp'], include_directories: inc)
time_operators = static_library('time_operators', ['time_operators.cpp'], include_directories: inc)
synced_fake_clock = static_library('synced_fake_clock', ['synced_fake_clock.cpp'], include_directories: inc)
libc_overrides = static_library('libc_overrides', ['libc_overrides.cpp'], include_directories: inc)
time_writer = static_library('time_writer', ['time_writer.cpp'], include_directories: inc)
time_reader = static_library('time_reader', ['time_reader.cpp'], include_directories: inc)
time_control = static_library(
    'time_control',
    ['time_reader.cpp', 'time_writer.cpp', 'ipc.cpp', 'ipc_server.cpp',
     'time_operators.cpp', 'synced_fake_clock.cpp'],
    include_directories: inc
)
time_control32 = static_library(
    'time_control32',
    ['time_reader.cpp', 'time_writer.cpp', 'ipc.cpp', 'ipc_server.cpp',
     'time_operators.cpp', 'synced_fake_clock.cpp'],
    include_directories: inc,
    cpp_args: common_cpp + ['-m32']
)

# Shared libraries
variants = [
  {'name':'time_controller', 'source': ['time_writer_entry.cpp'], 'cpp': common_cpp},
  {'name':'time_control', 'source': ['time_reader_entry.cpp', 'libc_overrides.cpp'], 'cpp': common_cpp},
  {'name':'time_control32', 'source': ['time_reader_entry.cpp', 'libc_overrides.cpp'], 'cpp': common_cpp + ['-m32'], 'link': ['-m32'], 'link_with': [time_control32]},
  {'name':'time_control_dlsym', 'source': ['time_reader_entry.cpp', 'libc_overrides.cpp'], 'cpp': common_cpp + ['-DDLSYM_OVERRIDE']},
  {'name':'time_control_dlsym32', 'source': ['time_reader_entry.cpp', 'libc_overrides.cpp'], 'cpp': common_cpp + ['-m32', '-DDLSYM_OVERRIDE'], 'link': ['-m32'], 'link_with': [time_control32]},
]
foreach v : variants
  shared_library(
    v['name'], v['source'],
    dependencies: lib_deps,
    cpp_args: v.get('cpp', []),
    link_args: v.get('link', []),
    link_with: v.get('link_with', [time_control]),
    install: true,
    include_directories: inc
  )
endforeach

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
