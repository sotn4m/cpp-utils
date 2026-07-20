cmake_minimum_required(VERSION 4.3)

project(cpp_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(CPP_UTILS_BUILD_TESTS "Build and register tests" ON)

add_library(cpp_utils INTERFACE)
add_library(Utils::cpp_utils ALIAS cpp_utils)

target_include_directories(cpp_utils
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CPP_UTILS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
