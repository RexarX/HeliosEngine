#include <pch.hpp>

#include <helios/input/systems/mouse.hpp>

#include <details/apply_button.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>

namespace helios::input {

using details::ApplyButtonState;

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

}  // namespace helios::input
