#pragma once

#include <helios/input/button_input.hpp>
#include <helios/input/keyboard.hpp>

#include <cstddef>

namespace helios::input::details {

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

template <size_t N>
void ApplyIndexedButtonState(IndexedButtonInput<N>& buttons, size_t index,
                             ButtonState state) noexcept {
  if (index >= N) [[unlikely]] {
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

}  // namespace helios::input::details
