#
# This is the CMake file for the target 'iface', a header-only library. Run 'vcpkg install' in the project root to get the dependencies referenced here.
#

cmake_minimum_required (VERSION 3.8)

add_library(iface INTERFACE)

find_path(BOOST_PREPROCESSOR_INCLUDE_DIRS "boost/preprocessor.hpp")
target_include_directories(iface INTERFACE ${BOOST_PREPROCESSOR_INCLUDE_DIRS})
target_include_directories(iface INTERFACE "include")
