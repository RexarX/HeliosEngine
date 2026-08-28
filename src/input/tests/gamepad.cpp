#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/gamepad.hpp>

using namespace helios::ecs;
using namespace helios::input;

TEST_SUITE("helios::input::Trigger") {
  TEST_CASE("helios::input::Trigger") {
    SUBCASE("Returns true for trigger axes") {
      CHECK(Trigger(GamepadAxis::kLeftTrigger));
      CHECK(Trigger(GamepadAxis::kRightTrigger));
    }

    SUBCASE("Returns false for stick axes") {
      CHECK_FALSE(Trigger(GamepadAxis::kLeftX));
      CHECK_FALSE(Trigger(GamepadAxis::kRightY));
    }
  }
}

TEST_SUITE("helios::input::LeftStick") {
  TEST_CASE("helios::input::LeftStick") {
    SUBCASE("Returns true for left stick axes") {
      CHECK(LeftStick(GamepadAxis::kLeftX));
      CHECK(LeftStick(GamepadAxis::kLeftY));
    }

    SUBCASE("Returns false for other axes") {
      CHECK_FALSE(LeftStick(GamepadAxis::kRightX));
      CHECK_FALSE(LeftStick(GamepadAxis::kLeftTrigger));
    }
  }
}

TEST_SUITE("helios::input::RightStick") {
  TEST_CASE("helios::input::RightStick") {
    SUBCASE("Returns true for right stick axes") {
      CHECK(RightStick(GamepadAxis::kRightX));
      CHECK(RightStick(GamepadAxis::kRightY));
    }

    SUBCASE("Returns false for other axes") {
      CHECK_FALSE(RightStick(GamepadAxis::kLeftY));
      CHECK_FALSE(RightStick(GamepadAxis::kRightTrigger));
    }
  }
}

TEST_SUITE("helios::input::GamepadAxisFilter") {
  TEST_CASE("helios::input::GamepadAxisFilter::ctor") {
    SUBCASE("Initializes trigger rest to -1") {
      const GamepadAxisFilter filter;
      CHECK_EQ(filter.raw.Get(GamepadAxis::kLeftTrigger),
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kRightTrigger)],
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.0F));
    }
  }

  TEST_CASE("helios::input::GamepadAxisFilter::Reset") {
    SUBCASE("Clears samples and restores default rest centers") {
      GamepadAxisFilter filter;
      filter.raw.Set(GamepadAxis::kLeftX, 0.5F);
      filter.center[static_cast<size_t>(GamepadAxis::kLeftX)] = 0.2F;
      filter.rest_frames[0] = 9;
      filter.seen[0] = 1;

      filter.Reset();

      CHECK_EQ(filter.raw.Get(GamepadAxis::kLeftX), doctest::Approx(0.0F));
      CHECK_EQ(filter.raw.Get(GamepadAxis::kLeftTrigger),
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.0F));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftTrigger)],
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.rest_frames[0], 0);
      CHECK_EQ(filter.seen[0], 0);
    }
  }
}

TEST_SUITE("helios::input::GamepadDirtyFlags") {
  TEST_CASE("helios::input::HasFlag") {
    SUBCASE("Detects a set gamepad dirty flag") {
      CHECK(HasFlag(GamepadDirtyFlags::kRumble | GamepadDirtyFlags::kLed,
                    GamepadDirtyFlags::kRumble));
    }

    SUBCASE("Returns false for an unset gamepad dirty flag") {
      CHECK_FALSE(HasFlag(GamepadDirtyFlags::kRumble, GamepadDirtyFlags::kLed));
    }

    SUBCASE("kNone never matches a concrete flag") {
      CHECK_FALSE(
          HasFlag(GamepadDirtyFlags::kNone, GamepadDirtyFlags::kRumble));
    }
  }
}

