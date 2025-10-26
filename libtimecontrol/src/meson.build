cc = meson.get_compiler('cpp')
ldl_dep = cc.find_library('dl')
latomic_dep = cc.find_library('atomic')
lrt_dep = cc.find_library('rt')
thread_dep = dependency('threads')
lgtest_dep = cc.find_library('gtest')
lgtest_main_dep = cc.find_library('gtest_main')


lib_deps = [ldl_dep, latomic_dep, thread_dep, lrt_dep]
common_cpp = []

inc = include_directories('../')

# Static libraries
libc_overrides = static_library(
    'libc_overrides',
    ['libc_overrides.cpp'],
    include_directories: inc
)
log = static_library(
    'log',
    ['util/log.cpp'],
    include_directories: inc
)
real_time_fns = static_library(
    'real_time_fns',
    ['real_time_fns.cpp'],
    include_directories: inc
)
time_operators = static_library(
    'time_operators',
    ['time_protocol/time_operators.cpp'],
    include_directories: inc
)
time_reader = static_library(
    'time_reader',
    ['time_protocol/time_reader.cpp'],
    include_directories: inc
)
time_writer = static_library(
    'time_writer',
    ['time_protocol/time_writer.cpp'],
    include_directories: inc
)
synced_fake_clock = static_library(
    'synced_fake_clock',
    ['time_protocol/synced_fake_clock.cpp'],
    include_directories: inc
)
synced_fake_clock_for_test = static_library(
    'synced_fake_clock_for_test',
    ['time_protocol/synced_fake_clock.cpp'],
    include_directories: inc,
    cpp_args: ['-DINIT_WRITER']
)



time_controller = shared_library(
  'time_controller', ['time_protocol/time_writer.cpp', 'time_protocol/time_control.cpp', 'time_protocol/channel_lease.cpp'],
  link_with: [
    time_operators, time_writer, real_time_fns, log
  ],
  install: true,
  include_directories: inc,
  dependencies: lib_deps
)

# Shared libraries. We build these from source instead of linking the pre-built
# libraries since we need to build under several different configurations.
time_control_srcs = [
  # All of the targets except for time_writer and synced_fake_clock_for_test.
  'libc_overrides.cpp', 'util/log.cpp', 'real_time_fns.cpp', 'time_protocol/time_operators.cpp', 'time_protocol/time_reader.cpp', 'time_protocol/synced_fake_clock.cpp', 'time_protocol/channel_lease.cpp'
]
variants = [
  {'name':'time_control', 'cpp': []},
  {'name':'time_control32', 'cpp': ['-m32'], 'link': ['-m32']},
  {'name':'time_control_dlsym', 'cpp': ['-DDLSYM_OVERRIDE']},
  {'name':'time_control_dlsym32', 'cpp': ['-m32', '-DDLSYM_OVERRIDE'], 'link': ['-m32']},
]
time_controls = []
foreach v : variants
  time_controls += shared_library(
    v['name'], time_control_srcs,
    dependencies: lib_deps,
    cpp_args: v.get('cpp', []),
    link_args: v.get('link', []),
    install: true,
    include_directories: inc
  )
endforeach

# Tests
test_deps = lib_deps + [lgtest_dep, lgtest_main_dep]
test_targets = [
  libc_overrides, log, real_time_fns, time_operators, time_reader, time_writer, synced_fake_clock_for_test, time_controller
]

libc_overrides_test = executable(
  'libc_overrides_test',
  ['libc_overrides_test.cpp'],
  dependencies: test_deps,
  include_directories: inc,
  link_with: test_targets)
sock_read_struct_test = executable(
  'sock_read_struct_test',
  ['ipc/sock_read_struct_test.cpp'],
  dependencies: test_deps,
  include_directories: inc,
  link_with: test_targets)
synced_fake_clock_test = executable(
  'synced_fake_clock_test',
  ['time_protocol/synced_fake_clock_test.cpp'],
  dependencies: test_deps,
  include_directories: inc,
  link_with: test_targets)
time_control_test = executable(
  'time_control_test',
  ['time_protocol/time_control_test.cpp'],
  dependencies: test_deps,
  include_directories: inc,
  link_with: test_targets)

test('libc_overrides', libc_overrides_test)
test('sock_read_struct_test', sock_read_struct_test)
test('synced_fake_clock', synced_fake_clock_test)
test('time_control_test', time_control_test)

subdir('async_test')
subdir('util')
