#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/input.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddInputMessages(World& world) {
  world.AddMessages<
      KeyboardInputMsg, TextInputMsg, MouseButtonInputMsg, CursorMovedMsg,
      MouseMotionMsg, MouseWheelMsg, GamepadConnectionMsg,
      GamepadButtonInputMsg, GamepadAxisChangedMsg, GamepadRemappedMsg,
      GamepadPowerChangedMsg, GamepadSensorUpdateMsg, GamepadTouchpadMsg,
      JoystickConnectionMsg, JoystickButtonInputMsg, JoystickAxisChangedMsg,
      JoystickHatChangedMsg, PenProximityMsg, PenTouchMsg, PenButtonInputMsg,
      PenMovedMsg, PenAxisChangedMsg>();
}

void RunUpdatePen(World& world) {
  SystemLocalData local_data = SystemLocalData::From();
  const AccessPolicy policy = BuildPolicyFromParams<Res<Pens>, PenMessages>();
  UpdatePenState{}(
      SystemParamTraits<Res<Pens>>::Make(world, local_data, policy),
      SystemParamTraits<PenMessages>::Make(world, local_data, policy));
}

void RunUpdateJoystick(World& world) {
  SystemLocalData local_data = SystemLocalData::From();
  const AccessPolicy policy =
      BuildPolicyFromParams<Res<Joysticks>, JoystickMessages>();
  UpdateJoystickState{}(
      SystemParamTraits<Res<Joysticks>>::Make(world, local_data, policy),
      SystemParamTraits<JoystickMessages>::Make(world, local_data, policy));
}

void RunUpdateGamepad(World& world) {
  SystemLocalData local_data = SystemLocalData::From();
  const AccessPolicy policy =
      BuildPolicyFromParams<Res<Gamepads>, Res<const Settings>,
                            GamepadMessages>();
  UpdateGamepadState{}(
      SystemParamTraits<Res<Gamepads>>::Make(world, local_data, policy),
      SystemParamTraits<Res<const Settings>>::Make(world, local_data, policy),
      SystemParamTraits<GamepadMessages>::Make(world, local_data, policy));
}

}  // namespace

TEST_SUITE("helios::input::ClearInputState") {
  TEST_CASE("helios::input::ClearInputState::operator()") {
    SUBCASE("Clears button edges and mouse deltas") {
      Keyboard keyboard;
      Mouse mouse;
      Gamepads gamepads;
      Joysticks joysticks;
      Pens pens;

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

      ClearInputState{}(State{
          .keyboard = Res<Keyboard>{keyboard},
          .mouse = Res<Mouse>{mouse},
          .gamepads = Res<Gamepads>{gamepads},
          .joysticks = Res<Joysticks>{joysticks},
          .pens = Res<Pens>{pens},
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
    }
  }
}

TEST_SUITE("helios::input::UpdateKeyboardState") {
  TEST_CASE("helios::input::UpdateKeyboardState::operator()") {
    SUBCASE("Applies press, release, repeat, and modifiers") {
      World world;
      world.InsertResources(Keyboard{});
      AddInputMessages(world);

      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kA,
          .state = ButtonState::kPressed,
          .modifiers = Modifiers::kShift,
      });
      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kA,
          .state = ButtonState::kRepeat,
          .modifiers = Modifiers::kShift | Modifiers::kControl,
      });

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Keyboard>, KeyboardMessages>();
      UpdateKeyboardState{}(
          SystemParamTraits<Res<Keyboard>>::Make(world, local_data, policy),
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy));

      const auto& keyboard = world.ReadResource<Keyboard>();
      CHECK(keyboard.keys.Pressed(Key::kA));
      CHECK(keyboard.keys.JustPressed(Key::kA));
      CHECK_EQ(keyboard.modifiers, Modifiers::kShift | Modifiers::kControl);
    }

    SUBCASE("Applies a release after a prior press") {
      World world;
      Keyboard keyboard;
      keyboard.keys.Press(Key::kB);
      keyboard.keys.Clear();
      world.InsertResources(std::move(keyboard));
      AddInputMessages(world);

      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kB,
          .state = ButtonState::kReleased,
      });

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Keyboard>, KeyboardMessages>();
      UpdateKeyboardState{}(
          SystemParamTraits<Res<Keyboard>>::Make(world, local_data, policy),
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy));

      const auto& updated = world.ReadResource<Keyboard>();
      CHECK_FALSE(updated.keys.Pressed(Key::kB));
      CHECK(updated.keys.JustReleased(Key::kB));
    }
  }
}