TEST_SUITE("helios::input::Gamepad") {
  TEST_CASE("helios::input::Gamepad::Reset") {
    SUBCASE("Clears identity, buttons, axes, and extras") {
      Gamepad pad;
      pad.name = "Pad";
      pad.guid = "guid";
      pad.mapping = "map";
      pad.buttons.Press(GamepadButton::kA);
      pad.axes.Set(GamepadAxis::kLeftX, 1.0F);
      pad.power.state = GamepadPowerState::kCharging;
      pad.gyro[0] = 1.0F;
      pad.touchpad_count = 1;
      pad.id = 2;
      pad.connected = true;
      pad.SetRumble(1, 2, 3);

      pad.Reset();

      CHECK(pad.name.empty());
      CHECK(pad.guid.empty());
      CHECK(pad.mapping.empty());
      CHECK_FALSE(pad.buttons.AnyPressed());
      CHECK_EQ(pad.axes.Get(GamepadAxis::kLeftX), doctest::Approx(0.0F));
      CHECK_EQ(pad.power.state, GamepadPowerState::kUnknown);
      CHECK_EQ(pad.gyro[0], doctest::Approx(0.0F));
      CHECK_EQ(pad.touchpad_count, 0);
      CHECK_FALSE(pad.id.has_value());
      CHECK_FALSE(pad.connected);
      CHECK_EQ(pad.dirty_flags, GamepadDirtyFlags::kNone);
    }
  }

  TEST_CASE("helios::input::Gamepad::MarkDirty") {
    SUBCASE("Sets a single dirty flag") {
      Gamepad pad;
      pad.MarkDirty(GamepadDirtyFlags::kRumble);

      CHECK(pad.Dirty(GamepadDirtyFlags::kRumble));
      CHECK_FALSE(pad.Dirty(GamepadDirtyFlags::kLed));
    }

    SUBCASE("Combines additional dirty flags") {
      Gamepad pad;
      pad.MarkDirty(GamepadDirtyFlags::kRumble);
      pad.MarkDirty(GamepadDirtyFlags::kLed | GamepadDirtyFlags::kSensors);

      CHECK(pad.Dirty(GamepadDirtyFlags::kRumble));
      CHECK(pad.Dirty(GamepadDirtyFlags::kLed));
      CHECK(pad.Dirty(GamepadDirtyFlags::kSensors));
    }
  }

  TEST_CASE("helios::input::Gamepad::ClearDirty") {
    SUBCASE("Clears selected flags and leaves others") {
      Gamepad pad;
      pad.MarkDirty(GamepadDirtyFlags::kRumble | GamepadDirtyFlags::kLed |
                    GamepadDirtyFlags::kSensors);
      pad.ClearDirty(GamepadDirtyFlags::kLed);

      CHECK(pad.Dirty(GamepadDirtyFlags::kRumble));
      CHECK_FALSE(pad.Dirty(GamepadDirtyFlags::kLed));
      CHECK(pad.Dirty(GamepadDirtyFlags::kSensors));
    }

    SUBCASE("Clears all flags") {
      Gamepad pad;
      pad.MarkDirty(GamepadDirtyFlags::kRumble |
                    GamepadDirtyFlags::kTriggerRumble);
      pad.ClearDirty();

      CHECK_EQ(pad.dirty_flags, GamepadDirtyFlags::kNone);
      CHECK_FALSE(pad.Dirty(GamepadDirtyFlags::kRumble));
    }
  }

  TEST_CASE("helios::input::Gamepad::SetRumble") {
    SUBCASE("Stores motor strengths and marks rumble dirty") {
      Gamepad pad;
      pad.SetRumble(10, 20, 30);

      CHECK_EQ(pad.rumble_low, 10);
      CHECK_EQ(pad.rumble_high, 20);
      CHECK_EQ(pad.rumble_duration_ms, 30U);
      CHECK(pad.Dirty(GamepadDirtyFlags::kRumble));
    }
  }

  TEST_CASE("helios::input::Gamepad::SetTriggerRumble") {
    SUBCASE("Stores trigger strengths and marks trigger rumble dirty") {
      Gamepad pad;
      pad.SetTriggerRumble(4, 5, 6);

      CHECK_EQ(pad.trigger_rumble_left, 4);
      CHECK_EQ(pad.trigger_rumble_right, 5);
      CHECK_EQ(pad.trigger_rumble_duration_ms, 6U);
      CHECK(pad.Dirty(GamepadDirtyFlags::kTriggerRumble));
    }
  }

  TEST_CASE("helios::input::Gamepad::SetLed") {
    SUBCASE("Stores LED color and marks LED dirty") {
      Gamepad pad;
      pad.SetLed(1, 2, 3);

      CHECK_EQ(pad.led_r, 1);
      CHECK_EQ(pad.led_g, 2);
      CHECK_EQ(pad.led_b, 3);
      CHECK(pad.Dirty(GamepadDirtyFlags::kLed));
    }
  }

  TEST_CASE("helios::input::Gamepad::SetGyroEnabled") {
    SUBCASE("Stores gyro enable and marks sensors dirty") {
      Gamepad pad;
      pad.SetGyroEnabled(true);

      CHECK(pad.gyro_enabled);
      CHECK(pad.Dirty(GamepadDirtyFlags::kSensors));
    }
  }

  TEST_CASE("helios::input::Gamepad::SetAccelEnabled") {
    SUBCASE("Stores accel enable and marks sensors dirty") {
      Gamepad pad;
      pad.SetAccelEnabled(true);

      CHECK(pad.accel_enabled);
      CHECK(pad.Dirty(GamepadDirtyFlags::kSensors));
    }
  }

  TEST_CASE("helios::input::Gamepad::Dirty") {
    SUBCASE("Returns false when the flag is unset") {
      const Gamepad pad{};
      CHECK_FALSE(pad.Dirty(GamepadDirtyFlags::kRumble));
    }

    SUBCASE("Returns true when the flag is set") {
      Gamepad pad;
      pad.MarkDirty(GamepadDirtyFlags::kLed);
      CHECK(pad.Dirty(GamepadDirtyFlags::kLed));
    }
  }
}
TEST_SUITE("helios::input::Gamepads") {
  TEST_CASE("helios::input::Gamepads") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(Gamepads{});
      CHECK(world.HasResource<Gamepads>());
    }
  }

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

    SUBCASE("Returns nullptr for an out-of-range id") {
      Gamepads gamepads;
      CHECK_EQ(gamepads.TryGet(static_cast<GamepadId>(Gamepads::kSlotCount)),
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

    SUBCASE("Returns nullptr for an out-of-range id") {
      Gamepads gamepads;
      CHECK_EQ(
          gamepads.TryGetFilter(static_cast<GamepadId>(Gamepads::kSlotCount)),
          nullptr);
    }
  }
}

