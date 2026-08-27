<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->

[![C++23][cpp-shield]][cpp-url]
[![MIT License][license-shield]][license-url]

[![Linux GCC](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/linux-gcc.yaml?branch=main&label=Linux%20GCC&logo=linux&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/linux-gcc.yaml)
[![Linux Clang](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/linux-clang.yaml?branch=main&label=Linux%20Clang&logo=llvm&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/linux-clang.yaml)
[![Windows MSVC](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/windows-msvc.yaml?branch=main&label=Windows%20MSVC&logo=windows&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/windows-msvc.yaml)
[![macOS Clang](https://img.shields.io/github/actions/workflow/status/RexarX/HeliosEngine/macos-clang.yaml?branch=main&label=macOS%20Clang&logo=apple&logoColor=white)](https://github.com/RexarX/HeliosEngine/actions/workflows/macos-clang.yaml)

<!-- PROJECT LOGO -->

<br />

<div align="center">

<img src="docs/img/logo.png" alt="Helios Engine Logo" width="200">

# Helios Engine

A modular, data-oriented C++23 game engine framework inspired by Bevy

<b><a href="#getting-started">Get Started »</a></b> · <a href="#key-features">Features</a> · <a href="#modules">Modules</a> · <a href="#building">Build</a> · <a href="#using-as-a-dependency">Third-Party</a> · <a href="#documentation">Docs</a>

</div>

---

## Table of Contents

- <a href="#about-the-project">About The Project</a>
  - <a href="#key-features">Key Features</a>
  - <a href="#design-philosophy">Design Philosophy</a>
- <a href="#modules">Modules</a>
- <a href="#getting-started">Getting Started</a>
  - <a href="#requirements">Requirements</a>
  - <a href="#installing-dependencies">Installing Dependencies</a>
  - <a href="#building">Building</a>
  - <a href="#c20-modules">C++20 modules</a>
  - <a href="#linking">Linking</a>
  - <a href="#run-the-example">Run the Example</a>
- <a href="#usage">Usage</a>
- <a href="#using-as-a-dependency">Using as a Dependency</a>
  - <a href="#method-1-add_subdirectory">add_subdirectory</a>
  - <a href="#method-2-fetchcontent">FetchContent</a>
  - <a href="#method-3-cpm">CPM</a>
  - <a href="#method-4-installed-package-find_package">Installed Package (find_package)</a>
- <a href="#documentation">Documentation</a>
- <a href="#development">Development</a>
  - <a href="#creating-a-custom-module">Creating a Custom Module</a>
- <a href="#roadmap">Roadmap</a>
- <a href="#acknowledgments">Acknowledgments</a>
- <a href="#license">License</a>
- <a href="#contact">Contact</a>

---

## About The Project

**Helios Engine** is a high-performance, ECS-based game engine framework written in C++23. It combines an configurable sparse-set/archetype based Entity Component System with deferred commands, double-buffered messages, and parallel system scheduling over a work-stealing task executor.

<a href="#readme-top">↑ Back to Top</a>

### Key Features

- **ECS** — archetype and sparse-set storage, deferred `Commands`, rich query iterators
- **Parallel scheduling** — access-conflict detection, topological execution, Taskflow-backed executors
- **Application layer** — `App` / `SubApp` lifecycle, builtin schedules, static and dynamic plugins
- **Modular build** — independent modules; enable only what you need
- **Modern C++23** — concepts, ranges, `std::expected`, PMR allocators
- **Flexible dependencies** — system packages first, [CPM](https://github.com/cpm-cmake/CPM.cmake) download as fallback

### Design Philosophy

1. **Data-oriented design** — components stored contiguously for cache-friendly iteration
2. **Composability** — behavior emerges from systems operating on component data
3. **Explicitness** — data access declared through system parameter types; schedules resolve ordering

<a href="#readme-top">↑ Back to Top</a>

---

## Modules

| Module        | Description                                              | Default | Documentation                       |
| ------------- | -------------------------------------------------------- | ------- | ----------------------------------- |
| `core`        | Asserts, UUID, stack traces, CStringView, etc.           | ON      | [README](src/core/README.md)        |
| `platform`    | Platform detection, `HELIOS_API`, debug break, etc.      | ON      | [README](src/platform/README.md)    |
| `compiler`    | Branch hints, feature detection macros, etc.             | ON      | [README](src/compiler/README.md)    |
| `utils`       | TypeId, timer, filesystem, adapters, etc.                | ON      | [README](src/utils/README.md)       |
| `container`   | SparseSet, MultiTypeMap, TypedBuffer, StaticString, etc. | ON      | [README](src/container/README.md)   |
| `memory`      | PMR allocators, `Rc`/`Arc`, etc.                         | ON      | [README](src/memory/README.md)      |
| `log`         | spdlog-based typed logging                               | ON      | [README](src/log/README.md)         |
| `async`       | Task graphs and work-stealing executor                   | ON      | [README](src/async/README.md)       |
| `ecs`         | World, entities, components, schedules, etc.             | ON      | [README](src/ecs/README.md)         |
| `app`         | Application framework, plugins, sub-apps, etc.           | ON      | [README](src/app/README.md)         |
| `profile`     | Tracy / flamegraph profiling (opt-in), etc.              | ON      | [README](src/profile/README.md)     |
| `window`      | Window ECS contract (no OS deps)                         | ON      | [README](src/window/README.md)      |
| `sdl3`        | SDL3 process runtime (init, event pump, window map)      | ON      | [README](src/sdl3/README.md)        |
| `sdl3_window` | Default SDL3 window backend                              | ON      | [README](src/sdl3_window/README.md) |
| `sdl3_input`  | Default SDL3 input backend                               | ON      | [README](src/sdl3_input/README.md)  |
| `glfw`        | Optional GLFW backend for `window` (+ optional `input`)  | OFF     | [README](src/glfw/README.md)        |
| `input`       | Keyboard, mouse, cursor, gamepad ECS contract            | ON      | [README](src/input/README.md)       |

```bash
cmake --preset linux-gcc-release -DHELIOS_BUILD_PROFILE=ON -DHELIOS_BUILD_WINDOW=OFF
```

<a href="#readme-top">↑ Back to Top</a>

---

## Getting Started

### Requirements

| Tool             | Minimum                         | Recommended                 |
| ---------------- | ------------------------------- | --------------------------- |
| **CMake**        | 3.25+                           | 3.28+                       |
| **C++ compiler** | GCC 14+, Clang 19+, MSVC 19.34+ | GCC 15+, Clang 21+          |
| **Generator**    | —                               | Ninja                       |
| **Python**       | 3.8+                            | 3.10+ (scripts, pre-commit) |

```bash
git clone https://github.com/RexarX/HeliosEngine.git
cd HeliosEngine
```

Clone with submodules if you intend to build docs locally:

```bash
git clone --recursive https://github.com/RexarX/HeliosEngine.git
cd HeliosEngine
```

### Installing Dependencies

Helios resolves dependencies per module: **system packages are tried first**, then **CPM download** if missing (`HELIOS_DOWNLOAD_PACKAGES=ON`, default). Pre-installing system packages speeds up configuration and avoids network fetches.

Some packages need extra OS development libraries. On Linux, vendored `glfw` and
`sdl3` both require X11/Wayland headers. SDL3 also needs Xfixes, XScrnSaver, and
XTest (`libxfixes-dev`, `libxss-dev`, `libxtst-dev`); configure fails if those
are missing. Windows and macOS need no extra packages for these backends.

Package names below match `INSTALL_HINTS` in [`cmake/dependencies/`](cmake/dependencies/).

#### All platforms — build tools

| Tool         | Purpose                                 |
| ------------ | --------------------------------------- |
| CMake ≥ 3.25 | Configure and build                     |
| Ninja        | Recommended generator (used by presets) |
| clang-format | Code formatting (`scripts/format.py`)   |
| Doxygen      | API docs (`scripts/docs.py`) — optional |

#### Linux (APT — Ubuntu / Debian)

```bash
sudo apt-get update
sudo apt-get install -y ninja-build libboost-all-dev libtbb-dev
```

Additional tools:

```bash
sudo apt-get install -y clang-format clang-tidy doxygen
```

Wayland/X11 (GLFW + SDL3):

```bash
sudo apt-get install -y libwayland-dev libxkbcommon-dev libx11-dev \
  libxrandr-dev libxinerama-dev libxi-dev libxcursor-dev libxext-dev \
  libxfixes-dev libxss-dev libxtst-dev
```

#### Linux (DNF — Fedora)

```bash
sudo dnf install -y ninja-build  boost-devel tbb-devel

```

Additional tools:

```bash
sudo dnf install -y clang-tools-extra doxygen
```

Wayland/X11 (GLFW + SDL3):

```bash
sudo dnf install -y wayland-devel libxkbcommon-devel libX11-devel \
  libXrandr-devel libXinerama-devel libXi-devel libXcursor-devel libXext-devel \
  libXfixes-devel libXScrnSaver-devel libXtst-devel
```

#### Linux (Pacman — Arch)

```bash
sudo pacman -S --needed ninja boost tbb

```

Additional tools:

```bash
sudo pacman -S --needed clang doxygen
```

Wayland/X11 (GLFW + SDL3):

```bash
sudo pacman -S --needed wayland libxkbcommon libx11 \
  libxrandr libxinerama libxi libxcursor libxext \
  libxfixes libxss libxtst
```

#### macOS (Homebrew)

```bash
brew install cmake ninja boost
```

Additional tools:

```bash
brew install clang-format doxygen
```

> **Note:** On Linux with **GCC + libstdc++**, [TBB](https://github.com/oneapi-src/oneTBB) is required for parallel STL (`libtbb-dev` / `tbb-devel` / `onetbb`). Clang on Linux and all macOS builds use libc++ and do not require TBB. spdlog is auto-downloaded via CPM when using Clang on Linux (system Ubuntu packages are incompatible).

#### Windows (MSVC)

No system packages required for a minimal build — MSVC + Ninja (via Visual Studio) is sufficient; missing libraries are fetched by CPM.

```bat
# Optional: LLVM clang-format for local formatting and clang-tidy for linting
choco install llvm
# Or use clang-format and clang-tidy bundled with Visual Studio 2022
```

### Building

Preset pattern: `{os}-{compiler}-{build_type}`

```bash
cmake --list-presets          # configure presets
cmake --list-presets build    # build presets
cmake --list-presets test     # test presets
```

#### Linux (GCC)

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release
```

#### Linux (Clang)

```bash
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release
ctest --preset linux-clang-release
```

#### Windows

Manually after `vcvars64.bat`:

```bat
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
```

Or generate Visual Studio solution and build from IDE:

```bat
cmake --preset windows-vs-release
```

#### macOS (Clang)

```bash
cmake --preset macos-clang-release
cmake --build --preset macos-clang-release
ctest --preset macos-clang-release
```

#### Recommended developer flags

```bash
cmake --preset linux-gcc-relwithdebinfo \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  #-DHELIOS_DEVELOPER_MODE=ON \
```

| Option                      | Default                    | Notes                                                |
| --------------------------- | -------------------------- | ---------------------------------------------------- |
| `HELIOS_BUILD_TESTS`        | ON (top-level)             | Module test suites                                   |
| `HELIOS_BUILD_EXAMPLES`     | ON (top-level)             | Example applications                                 |
| `HELIOS_ENABLE_CPP_MODULES` | OFF                        | C++20 named modules (`import helios` / `helios.*`)   |
| `HELIOS_DEVELOPER_MODE`     | OFF                        | Sanitizers and dev checks                            |
| `HELIOS_DOWNLOAD_PACKAGES`  | ON                         | CPM fallback for missing deps                        |
| `HELIOS_BUILD_{MODULE}`     | module default             | Per-module toggle                                    |
| `HELIOS_LINKER`             | AUTO (top-level) / DEFAULT | Fast linker: `AUTO`, `MOLD`, `LLD`, `RAD`, `DEFAULT` |
| `HELIOS_MANAGE_TOOLCHAIN`   | ON (top-level) / OFF       | Set `CMAKE_LINKER` / `CMAKE_AR` project-wide         |

### C++20 modules

Default builds stay **headers + PCH**. Named modules are opt-in.
To enable them just provide `-DHELIOS_ENABLE_CPP_MODULES=ON` at configure time.

Requires **CMake 3.28+** and **Ninja** or Visual Studio 17.4+. Unix Makefiles fail at configure. `import std` is probed separately (`HELIOS_ENABLE_IMPORT_STD`); it needs CMake 3.30+ and a toolchain that can compile `import std;`.

> **Note:** Most of compilers still have incomplete C++20 module support, and some have bugs. Make sure to use latest compiler versions for better experience.

**How it is implemented.** Each engine module still ships textual headers. When modules are on, a `.cppm` interface unit (`modules/helios.<name>.cppm`, listed in `MODULE_SOURCES`) wraps those headers in Boost-style `export extern "C++"` so declarations keep classic C++ ABI.
`#include <helios/...>` and `import helios.X` name the same types and link the same library.
Implementation `.cpp` files stay classic translation units (no `module helios.X;`).
Named modules cannot export macros — `#include` `<helios/assert.hpp>`, `<helios/compiler/compiler.hpp>`, `<helios/platform/platform.hpp>`, or `<helios/utils/macro.hpp>` when you need those macros.

| Consumer code                                      | Link                          |
| -------------------------------------------------- | ----------------------------- |
| `import helios.app;` (or `ecs`, `log`, ...)        | `helios::module::app`         |
| `import helios.sdl3.window;`                       | `helios::module::sdl3_window` |
| `import helios;` (re-exports every enabled module) | `helios::helios`              |

On MSVC without `import std`, put standard-library `#include`s **before** `import`. Do not scan TUs that only `#include` Helios — MSVC injects `import` into every scanned file.

A runnable `import` sample is configured only when this flag is on: [examples/cxx_modules](examples/cxx_modules).

```bash
cmake --build --preset linux-gcc-debug --target cxx_modules_example
```

**Using Helios modules from another project.** Enable the flag when Helios is configured, link the same `helios::module::*` targets, and scan only the sources that `import`:

```cmake
set(HELIOS_ENABLE_CPP_MODULES ON CACHE BOOL "" FORCE)
set(HELIOS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(HELIOS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/HeliosEngine)

add_executable(my_game src/main.cpp)
helios_link_modules(TARGET my_game MODULES PUBLIC app log)
helios_source_scan_for_cxx_modules(my_game src/main.cpp)
```

```cpp
import helios.app;

int main() {
  helios::app::App app;
  return std::to_underlying(app.Run());
}
```

`helios_link_modules()` already marks the consumer as a named-module client. `helios_source_scan_for_cxx_modules()` is required on each TU that contains `import`. Header-only TUs must stay unscanned.

A full `App` loop with `#include` is in [examples/simple](examples/simple). Instantiating `App` after `import helios.app` currently fails on MSVC (Taskflow `C1116`); import still exposes other app types.

### Linking

Helios picks a faster linker when one is installed (`cmake/helpers/Linker.cmake`). Selection is controlled by `HELIOS_LINKER`:

| Value     | Meaning                                                                                                              |
| --------- | -------------------------------------------------------------------------------------------------------------------- |
| `AUTO`    | Default for a top-level Helios build. Prefer the fastest available linker for the platform.                          |
| `MOLD`    | Force [mold](https://github.com/rui314/mold) (`mold` on `PATH`). ELF only; not used on macOS.                        |
| `LLD`     | Force LLVM lld (`ld.lld` on Unix, `lld-link` on the MSVC ABI).                                                       |
| `RAD`     | Force [RAD Linker](https://github.com/EpicGames/raddebugger) (`radlink` on `PATH` or `$RAD_ROOT`). Windows MSVC ABI. |
| `DEFAULT` | Platform default (`ld` / Apple `ld` / MSVC `link.exe`). Default when Helios is embedded.                             |

**`AUTO` by platform**

- **Linux (ELF):** [mold](https://github.com/rui314/mold) if found, otherwise lld, otherwise the system linker.
- **macOS:** mold is skipped (it does not link Mach-O). lld is used if `ld.lld` is on `PATH`; otherwise Apple `ld`.
- **Windows (MSVC):** [RAD Linker](https://github.com/EpicGames/raddebugger) for Debug and non-LTO RelWithDebInfo when `radlink` is available, then `lld-link`, then `link.exe`. **clang-cl** prefers `lld-link` (RAD can fail on some clang-cl C++ COMDATs). Release LTO (MSVC LTCG) and RelWithDebInfo with `HELIOS_ENABLE_LTO_RELWITHDEBINFO=ON` keep `link.exe`; clang-cl LTO uses `lld-link`. RAD is not used with MSVC AddressSanitizer (`HELIOS_DEVELOPER_MODE=ON`).

Override at configure time (must be on `PATH`, or for RAD also `$RAD_ROOT`):

```bash
cmake --preset linux-gcc-release -DHELIOS_LINKER=MOLD
cmake --preset linux-clang-release -DHELIOS_LINKER=LLD
cmake --preset windows-msvc-debug -DHELIOS_LINKER=RAD
cmake --preset linux-gcc-release -DHELIOS_LINKER=DEFAULT   # disable fast linkers
```

**`HELIOS_MANAGE_TOOLCHAIN`** (see `CMakeLists.txt` and `cmake/helpers/ClangArchiver.cmake`)

When `ON` (default for a standalone Helios build), Helios may set `CMAKE_LINKER`, `CMAKE_AR`, and `CMAKE_RANLIB` for the **whole** CMake project, and apply `-fuse-ld=` / linker type globally.

When `OFF` (default if Helios is added via `add_subdirectory` / FetchContent), those cache variables are left to the parent. A selected fast linker is applied only to Helios targets. Consumers can opt in:

```bash
cmake -DHELIOS_MANAGE_TOOLCHAIN=ON -DHELIOS_LINKER=AUTO ...
```

Per-config RAD vs `link.exe` on the Visual Studio generator needs CMake 3.29+; Ninja presets work from CMake 3.25.

### Run the Example

```bash
cmake --preset linux-gcc-release -DHELIOS_BUILD_EXAMPLES=ON
cmake --build --preset linux-gcc-release --target simple_example
./bin/examples/debug-linux-x86_64/simple_example
```

See [examples/simple/simple.cpp](examples/simple/simple.cpp) for schedules and plugins.
With `-DHELIOS_ENABLE_CPP_MODULES=ON`, [examples/cxx_modules](examples/cxx_modules) shows `import helios.app` and `import helios.container`.

<a href="#readme-top">↑ Back to Top</a>

---

## Usage

Systems are plain structs — `operator()` parameters declare data access or regular C++ lambdas.
`App` owns the main world, executor, and frame scheduler.

```cpp
#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>

#include <utility>

inline constexpr float kVelocityBoost = 1.25F;

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 1.0F;
  float dy = 0.0F;
};

struct SpawnEntities {
  void operator()(helios::ecs::Res<const helios::app::FrameCount> frame_count,
                  helios::ecs::Commands commands) {
    // Spawning new entity every even frame
    if (frame_count->count % 2 == 0) {
      commands.Spawn().AddComponents(Position{}, Velocity{});
    }
  }
};

struct ChangePosition {
  void operator()(helios::ecs::Query<Position&, const Velocity&> movables) {
    movables.ForEach([](Position& pos, const Velocity& vel) {
      pos.x += vel.dx;
      pos.y += vel.dy;
    });
  }
};

struct ChangeVelocity {
  void operator()(helios::ecs::Query<Velocity&> movables) {
    movables.ForEach([](Velocity& vel) {
      vel.dx *= kVelocityBoost;
      vel.dy *= kVelocityBoost;
    });
  }
};

struct RequestExit {
  void operator()(helios::ecs::MessageWriter<helios::app::AppExit> exit) {
    // Sending message that would be handled in the next schedule
    exit.Write(helios::app::AppExit::Success());
  }
};

int main() {
  helios::app::App app;
  app.AddPlugins(helios::app::FrameCountPlugin{});

  app.AddSystem(helios::app::kPreUpdate, SpawnEntities{}); // Spawn entities, before modifying
  app.AddSystems(helios::app::kUpdate, ChangePosition{}, ChangeVelocity{})
      .Sequence(); // Sytems will run one after another (ChangePosition -> ChangeVelocity)
  app.AddSystem(helios::app::kPostUpdate, RequestExit{}) // Shutdown after 500 frames
      .RunIf("IsFrameCountReached",
             [](helios::ecs::Res<const helios::app::FrameCount> frame_count) {
               return frame_count->count > 500;
             });

  app.AddMessages<helios::app::AppExit>();
  return std::to_underlying(app.Run());
}
```

| Parameter                                         | Purpose                                                                                   |
| ------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| `Res<T>` / `Res<const T>`                         | Singleton resource access                                                                 |
| `Query<Args...>`                                  | Entity iteration with `With<>` / `Without<>` filters                                      |
| `Commands`                                        | Deferred spawn, destroy, component, and resource mutations                                |
| `Local<T>`                                        | Per-system persistent state                                                               |
| `MessageWriter<T>` / `MessageReader<T>`           | Frame-delayed inter-system messages Live two frames by default, visible between schedules |
| `AsyncMessageWriter<T>` / `AsyncMessageReader<T>` | Lock-free async messages between systems                                                  |

Builtin schedules are available through `<helios/app/app.hpp>`: `MainStartup` → `PreStartup` → `Startup` → `PostStartup` → `First` → `PreUpdate` → `Update` → `PostUpdate` → `Last` → `Extract` → shutdown stages.
By default, `MainStartup` and `Shutdown` use the main-thread schedule executor; the other built-in schedules, including `First` and `Last`, use the multi-threaded executor.

<a href="#readme-top">↑ Back to Top</a>

---

## Using as a Dependency

Helios can be consumed from another CMake project in several ways. All methods expose targets as `helios::module::<name>` and the helper `helios_link_modules()`.

Typical consumer settings when embedding:

```cmake
set(HELIOS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(HELIOS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(HELIOS_ENABLE_CPP_MODULES ON CACHE BOOL "" FORCE)  # import helios.*
# Optional overrides (safer defaults already apply when embedded):
# set(HELIOS_ENABLE_LTO OFF CACHE BOOL "" FORCE)
# set(HELIOS_ENABLE_LTO_RELWITHDEBINFO OFF CACHE BOOL "" FORCE)
# set(HELIOS_MANAGE_TOOLCHAIN OFF CACHE BOOL "" FORCE)
# set(HELIOS_LINKER DEFAULT CACHE STRING "" FORCE)
```

### Method 1: `add_subdirectory`

Best for vendoring a copy inside your tree (e.g. `third_party/HeliosEngine`).

```cmake
# YourProject/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(MyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(third_party/HeliosEngine)

add_executable(my_game src/main.cpp)
# Optional: adopt Helios warning/optimization/sanitizer profile
# helios_apply_conventions(my_game)
helios_link_modules(
    TARGET my_game
    MODULES PUBLIC app
)
```

### Method 2: `FetchContent`

Fetches at configure time without a manual submodule.

```cmake
include(FetchContent)

FetchContent_Declare(
    HeliosEngine
    GIT_REPOSITORY https://github.com/RexarX/HeliosEngine.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

set(HELIOS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(HELIOS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(HeliosEngine)

add_executable(my_game src/main.cpp)
helios_link_modules(
    TARGET my_game
    MODULES PUBLIC app
)
```

### Method 3: CPM

Uses the same CPM integration as Helios itself ([`cmake/DownloadUsingCPM.cmake`](cmake/DownloadUsingCPM.cmake)).

```cmake
# Download CPM once (or vendor cmake/CPM.cmake)
include(cmake/DownloadUsingCPM.cmake)  # from Helios, or your own CPM bootstrap

CPMAddPackage(
    NAME HeliosEngine
    GITHUB_REPOSITORY RexarX/HeliosEngine
    GIT_TAG main
    OPTIONS
        "HELIOS_BUILD_TESTS OFF"
        "HELIOS_BUILD_EXAMPLES OFF"
)

add_executable(my_game src/main.cpp)
helios_link_modules(
    TARGET my_game
    MODULES PUBLIC app
)
```

### Method 4: Installed Package (`find_package`)

Build and install Helios, then use the generated CMake package config.

```bash
cmake --preset linux-gcc-release -DHELIOS_ENABLE_INSTALL=ON
cmake --build --preset linux-gcc-release
cmake --install build/linux-gcc-release --prefix /opt/helios
```

```cmake
list(APPEND CMAKE_PREFIX_PATH "/opt/helios")
find_package(Helios REQUIRED CONFIG)

add_executable(my_game src/main.cpp)
helios_link_modules(
    TARGET my_game
    MODULES PUBLIC app
)
```

Installed artifacts: `HeliosConfig.cmake`, `HeliosConfigVersion.cmake`, `HeliosTargets.cmake` (see [`cmake/Install.cmake`](cmake/Install.cmake)).

<a href="#readme-top">↑ Back to Top</a>

---

## Documentation

### API reference (Doxygen)

```bash
python scripts/docs.py
# → docs/doxygen/html/index.html
```

Config: [`docs/doxygen/Doxyfile.in`](docs/doxygen/Doxyfile.in) — version from `project(VERSION …)` via CMake (`cmake --preset docs`, `cmake --build --preset docs`) or `scripts/docs.py`.

### Project guidelines

[docs/guidelines.md](docs/guidelines.md) — code style, module layout, testing, build options.

---

## Development

### Code formatting

Formatting runs automatically on every local commit (via [pre-commit](https://pre-commit.com/)) and is verified on every push / PR ([`.github/workflows/format.yaml`](.github/workflows/format.yaml)). Unformatted code cannot land on `main` if hooks and CI are used.

| When                     | Mechanism                                                         |
| ------------------------ | ----------------------------------------------------------------- |
| **Every commit** (local) | `pre-commit install` — runs `python scripts/format.py` (auto-fix) |
| **Every push / PR**      | CI — `python scripts/format.py --check`                           |

```bash
# Format all sources
python scripts/format.py

# Check only (same as CI)
python scripts/format.py --check

# Install local commit hook (auto-formats on commit)
pip install pre-commit
pre-commit install
```

### Creating a Custom Module

Helios modules live under `src/` by default. Register additional search paths with `helios_add_extra_module_dirs()` (before discovery) or `HELIOS_EXTRA_MODULE_DIRS`. The `greeting` example path is registered automatically when `HELIOS_BUILD_EXAMPLES=ON`. See the full walkthrough:

**[examples/custom_module/README.md](examples/custom_module/README.md)**

That example defines a minimal `greeting` module (registration, build target, tests, and a demo executable) under `examples/custom_module/`, discovered through the extra module path mechanism.

```bash
# From a parent CMake project (before add_subdirectory(HeliosEngine)):
# list(APPEND CMAKE_MODULE_PATH ".../HeliosEngine/cmake")
# include(ModuleRegistry)
# helios_add_extra_module_dirs("${CMAKE_CURRENT_SOURCE_DIR}/modules")

# Or via cache variable:
cmake --preset linux-gcc-release \
  -DHELIOS_EXTRA_MODULE_DIRS="/path/to/my/modules"
```

When `HELIOS_BUILD_EXAMPLES=ON`, Helios calls `helios_add_extra_module_dirs(examples/custom_module)` before discovery.

Quick layout:

```
examples/custom_module/
├── CMakeLists.txt            # helios_module(...) + demo target
├── README.md                 # Step-by-step guide
├── include/helios/greeting/  # Public headers
├── modules/                  # Optional module sources (.cppm)
├── src/                      # Optional private sources
└── tests/                    # doctest suite
```

Link it from your executable with `helios_link_modules(TARGET … MODULES PUBLIC greeting)`.

See also [docs/guidelines.md](docs/guidelines.md) for module conventions.

### Other scripts

```bash
python scripts/lint.py --build-dir build/linux-gcc-release  # clang-tidy
python scripts/docs.py                                      # Doxygen
ctest --preset linux-gcc-release                            # tests
```

<a href="#readme-top">↑ Back to Top</a>

---

## Roadmap

- `render` module (ECS contract for graphics)
- `nvrhi_render` module (NVRHI backend for `render`)

<a href="#readme-top">↑ Back to Top</a>

---

## Acknowledgments

Inspired by [Bevy](https://bevyengine.org/), [EnTT](https://github.com/skypjack/entt), and [Taskflow](https://taskflow.github.io/).

<a href="#readme-top">↑ Back to Top</a>

---

## License

Distributed under the MIT License. See [LICENSE][license-url] for details.

<a href="#readme-top">↑ Back to Top</a>

---

## Contact

**RexarX** — who727cares@gmail.com

**Project:** [github.com/RexarX/HeliosEngine](https://github.com/RexarX/HeliosEngine)

<a href="#readme-top">↑ Back to Top</a>

<!-- MARKDOWN LINKS & IMAGES -->

[license-shield]: https://img.shields.io/github/license/RexarX/HeliosEngine.svg?style=for-the-badge
[license-url]: https://github.com/RexarX/HeliosEngine/blob/main/LICENSE
[cpp-shield]: https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=for-the-badge&logo=c%2B%2B
[cpp-url]: https://en.cppreference.com/w/cpp/23
