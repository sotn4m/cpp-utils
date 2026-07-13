# cpp-utils

Small C++23 module library.

## Modules

| Module | Description |
|--------|-------------|
| `utils.timing` | `utils::measure_time()` — time a callable |

## Build (standalone)

Requires **CMake 4.3+** and the **Ninja** generator (C++ modules are not supported with Unix Makefiles).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/example
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

Configure the consumer with Ninja:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

In source, **all `#include` lines must come before any `import`**:

```cpp
#include <iostream>

import utils.timing;

int main() {
  auto ms = utils::measure_time<std::chrono::milliseconds>([] {
    // ...
  });
}
```

Linking against `cpp_utils` propagates the module interface — no extra flags needed.
