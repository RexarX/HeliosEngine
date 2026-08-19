#include <pch.hpp>

#include <helios/input/systems.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/input/axis.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/params.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/resources.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace helios::input {

namespace {

template <ButtonTrait T>
constexpr void ApplyButtonState(ButtonInput<T>& buttons, T button,
                                ButtonState state) noexcept {
  switch (state) {
    using enum ButtonState;
    case kPressed:
    case kRepeat:
      buttons.Press(button);
      break;
    case kReleased:
      buttons.Release(button);
      break;
  }
}

void ApplyIndexedButtonState(IndexedButtonInput<Joystick::kMaxButtons>& buttons,
                             size_t index, ButtonState state) noexcept {
  if (index >= Joystick::kMaxButtons) [[unlikely]] {
    return;
  }
  switch (state) {
    using enum ButtonState;
    case kPressed:
    case kRepeat:
      buttons.Press(index);
      break;
    case kReleased:
      buttons.Release(index);
      break;
  }
}

[[nodiscard]] size_t AxisIndex(GamepadAxis axis) noexcept {
  return static_cast<size_t>(axis);
}

void CaptureRestIfFirstSample(GamepadAxisFilter& filter, GamepadAxis axis,
                              float raw, bool auto_calibrate) noexcept {
  const size_t index = AxisIndex(axis);
  if (filter.seen[index] != 0) {
    return;
  }
  filter.seen[index] = 1;
  if (!auto_calibrate) {
    return;
  }

  const float expected = Trigger(axis) ? GamepadAxisFilter::kTriggerRest : 0.0F;
  if (std::fabs(raw - expected) <= GamepadAxisFilter::kRestCapture) {
    filter.center[index] = raw;
  }
}

void RecenterAxis(GamepadAxisFilter& filter, GamepadAxis axis, bool resting,
                  uint16_t rest_frames, bool auto_calibrate) noexcept {
  const size_t index = AxisIndex(axis);
  if (!auto_calibrate) {
    filter.rest_frames[index] = 0;
    return;
  }

  if (!resting) {
    filter.rest_frames[index] = 0;
    return;
  }

  if (filter.rest_frames[index] < rest_frames) {
    ++filter.rest_frames[index];
  }
  if (filter.rest_frames[index] >= rest_frames) {
    filter.center[index] = filter.raw.Get(axis);
    filter.rest_frames[index] = 0;
  }
}

void RecenterStick(GamepadAxisFilter& filter, GamepadAxis x_axis,
                   GamepadAxis y_axis, const AxisFilter& stick,
                   uint16_t rest_frames, bool auto_calibrate) noexcept {
  const float dx = filter.raw.Get(x_axis) - filter.center[AxisIndex(x_axis)];
  const float dy = filter.raw.Get(y_axis) - filter.center[AxisIndex(y_axis)];
  const bool resting = std::hypot(dx, dy) < stick.deadzone;
  RecenterAxis(filter, x_axis, resting, rest_frames, auto_calibrate);
  RecenterAxis(filter, y_axis, resting, rest_frames, auto_calibrate);
}

void RecenterTrigger(GamepadAxisFilter& filter, GamepadAxis axis,
                     const AxisFilter& trigger, uint16_t rest_frames,
                     bool auto_calibrate) noexcept {
  const size_t index = AxisIndex(axis);
  const float remapped =
      RemapTrigger(filter.raw.Get(axis), filter.center[index]);
  RecenterAxis(filter, axis, remapped < trigger.deadzone, rest_frames,
               auto_calibrate);
}

void ApplyStick(Gamepad& pad, const GamepadAxisFilter& filter,
                GamepadAxis x_axis, GamepadAxis y_axis,
                const AxisFilter& stick) noexcept {
  const auto [x, y] = ApplyRadialDeadzone(
      filter.raw.Get(x_axis) - filter.center[AxisIndex(x_axis)],
      filter.raw.Get(y_axis) - filter.center[AxisIndex(y_axis)], stick);
  pad.axes.Set(x_axis, x);
  pad.axes.Set(y_axis, y);
}

void ApplyTrigger(Gamepad& pad, const GamepadAxisFilter& filter,
                  GamepadAxis axis, const AxisFilter& trigger) noexcept {
  const size_t index = AxisIndex(axis);
  const float remapped =
      RemapTrigger(filter.raw.Get(axis), filter.center[index]);
  pad.axes.Set(axis, ApplyLinearDeadzone(remapped, trigger));
}

void FilterConnectedPad(Gamepad& pad, GamepadAxisFilter& filter,
                        const Settings& settings) noexcept {
  RecenterStick(filter, GamepadAxis::kLeftX, GamepadAxis::kLeftY,
                settings.stick, settings.rest_frames, settings.auto_calibrate);
  RecenterStick(filter, GamepadAxis::kRightX, GamepadAxis::kRightY,
                settings.stick, settings.rest_frames, settings.auto_calibrate);
  RecenterTrigger(filter, GamepadAxis::kLeftTrigger, settings.trigger,
                  settings.rest_frames, settings.auto_calibrate);
  RecenterTrigger(filter, GamepadAxis::kRightTrigger, settings.trigger,
                  settings.rest_frames, settings.auto_calibrate);

  ApplyStick(pad, filter, GamepadAxis::kLeftX, GamepadAxis::kLeftY,
             settings.stick);
  ApplyStick(pad, filter, GamepadAxis::kRightX, GamepadAxis::kRightY,
             settings.stick);
  ApplyTrigger(pad, filter, GamepadAxis::kLeftTrigger, settings.trigger);
  ApplyTrigger(pad, filter, GamepadAxis::kRightTrigger, settings.trigger);
}

void ApplyPenPosition(Pen& pen, double x, double y) noexcept {
  if (pen.has_position) {
    pen.delta_x += x - pen.position_x;
    pen.delta_y += y - pen.position_y;
  }
  pen.position_x = x;
  pen.position_y = y;
  pen.has_position = true;
}

}  // namespace

