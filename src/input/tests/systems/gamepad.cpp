#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/gamepad.hpp>

#include <cstddef>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddGamepadMessages(World& world) {
  world.AddMessages<GamepadConnectionMsg, GamepadButtonInputMsg,
                    GamepadAxisChangedMsg, GamepadRemappedMsg,
                    GamepadPowerChangedMsg, GamepadSensorUpdateMsg,
                    GamepadTouchpadMsg>();
}

void RunUpdateGamepad(World& world) {
  auto local_data = SystemLocalData::From();
  const AccessPolicy policy =
      BuildPolicyFromParams<Res<Gamepads>, Res<const Settings>,
                            GamepadMessages>();
  UpdateGamepadState{}(
      SystemParamTraits<Res<Gamepads>>::Make(world, local_data, policy),
      SystemParamTraits<Res<const Settings>>::Make(world, local_data, policy),
      SystemParamTraits<GamepadMessages>::Make(world, local_data, policy));
}

}  // namespace

TEST_SUITE("helios::input::UpdateGamepadState") {
  TEST_CASE("helios::input::UpdateGamepadState::operator()") {
    SUBCASE("Connects a pad, applies buttons, and filters stick axes") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.stick = {.deadzone = 0.2F,
                                                           .livezone = 1.0F,
                                                           .rescale = true},
                                                 .auto_calibrate = false});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write({
          .id = 0,
          .connected = true,
          .name = "Test Pad",
      });
      world.WriteMessages<GamepadButtonInputMsg>().Write({
          .id = 0,
          .button = GamepadButton::kA,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.1F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftY,
          .value = 0.5F,
      });

      RunUpdateGamepad(world);

      const auto [expected_x, expected_y] =
          ApplyRadialDeadzone(0.1F, 0.5F, 0.2F, 1.0F, true);
      const Gamepad& pad = world.ReadResource<Gamepads>().pads[0];
      CHECK(pad.connected);
      CHECK_EQ(pad.id, 0);
      CHECK_EQ(pad.name, "Test Pad");
      CHECK(pad.buttons.Pressed(GamepadButton::kA));
      CHECK_EQ(pad.axes.Get(GamepadAxis::kLeftX), doctest::Approx(expected_x));
      CHECK_EQ(pad.axes.Get(GamepadAxis::kLeftY), doctest::Approx(expected_y));
    }

    SUBCASE("Remaps GLFW triggers from -1..1 to 0..1") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.auto_calibrate = false});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad"});
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftTrigger,
          .value = -1.0F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kRightTrigger,
          .value = 1.0F,
      });

      RunUpdateGamepad(world);

      const Gamepad& pad = world.ReadResource<Gamepads>().pads[0];
      CHECK_EQ(pad.axes.Get(GamepadAxis::kLeftTrigger), doctest::Approx(0.0F));
      CHECK_EQ(pad.axes.Get(GamepadAxis::kRightTrigger), doctest::Approx(1.0F));
    }

    SUBCASE(
        "Keeps a diagonal stick deflection that an axial deadzone would drop") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.stick = {.deadzone = 0.1F,
                                                           .livezone = 1.0F,
                                                           .rescale = true},
                                                 .auto_calibrate = false});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad"});
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.09F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftY,
          .value = 0.09F,
      });

      RunUpdateGamepad(world);

      const Gamepad& pad = world.ReadResource<Gamepads>().pads[0];
      CHECK_GT(pad.axes.Get(GamepadAxis::kLeftX), 0.0F);
      CHECK_GT(pad.axes.Get(GamepadAxis::kLeftY), 0.0F);
    }

    SUBCASE("Captures rest center from the first sample") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.auto_calibrate = true});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad"});
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.2F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftY,
          .value = 0.0F,
      });

      RunUpdateGamepad(world);

      const auto& gamepads = world.ReadResource<Gamepads>();
      CHECK_EQ(
          gamepads.filters[0].center[static_cast<size_t>(GamepadAxis::kLeftX)],
          doctest::Approx(0.2F));
      CHECK_EQ(gamepads.pads[0].axes.Get(GamepadAxis::kLeftX),
               doctest::Approx(0.0F));
    }

    SUBCASE("Skips rest capture when the first sample is deflected") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.auto_calibrate = true});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad"});
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.8F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftY,
          .value = 0.0F,
      });

      RunUpdateGamepad(world);

      const auto& gamepads = world.ReadResource<Gamepads>();
      CHECK_EQ(
          gamepads.filters[0].center[static_cast<size_t>(GamepadAxis::kLeftX)],
          doctest::Approx(0.0F));
      CHECK_GT(gamepads.pads[0].axes.Get(GamepadAxis::kLeftX), 0.0F);
    }

    SUBCASE("Recenters after rest_frames while near rest") {
      World world;
      world.InsertResources(Gamepads{},
                            Settings{.rest_frames = 3, .auto_calibrate = true});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad"});
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.0F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftY,
          .value = 0.0F,
      });
      RunUpdateGamepad(world);

      world.ClearMessages();
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.12F,
      });
      RunUpdateGamepad(world);
      CHECK_EQ(world.ReadResource<Gamepads>()
                   .filters[0]
                   .center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.0F));

      world.ClearMessages();
      RunUpdateGamepad(world);

      const GamepadAxisFilter& filter =
          world.ReadResource<Gamepads>().filters[0];
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.12F));
    }

    SUBCASE("Leaves default centers when auto_calibrate is disabled") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.auto_calibrate = false});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad"});
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftX,
          .value = 0.2F,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 0,
          .axis = GamepadAxis::kLeftTrigger,
          .value = -0.8F,
      });
      RunUpdateGamepad(world);

      const auto& filter = world.ReadResource<Gamepads>().filters[0];
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.0F));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftTrigger)],
               doctest::Approx(-1.0F));
    }

    SUBCASE("Disconnect clears name, buttons, axes, and filters") {
      World world;
      Gamepads gamepads;
      gamepads.pads[1].id = 1;
      gamepads.pads[1].connected = true;
      gamepads.pads[1].name = "Old Pad";
      gamepads.pads[1].buttons.Press(GamepadButton::kB);
      gamepads.pads[1].axes.Set(GamepadAxis::kRightX, 1.0F);
      world.InsertResources(std::move(gamepads), Settings{});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write({
          .id = 1,
          .connected = false,
          .name = "Old Pad",
      });

      RunUpdateGamepad(world);

      const auto& updated = world.ReadResource<Gamepads>();
      const Gamepad& pad = updated.pads[1];
      CHECK_FALSE(pad.connected);
      CHECK(pad.name.empty());
      CHECK_FALSE(pad.buttons.Pressed(GamepadButton::kB));
      CHECK_EQ(pad.axes.Get(GamepadAxis::kRightX), doctest::Approx(0.0F));
      CHECK_EQ(updated.filters[1].raw.Get(GamepadAxis::kLeftTrigger),
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(
          updated.filters[1].seen[static_cast<size_t>(GamepadAxis::kLeftX)], 0);
    }

    SUBCASE("Ignores out-of-range gamepad ids") {
      World world;
      world.InsertResources(Gamepads{}, Settings{});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadButtonInputMsg>().Write({
          .id = 99,
          .button = GamepadButton::kA,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<GamepadAxisChangedMsg>().Write({
          .id = 99,
          .axis = GamepadAxis::kLeftX,
          .value = 1.0F,
      });

      RunUpdateGamepad(world);

      const auto& gamepads = world.ReadResource<Gamepads>();
      CHECK_FALSE(gamepads.pads[0].connected);
      CHECK_FALSE(gamepads.pads[0].buttons.AnyPressed());
    }

    SUBCASE("Applies remap, power, sensor, and touchpad extras") {
      World world;
      world.InsertResources(Gamepads{}, Settings{});
      AddGamepadMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = 0, .connected = true, .name = "Pad", .guid = "guid"});
      world.WriteMessages<GamepadRemappedMsg>().Write(
          {.id = 0, .mapping = "a:b0"});
      world.WriteMessages<GamepadPowerChangedMsg>().Write({
          .id = 0,
          .power = {.state = GamepadPowerState::kOnBattery, .percent = 80},
      });
      world.WriteMessages<GamepadSensorUpdateMsg>().Write({
          .id = 0,
          .sensor = GamepadSensor::kGyro,
          .value = {0.1F, 0.2F, 0.3F},
      });
      world.WriteMessages<GamepadSensorUpdateMsg>().Write({
          .id = 0,
          .sensor = GamepadSensor::kAccel,
          .value = {1.0F, 2.0F, 3.0F},
      });
      world.WriteMessages<GamepadTouchpadMsg>().Write({
          .id = 0,
          .x = 0.25F,
          .y = 0.75F,
          .pressure = 1.0F,
          .finger = 0,
          .down = true,
      });

      RunUpdateGamepad(world);

      const Gamepad& pad = world.ReadResource<Gamepads>().pads[0];
      CHECK_EQ(pad.guid, "guid");
      CHECK_EQ(pad.mapping, "a:b0");
      CHECK_EQ(pad.power.state, GamepadPowerState::kOnBattery);
      CHECK_EQ(pad.power.percent, 80);
      CHECK_EQ(pad.gyro[0], doctest::Approx(0.1F));
      CHECK_EQ(pad.accel[2], doctest::Approx(3.0F));
      CHECK(pad.touchpads[0][0].down);
      CHECK_EQ(pad.touchpads[0][0].x, doctest::Approx(0.25F));
      CHECK_EQ(pad.touchpad_count, 1);
    }
  }
}
