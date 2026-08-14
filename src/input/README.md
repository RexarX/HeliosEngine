# `input` — Input ECS Contract

Public ECS interface for keyboard, mouse, cursor, and gamepad state without OS
dependencies.

## Public API

- `Key`, `Modifiers`, `ButtonState`
- `MouseButton`, `CursorIcon`, `CursorImage`, `Cursor`
- `GamepadButton`, `GamepadAxis`, `Gamepad`, `GamepadAxisFilter`
- `Trigger`, `LeftStick`, `RightStick`
- `ButtonInput`, `Axis`, `AxisFilter`
- `ApplyLinearDeadzone`, `ApplyRadialDeadzone`, `RemapTrigger`
- `Settings`, `Keyboard`, `Mouse`, `Gamepads`
- Input messages (`KeyboardInputMsg`, `MouseButtonInputMsg`, ...)
- Composite params: `State`, `StateView`, `Messages`, `Writers`, and per-device
  `*Messages` / `*Writers` groups
- `ClearInputState`, `UpdateKeyboardState`, `UpdateMouseState`,
  `UpdateGamepadState`
- `Plugin` — registers ECS types and frame systems

`Gamepad::axes` is filtered (circular sticks, triggers in `[0, 1]`).
`GamepadAxisChangedMsg` carries raw backend values. `Settings::stick` /
`Settings::trigger` configure deadzone, livezone, and rescale;
`auto_calibrate` tracks rest center. Unmapped joysticks, wheels, pedals, and
force feedback are out of scope.

## Backends

Platform backends (for example GLFW) emit input messages; this module aggregates
them into resources. GLFW polls only mapped gamepads and seeds axis values on
connect.

## Dependencies

- `app`, `ecs`, `log`
