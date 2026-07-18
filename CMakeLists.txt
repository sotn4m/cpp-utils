cmake_minimum_required(VERSION 4.3)

project(cpp_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(NOT DEFINED CPP_UTILS_BUILD_TESTS)
  set(CPP_UTILS_BUILD_TESTS ${PROJECT_IS_TOP_LEVEL})
endif()

add_library(cpp_utils INTERFACE)

target_include_directories(cpp_utils
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CPP_UTILS_BUILD_TESTS)
  include(FetchContent)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(cpp_utils_tests
    tests/spsc_ring_buffer_test.cpp
    tests/mpsc_ring_buffer_test.cpp
    tests/scoped_temp_dir_test.cpp
  )
  target_link_libraries(cpp_utils_tests PRIVATE cpp_utils GTest::gtest_main)
  add_test(NAME cpp_utils_tests COMMAND cpp_utils_tests)
endif()
