#
# This is the top-level CMake file. It specifies global configuration and
# references subdirectories containing individual targets of the project.
#

cmake_minimum_required(VERSION 3.14)

# We use vcpkg as a submodule
set(CMAKE_TOOLCHAIN_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")
include(${CMAKE_TOOLCHAIN_FILE})

project(
  iface
  VERSION 0.0.1
  LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("iface")
add_subdirectory("test")
