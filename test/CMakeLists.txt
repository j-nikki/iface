#
# This CMake file is concerned with the testing of the iface library.
#

enable_testing()

# Target without output, running it will run all unit tests
add_custom_target(
  iface-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  USES_TERMINAL)

# W4 and WX help catching bugs at compile time
string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

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
  add_dependencies(iface-tests ${target})

endforeach()
