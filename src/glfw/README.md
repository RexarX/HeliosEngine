# `glfw` — GLFW Window Backend

GLFW implementation of the `window` ECS contract, with optional `input`
integration via `OPTIONAL_DEPENDS`.

## Public API

- `Plugin` — registers GLFW systems on `kEvents` and startup/shutdown schedules
  (`CreateNativeWindows`, `PollEvents`, `ApplyChanges`, `DestroyClosedWindows`,
  plus input systems when the `input` plugin is present)
- `WindowPlugin` — bundles `glfw::Plugin` + `window::Plugin`
- `WindowInputPlugin` — bundles window + glfw + `input::Plugin` (when `input`
  is linked)

`PollGamepads` only emits mapped Xbox-style gamepads. On connect it writes a
full axis snapshot (and pressed buttons) for rest-center calibration. Unmapped
joysticks, wheels, pedals, and force feedback are out of scope.

## Frame pump

`Plugin::Finish` installs a nested frame pump that calls
`App::RunFrameOrder(FramePumpOrder)`. That order contains only `kUpdateStage`.
Builtin `advance_messages` on Update ages message buffers at the end of each
nested pump without re-entering window/extract stages. The pump runs from
size / position / content-scale callbacks while `glfwPollEvents` is blocked in
an OS modal loop (live resize/move), not from `WM_PAINT` / window-refresh.

`PollEvents` honors `window::Settings::event_mode`: `glfwPollEvents` by default,
or `glfwWaitEventsTimeout` when set to `kWaitTimeout`. Clipboard OS reads are
change-driven (Win32 sequence number; other platforms on write / focus), not
polled every frame.

## Usage

```cpp
// Window only
app.AddPluginGroups(helios::glfw::WindowPlugin{});

// Window + input
app.AddPluginGroups(helios::glfw::WindowInputPlugin{});
```

## Dependencies

- Required: `window`, `app`, GLFW
- Optional: `input` (`HELIOS_MODULE_INPUT_AVAILABLE`)

## System packages

Needed on Linux when building vendored GLFW (X11 + Wayland). Windows and macOS
need no extra packages.

Debian / Ubuntu:

```bash
sudo apt-get install -y libwayland-dev libxkbcommon-dev libx11-dev \
  libxrandr-dev libxinerama-dev libxi-dev libxcursor-dev libxext-dev
```

Fedora, Arch, and shared build tools are listed under
[Installing Dependencies](../../README.md#installing-dependencies).
