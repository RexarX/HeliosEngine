# Creating Custom Modules

This guide explains how to create custom modules for Helios Engine. Modules are self-contained libraries that extend the engine's functionality while maintaining a consistent build configuration and integration with the core ECS system.

## Table of Contents

- [Overview](#overview)
- [Module Structure](#module-structure)
- [Quick Start](#quick-start)
- [Step-by-Step Guide](#step-by-step-guide)
    - [1. Create Directory Structure](#1-create-directory-structure)
    - [2. Create Module.cmake (Optional)](#2-create-modulecmake-optional)
    - [3. Create CMakeLists.txt](#3-create-cmakeliststxt)
    - [4. Implement Your Module](#4-implement-your-module)
- [CMake Functions Reference](#cmake-functions-reference)
    - [helios_register_module](#helios_register_module)
    - [helios_add_module](#helios_add_module)
    - [helios_define_module](#helios_define_module)
- [Module Dependencies](#module-dependencies)
- [Build Options](#build-options)
- [Best Practices](#best-practices)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [See Also](#see-also)

---

## Overview

Helios Engine uses a modular architecture where functionality is organized into separate, independently buildable modules. Each module:

- Has its own build option (`HELIOS_BUILD_{NAME}_MODULE`)
- Can depend on other modules or external libraries
- Follows a standardized directory structure
- Integrates automatically with the engine's build system

## Module Structure

Every module follows this directory structure:

```
src/modules/{module_name}/
├── Module.cmake              # Optional: Module registration
├── CMakeLists.txt            # Required: Module build configuration
├── include/
│   └── helios/
│       └── {module_name}/
│           ├── {module_name}.hpp
│           └── ...
└── src/
    ├── {module_name}.cpp
    └── ...
```

### File Descriptions

| File             | Required    | Description                                                                  |
| ---------------- | ----------- | ---------------------------------------------------------------------------- |
| `Module.cmake`   | Optional    | Registers the module and its build option before processing `CMakeLists.txt` |
| `CMakeLists.txt` | Required    | Defines the module target using `helios_add_module()`                        |
| `include/`       | Recommended | Public headers exposed to users of the module                                |
| `src/`           | Recommended | Private implementation files                                                 |

---

## Quick Start

For a simple module, you can use the convenience function `helios_define_module()` which handles both registration and creation:

```cmake
# src/modules/my_module/CMakeLists.txt

helios_define_module(
    NAME my_module
    DESCRIPTION "My custom module"
    DEFAULT ON
    VERSION 1.0.0
    SOURCES
        src/my_module.cpp
    HEADERS
        include/helios/my_module/my_module.hpp
)
```

This single call:

1. Creates the build option `HELIOS_BUILD_MY_MODULE_MODULE`
2. Creates the library target `helios_module_my_module`
3. Creates the alias `helios::module::my_module`
4. Configures all standard settings (C++23, warnings, etc.)
5. Links to `helios::core` automatically

---

## Step-by-Step Guide

### 1. Create Directory Structure

```bash
mkdir -p src/modules/my_module/{include/helios/my_module,src}
```

### 2. Create Module.cmake (Optional)

The `Module.cmake` file is used for early registration of the module. This is useful when:

- You have complex dependencies that need to be resolved before building
- You want to configure build options with custom descriptions
- Your module has dependencies on other Helios modules

```cmake
# src/modules/my_module/Module.cmake

helios_register_module(
    NAME my_module
    DESCRIPTION "My custom module for Helios Engine"
    DEFAULT ON
    DEPENDS window              # Requires the window module
    OPTIONAL_DEPENDS audio      # Can use audio if available
    EXTERNAL_DEPENDS glfw       # Requires external GLFW library
)
```

### 3. Create CMakeLists.txt

```cmake
# src/modules/my_module/CMakeLists.txt

# If not using Module.cmake, register here (optional but recommended)
helios_register_module(
    NAME my_module
    DESCRIPTION "My custom module"
    DEFAULT ON
)

# Define headers
set(MY_MODULE_HEADERS
    include/helios/my_module/my_module.hpp
    include/helios/my_module/types.hpp
)

# Define sources
set(MY_MODULE_SOURCES
    src/my_module.cpp
    src/types.cpp
)

# Create the module
helios_add_module(
    NAME my_module
    VERSION 1.0.0

    HEADERS ${MY_MODULE_HEADERS}
    SOURCES ${MY_MODULE_SOURCES}

    # External dependencies
    PUBLIC_DEPENDENCIES
        glfw

    # Other Helios modules
    MODULE_DEPENDENCIES
        window

    # Optional: Precompiled header
    # PCH include/helios/my_module/my_module_pch.hpp
)
```

### 4. Implement Your Module

Create the header file:

```cpp
// include/helios/my_module/my_module.hpp

#pragma once

#include <helios/core/app.hpp>

namespace helios::my_module {

/// @brief Example component for the module.
struct MyComponent {
  int value = 0;
};

/// @brief Example system that operates on MyComponent.
struct MySystem final : public ecs::System {
  static constexpr std::string_view GetName() noexcept {
    return "MySystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return app::AccessPolicy().Query<MyComponent&>();
  }

  void Update(app::SystemContext& ctx) override;
};

/// @brief Module class for easy integration.
struct MyModule final : public app::Module {
  static constexpr std::string_view GetName() noexcept {
    return "MyModule";
  }

  void Build(app::App& app) override;
  void Destroy(app::App& app) override;
};

}  // namespace helios::my_module
```

Create the implementation:

```cpp
// src/my_module.cpp

#include <helios/my_module/my_module.hpp>

namespace helios::my_module {

void MySystem::Update(app::SystemContext& ctx) {
  auto query = ctx.Query().Get<MyComponent&>();

  query.ForEach([](MyComponent& comp) {
    ++comp.value;
  });
}

void MyModule::Build(app::App& app) {
  app.AddSystem<MySystem>(app::kUpdate);
}

void MyModule::Destroy(app::App& app) {
  // Cleanup if needed
}

}  // namespace helios::my_module
```

---

## CMake Functions Reference

### helios_register_module

Registers a module and creates its build option. Call this before `helios_add_module()` or use `Module.cmake`.

```cmake
helios_register_module(
    NAME <name>                     # Required: Module name (e.g., "window")
    DESCRIPTION <desc>              # Optional: Option description
    DEFAULT <ON|OFF>                # Optional: Default value (default: ON)
    DEPENDS <modules...>            # Optional: Required Helios modules
    OPTIONAL_DEPENDS <modules...>   # Optional: Optional Helios modules
    EXTERNAL_DEPENDS <packages...>  # Optional: Required external packages
)
```

**Effects:**

- Creates option `HELIOS_BUILD_{NAME}_MODULE`
- Stores metadata for dependency resolution
- Adds module to the registered modules list

### helios_add_module

Creates the module library target with full configuration.

```cmake
helios_add_module(
    NAME <name>                          # Required: Module name
    VERSION <version>                    # Optional: Version (default: 1.0.0)

    SOURCES <files...>                   # Source files (.cpp)
    HEADERS <files...>                   # Header files (.hpp)
    PUBLIC_HEADERS <files...>            # Alternative: Public headers
    PRIVATE_HEADERS <files...>           # Alternative: Private headers

    DEPENDENCIES <targets...>            # Public external dependencies
    PUBLIC_DEPENDENCIES <targets...>     # Explicit public dependencies
    PRIVATE_DEPENDENCIES <targets...>    # Private dependencies
    MODULE_DEPENDENCIES <modules...>     # Other Helios modules

    COMPILE_DEFINITIONS <defs...>        # Compile definitions
    COMPILE_OPTIONS <opts...>            # Compile options
    INCLUDE_DIRECTORIES <dirs...>        # Additional include directories

    PCH <file>                           # Precompiled header file

    # Library type options (mutually exclusive)
    HEADER_ONLY                          # Interface/header-only library
    STATIC_ONLY                          # Force static library
    SHARED_ONLY                          # Force shared library

    NO_CORE_LINK                         # Don't link to helios::core

    FOLDER <folder>                      # IDE folder (default: "Helios/Modules")
    OUTPUT_NAME <name>                   # Override output library name
    TARGET_NAME <name>                   # Override target name
)
```

**Creates:**

- Target: `helios_module_{name}`
- Alias: `helios::module::{name}`

### helios_define_module

Convenience wrapper that combines registration and creation.

```cmake
helios_define_module(
    NAME <name>
    DESCRIPTION <desc>
    DEFAULT <ON|OFF>
    VERSION <version>
    SOURCES <files...>
    HEADERS <files...>
    DEPENDENCIES <targets...>
    MODULE_DEPENDENCIES <modules...>
    EXTERNAL_DEPENDS <packages...>
    HEADER_ONLY | STATIC_ONLY | SHARED_ONLY
)
```

---

## Module Dependencies

### Depending on Other Helios Modules

Use `MODULE_DEPENDENCIES` to depend on other Helios modules:

```cmake
helios_add_module(
    NAME renderer
    SOURCES src/renderer.cpp
    MODULE_DEPENDENCIES
        window
        assets
)
```

The build system will:

1. Ensure dependent modules are built first
2. Link to `helios::module::window` and `helios::module::assets`
3. If a required module is disabled, it will be automatically enabled

### Depending on External Libraries

Use `DEPENDENCIES` or `PUBLIC_DEPENDENCIES`:

```cmake
helios_add_module(
    NAME graphics
    SOURCES src/graphics.cpp
    PUBLIC_DEPENDENCIES
        Vulkan::Vulkan
        glfw
    PRIVATE_DEPENDENCIES
        SPIRV-Tools
)
```

### Conditional Dependencies

For optional module integration:

```cmake
# In your CMakeLists.txt
helios_module_enabled(audio _audio_enabled)

if(_audio_enabled)
    target_compile_definitions(helios_module_my_module PRIVATE
        HELIOS_HAS_AUDIO_MODULE
    )
    target_link_libraries(helios_module_my_module PRIVATE
        helios::module::audio
    )
endif()
```

Or use the helper function:

```cmake
helios_link_modules_if_enabled(
    TARGET helios_module_my_module
    MODULES audio physics
)
```

---

## Build Options

Each registered module creates a build option:

```
HELIOS_BUILD_{NAME}_MODULE
```

For example:

- `my_module` → `HELIOS_BUILD_MY_MODULE_MODULE`
- `window` → `HELIOS_BUILD_WINDOW_MODULE`
- `physics_2d` → `HELIOS_BUILD_PHYSICS_2D_MODULE`

### Using Build Options

From command line:

```bash
# Enable specific modules
cmake -DHELIOS_BUILD_WINDOW_MODULE=ON -DHELIOS_BUILD_AUDIO_MODULE=OFF ..

# Or with the build script
python scripts/build.py -DHELIOS_BUILD_WINDOW_MODULE=ON
```

### Querying Module Status

In CMake:

```cmake
helios_module_enabled(window _is_enabled)
if(_is_enabled)
    message(STATUS "Window module is enabled")
endif()
```

In C++ code:

```cpp
#if defined(HELIOS_HAS_WINDOW_MODULE)
    // Window-specific code
#endif
```

---

## Best Practices

### 1. Use Consistent Naming

- Module directory: `snake_case` (e.g., `my_module`)
- Namespace: `helios::module_name` (e.g., `helios::my_module`)
- Target: automatically becomes `helios_module_{name}`
- Alias: automatically becomes `helios::module::{name}`

### 2. Organize Headers Properly

Public API goes in `include/helios/{name}/`:

```
include/helios/my_module/
├── my_module.hpp     # Main header (includes others)
├── ...
```

### 3. Minimize Public Dependencies

- Use `PRIVATE_DEPENDENCIES` for implementation details
- Only expose what users of your module need in `PUBLIC_DEPENDENCIES`

### 4. Document Your Module

Create a `README.md` in your module directory:

````markdown
# My Module

Brief description of what this module does.

## Features

- Feature 1
- Feature 2

## Dependencies

- window module
- GLFW (external)

## Usage

\```cpp
#include <helios/my_module/my_module.hpp>

app.AddModule<helios::my_module::MyModule>();
\```
````

### 5. Write Tests

Create tests in `tests/modules/my_module/`:

```cpp
#include <doctest/doctest.h>
#include <helios/my_module/my_module.hpp>

TEST_CASE("MyModule basic functionality") {
    helios::my_module::MyComponent comp{42};
    CHECK_EQ(comp.value, 42);
}
```

### 6. Use Precompiled Headers for Large Modules

```cmake
helios_add_module(
    NAME my_large_module
    SOURCES src/file1.cpp src/file2.cpp src/file3.cpp
    PCH include/helios/my_large_module/pch.hpp
)
```

---

## Examples

### Minimal Module

```cmake
# src/modules/simple/CMakeLists.txt

helios_define_module(
    NAME simple
    SOURCES src/simple.cpp
    HEADERS include/helios/simple/simple.hpp
)
```

### Module with Dependencies

```cmake
# src/modules/renderer/Module.cmake

helios_register_module(
    NAME renderer
    DESCRIPTION "Graphics rendering module"
    DEPENDS window
    EXTERNAL_DEPENDS Vulkan
)
```

```cmake
# src/modules/renderer/CMakeLists.txt

find_package(Vulkan REQUIRED)

helios_add_module(
    NAME renderer
    VERSION 1.0.0

    SOURCES
        src/renderer.cpp
        src/pipeline.cpp
        src/shader.cpp

    HEADERS
        include/helios/renderer/renderer.hpp
        include/helios/renderer/pipeline.hpp
        include/helios/renderer/shader.hpp

    MODULE_DEPENDENCIES
        window

    PUBLIC_DEPENDENCIES
        Vulkan::Vulkan

    PRIVATE_DEPENDENCIES
        SPIRV-Tools
)
```

### Header-Only Module

```cmake
# src/modules/math_utils/CMakeLists.txt

helios_define_module(
    NAME math_utils
    DESCRIPTION "Math utility functions (header-only)"
    HEADER_ONLY
    HEADERS
        include/helios/math_utils/math_utils.hpp
        include/helios/math_utils/vector.hpp
        include/helios/math_utils/matrix.hpp
)
```

### Module with Optional Features

```cmake
# src/modules/input/CMakeLists.txt

helios_register_module(
    NAME input
    DESCRIPTION "Input handling module"
    OPTIONAL_DEPENDS gamepad
)

helios_add_module(
    NAME input
    SOURCES src/input.cpp src/keyboard.cpp src/mouse.cpp
    HEADERS include/helios/input/input.hpp
    MODULE_DEPENDENCIES window
)

# Add optional gamepad support
helios_module_enabled(gamepad _gamepad_enabled)
if(_gamepad_enabled)
    target_sources(helios_module_input PRIVATE src/gamepad.cpp)
    target_compile_definitions(helios_module_input PUBLIC HELIOS_HAS_GAMEPAD)
    target_link_libraries(helios_module_input PUBLIC helios::module::gamepad)
endif()
```

---

## Troubleshooting

### Module Not Found

If you get an error about a module target not existing:

1. Check that the module directory exists under `src/modules/`
2. Verify `CMakeLists.txt` exists in the module directory
3. Check if the module is enabled: `-DHELIOS_BUILD_{NAME}_MODULE=ON`

### Dependency Errors

If a module dependency fails:

1. Check that the dependency module is registered before yours
2. Verify the dependency name is correct (case-sensitive)
3. Use `helios_print_modules()` to see all registered modules

### Build Order Issues

The build system automatically resolves dependencies, but if you have issues:

1. Use `Module.cmake` for registration (processed before `CMakeLists.txt`)
2. Ensure all `DEPENDS` are correctly specified
3. Check for circular dependencies

---

## See Also

- [Core Module Documentation](../../src/core/README.md)
- [CMake ModuleUtils.cmake](../../cmake/ModuleUtils.cmake)
- [Build System Documentation](../README.md)
