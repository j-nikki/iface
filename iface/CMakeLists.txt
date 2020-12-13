#
# This is the CMake file for the target 'iface', a header-only library. Run 'vcpkg install' in the project root to get the dependencies referenced here.
#

cmake_minimum_required (VERSION 3.8)

add_library(iface INTERFACE "iface.h")

find_package(benchmark CONFIG REQUIRED)
target_link_libraries(iface INTERFACE  benchmark::benchmark benchmark::benchmark_main)
find_path(BOOST_PREPROCESSOR_INCLUDE_DIRS "boost/preprocessor.hpp")
target_include_directories(iface INTERFACE ${BOOST_PREPROCESSOR_INCLUDE_DIRS})
