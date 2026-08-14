#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/system_param.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/input.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddInputMessages(World& world) {
  world.AddMessages<KeyboardInputMsg, TextInputMsg, MouseButtonInputMsg,
                    CursorMovedMsg, MouseMotionMsg, MouseWheelMsg,
                    GamepadConnectionMsg, GamepadButtonInputMsg,
                    GamepadAxisChangedMsg>();
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
      world.InsertResources(Keyboard{}, Mouse{}, Gamepads{});

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<StateView>();
      const StateView view =
          SystemParamTraits<StateView>::Make(world, local_data, policy);

      CHECK_FALSE(view.keyboard->keys.AnyPressed());
      CHECK_EQ(view.mouse->position_x, 0.0);
      CHECK_EQ(view.gamepads->pads.size(), Gamepads::kSlotCount);
    }

    SUBCASE("RegisterAccess requests read-only access") {
      const auto policy = BuildPolicyFromParams<StateView>();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Keyboard>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Mouse>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Gamepads>()));
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
      AddInputMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<KeyboardMessages>();
      KeyboardMessages messages =
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy);

      CHECK(messages.keys.Empty());
      CHECK(messages.text.Empty());
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
      AddInputMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<MouseMessages>();
      MouseMessages messages =
          SystemParamTraits<MouseMessages>::Make(world, local_data, policy);

      CHECK(messages.buttons.Empty());
      CHECK(messages.cursor.Empty());
      CHECK(messages.motion.Empty());
      CHECK(messages.wheel.Empty());
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
      AddInputMessages(world);

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy = BuildPolicyFromParams<GamepadMessages>();
      GamepadMessages messages =
          SystemParamTraits<GamepadMessages>::Make(world, local_data, policy);

      CHECK(messages.connection.Empty());
      CHECK(messages.buttons.Empty());
      CHECK(messages.axes.Empty());
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
      CHECK(SystemParam<Writers>);
    }
  }
}
