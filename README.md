# cpp-utils

Small C++23 header library.

## Headers

| Header | Description |
|--------|-------------|
| `<utils/timing.hpp>` | `utils::measure_time()` — time a callable |

## Build (standalone)

Requires **CMake 4.3+**.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/example
```

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

# Skip cpp-utils examples when pulled in as a dependency
set(CPP_UTILS_BUILD_EXAMPLES OFF)

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
