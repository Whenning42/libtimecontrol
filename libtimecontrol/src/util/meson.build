# TODO: Make sure we can build these targets with the correct flags
# needed by libtimecontrol wrt 32 vs 64 builds.

cc = meson.get_compiler('cpp')
lgtest_dep = cc.find_library('gtest')
lgtest_main_dep = cc.find_library('gtest_main')

inc = include_directories('../../')

# Static libraries
helpers = static_library(
    'helpers',
    ['helpers.cpp'],
    include_directories: inc
)

ephemeral_directory = static_library(
    'ephemeral_directory',
    ['ephemeral_directory.cpp'],
    include_directories: inc,
    link_with: [helpers]
)

# Tests
test_deps = [lgtest_dep, lgtest_main_dep]

helpers_test = executable(
    'helpers_test',
    ['helpers_test.cpp'],
    dependencies: test_deps,
    include_directories: inc,
    link_with: [helpers]
)

ephemeral_directory_test = executable(
    'ephemeral_directory_test',
    ['ephemeral_directory_test.cpp'],
    dependencies: test_deps,
    include_directories: inc,
    link_with: [ephemeral_directory, helpers]
)

test('helpers', helpers_test)
test('ephemeral_directory', ephemeral_directory_test)
