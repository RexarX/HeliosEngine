#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/params.hpp>
#include <helios/input/systems/clear.hpp>

using namespace helios::ecs;
using namespace helios::input;

TEST_SUITE("helios::input::ClearInputState") {
  TEST_CASE("helios::input::ClearInputState::operator()") {
    SUBCASE("Clears button edges and mouse deltas") {
      Keyboard keyboard;
      Mouse mouse;
      Gamepads gamepads;
      Joysticks joysticks;
      Pens pens;
      Touches touches;
      Sensors sensors;

      keyboard.keys.Press(Key::kA);
      mouse.buttons.Press(MouseButton::kLeft);
      mouse.delta_x = 10.0;
      mouse.delta_y = 5.0;
      mouse.scroll_x = 0.5;
      mouse.scroll_y = 1.0;
      gamepads.pads[0].buttons.Press(GamepadButton::kA);
      joysticks.sticks[0].buttons.Press(0);
      pens.pens[0].buttons.Press(PenButton::kBarrel1);
      pens.pens[0].delta_x = 4.0;
      pens.pens[0].delta_y = 6.0;
      touches.fingers[0].delta_x = 2.0;
      touches.fingers[0].delta_y = 3.0;
      touches.fingers[0].down = false;
      touches.fingers[0].id = 0;

      ClearInputState{}(State{
          .keyboard = Res<Keyboard>{keyboard},
          .mouse = Res<Mouse>{mouse},
          .gamepads = Res<Gamepads>{gamepads},
          .joysticks = Res<Joysticks>{joysticks},
          .pens = Res<Pens>{pens},
          .touches = Res<Touches>{touches},
          .sensors = Res<Sensors>{sensors},
      });

      CHECK(keyboard.keys.Pressed(Key::kA));
      CHECK_FALSE(keyboard.keys.JustPressed(Key::kA));
      CHECK(mouse.buttons.Pressed(MouseButton::kLeft));
      CHECK_FALSE(mouse.buttons.JustPressed(MouseButton::kLeft));
      CHECK_EQ(mouse.delta_x, 0.0);
      CHECK_EQ(mouse.delta_y, 0.0);
      CHECK_EQ(mouse.scroll_x, 0.0);
      CHECK_EQ(mouse.scroll_y, 0.0);
      CHECK(gamepads.pads[0].buttons.Pressed(GamepadButton::kA));
      CHECK_FALSE(gamepads.pads[0].buttons.JustPressed(GamepadButton::kA));
      CHECK(joysticks.sticks[0].buttons.Pressed(0));
      CHECK_FALSE(joysticks.sticks[0].buttons.JustPressed(0));
      CHECK(pens.pens[0].buttons.Pressed(PenButton::kBarrel1));
      CHECK_FALSE(pens.pens[0].buttons.JustPressed(PenButton::kBarrel1));
      CHECK_EQ(pens.pens[0].delta_x, 0.0);
      CHECK_EQ(pens.pens[0].delta_y, 0.0);
      CHECK_EQ(touches.fingers[0].delta_x, 0.0);
      CHECK_EQ(touches.fingers[0].delta_y, 0.0);
      CHECK_FALSE(touches.fingers[0].id.has_value());
    }
  }
}