TEST_SUITE("helios::input::UpdateMouseState") {
  TEST_CASE("helios::input::UpdateMouseState::operator()") {
    SUBCASE("Applies buttons, last cursor position, and accumulated motion") {
      World world;
      world.InsertResources(Mouse{});
      AddInputMessages(world);

      world.WriteMessages<MouseButtonInputMsg>().Write({
          .button = MouseButton::kLeft,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<CursorMovedMsg>().Write({.x = 1.0, .y = 2.0});
      world.WriteMessages<CursorMovedMsg>().Write({.x = 10.0, .y = 20.0});
      world.WriteMessages<MouseMotionMsg>().Write(
          {.delta_x = 3.0, .delta_y = -1.5});
      world.WriteMessages<MouseMotionMsg>().Write(
          {.delta_x = 1.0, .delta_y = 0.5});
      world.WriteMessages<MouseWheelMsg>().Write({.x = 0.25, .y = 1.0});
      world.WriteMessages<MouseWheelMsg>().Write({.x = 0.25, .y = 0.5});

      SystemLocalData local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Mouse>, MouseMessages>();
      UpdateMouseState{}(
          SystemParamTraits<Res<Mouse>>::Make(world, local_data, policy),
          SystemParamTraits<MouseMessages>::Make(world, local_data, policy));

      const auto& mouse = world.ReadResource<Mouse>();
      CHECK(mouse.buttons.Pressed(MouseButton::kLeft));
      CHECK_EQ(mouse.position_x, 10.0);
      CHECK_EQ(mouse.position_y, 20.0);
      CHECK_EQ(mouse.delta_x, 4.0);
      CHECK_EQ(mouse.delta_y, -1.0);
      CHECK_EQ(mouse.scroll_x, 0.5);
      CHECK_EQ(mouse.scroll_y, 1.5);
    }
  }
}

TEST_SUITE("helios::input::UpdateGamepadState") {
  TEST_CASE("helios::input::UpdateGamepadState::operator()") {
    SUBCASE("Connects a pad, applies buttons, and filters stick axes") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.stick = {.deadzone = 0.2F,
                                                           .livezone = 1.0F,
                                                           .rescale = true},
                                                 .auto_calibrate = false});
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

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

      const auto& filter = world.ReadResource<Gamepads>().filters[0];
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.12F));
    }

    SUBCASE("Leaves default centers when auto_calibrate is disabled") {
      World world;
      world.InsertResources(Gamepads{}, Settings{.auto_calibrate = false});
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

      world.WriteMessages<GamepadConnectionMsg>().Write(
          {.id = -1, .connected = true, .name = "bad"});
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
      AddInputMessages(world);

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
      CHECK(pad.touchpad[0].down);
      CHECK_EQ(pad.touchpad[0].x, doctest::Approx(0.25F));
      CHECK_EQ(pad.touchpad_count, 1);
    }
  }
}

