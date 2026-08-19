include(FetchContent)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(
    cpp_utils_tests
    spsc_ring_buffer_test.cpp
    mpsc_ring_buffer_test.cpp
    scoped_temp_dir_test.cpp
    joining_thread_test.cpp
    threadsafe_lookup_table_test.cpp
    grid_test.cpp
    rolling_time_window_test.cpp
    percent_change_test.cpp
    cooldown_tracker_test.cpp
    line_buffer_test.cpp
    quicksort_test.cpp
    lru_cache_test.cpp
)

target_compile_features(cpp_utils_tests PRIVATE cxx_std_23)
set_target_properties(cpp_utils_tests PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
    cpp_utils_tests
    PRIVATE Utils::cpp_utils GTest::gtest_main GTest::gmock
)

target_include_directories(cpp_utils_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME cpp_utils_tests COMMAND cpp_utils_tests)
