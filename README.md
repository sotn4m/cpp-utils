# cpp-utils

Small C++23 header library.

## Headers

| Header | Description |
|--------|-------------|
| `<utils/timing.hpp>` | `utils::measure_time()` — time a callable |
| `<utils/joining-thread.hpp>` | `utils::joining_thread` — RAII `std::thread` that joins on destruction |
| `<utils/spsc-ring-buffer.hpp>` | `utils::spsc_ring_buffer<T, N>` — lock-free SPSC ring buffer |

## Build (standalone)

Requires **CMake 4.3+**.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/example
```

## Tests

Requires **Google Test** v1.17.0 (fetched automatically by CMake when tests are enabled).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run the test binary directly:

```bash
./build/cpp_utils_tests
```

Run a single test:

```bash
./build/cpp_utils_tests --gtest_filter=SpscRingBufferTest.PushPopFifo
```

Tests are built only when `cpp-utils` is the top-level project. When pulled in as a dependency, pass `-DCPP_UTILS_BUILD_TESTS=ON` to enable them, or leave the default (`OFF`).

## Clean

Remove build artifacts, downloaded GTest, and CTest output:

```bash
rm -rf build Testing
```

Always run `ctest` with `--test-dir build`. Running `ctest` from the project root creates a stray `Testing/` folder in the source tree.

Ninja is optional but recommended for faster builds:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## Use from another project

In the consumer `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 4.3)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

# Skip cpp-utils examples and tests when pulled in as a dependency
set(CPP_UTILS_BUILD_TESTS OFF)

add_subdirectory(/path/to/cpp-utils ${CMAKE_BINARY_DIR}/cpp-utils)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE cpp_utils)
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

In source:

```cpp
#include <iostream>
#include <utils/timing.hpp>

int main() {
  auto ms = utils::measure_time<std::chrono::milliseconds>([] {
    // ...
  });
}
```

Linking against `cpp_utils` propagates the include path — no extra flags needed.
