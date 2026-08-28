#include <pch.hpp>

#include <helios/input/systems/joystick.hpp>

#include <details/apply_button.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/params.hpp>

namespace helios::input {

using details::ApplyIndexedButtonState;

void UpdateJoystickState::operator()(ecs::Res<Joysticks> joysticks,
                                     JoystickMessages messages) const {
  for (const auto msg : messages.connection) {
    Joystick* stick = joysticks->TryGet(msg->id);
    if (stick == nullptr) [[unlikely]] {
      continue;
    }

    stick->Reset();
    stick->connected = msg->connected;
    if (msg->connected) {
      stick->id = msg->id;
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

}  // namespace helios::input
