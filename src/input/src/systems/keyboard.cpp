#include <pch.hpp>

#include <helios/input/systems/keyboard.hpp>

#include <details/apply_button.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/params.hpp>

namespace helios::input {

using details::ApplyButtonState;

void UpdateKeyboardState::operator()(ecs::Res<Keyboard> keyboard,
                                     KeyboardMessages messages) const {
  for (const auto msg : messages.keys) {
    ApplyButtonState(keyboard->keys, msg->key, msg->state);
    keyboard->modifiers = msg->modifiers;
  }

  for (const auto msg : messages.editing) {
    keyboard->composition = msg->composition;
    keyboard->composition_start = msg->start;
    keyboard->composition_length = msg->length;
    if (msg->composition.empty()) {
      keyboard->composition_start = 0;
      keyboard->composition_length = 0;
    }
  }

  for ([[maybe_unused]] const auto msg : messages.text) {
    keyboard->composition.clear();
    keyboard->composition_start = 0;
    keyboard->composition_length = 0;
  }
}

}  // namespace helios::input