void ClearInputState::operator()(State state) const {
  state.keyboard->keys.Clear();
  state.mouse->buttons.Clear();
  state.mouse->delta_x = 0.0;
  state.mouse->delta_y = 0.0;
  state.mouse->scroll_x = 0.0;
  state.mouse->scroll_y = 0.0;

  for (Gamepad& pad : state.gamepads->pads) {
    pad.buttons.Clear();
  }

  for (Joystick& stick : state.joysticks->sticks) {
    stick.buttons.Clear();
  }

  for (Pen& pen : state.pens->pens) {
    pen.buttons.Clear();
    pen.delta_x = 0.0;
    pen.delta_y = 0.0;
  }
}

void UpdateKeyboardState::operator()(ecs::Res<Keyboard> keyboard,
                                     KeyboardMessages messages) const {
  for (const auto msg : messages.keys) {
    ApplyButtonState(keyboard->keys, msg->key, msg->state);
    keyboard->modifiers = msg->modifiers;
  }
}

void UpdateMouseState::operator()(ecs::Res<Mouse> mouse,
                                  MouseMessages messages) const {
  for (const auto msg : messages.buttons) {
    ApplyButtonState(mouse->buttons, msg->button, msg->state);
  }

  for (const auto msg : messages.cursor) {
    mouse->position_x = msg->x;
    mouse->position_y = msg->y;
  }

  for (const auto msg : messages.motion) {
    mouse->delta_x += msg->delta_x;
    mouse->delta_y += msg->delta_y;
  }

  for (const auto msg : messages.wheel) {
    mouse->scroll_x += msg->x;
    mouse->scroll_y += msg->y;
  }
}

