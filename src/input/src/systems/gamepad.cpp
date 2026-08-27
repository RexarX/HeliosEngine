#include <pch.hpp>

#include <helios/input/systems/gamepad.hpp>

#include <details/apply_button.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/axis.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/params.hpp>
#include <helios/input/settings.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace helios::input {

using details::ApplyButtonState;

namespace {

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

}  // namespace

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
    pad->connected = msg->connected;
    filter->Reset();
    if (msg->connected) {
      pad->id = msg->id;
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
    if (msg->touchpad >= Gamepad::kMaxTouchpads ||
        msg->finger >= Gamepad::kMaxTouchpadFingers) [[unlikely]] {
      continue;
    }
    GamepadTouchpadFinger& finger = pad->touchpads[msg->touchpad][msg->finger];
    finger.x = msg->x;
    finger.y = msg->y;
    finger.pressure = msg->pressure;
    finger.down = msg->down;
    const uint8_t needed = static_cast<uint8_t>(msg->touchpad + 1U);
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

}  // namespace helios::input
