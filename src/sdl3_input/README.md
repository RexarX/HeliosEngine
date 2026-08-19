# `sdl3_input` — SDL3 Input Backend

SDL3 implementation of the `input` ECS contract under `helios::sdl3::input`.

## Public API

- `Plugin` — input event handlers plus gamepad / cursor systems on `kEvents`

`#include <helios/sdl3/input/input.hpp>` (and `plugin.hpp`) does **not** include
`<SDL3/SDL.h>`. Details headers such as `input_map.hpp` use SDL types and may
be included when you need them. SDL3 is a private link dependency.

Add with `helios::sdl3::Plugin`, `helios::window::Plugin`, `helios::input::Plugin`,
and optionally `helios::sdl3::window::Plugin` when using SDL3 windows.

Keyboard, mouse, and pen messages are emitted from `sdl3::PumpEvents` handlers
on the main thread. Motion deltas use SDL `xrel` / `yrel` (correct in relative
mode). Pen pressure / tilt / proximity come from `SDL_EVENT_PEN_*`; SDL also
synthesizes mouse events from the pen by default.

`ApplyGamepadMappings`, `PollGamepads`, `ApplyGamepadOutputs`, `ApplyCursors`,
and `ApplyRawMouseMotion` run as a sequence in `sdl3::input::ApplySet` on
`window::kEvents` after `sdl3::EventPumpSet` and before `sdl3::window::ApplySet`
(when the window backend is linked). That is no later than `app::kFirst`, so
`kPreUpdate` and later see up-to-date input and window state.

`input::ClearInputState` / `Update*State` stay on `app::kFirst` and can run in
parallel.

## Usage

```cpp
// Adds `helios::sdl3::Plugin` and `helios::sdl3::input::Plugin` to the app.
app.AddPluginGroups(helios::sdl3::InputPlugin{});
```

## Dependencies

- Required: `sdl3`, `input`, `window`, `app`, `ecs`, `log`
- Optional: `sdl3_window`

## System packages

Same as [`sdl3`](../sdl3/README.md#system-packages). This backend does not add
OS packages beyond those required to build vendored SDL3.
