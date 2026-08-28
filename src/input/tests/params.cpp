#include <doctest/doctest.h>

#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/params.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddKeyboardMessages(World& world) {
  world.AddMessages<KeyboardInputMsg, TextInputMsg, TextEditingMsg,
                    TextEditingCandidatesMsg, KeyboardConnectionMsg>();
}

void AddMouseMessages(World& world) {
  world.AddMessages<MouseButtonInputMsg, CursorMovedMsg, MouseMotionMsg,
                    MouseWheelMsg, MouseConnectionMsg>();
}

void AddGamepadMessages(World& world) {
  world.AddMessages<GamepadConnectionMsg, GamepadButtonInputMsg,
                    GamepadAxisChangedMsg, GamepadRemappedMsg,
                    GamepadPowerChangedMsg, GamepadSensorUpdateMsg,
                    GamepadTouchpadMsg>();
}

void AddJoystickMessages(World& world) {
  world.AddMessages<JoystickConnectionMsg, JoystickButtonInputMsg,
                    JoystickAxisChangedMsg, JoystickHatChangedMsg>();
}

void AddPenMessages(World& world) {
  world.AddMessages<PenProximityMsg, PenTouchMsg, PenButtonInputMsg,
                    PenMovedMsg, PenAxisChangedMsg>();
}

void AddTouchMessages(World& world) {
  world.AddMessages<TouchInputMsg>();
}

void AddSensorMessages(World& world) {
  world.AddMessages<SensorConnectionMsg, SensorUpdateMsg>();
}

void AddInputMessages(World& world) {
  AddKeyboardMessages(world);
  AddMouseMessages(world);
  AddGamepadMessages(world);
  AddJoystickMessages(world);
  AddPenMessages(world);
  AddTouchMessages(world);
  AddSensorMessages(world);
}

}  // namespace

TEST_SUITE("helios::input::State") {
  TEST_CASE("helios::input::State") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<State>);
    }

    SUBCASE("RegisterAccess requests write access to input resources") {
      const auto policy = BuildPolicyFromParams<State>();
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Keyboard>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Mouse>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Gamepads>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Joysticks>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Pens>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Touches>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Sensors>()));
    }
  }
}

TEST_SUITE("helios::input::StateView") {
  TEST_CASE("helios::input::StateView") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<StateView>);
    }

    SUBCASE("Make reads empty default state") {
      World world;
      world.InsertResources(Keyboard{}, Mouse{}, Gamepads{}, Joysticks{},
                            Pens{}, Touches{}, Sensors{});

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<StateView>();
      const StateView view =
          SystemParamTraits<StateView>::Make(world, local_data, policy);

      CHECK_FALSE(view.keyboard->keys.AnyPressed());
      CHECK_EQ(view.mouse->position_x, 0.0);
      CHECK_EQ(view.gamepads->pads.size(), Gamepads::kSlotCount);
      CHECK_EQ(view.joysticks->sticks.size(), Joysticks::kSlotCount);
      CHECK_EQ(view.pens->pens.size(), Pens::kSlotCount);
      CHECK_EQ(view.touches->fingers.size(), Touches::kSlotCount);
      CHECK_EQ(view.sensors->devices.size(), Sensors::kSlotCount);
    }

    SUBCASE("RegisterAccess requests read-only access") {
      const auto policy = BuildPolicyFromParams<StateView>();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Keyboard>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Mouse>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Gamepads>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Joysticks>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Pens>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Touches>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Sensors>()));
      CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Keyboard>()));
    }
  }
}

TEST_SUITE("helios::input::KeyboardMessages") {
  TEST_CASE("helios::input::KeyboardMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<KeyboardMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddKeyboardMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<KeyboardMessages>();
      KeyboardMessages messages =
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy);

      CHECK(messages.keys.Empty());
      CHECK(messages.text.Empty());
      CHECK(messages.editing.Empty());
      CHECK(messages.candidates.Empty());
      CHECK(messages.connection.Empty());
    }
  }
}

