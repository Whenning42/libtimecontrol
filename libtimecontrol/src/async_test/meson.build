inc = include_directories('../../')

controller = executable(
  'controller',
  ['controller.cpp'],
  link_with: [time_operators, time_writer, real_time_fns, log, time_controller],
  include_directories: inc,
  dependencies: lib_deps)
controllee = executable(
  'controllee',
  ['controllee.cpp', '../time_protocol/time_operators.cpp', '../util/log.cpp'],
  cpp_args: ['-m32'],
  link_args: ['-m32'],
  link_with: [time_controls[3]],
  include_directories: inc,
  dependencies: lib_deps)

test('async_test', controller)
