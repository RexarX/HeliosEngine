# Creating a Custom Helios Module

## What you get

| Artifact        | Target / output                                 |
| --------------- | ----------------------------------------------- |
| Module library  | `helios::module::greeting`                      |
| Unit tests      | `greeting_tests` (when `HELIOS_BUILD_TESTS=ON`) |
| Demo executable | `greeting_demo`                                 |

## Layout

```
custom_module/
├── Module.cmake                      # Register in the dependency graph
├── CMakeLists.txt                    # Build definition + demo app
├── README.md                         # This file
├── include/helios/greeting/
│   └── greeting.hpp                  # Public API
├── src/
│   ├── greeting.cpp                  # Implementation
│   └── demo.cpp                      # Example consumer
└── tests/
    ├── main.cpp                      # doctest entry
    └── greeting.cpp                  # Unit tests
```

## Step 1 — Register the module (`Module.cmake`)

`helios_register_module` adds the module to Helios's dependency graph and creates the `HELIOS_BUILD_GREETING_MODULE` CMake option:

```cmake
helios_register_module(
    NAME greeting
    DESCRIPTION "Example custom Helios module (greeting utilities)"
    DEFAULT ON
    DEPENDS core utils
)
```

- `DEPENDS` — other Helios modules that must be enabled (resolved automatically).
- `DEFAULT ON` — this example is enabled once discovered (see discovery below).

## Step 2 — Define the build (`CMakeLists.txt`)

```cmake
helios_module(
    NAME greeting
    VERSION 0.1.0
    DESCRIPTION "Example custom Helios module (greeting utilities)"

    HEADERS
        include/helios/greeting/greeting.hpp

    SOURCES
        src/greeting.cpp

    DEPENDS
        PUBLIC core
        PUBLIC utils

    TEST_SOURCES
        tests/greeting.cpp
        tests/main.cpp
)
```

`helios_module` creates:

- CMake target `helios_module_greeting`
- Alias `helios::module::greeting`
- Test binary when `HELIOS_BUILD_TESTS=ON`

## Step 3 — Discovery (no manual `include()`)

Cmake scans `src/` automatically. For out-of-tree modules, register search paths **before module discovery** with `helios_add_extra_module_dirs()`:

```cmake
# Parent project CMakeLists.txt — before add_subdirectory(HeliosEngine)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/HeliosEngine/cmake")
include(ModuleRegistry)

helios_add_extra_module_dirs("${CMAKE_CURRENT_SOURCE_DIR}/modules")

add_subdirectory(third_party/HeliosEngine)
```

Alternatively, pass paths through the cache variable (merged at discovery time):

```bash
cmake --preset linux-gcc-release \
  -DHELIOS_EXTRA_MODULE_DIRS="/path/to/my/modules;/another/module/root"
```

Each entry may be either:

- **A module container** — immediate child directories with `CMakeLists.txt` are registered (same as `src/`).
- **A single module root** — the directory itself is registered when it contains `Module.cmake` (this example).

When `HELIOS_BUILD_EXAMPLES=ON`, Helios registers `examples/custom_module` via `helios_add_extra_module_dirs()` before discovery.

## Step 4 — Public API

Headers live under `include/helios/<module>/` and use the `helios::<module>` namespace:

```cpp
#include <helios/greeting/greeting.hpp>

auto message = helios::greeting::Format("World");
// "Hello, World!"
```

## Step 5 — Consume from an executable

The demo target in `CMakeLists.txt` shows linking:

```cmake
add_executable(greeting_demo src/demo.cpp)

helios_link_modules(
    TARGET greeting_demo
    MODULES
        PUBLIC greeting
        PUBLIC log
)
```

## Further reading

- [docs/guidelines.md](../../docs/guidelines.md) — coding standards and module conventions
- [README.md](../../README.md) — project overview
