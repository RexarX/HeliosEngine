#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/joystick.hpp>

#include <cstdint>

using namespace helios::ecs;
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
      CHECK_FALSE(stick.id.has_value());
      CHECK_FALSE(stick.connected);
    }
  }
}
TEST_SUITE("helios::input::Joysticks") {
  TEST_CASE("helios::input::Joysticks") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(Joysticks{});
      CHECK(world.HasResource<Joysticks>());
    }
  }

  TEST_CASE("helios::input::Joysticks::TryGet") {
    SUBCASE("Returns a mutable slot for a valid id") {
      Joysticks joysticks;
      Joystick* stick = joysticks.TryGet(0);
      REQUIRE_NE(stick, nullptr);
      stick->connected = true;
      CHECK(joysticks.sticks[0].connected);
    }

    SUBCASE("Returns a const slot for a valid id") {
      Joysticks joysticks;
      joysticks.sticks[3].id = 3;
      const Joysticks& view = joysticks;
      const Joystick* stick = view.TryGet(3);
      REQUIRE_NE(stick, nullptr);
      CHECK_EQ(stick->id, 3);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Joysticks joysticks;
      CHECK_EQ(joysticks.TryGet(static_cast<JoystickId>(Joysticks::kSlotCount)),
               nullptr);
    }
  }
}

TEST_SUITE("helios::input::JoystickConnectionMsg") {
  TEST_CASE("helios::input::JoystickConnectionMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<JoystickConnectionMsg>();
      CHECK(world.HasMessage<JoystickConnectionMsg>());
      world.WriteMessages<JoystickConnectionMsg>().Write(
          {.name = "stick", .id = 1, .connected = true});
    }
  }
}

TEST_SUITE("helios::input::JoystickButtonInputMsg") {
  TEST_CASE("helios::input::JoystickButtonInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<JoystickButtonInputMsg>();
      CHECK(world.HasMessage<JoystickButtonInputMsg>());
      world.WriteMessages<JoystickButtonInputMsg>().Write(
          {.id = 1, .button = 0, .state = ButtonState::kPressed});
    }
  }
}

TEST_SUITE("helios::input::JoystickAxisChangedMsg") {
  TEST_CASE("helios::input::JoystickAxisChangedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<JoystickAxisChangedMsg>();
      CHECK(world.HasMessage<JoystickAxisChangedMsg>());
      world.WriteMessages<JoystickAxisChangedMsg>().Write(
          {.id = 1, .axis = 0, .value = 0.5F});
    }
  }
}

TEST_SUITE("helios::input::JoystickHatChangedMsg") {
  TEST_CASE("helios::input::JoystickHatChangedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<JoystickHatChangedMsg>();
      CHECK(world.HasMessage<JoystickHatChangedMsg>());
      world.WriteMessages<JoystickHatChangedMsg>().Write(
          {.id = 1, .hat = 0, .value = JoystickHat::kUp});
    }
  }
}
