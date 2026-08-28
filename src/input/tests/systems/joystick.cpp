#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/joystick.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddJoystickMessages(World& world) {
  world.AddMessages<JoystickConnectionMsg, JoystickButtonInputMsg,
                    JoystickAxisChangedMsg, JoystickHatChangedMsg>();
}

void RunUpdateJoystick(World& world) {
  auto local_data = SystemLocalData::From();
  const AccessPolicy policy =
      BuildPolicyFromParams<Res<Joysticks>, JoystickMessages>();
  UpdateJoystickState{}(
      SystemParamTraits<Res<Joysticks>>::Make(world, local_data, policy),
      SystemParamTraits<JoystickMessages>::Make(world, local_data, policy));
}

}  // namespace

TEST_SUITE("helios::input::UpdateJoystickState") {
  TEST_CASE("helios::input::UpdateJoystickState::operator()") {
    SUBCASE("Connects and applies button, axis, and hat samples") {
      World world;
      world.InsertResources(Joysticks{});
      AddJoystickMessages(world);

      world.WriteMessages<JoystickConnectionMsg>().Write({
          .name = "HOTAS",
          .guid = "guid",
          .id = 1,
          .axis_count = 2,
          .button_count = 3,
          .hat_count = 1,
          .connected = true,
      });
      world.WriteMessages<JoystickButtonInputMsg>().Write({
          .id = 1,
          .button = 2,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<JoystickAxisChangedMsg>().Write({
          .id = 1,
          .axis = 1,
          .value = 0.5F,
      });
      world.WriteMessages<JoystickHatChangedMsg>().Write({
          .id = 1,
          .hat = 0,
          .value = JoystickHat::kUp,
      });

      RunUpdateJoystick(world);

      const Joystick& stick = world.ReadResource<Joysticks>().sticks[1];
      CHECK(stick.connected);
      CHECK_EQ(stick.id, 1);
      CHECK_EQ(stick.name, "HOTAS");
      CHECK_EQ(stick.guid, "guid");
      CHECK_EQ(stick.axis_count, 2);
      CHECK_EQ(stick.button_count, 3);
      CHECK(stick.buttons.Pressed(2));
      CHECK_EQ(stick.axes[1], doctest::Approx(0.5F));
      CHECK_EQ(stick.hats[0], JoystickHat::kUp);
    }

    SUBCASE("Ignores samples beyond reported counts") {
      World world;
      world.InsertResources(Joysticks{});
      AddJoystickMessages(world);

      world.WriteMessages<JoystickConnectionMsg>().Write({
          .name = "Stick",
          .id = 0,
          .axis_count = 1,
          .button_count = 1,
          .hat_count = 1,
          .connected = true,
      });
      world.WriteMessages<JoystickButtonInputMsg>().Write({
          .id = 0,
          .button = 5,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<JoystickAxisChangedMsg>().Write({
          .id = 0,
          .axis = 3,
          .value = 1.0F,
      });
      world.WriteMessages<JoystickHatChangedMsg>().Write({
          .id = 0,
          .hat = 2,
          .value = JoystickHat::kDown,
      });

      RunUpdateJoystick(world);

      const Joystick& stick = world.ReadResource<Joysticks>().sticks[0];
      CHECK_FALSE(stick.buttons.AnyPressed());
      CHECK_EQ(stick.axes[3], doctest::Approx(0.0F));
      CHECK_EQ(stick.hats[2], JoystickHat::kCentered);
    }

    SUBCASE("Disconnect clears identity and samples") {
      World world;
      Joysticks joysticks;
      joysticks.sticks[0].id = 0;
      joysticks.sticks[0].connected = true;
      joysticks.sticks[0].name = "Old";
      joysticks.sticks[0].button_count = 2;
      joysticks.sticks[0].buttons.Press(0);
      joysticks.sticks[0].axes[0] = 1.0F;
      world.InsertResources(std::move(joysticks));
      AddJoystickMessages(world);

      world.WriteMessages<JoystickConnectionMsg>().Write(
          {.name = "Old", .id = 0, .connected = false});

      RunUpdateJoystick(world);

      const Joystick& stick = world.ReadResource<Joysticks>().sticks[0];
      CHECK_FALSE(stick.connected);
      CHECK(stick.name.empty());
      CHECK_FALSE(stick.buttons.AnyPressed());
      CHECK_EQ(stick.axes[0], doctest::Approx(0.0F));
    }

    SUBCASE("Ignores out-of-range joystick ids") {
      World world;
      world.InsertResources(Joysticks{});
      AddJoystickMessages(world);

      world.WriteMessages<JoystickButtonInputMsg>().Write({
          .id = 99,
          .button = 0,
          .state = ButtonState::kPressed,
      });

      RunUpdateJoystick(world);

      CHECK_FALSE(world.ReadResource<Joysticks>().sticks[0].connected);
    }
  }
}
