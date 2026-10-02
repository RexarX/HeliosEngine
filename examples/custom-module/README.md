# Creating A Custom Helios Module

## What You Get

| Artifact        | Target / output                                       |
| --------------- | ----------------------------------------------------- |
| Module library  | `example::greeting`                                   |
| Unit tests      | `example_greeting_tests` when `HELIOS_BUILD_TESTS=ON` |
| Demo executable | `greeting_demo`                                       |

Metadata is `module.toml`. The CMake file creates the library and the demo.

## Layout

```text
custom-module/
|-- module.toml
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

The module id is `example_greeting`. Names under `src/` must start with `helios_`. This example lives outside `src/`, so it must not.

When `HELIOS_BUILD_EXAMPLES=ON`, the engine registers this directory with `helios_add_extra_module_dirs()`. See [agents/building.md](../../../agents/building.md) for embedded and installed setups.
