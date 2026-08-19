# `input` — Input ECS Contract

Public ECS interface for keyboard, mouse, cursor, pen, and gamepad state
without OS dependencies.

## Public API

- `Key`, `Modifiers`, `ButtonState`
- `MouseButton`, `CursorIcon`, `CursorImage`, `Cursor`
- `PenButton`, `PenAxis`, `PenDeviceType`, `Pen`, `Pens`
- `GamepadButton`, `GamepadAxis`, `Gamepad`, `GamepadAxisFilter`
- `Trigger`, `LeftStick`, `RightStick`
- `ButtonInput`, `Axis`, `AxisFilter`
- `ApplyLinearDeadzone`, `ApplyRadialDeadzone`, `RemapTrigger`
- `Settings`, `Keyboard`, `Mouse`, `Gamepads`, `Joysticks`, `Pens`
- Input messages (`KeyboardInputMsg`, `MouseButtonInputMsg`, `PenMovedMsg`, ...)
- Composite params: `State`, `StateView`, `Messages`, `Writers`, and per-device
  `*Messages` / `*Writers` groups
- `ClearInputState`, `UpdateKeyboardState`, `UpdateMouseState`,
  `UpdateGamepadState`, `UpdateJoystickState`, `UpdatePenState`
- `Plugin` — registers ECS types and frame systems

`Gamepad::axes` is filtered (circular sticks, triggers in `[0, 1]`).
`GamepadAxisChangedMsg` carries raw backend values. `Settings::stick` /
`Settings::trigger` configure deadzone, livezone, and rescale;
`auto_calibrate` tracks rest center. Pen axes are raw SDL samples (pressure
0..1, tilt/rotation in degrees). GLFW has no tablet API; a stylus still
arrives as mouse there.

## Backends

SDL3 backends (`helios::sdl3::input`) emit input messages by default.
GLFW remains opt-in via `-DHELIOS_BUILD_GLFW=ON`.

## Dependencies

- `app`, `ecs`, `log`
