# Example Module

A minimal example module demonstrating the Helios Engine module system.

## Overview

This module serves as a template and reference for creating custom modules. It shows the basic structure and patterns used in Helios Engine modules, including:

- Module registration with `Module.cmake`
- Module build configuration with `CMakeLists.txt`
- Component, Resource, System, and Module class definitions
- Proper namespace and file organization

## Build Option

This module can be enabled or disabled via CMake:

```bash
-DHELIOS_BUILD_EXAMPLE_MODULE=ON   # Enable (default)
-DHELIOS_BUILD_EXAMPLE_MODULE=OFF  # Disable
```

## Directory Structure

```
example/
├── Module.cmake                           # Module registration (creates build option)
├── CMakeLists.txt                         # Module build configuration
├── README.md                              # This file
├── include/
│   └── helios/
│       └── example/
│           └── example.hpp                # Public header
└── src/
    └── example.cpp                        # Implementation
```

## Usage

### Adding the Module to Your App

```cpp
#include <helios/core/app/app.hpp>
#include <helios/example/example.hpp>

int main() {
    helios::app::App app;

    // Add the example module
    app.AddModule<helios::example::ExampleModule>();

    return std::to_underlying(app.Run());
}
```

### Using the Example Component

```cpp
#include <helios/example/example.hpp>

// Create an entity with ExampleComponent
auto entity_cmd = ctx.EntityCommands(ctx.ReserveEntity());
entity_cmd.AddComponents(helios::example::ExampleComponent{42});
```

### Querying the Example Resource

```cpp
#include <helios/example/example.hpp>

// In a system's Update method:
const auto& resource = ctx.ReadResource<helios::example::ExampleResource>();
int current_count = resource.counter;
```

## Components

### ExampleComponent

A simple component with an integer value.

```cpp
struct ExampleComponent final {
    int value = 0;
};
```

## Resources

### ExampleResource

A simple resource with a counter that is incremented by the ExampleSystem.

```cpp
struct ExampleResource final {
    int counter = 0;
};
```

## Systems

### ExampleSystem

A system that increments the counter in ExampleResource each update.

- **Schedule**: Update
- **Access**: WriteResources<ExampleResource>

## Creating Your Own Module

Use this module as a starting point:

1. Copy the `example/` directory to `src/modules/your_module/`
2. Rename files and update namespaces
3. Modify `Module.cmake` with your module's registration
4. Update `CMakeLists.txt` with your sources and dependencies
5. Implement your components, systems, and module class

For detailed instructions, see [Creating Custom Modules](../../../docs/guides/creating-modules.md).