TEST_SUITE("helios::input::UpdateJoystickState") {
  TEST_CASE("helios::input::UpdateJoystickState::operator()") {
    SUBCASE("Connects and applies button, axis, and hat samples") {
      World world;
      world.InsertResources(Joysticks{});
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

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
      AddInputMessages(world);

      world.WriteMessages<JoystickConnectionMsg>().Write(
          {.name = "bad", .id = -1, .connected = true});
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

TEST_SUITE("helios::input::UpdatePenState") {
  TEST_CASE("helios::input::UpdatePenState::operator()") {
    SUBCASE(
        "Applies proximity, first position without a delta spike, then "
        "motion") {
      World world;
      world.InsertResources(Pens{});
      AddInputMessages(world);

      world.WriteMessages<PenProximityMsg>().Write({
          .id = 0,
          .device_type = PenDeviceType::kIndirect,
          .in_proximity = true,
      });
      world.WriteMessages<PenMovedMsg>().Write({
          .x = 10.0,
          .y = 20.0,
          .id = 0,
      });
      world.WriteMessages<PenMovedMsg>().Write({
          .x = 14.0,
          .y = 24.0,
          .id = 0,
      });
      world.WriteMessages<PenTouchMsg>().Write({
          .x = 14.0,
          .y = 24.0,
          .id = 0,
          .down = true,
          .eraser = false,
      });
      world.WriteMessages<PenButtonInputMsg>().Write({
          .id = 0,
          .button = PenButton::kBarrel1,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<PenAxisChangedMsg>().Write({
          .x = 14.0,
          .y = 24.0,
          .value = 0.75F,
          .id = 0,
          .axis = PenAxis::kPressure,
      });

      RunUpdatePen(world);

      const Pen& pen = world.ReadResource<Pens>().pens[0];
      CHECK(pen.in_proximity);
      CHECK_EQ(pen.id, 0);
      CHECK_EQ(pen.device_type, PenDeviceType::kIndirect);
      CHECK(pen.down);
      CHECK_FALSE(pen.eraser);
      CHECK(pen.buttons.Pressed(PenButton::kBarrel1));
      CHECK_EQ(pen.position_x, 14.0);
      CHECK_EQ(pen.position_y, 24.0);
      CHECK_EQ(pen.delta_x, 4.0);
      CHECK_EQ(pen.delta_y, 4.0);
      CHECK_EQ(pen.axes.Get(PenAxis::kPressure), doctest::Approx(0.75F));
    }

    SUBCASE("Proximity out resets the slot") {
      World world;
      Pens pens;
      pens.pens[1].id = 1;
      pens.pens[1].in_proximity = true;
      pens.pens[1].down = true;
      pens.pens[1].buttons.Press(PenButton::kBarrel2);
      pens.pens[1].position_x = 5.0;
      world.InsertResources(std::move(pens));
      AddInputMessages(world);

      world.WriteMessages<PenProximityMsg>().Write({
          .id = 1,
          .device_type = PenDeviceType::kDirect,
          .in_proximity = false,
      });

      RunUpdatePen(world);

      const Pen& pen = world.ReadResource<Pens>().pens[1];
      CHECK_FALSE(pen.in_proximity);
      CHECK_EQ(pen.id, 1);
      CHECK_FALSE(pen.down);
      CHECK_FALSE(pen.buttons.Pressed(PenButton::kBarrel2));
      CHECK_EQ(pen.position_x, 0.0);
    }

    SUBCASE("Ignores samples while not in proximity") {
      World world;
      world.InsertResources(Pens{});
      AddInputMessages(world);

      world.WriteMessages<PenMovedMsg>().Write({
          .x = 1.0,
          .y = 2.0,
          .id = 0,
      });
      world.WriteMessages<PenButtonInputMsg>().Write({
          .id = 0,
          .button = PenButton::kBarrel1,
          .state = ButtonState::kPressed,
      });

      RunUpdatePen(world);

      const Pen& pen = world.ReadResource<Pens>().pens[0];
      CHECK_FALSE(pen.in_proximity);
      CHECK_EQ(pen.position_x, 0.0);
      CHECK_FALSE(pen.buttons.AnyPressed());
    }

    SUBCASE("Ignores out-of-range pen ids") {
      World world;
      world.InsertResources(Pens{});
      AddInputMessages(world);

      world.WriteMessages<PenProximityMsg>().Write({
          .id = -1,
          .device_type = PenDeviceType::kDirect,
          .in_proximity = true,
      });
      world.WriteMessages<PenProximityMsg>().Write({
          .id = static_cast<int32_t>(Pens::kSlotCount),
          .device_type = PenDeviceType::kDirect,
          .in_proximity = true,
      });

      RunUpdatePen(world);

      CHECK_FALSE(world.ReadResource<Pens>().pens[0].in_proximity);
    }
  }
}
