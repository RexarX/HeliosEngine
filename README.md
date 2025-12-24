<a name="readme-top"></a>

<!-- PROJECT SHIELDS -->

[![C++23][cpp-shield]][cpp-url]
[![MIT License][license-shield]][license-url]

[![Format Check](https://github.com/RexarX/HeliosEngine/workflows/Format%20Check/badge.svg)](https://github.com/RexarX/HeliosEngine/actions/workflows/format.yaml)
[![Linux GCC](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/build-test.yaml?branch=main&label=Linux%20GCC&logo=linux&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/build-test.yaml)
[![Linux Clang](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/build-test.yaml?branch=main&label=Linux%20Clang&logo=llvm&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/build-test.yaml)
[![Windows MSVC](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/build-test.yaml?branch=main&label=Windows%20MSVC&logo=windows&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/build-test.yaml)
[![macOS Clang](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/build-test.yaml?branch=main&label=macOS%20Clang&logo=apple&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/build-test.yaml)

<!-- PROJECT LOGO -->
<br />
<div align="center">
  <img src="https://github.com/RexarX/HeliosEngine/blob/main/docs/img/logo.png" alt="Helios Engine Logo" width="100" height="100">
  <h1 align="center">Helios Engine</h1>
  <p align="center">
    A modern, ECS based modular game engine framework built with C++23
    <br />
    <a href="#getting-started"><strong>Get Started »</strong></a>
    <br />
    <br />
    <a href="#key-features">Features</a>
    <a href="#architecture">Architecture</a>
    <a href="#usage-examples">Usage Examples</a>
    <a href="#contact">Contact</a>
  </p>
</div>

---

## Table of Contents

- [About The Project](#about-the-project)
    - [Key Features](#key-features)
    - [Design Philosophy](#design-philosophy)
- [Architecture](#architecture)
    - [Entity Component System (ECS)](#entity-component-system-ecs)
    - [Modular Design](#modular-design)
    - [Creating Custom Modules](#creating-custom-modules)
- [Getting Started](#getting-started)
    - [Requirements](#requirements)
    - [Dependencies](#dependencies)
    - [Quick Start](#quick-start)
    - [Build Options](#build-options)
- [Usage Examples](#usage-examples)
- [Core Module](#core-module)
- [Roadmap](#roadmap)
- [Acknowledgments](#acknowledgments)
- [License](#license)
- [Contact](#contact)

---

<a name="about-the-project"></a>

## About The Project

**Helios Engine** is a modern, high-performance game engine framework written in C++23 that embraces data-oriented design principles. Inspired by [Bevy](https://bevyengine.org/), it features a powerful Entity Component System (ECS) architecture that enables efficient, scalable game development.

<a name="key-features"></a>

### Key Features

- **ECS Architecture** - Archetype-based design built from scratch for C++23
- **High Performance** - Data-oriented design with cache-friendly memory layouts
- **Parallel Execution** - Multi-threaded system scheduling with automatic dependency resolution
- **Event System** - Type-safe, efficient event handling with readers/writers
- **Query System** - Powerful component queries with filters and optional components
- **Resource Management** - Global resources accessible from any system
- **Modern C++23** - Leverages coroutines, concepts, and ranges
- **Modular Design** - Build only what you need (Core, Renderer, Runtime)
- **Flexible Dependencies** - Conan, system packages, or automatic download via CPM

<a name="design-philosophy"></a>

### Design Philosophy

Helios Engine is built on three core principles:

1. **Data-Oriented Design**: Components are stored in contiguous memory (archetypes) for optimal cache performance
2. **Composability**: Entities are composed of components, behavior emerges from systems
3. **Explicitness**: System dependencies and data access patterns are declared explicitly

[↑ Back to Top](#readme-top)

---

<a name="architecture"></a>

## Architecture

<a name="project-entity-component-system-ecs"></a>

### Entity Component System (ECS)

Helios uses an **archetype-based ECS**, similar to Bevy:

#### **Entities**

- Unique identifiers (UUID-based)
- Lightweight containers for components
- Efficient creation, deletion, and querying

#### **Components**

- Plain data structures
- Stored contiguously in memory by archetype
- No inheritance required

```cpp
struct Transform {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
  float rotation = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
};

struct Health {
  int max_health = 100;
  int current_health = 100;

  bool IsDead() const noexcept { return current_health <= 0; }
};
```

#### **Systems**

- Classes that operate on entities with specific components
- Declare data access patterns at compile-time via `GetAccessPolicy()`
- Automatically parallelized based on data dependencies
- Access all data through `SystemContext`

#### **World**

- Central container for all entities and components
- Manages component storage and archetype transitions
- Provides query interface and entity operations

#### **Scheduling**

- Systems are organized into schedules (Main, PreUpdate, Update, PostUpdate, etc.)
- Automatic parallelization based on declared data access
- Conflict detection ensures data race freedom
- Custom schedules supported

<a name="modular-design"></a>

### Modular Design

The engine is divided into independent modules that can be enabled or disabled:

| Module   | Description                                 | Status   | Documentation                     |
| -------- | ------------------------------------------- | -------- | --------------------------------- |
| **Core** | ECS, async runtime, event system, utilities | Complete | [Core Module](src/core/README.md) |

Each module can be individually enabled or disabled via CMake options:

```bash
# Enable/disable specific modules
cmake -DHELIOS_BUILD_WINDOW_MODULE=ON -DHELIOS_BUILD_AUDIO_MODULE=OFF ..

# Build only core (no modules)
python scripts/build.py --core-only
```

<a name="creating-custom-modules"></a>

### Creating Custom Modules

Helios Engine provides a streamlined system for creating custom modules. Each module:

- Has its own build option (`HELIOS_BUILD_{NAME}_MODULE`)
- Can depend on other modules or external libraries
- Follows a standardized directory structure
- Integrates automatically with the build system

**Quick Example:**

```cmake
# src/modules/my_module/CMakeLists.txt
helios_define_module(
    NAME my_module
    DESCRIPTION "My custom module"
    SOURCES src/my_module.cpp
    HEADERS include/helios/my_module/my_module.hpp
)
```

**[Full Module Creation Guide](docs/guides/creating-modules.md)** - Comprehensive documentation on creating custom modules, including:

- Directory structure and conventions
- CMake function reference (`helios_register_module`, `helios_add_module`, `helios_define_module`)
- Module dependencies and build options
- Best practices and examples

[↑ Back to Top](#readme-top)

---

<a name="getting-started"></a>

## Getting Started

<a name="requirements"></a>

### Requirements

| Tool             | Minimum Version                | Recommended        |
| ---------------- | ------------------------------ | ------------------ |
| **CMake**        | 3.25+                          | 3.28+              |
| **C++ Compiler** | GCC 14+, Clang 19+, MSVC 2022+ | GCC 15+, Clang 21+ |
| **Python**       | 3.8+                           | 3.10+              |

**Compiler Support:**

- GCC 14+ (tested on 14.2.0)
- Clang 19+ (tested on 20.1.8)
- MSVC 19.34+ (Visual Studio 2022 17.4+)

<a name="dependencies"></a>

### Dependencies

#### Core Dependencies

- [Boost](https://www.boost.org/) (1.87+) - stacktrace, pool, unordered containers
- [spdlog](https://github.com/gabime/spdlog) (1.12+) - Fast logging
- [stduuid](https://github.com/mariusbancila/stduuid) (1.2+) - UUID generation
- [Taskflow](https://github.com/taskflow/taskflow) (3.10+) - Parallel task programming

#### Test Dependencies

- [doctest](https://github.com/doctest/doctest) (2.4+) - Testing framework

#### Installation Methods

**Option 1: Conan (Recommended)**

```bash
# Quick install with automatic Conan setup
make install-deps

# Or manually with Python script
python scripts/install-deps.py
```

**Option 2: System Packages**

```bash
# Arch Linux
sudo pacman -S boost intel-tbb spdlog

# Ubuntu/Debian (22.04+)
sudo apt install libboost-all-dev libtbb-dev libspdlog-dev

# Fedora
sudo dnf install boost-devel tbb-devel spdlog-devel

# macOS
brew install boost tbb spdlog
```

**Option 3: CPM (Automatic Fallback)**

- Dependencies are automatically downloaded if not found
- No manual intervention required

<a name="quick-start"></a>

### Quick Start

#### 1. Clone the Repository

```bash
git clone --recursive https://github.com/RexarX/HeliosEngine.git
cd HeliosEngine
```

#### 2. Install Dependencies

```bash
# Interactive installation (asks about Conan)
make install-deps

# Or directly with Python
python scripts/install_deps.py
```

#### 3. Configure the Project

```bash
# Interactive configuration
python scripts/configure.py

# Or non-interactive with specific options
python scripts/configure.py --type Release --compiler gcc --use-conan
```

#### 4. Build the Project

```bash
# Build (calls configure.py first if needed)
make build

# Or directly with Python
python scripts/build.py

# Specific build configurations
python scripts/build.py --type Debug --core-only --tests
```

#### 5. Run Tests

```bash
# Run tests
make test BUILD_TYPE={debug,relwithdebinfo,release}

# Or run specific test manually with ctest
cd build/debug/linux  # or your platform
ctest -L core
```

<a name="build-options"></a>

### Build Options

#### Using Makefile (Recommended)

```bash
# Install dependencies
make install-deps

# Configure and build
make build

# Or configure separately
make configure
make build

# Clean and rebuild
make clean build
```

#### Using Python Scripts Directly

```bash
# 1. Install dependencies (interactive)
python scripts/install_deps.py

# 2. Configure CMake (interactive)
python scripts/configure.py

# 3. Build the project
python scripts/build.py

# Or with specific options (non-interactive)
python scripts/install_deps.py --use-conan --no-interactive
python scripts/configure.py --type Release --compiler gcc --use-conan --no-interactive
python scripts/build.py --type Release --jobs 8
```

#### Using CMake Presets

```bash
# List available presets
cmake --list-presets

# Configure with preset
cmake --preset linux-gcc-debug

# Build
cmake --build --preset linux-gcc-debug

# Test
ctest --preset linux-gcc-debug
```

**Available Presets:**

- `linux-gcc-debug` / `linux-gcc-release` / `linux-gcc-relwithdebinfo`
- `linux-clang-debug` / `linux-clang-release` / `linux-clang-relwithdebinfo`
- `windows-msvc-debug` / `windows-msvc-release` / `windows-msvc-relwithdebinfo`
- `macos-clang-debug` / `macos-clang-release` / `macos-clang-relwithdebinfo`
- `dev` - Development preset with all features
- `ci-*` - CI presets with strict warnings

[↑ Back to Top](#readme-top)

---

## Usage Examples

### Basic Application Setup

```cpp
#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>

using namespace helios::app;
using namespace helios::ecs;

// Define a simple system
struct TimeUpdateSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "TimeUpdateSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().WriteResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    time.delta_time = 0.016F;
    time.total_time += time.delta_time;
    ++time.frame_count;
  }
};

int main() {
  App app;

  // Insert resources
  app.InsertResource(GameTime{});

  // Add systems to schedules
  app.AddSystem<TimeUpdateSystem>(kMain);

  // Run the application
  return std::to_underlying(app.Run());
}
```

### Creating Entities

Systems can create entities through `SystemContext`:

```cpp
struct SetupSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "SetupSystem";
  }

  static constexpr AccessPolicy GetAccessPolicy() noexcept {
    return {};
  }

  void Update(SystemContext& ctx) override {
    // Reserve an entity and get its command buffer
    auto entity_cmd = ctx.EntityCommands(ctx.ReserveEntity());

    // Add components to the entity
    entity_cmd.AddComponents(
        Transform{0.0F, 0.0F, 0.0F},
        Velocity{1.0F, 0.0F, 0.0F},
        Health{100, 100});
  }
```

### Querying Entities

Systems query entities using `ctx.Query()`:

```cpp
struct MovementSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "MovementSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Transform&, const Velocity&>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    auto query = ctx.Query().Get<Transform&, const Velocity&>();

    query.ForEach([&time](Transform& transform, const Velocity& velocity) {
      transform.x += velocity.dx * time.delta_time;
      transform.y += velocity.dy * time.delta_time;
      transform.z += velocity.dz * time.delta_time;
    });
  }
```

### Using Modules

Organize systems and resources into reusable modules:

```cpp
struct GameModule final : public Module {
  void Build(App& app) override {
    app.InsertResource(GameTime{})
        .InsertResource(GameStats{})
        .AddSystem<TimeUpdateSystem>(kMain)
        .AddSystem<MovementSystem>(kUpdate)
        .AddSystem<RenderSystem>(kPostUpdate);
  }

  void Destroy(App& app) override {}

  static constexpr std::string_view GetName() noexcept {
    return "GameModule";
  }
};

// Use the module
App app;
app.AddModule<GameModule>();
```

[↑ Back to Top](#readme-top)

---

<a name="core-module"></a>

## Core Module

### Overview

See the [Core Module Documentation](src/core/README.md) for details.

[↑ Back to Top](#readme-top)

---

<a name="roadmap"></a>

## Roadmap

### Completed

- [x] Core ECS implementation (archetype-based)
- [x] System scheduling with automatic parallelization
- [x] Event system with readers/writers
- [x] Resource management
- [x] Query system with filters and optional components
- [x] Command buffers
- [x] Comprehensive test suite
- [x] Cross-platform build system (Linux, Windows, macOS)
- [x] Flexible dependency management (Conan, system, CPM)
- [x] Python build scripts with full automation

### In Progress

#### **Runtime modules**

- Actual engine implementation (scenes, assets, scripting, etc.)

#### **Renderer modules**

- Modern graphics API abstraction
- **[DiligentEngine](https://github.com/DiligentGraphics/DiligentEngine)** integration for cross-platform rendering

[↑ Back to Top](#readme-top)

---

<a name="acknowledgments"></a>

## Acknowledgments

This project was inspired by and builds upon ideas from:

- [Bevy Engine](https://bevyengine.org/) - ECS architecture and design philosophy
- [EnTT](https://github.com/skypjack/entt) - High-performance ECS implementation
- [Taskflow](https://taskflow.github.io/) - Modern parallel task programming
- [DiligentEngine](https://github.com/DiligentGraphics/DiligentEngine) - Cross-platform graphics engine

[↑ Back to Top](#readme-top)

---

<a name="license"></a>

## License

Distributed under the MIT License. See `LICENSE` for more information.

[↑ Back to Top](#readme-top)

---

<a name="contact"></a>

## Contact

**RexarX** - who727cares@gmail.com

**Project Link:** [https://github.com/RexarX/HeliosEngine](https://github.com/RexarX/HeliosEngine)

[↑ Back to Top](#readme-top)

<!-- MARKDOWN LINKS & IMAGES -->

[license-shield]: https://img.shields.io/github/license/RexarX/HeliosEngine.svg?style=for-the-badge
[license-url]: https://github.com/RexarX/HeliosEngine/blob/main/LICENSE
[cpp-shield]: https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=for-the-badge&logo=c%2B%2B
[cpp-url]: https://en.cppreference.com/w/cpp/23
