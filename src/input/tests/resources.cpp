#include <doctest/doctest.h>

#include <helios/input/resources.hpp>

using namespace helios::input;

TEST_SUITE("helios::input::Gamepads") {
  TEST_CASE("helios::input::Gamepads::TryGet") {
    SUBCASE("Returns a mutable slot for a valid id") {
      Gamepads gamepads;
      Gamepad* pad = gamepads.TryGet(0);
      REQUIRE_NE(pad, nullptr);
      pad->connected = true;
      CHECK(gamepads.pads[0].connected);
    }

    SUBCASE("Returns a const slot for a valid id") {
      Gamepads gamepads;
      gamepads.pads[3].id = 3;
      const Gamepads& view = gamepads;
      const Gamepad* pad = view.TryGet(3);
      REQUIRE_NE(pad, nullptr);
      CHECK_EQ(pad->id, 3);
    }

    SUBCASE("Returns nullptr for a negative id") {
      Gamepads gamepads;
      CHECK_EQ(gamepads.TryGet(-1), nullptr);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Gamepads gamepads;
      CHECK_EQ(gamepads.TryGet(static_cast<int32_t>(Gamepads::kSlotCount)),
               nullptr);
    }
  }

  TEST_CASE("helios::input::Gamepads::TryGetFilter") {
    SUBCASE("Returns a mutable filter slot for a valid id") {
      Gamepads gamepads;
      GamepadAxisFilter* filter = gamepads.TryGetFilter(2);
      REQUIRE_NE(filter, nullptr);
      filter->center[0] = 0.25F;
      CHECK_EQ(gamepads.filters[2].center[0], doctest::Approx(0.25F));
    }

    SUBCASE("Returns a const filter slot for a valid id") {
      Gamepads gamepads;
      gamepads.filters[4].seen[1] = 1;
      const Gamepads& view = gamepads;
      const GamepadAxisFilter* filter = view.TryGetFilter(4);
      REQUIRE_NE(filter, nullptr);
      CHECK_EQ(filter->seen[1], 1);
    }

    SUBCASE("Returns nullptr for a negative id") {
      Gamepads gamepads;
      CHECK_EQ(gamepads.TryGetFilter(-1), nullptr);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Gamepads gamepads;
      CHECK_EQ(
          gamepads.TryGetFilter(static_cast<int32_t>(Gamepads::kSlotCount)),
          nullptr);
    }
  }
}

TEST_SUITE("helios::input::Joysticks") {
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

    SUBCASE("Returns nullptr for a negative id") {
      Joysticks joysticks;
      CHECK_EQ(joysticks.TryGet(-1), nullptr);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Joysticks joysticks;
      CHECK_EQ(joysticks.TryGet(static_cast<int32_t>(Joysticks::kSlotCount)),
               nullptr);
    }
  }
}

TEST_SUITE("helios::input::GamepadMappings") {
  TEST_CASE("helios::input::GamepadMappings::Add") {
    SUBCASE("Queues a mapping line and marks dirty") {
      GamepadMappings mappings;
      mappings.Add("03000000,Test,a:b0,");

      CHECK_EQ(mappings.pending_lines.size(), 1);
      CHECK_EQ(mappings.pending_lines[0], "03000000,Test,a:b0,");
      CHECK(mappings.dirty);
    }
  }

  TEST_CASE("helios::input::GamepadMappings::AddFromFile") {
    SUBCASE("Queues a file path and marks dirty") {
      GamepadMappings mappings;
      mappings.AddFromFile("gamecontrollerdb.txt");

      CHECK_EQ(mappings.pending_files.size(), 1);
      CHECK_EQ(mappings.pending_files[0], "gamecontrollerdb.txt");
      CHECK(mappings.dirty);
    }
  }

  TEST_CASE("helios::input::GamepadMappings::ClearPending") {
    SUBCASE("Clears queued mappings and dirty") {
      GamepadMappings mappings;
      mappings.Add("line");
      mappings.AddFromFile("file.txt");

      mappings.ClearPending();

      CHECK(mappings.pending_lines.empty());
      CHECK(mappings.pending_files.empty());
      CHECK_FALSE(mappings.dirty);
    }
  }
}