TEST_SUITE("helios::input::MouseMessages") {
  TEST_CASE("helios::input::MouseMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<MouseMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddMouseMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<MouseMessages>();
      MouseMessages messages =
          SystemParamTraits<MouseMessages>::Make(world, local_data, policy);

      CHECK(messages.buttons.Empty());
      CHECK(messages.cursor.Empty());
      CHECK(messages.motion.Empty());
      CHECK(messages.wheel.Empty());
      CHECK(messages.connection.Empty());
    }
  }
}

TEST_SUITE("helios::input::GamepadMessages") {
  TEST_CASE("helios::input::GamepadMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<GamepadMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddGamepadMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<GamepadMessages>();
      GamepadMessages messages =
          SystemParamTraits<GamepadMessages>::Make(world, local_data, policy);

      CHECK(messages.connection.Empty());
      CHECK(messages.buttons.Empty());
      CHECK(messages.axes.Empty());
      CHECK(messages.remapped.Empty());
      CHECK(messages.power.Empty());
      CHECK(messages.sensors.Empty());
      CHECK(messages.touchpad.Empty());
    }
  }
}

TEST_SUITE("helios::input::JoystickMessages") {
  TEST_CASE("helios::input::JoystickMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<JoystickMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddJoystickMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<JoystickMessages>();
      JoystickMessages messages =
          SystemParamTraits<JoystickMessages>::Make(world, local_data, policy);

      CHECK(messages.connection.Empty());
      CHECK(messages.buttons.Empty());
      CHECK(messages.axes.Empty());
      CHECK(messages.hats.Empty());
    }
  }
}

TEST_SUITE("helios::input::PenMessages") {
  TEST_CASE("helios::input::PenMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<PenMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddPenMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<PenMessages>();
      PenMessages messages =
          SystemParamTraits<PenMessages>::Make(world, local_data, policy);

      CHECK(messages.proximity.Empty());
      CHECK(messages.touch.Empty());
      CHECK(messages.buttons.Empty());
      CHECK(messages.moved.Empty());
      CHECK(messages.axes.Empty());
    }
  }
}

TEST_SUITE("helios::input::TouchMessages") {
  TEST_CASE("helios::input::TouchMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<TouchMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddTouchMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<TouchMessages>();
      TouchMessages messages =
          SystemParamTraits<TouchMessages>::Make(world, local_data, policy);

      CHECK(messages.fingers.Empty());
    }
  }
}

TEST_SUITE("helios::input::SensorMessages") {
  TEST_CASE("helios::input::SensorMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<SensorMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddSensorMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<SensorMessages>();
      SensorMessages messages =
          SystemParamTraits<SensorMessages>::Make(world, local_data, policy);

      CHECK(messages.connection.Empty());
      CHECK(messages.samples.Empty());
    }
  }
}

TEST_SUITE("helios::input::Messages") {
  TEST_CASE("helios::input::Messages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<Messages>);
    }

    SUBCASE("Make produces grouped empty readers") {
      World world;
      AddInputMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<Messages>();
      Messages messages =
          SystemParamTraits<Messages>::Make(world, local_data, policy);

      CHECK(messages.keyboard.keys.Empty());
      CHECK(messages.mouse.motion.Empty());
      CHECK(messages.gamepad.connection.Empty());
      CHECK(messages.joystick.connection.Empty());
      CHECK(messages.pen.proximity.Empty());
      CHECK(messages.touch.fingers.Empty());
      CHECK(messages.sensor.connection.Empty());
    }

    SUBCASE("Make observes messages written on the world") {
      World world;
      AddInputMessages(world);
      world.WriteMessages<KeyboardInputMsg>().Write(
          {.key = Key::kA, .state = ButtonState::kPressed});

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<Messages>();
      Messages messages =
          SystemParamTraits<Messages>::Make(world, local_data, policy);

      CHECK_FALSE(messages.keyboard.keys.Empty());
    }
  }
}

