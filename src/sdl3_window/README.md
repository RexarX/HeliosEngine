# `sdl3_window` — SDL3 Window Backend

SDL3 implementation of the `window` ECS contract under `helios::sdl3::window`.

## Public API

- `Plugin` — window backend systems on `kEvents` and startup/shutdown

`#include <helios/sdl3/window/window.hpp>` (and `plugin.hpp`) does **not**
include `<SDL3/SDL.h>`. Details headers such as `sdl_sync.hpp` and
`window_map.hpp` use SDL types and may be included when you need them. SDL3 is
a private link dependency.

Add with `helios::sdl3::Plugin` and `helios::window::Plugin`.

`kEvents` order (main thread):

`CreateSet` (`CreateNativeWindows`) → `sdl3::EventPumpSet` → `ApplySet`
(`PollEvents` → `ApplyChanges` → `DestroyClosedWindows`)

Input systems (`sdl3::input::ApplySet`) insert after `EventPumpSet` and before
`ApplySet` when the input plugin is present. Those three are not sequenced with
each other so the scheduler can overlap them with clipboard sync.

Window `Init` is in `StartupSet` after `sdl3::StartupSet`. Window `Shutdown` is
in `ShutdownSet` before `sdl3::ShutdownSet`.

## Usage

```cpp
// Adds `helios::sdl3::Plugin` and `helios::sdl3::window::Plugin` to the app.
app.AddPluginGroups(helios::sdl3::WindowPlugin{});
```

## Dependencies

- Required: `sdl3`, `window`, `app`, `ecs`, `memory`, `log`
- Optional: `input`

## System packages

Same as [`sdl3`](../sdl3/README.md#system-packages). This backend does not add
OS packages beyond those required to build vendored SDL3.
