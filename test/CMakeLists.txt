#
# This CMake file is concerned with the testing of the iface library.
#

enable_testing()

# Target without output, running it will run all unit tests
add_custom_target(tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  USES_TERMINAL)

# Iterate over all .cpp files from this dir
file(GLOB_RECURSE UNIT_TESTS "*.cpp")
foreach(ut IN LISTS UNIT_TESTS)

    # Target name will be the extensionless file name
    string(REGEX MATCH "([^\\/]+)\.cpp$" _ ${ut})
    set(target ${CMAKE_MATCH_1})

    # Target is a test and dependency on tests
    add_executable(${target} EXCLUDE_FROM_ALL "${ut}" "test_utils.h")
    add_test(${target} ${target})
    target_link_libraries(${target} iface)
    add_dependencies(tests ${target})

endforeach()