TEST_SUITE("helios::input::Writers") {
  TEST_CASE("helios::input::Writers") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<Writers>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddInputMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<Writers>();
      Writers writers =
          SystemParamTraits<Writers>::Make(world, local_data, policy);

      writers.keyboard.keys.Write(
          KeyboardInputMsg{.key = Key::kA, .state = ButtonState::kPressed});
      writers.mouse.motion.Write(
          MouseMotionMsg{.delta_x = 1.0, .delta_y = 2.0});
      writers.gamepad.connection.Write(
          GamepadConnectionMsg{.id = 0, .connected = true, .name = "pad"});
      writers.joystick.connection.Write(
          JoystickConnectionMsg{.name = "stick", .id = 1, .connected = true});
      writers.pen.proximity.Write(
          PenProximityMsg{.id = 0, .in_proximity = true});
      writers.touch.fingers.Write(TouchInputMsg{.id = 0});
      writers.sensor.connection.Write(
          SensorConnectionMsg{.id = 0, .connected = true});
      CHECK(SystemParam<Writers>);
    }
  }
}

TEST_SUITE("helios::input::KeyboardWriters") {
  TEST_CASE("helios::input::KeyboardWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<KeyboardWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddKeyboardMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<KeyboardWriters>();
      KeyboardWriters writers =
          SystemParamTraits<KeyboardWriters>::Make(world, local_data, policy);

      writers.keys.Write(
          KeyboardInputMsg{.key = Key::kA, .state = ButtonState::kPressed});
      CHECK(SystemParam<KeyboardWriters>);
    }
  }
}

TEST_SUITE("helios::input::MouseWriters") {
  TEST_CASE("helios::input::MouseWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<MouseWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddMouseMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<MouseWriters>();
      MouseWriters writers =
          SystemParamTraits<MouseWriters>::Make(world, local_data, policy);

      writers.motion.Write(MouseMotionMsg{.delta_x = 1.0, .delta_y = 2.0});
      CHECK(SystemParam<MouseWriters>);
    }
  }
}

TEST_SUITE("helios::input::GamepadWriters") {
  TEST_CASE("helios::input::GamepadWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<GamepadWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddGamepadMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<GamepadWriters>();
      GamepadWriters writers =
          SystemParamTraits<GamepadWriters>::Make(world, local_data, policy);

      writers.connection.Write(
          GamepadConnectionMsg{.id = 0, .connected = true, .name = "pad"});
      CHECK(SystemParam<GamepadWriters>);
    }
  }
}

TEST_SUITE("helios::input::JoystickWriters") {
  TEST_CASE("helios::input::JoystickWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<JoystickWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddJoystickMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<JoystickWriters>();
      JoystickWriters writers =
          SystemParamTraits<JoystickWriters>::Make(world, local_data, policy);

      writers.connection.Write(
          JoystickConnectionMsg{.name = "stick", .id = 1, .connected = true});
      CHECK(SystemParam<JoystickWriters>);
    }
  }
}

TEST_SUITE("helios::input::PenWriters") {
  TEST_CASE("helios::input::PenWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<PenWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddPenMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<PenWriters>();
      PenWriters writers =
          SystemParamTraits<PenWriters>::Make(world, local_data, policy);

      writers.proximity.Write(PenProximityMsg{.id = 0, .in_proximity = true});
      CHECK(SystemParam<PenWriters>);
    }
  }
}

TEST_SUITE("helios::input::TouchWriters") {
  TEST_CASE("helios::input::TouchWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<TouchWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddTouchMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<TouchWriters>();
      TouchWriters writers =
          SystemParamTraits<TouchWriters>::Make(world, local_data, policy);

      writers.fingers.Write(TouchInputMsg{.id = 0});
      CHECK(SystemParam<TouchWriters>);
    }
  }
}

TEST_SUITE("helios::input::SensorWriters") {
  TEST_CASE("helios::input::SensorWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<SensorWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddSensorMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<SensorWriters>();
      SensorWriters writers =
          SystemParamTraits<SensorWriters>::Make(world, local_data, policy);

      writers.connection.Write(SensorConnectionMsg{.id = 0, .connected = true});
      CHECK(SystemParam<SensorWriters>);
    }
  }
}