TEST_SUITE("helios::input::GamepadMappings") {
  TEST_CASE("helios::input::GamepadMappings") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(GamepadMappings{});
      CHECK(world.HasResource<GamepadMappings>());
    }
  }

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

TEST_SUITE("helios::input::GamepadConnectionMsg") {
  TEST_CASE("helios::input::GamepadConnectionMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadConnectionMsg>();
      CHECK(world.HasMessage<GamepadConnectionMsg>());
      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "pad"});
    }
  }
}

TEST_SUITE("helios::input::GamepadButtonInputMsg") {
  TEST_CASE("helios::input::GamepadButtonInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadButtonInputMsg>();
      CHECK(world.HasMessage<GamepadButtonInputMsg>());
      world.WriteMessages<GamepadButtonInputMsg>().Write(
          {.id = 0,
           .button = GamepadButton::kA,
           .state = ButtonState::kPressed});
    }
  }
}

TEST_SUITE("helios::input::GamepadAxisChangedMsg") {
  TEST_CASE("helios::input::GamepadAxisChangedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadAxisChangedMsg>();
      CHECK(world.HasMessage<GamepadAxisChangedMsg>());
      world.WriteMessages<GamepadAxisChangedMsg>().Write(
          {.id = 0, .axis = GamepadAxis::kLeftX, .value = 0.5F});
    }
  }
}

TEST_SUITE("helios::input::GamepadRemappedMsg") {
  TEST_CASE("helios::input::GamepadRemappedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadRemappedMsg>();
      CHECK(world.HasMessage<GamepadRemappedMsg>());
      world.WriteMessages<GamepadRemappedMsg>().Write(
          {.id = 0, .mapping = "map"});
    }
  }
}

TEST_SUITE("helios::input::GamepadPowerChangedMsg") {
  TEST_CASE("helios::input::GamepadPowerChangedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadPowerChangedMsg>();
      CHECK(world.HasMessage<GamepadPowerChangedMsg>());
      world.WriteMessages<GamepadPowerChangedMsg>().Write({
          .id = 0,
          .power = {.state = GamepadPowerState::kCharging, .percent = 80},
      });
    }
  }
}

TEST_SUITE("helios::input::GamepadSensorUpdateMsg") {
  TEST_CASE("helios::input::GamepadSensorUpdateMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadSensorUpdateMsg>();
      CHECK(world.HasMessage<GamepadSensorUpdateMsg>());
      world.WriteMessages<GamepadSensorUpdateMsg>().Write(
          {.id = 0,
           .sensor = GamepadSensor::kGyro,
           .value = {1.0F, 0.0F, 0.0F}});
    }
  }
}

TEST_SUITE("helios::input::GamepadTouchpadMsg") {
  TEST_CASE("helios::input::GamepadTouchpadMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<GamepadTouchpadMsg>();
      CHECK(world.HasMessage<GamepadTouchpadMsg>());
      world.WriteMessages<GamepadTouchpadMsg>().Write(
          {.id = 0, .x = 0.5F, .y = 0.25F, .down = true});
    }
  }
}
