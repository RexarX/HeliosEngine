# `sdl3` — SDL3 Process Runtime

Process-global SDL3 lifetime, event pump, and handler registry.
Window and input backends share this module instead of calling `SDL_Init` or
`SDL_PollEvent` independently.

## Public API

`#include <helios/sdl3/sdl3.hpp>` (and `plugin.hpp`) does **not** include
`<SDL3/SDL.h>`. SDL types live in `details/` headers; include those explicitly
when you need them. SDL3 is a private link dependency.

- `Retain` / `Release` — refcounted subsystem init (`SDL_InitFlags`)
- `Context` — world pointer and nested frame-pump state
- `EventDispatcher` — ordered `void(*)(const SDL_Event&, World&)` handlers
- `StartupSet` / `EventPumpSet` / `ShutdownSet` — named sets for ordering
- `Init` — stores the main world pointer on `kMainStartup`
- `PumpEvents` — main-thread poll / wait-timeout on `window::kEvents`
- `Shutdown` — clears runtime pointers on `kShutdown`
- `Plugin` — inserts resources and adds `Init`, `PumpEvents`, and `Shutdown`

`PumpEvents` honors `window::Settings::event_mode`. Wait-timeout waits once,
then drains the queue with `SDL_PollEvent` so a single event cannot stall the
frame. Size / move / scale events request a nested `FramePumpOrder` while
polling (same rule as GLFW).

## Usage

```cpp
// Window only
app.AddPlugins(helios::sdl3::Plugin{});
```

`sdl3::Plugin` owns `Init` / `PumpEvents` / `Shutdown` as named sets
(`StartupSet`, `EventPumpSet`, `ShutdownSet`). Window and input plugins order
against those sets.

## Dependencies

- `app`, `ecs`, `log`, `window`, SDL3 (public)

## System packages

Needed on Linux when building vendored SDL3 (X11 + Wayland). Windows and macOS
need no extra packages. SDL3 configure **fails** if XScrnSaver / XTest / Xfixes
headers are missing while those features are enabled (the default).

Debian / Ubuntu:

```bash
sudo apt-get install -y libwayland-dev libxkbcommon-dev libx11-dev \
  libxrandr-dev libxi-dev libxcursor-dev libxext-dev \
  libxfixes-dev libxss-dev libxtst-dev
```

Fedora, Arch, and shared build tools are listed under
[Installing Dependencies](../../README.md#installing-dependencies).
