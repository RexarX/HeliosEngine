# Creating A Custom Helios Module

## What You Get

| Artifact        | Target / output                                      |
| --------------- | ---------------------------------------------------- |
| Module library  | `helios::module::greeting`                           |
| Unit tests      | `helios_greeting_tests` when `HELIOS_BUILD_TESTS=ON` |
| Demo executable | `greeting_demo`                                      |

## Layout

```text
custom_module/
|-- CMakeLists.txt
|-- README.md
|-- include/helios/greeting/
|   `-- greeting.hpp
|-- src/
|   |-- greeting.cpp
|   `-- demo.cpp
`-- tests/
    |-- main.cpp
    `-- greeting.cpp
```

There is no `Module.cmake`. The module is registered and built from the same
`CMakeLists.txt`.

## Module Definition

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

add_executable(greeting_demo src/demo.cpp)

helios_link_modules(
    TARGET greeting_demo
    MODULES
        PUBLIC greeting
        PUBLIC log
)
```

During module discovery Helios includes this file with
`HELIOS_REGISTRATION_PASS=ON`. The `helios_module()` macro records module
metadata and returns from the file, so the `greeting_demo` target is created
only during the build pass.

Authoring rule:

- Put `option()` calls and simple variable setup above `helios_module()` if
  needed.
- Put dependency loading, target mutation, generated files, and custom targets
  below `helios_module()`.

## Discovery

By default Helios scans `src/`. For out-of-tree modules, register search paths
before discovery with `helios_add_extra_module_dirs()`:

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/HeliosEngine/cmake")
include(ModuleRegistry)

helios_add_extra_module_dirs("${CMAKE_CURRENT_SOURCE_DIR}/modules")

add_subdirectory(third_party/HeliosEngine)
```

Or pass paths through the cache variable:

```bash
cmake --preset linux-gcc-release \
  -DHELIOS_EXTRA_MODULE_DIRS="/path/to/my/modules;/another/module/root"
```

Each entry may be a module container whose child directories contain
`CMakeLists.txt`, or a single module root containing its own `CMakeLists.txt`.
When `HELIOS_BUILD_EXAMPLES=ON`, Helios registers this example module through
`helios_add_extra_module_dirs()`.

## Public API

Headers live under `include/helios/<module>/` and use the
`helios::<module>` namespace:

```cpp
#include <helios/greeting/greeting.hpp>

auto message = helios::greeting::Format("World");
```

## Further Reading

- [docs/guidelines.md](../../docs/guidelines.md)
- [README.md](../../README.md)
