#include <pch.hpp>

#include <helios/input/systems.hpp>

#include <helios/ecs/resource/param.hpp>
#include <helios/input/axis.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/params.hpp>
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

    pad->id = msg->id;
    pad->connected = msg->connected;
    pad->buttons.Reset();
    pad->axes.Clear();
    filter->Reset();
    if (msg->connected) {
      pad->name = msg->name;
    } else {
      pad->name.clear();
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
