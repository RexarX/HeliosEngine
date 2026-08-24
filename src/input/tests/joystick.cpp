#include <doctest/doctest.h>

#include <helios/input/joystick.hpp>

using namespace helios::input;

TEST_SUITE("helios::input::Joystick") {
  TEST_CASE("helios::input::Joystick::Reset") {
    SUBCASE("Clears identity, counts, axes, buttons, and hats") {
      Joystick stick;
      stick.name = "HOTAS";
      stick.guid = "abcd";
      stick.axes[0] = 0.5F;
      stick.buttons.Press(1);
      stick.hats[0] = JoystickHat::kUp;
      stick.axis_count = 4;
      stick.button_count = 8;
      stick.hat_count = 1;
      stick.id = 3;
      stick.connected = true;

      stick.Reset();

      CHECK(stick.name.empty());
      CHECK(stick.guid.empty());
      CHECK_EQ(stick.axes[0], doctest::Approx(0.0F));
      CHECK_FALSE(stick.buttons.AnyPressed());
      CHECK_EQ(stick.hats[0], JoystickHat::kCentered);
      CHECK_EQ(stick.axis_count, 0);
      CHECK_EQ(stick.button_count, 0);
      CHECK_EQ(stick.hat_count, 0);
      CHECK_EQ(stick.id, -1);
      CHECK_FALSE(stick.connected);
    }
  }
}
