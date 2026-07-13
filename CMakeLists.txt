cmake_minimum_required(VERSION 4.3)

project(cpp_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CPP_UTILS_BUILD_EXAMPLES)
  set(CPP_UTILS_BUILD_EXAMPLES ${PROJECT_IS_TOP_LEVEL})
endif()

add_library(cpp_utils)

target_sources(cpp_utils
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      src/timing.cpp
)

if(CPP_UTILS_BUILD_EXAMPLES)
  add_executable(example examples/measure_time.cpp)
  target_link_libraries(example PRIVATE cpp_utils)
endif()
