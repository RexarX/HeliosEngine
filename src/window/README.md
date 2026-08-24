# `window` — Window ECS Contract

Public ECS interface for window management without OS dependencies.

## Public API

- `Properties`, `Window`, `PrimaryWindow`, `CreationFailed`
- `Settings`, `kEvents`, `RegisterEventsSchedule`
- Lifecycle and property event messages (`ResizedMsg`, `CreationFailedMsg`, ...)
- Composite params: `Windows`, `Messages`, `Writers`, `CreationWriters`, and
  lifecycle / geometry / appearance / platform groups
- `Plugin` — registers ECS types; does not create OS windows

Creation-time extras: `transparent_framebuffer`, `scale_to_monitor`,
`scale_framebuffer`, `mouse_passthrough`. Runtime: `RequestFocus()`,
`SetMousePassthrough()`.

Failed OS window creation tags the entity with `CreationFailed` and emits
`CreationFailedMsg` instead of aborting the process.

## Backends

Use `helios::sdl3::Plugin`, `helios::sdl3::window::Plugin`, and
`helios::sdl3::input::Plugin` with `window::Plugin` / `input::Plugin` as needed.
GLFW remains available via `-DHELIOS_BUILD_GLFW=ON` and
`-DHELIOS_BUILD_SDL3_WINDOW=OFF`.

## Dependencies

- `app`, `ecs`