void UpdateGamepadState::operator()(ecs::Res<Gamepads> gamepads,
                                    ecs::Res<const Settings> settings,
                                    GamepadMessages messages) const {
  for (const auto msg : messages.connection) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    GamepadAxisFilter* filter = gamepads->TryGetFilter(msg->id);
    if (pad == nullptr || filter == nullptr) [[unlikely]] {
      continue;
    }

    pad->Reset();
    pad->id = msg->id;
    pad->connected = msg->connected;
    filter->Reset();
    if (msg->connected) {
      pad->name = msg->name;
      pad->guid = msg->guid;
    }
  }

  for (const auto msg : messages.remapped) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    if (pad == nullptr || !pad->connected) [[unlikely]] {
      continue;
    }
    pad->mapping = msg->mapping;
  }

  for (const auto msg : messages.power) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    if (pad == nullptr || !pad->connected) [[unlikely]] {
      continue;
    }
    pad->power = msg->power;
  }

  for (const auto msg : messages.sensors) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    if (pad == nullptr || !pad->connected) [[unlikely]] {
      continue;
    }
    if (msg->sensor == GamepadSensor::kGyro) {
      pad->gyro = msg->value;
    } else if (msg->sensor == GamepadSensor::kAccel) {
      pad->accel = msg->value;
    }
  }

  for (const auto msg : messages.touchpad) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    if (pad == nullptr || !pad->connected) [[unlikely]] {
      continue;
    }
    if (msg->finger >= Gamepad::kMaxTouchpadFingers) [[unlikely]] {
      continue;
    }
    GamepadTouchpadFinger& finger = pad->touchpad[msg->finger];
    finger.x = msg->x;
    finger.y = msg->y;
    finger.pressure = msg->pressure;
    finger.down = msg->down;
    const uint8_t needed = static_cast<uint8_t>(msg->finger + 1U);
    if (pad->touchpad_count < needed) {
      pad->touchpad_count = needed;
    }
  }

  for (const auto msg : messages.buttons) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    if (pad == nullptr) [[unlikely]] {
      continue;
    }
    ApplyButtonState(pad->buttons, msg->button, msg->state);
  }

  for (const auto msg : messages.axes) {
    Gamepad* pad = gamepads->TryGet(msg->id);
    GamepadAxisFilter* filter = gamepads->TryGetFilter(msg->id);
    if (pad == nullptr || filter == nullptr) [[unlikely]] {
      continue;
    }
    filter->raw.Set(msg->axis, msg->value);
    CaptureRestIfFirstSample(*filter, msg->axis, msg->value,
                             settings->auto_calibrate);
  }

  for (size_t i = 0; i < gamepads->pads.size(); ++i) {
    Gamepad& pad = gamepads->pads[i];
    if (!pad.connected) {
      continue;
    }
    FilterConnectedPad(pad, gamepads->filters[i], *settings);
  }
}

void UpdateJoystickState::operator()(ecs::Res<Joysticks> joysticks,
                                     JoystickMessages messages) const {
  for (const auto msg : messages.connection) {
    Joystick* stick = joysticks->TryGet(msg->id);
    if (stick == nullptr) [[unlikely]] {
      continue;
    }

    stick->Reset();
    stick->id = msg->id;
    stick->connected = msg->connected;
    if (msg->connected) {
      stick->name = msg->name;
      stick->guid = msg->guid;
      stick->axis_count = msg->axis_count;
      stick->button_count = msg->button_count;
      stick->hat_count = msg->hat_count;
    }
  }

  for (const auto msg : messages.buttons) {
    Joystick* stick = joysticks->TryGet(msg->id);
    if (stick == nullptr || !stick->connected) [[unlikely]] {
      continue;
    }
    if (msg->button >= stick->button_count) [[unlikely]] {
      continue;
    }
    ApplyIndexedButtonState(stick->buttons, msg->button, msg->state);
  }

  for (const auto msg : messages.axes) {
    Joystick* stick = joysticks->TryGet(msg->id);
    if (stick == nullptr || !stick->connected) [[unlikely]] {
      continue;
    }
    if (msg->axis >= stick->axis_count || msg->axis >= Joystick::kMaxAxes)
        [[unlikely]] {
      continue;
    }
    stick->axes[msg->axis] = msg->value;
  }

  for (const auto msg : messages.hats) {
    Joystick* stick = joysticks->TryGet(msg->id);
    if (stick == nullptr || !stick->connected) [[unlikely]] {
      continue;
    }
    if (msg->hat >= stick->hat_count || msg->hat >= Joystick::kMaxHats)
        [[unlikely]] {
      continue;
    }
    stick->hats[msg->hat] = msg->value;
  }
}

void UpdatePenState::operator()(ecs::Res<Pens> pens,
                                PenMessages messages) const {
  for (const auto msg : messages.proximity) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr) [[unlikely]] {
      continue;
    }

    pen->Reset();
    pen->id = msg->id;
    pen->in_proximity = msg->in_proximity;
    if (msg->in_proximity) {
      pen->device_type = msg->device_type;
    }
  }

  for (const auto msg : messages.moved) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    ApplyPenPosition(*pen, msg->x, msg->y);
  }

  for (const auto msg : messages.axes) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    pen->axes.Set(msg->axis, msg->value);
    ApplyPenPosition(*pen, msg->x, msg->y);
  }

  for (const auto msg : messages.touch) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    pen->down = msg->down;
    pen->eraser = msg->eraser;
    ApplyPenPosition(*pen, msg->x, msg->y);
  }

  for (const auto msg : messages.buttons) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    ApplyButtonState(pen->buttons, msg->button, msg->state);
  }
}

}  // namespace helios::input
