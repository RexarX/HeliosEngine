#include <pch.hpp>

#include <helios/input/systems/clear.hpp>

#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/sensor.hpp>
#include <helios/input/touch.hpp>

namespace helios::input {

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

  for (TouchFinger& finger : state.touches->fingers) {
    finger.delta_x = 0.0;
    finger.delta_y = 0.0;
    if (!finger.down) {
      finger.Reset();
    }
  }
}

}  // namespace helios::input
