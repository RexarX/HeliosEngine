#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/plugin.hpp>
#include <helios/input/sensor.hpp>
#include <helios/input/touch.hpp>

using namespace helios;
using namespace helios::input;

TEST_SUITE("helios::input::Plugin") {
  TEST_CASE("helios::input::Plugin::ctor") {
    SUBCASE("Default settings use stick and trigger filters") {
      const Plugin plugin;
      CHECK_EQ(plugin.settings_.stick.deadzone,
               doctest::Approx(AxisFilter::kDefaultDeadzone));
      CHECK_EQ(plugin.settings_.trigger.deadzone,
               doctest::Approx(AxisFilter::kDefaultTriggerDeadzone));
      CHECK_EQ(plugin.settings_.rest_frames, Settings::kDefaultRestFrames);
      CHECK(plugin.settings_.auto_calibrate);
      CHECK_FALSE(plugin.settings_.raw_mouse_motion);
    }

    SUBCASE("Stores constructor-provided settings") {
      const Plugin plugin{{.stick = {.deadzone = 0.25F},
                           .auto_calibrate = false,
                           .raw_mouse_motion = true}};
      CHECK_EQ(plugin.settings_.stick.deadzone, doctest::Approx(0.25F));
      CHECK_FALSE(plugin.settings_.auto_calibrate);
      CHECK(plugin.settings_.raw_mouse_motion);
    }
  }

  TEST_CASE("helios::input::Plugin::Build") {
    SUBCASE("Inserts resources and registers messages") {
      app::App app;
      Plugin plugin;
      plugin.Build(app);

      CHECK(app.GetWorld().HasResource<Settings>());
      CHECK(app.GetWorld().HasResource<Keyboard>());
      CHECK(app.GetWorld().HasResource<Mouse>());
      CHECK(app.GetWorld().HasResource<Gamepads>());
      CHECK(app.GetWorld().HasResource<Joysticks>());
      CHECK(app.GetWorld().HasResource<Pens>());
      CHECK(app.GetWorld().HasResource<Touches>());
      CHECK(app.GetWorld().HasResource<Sensors>());
      CHECK(app.GetWorld().HasResource<GamepadMappings>());

      CHECK(app.GetWorld().HasMessage<KeyboardInputMsg>());
      CHECK(app.GetWorld().HasMessage<TextInputMsg>());
      CHECK(app.GetWorld().HasMessage<TextEditingMsg>());
      CHECK(app.GetWorld().HasMessage<TextEditingCandidatesMsg>());
      CHECK(app.GetWorld().HasMessage<KeyboardConnectionMsg>());
      CHECK(app.GetWorld().HasMessage<MouseButtonInputMsg>());
      CHECK(app.GetWorld().HasMessage<CursorMovedMsg>());
      CHECK(app.GetWorld().HasMessage<MouseMotionMsg>());
      CHECK(app.GetWorld().HasMessage<MouseWheelMsg>());
      CHECK(app.GetWorld().HasMessage<MouseConnectionMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadConnectionMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadButtonInputMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadAxisChangedMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadRemappedMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadPowerChangedMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadSensorUpdateMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadTouchpadMsg>());
      CHECK(app.GetWorld().HasMessage<JoystickConnectionMsg>());
      CHECK(app.GetWorld().HasMessage<JoystickButtonInputMsg>());
      CHECK(app.GetWorld().HasMessage<JoystickAxisChangedMsg>());
      CHECK(app.GetWorld().HasMessage<JoystickHatChangedMsg>());
      CHECK(app.GetWorld().HasMessage<PenProximityMsg>());
      CHECK(app.GetWorld().HasMessage<PenTouchMsg>());
      CHECK(app.GetWorld().HasMessage<PenButtonInputMsg>());
      CHECK(app.GetWorld().HasMessage<PenMovedMsg>());
      CHECK(app.GetWorld().HasMessage<PenAxisChangedMsg>());
      CHECK(app.GetWorld().HasMessage<TouchInputMsg>());
      CHECK(app.GetWorld().HasMessage<SensorConnectionMsg>());
      CHECK(app.GetWorld().HasMessage<SensorUpdateMsg>());
    }

    SUBCASE("Inserts constructor-provided settings") {
      app::App app;
      Plugin{{.stick = {.deadzone = 0.25F}, .raw_mouse_motion = true}}.Build(
          app);

      const auto& settings = app.GetWorld().ReadResource<Settings>();
      CHECK_EQ(settings.stick.deadzone, doctest::Approx(0.25F));
      CHECK(settings.raw_mouse_motion);
    }

    SUBCASE("Does not overwrite settings inserted by an earlier build") {
      app::App app;
      Plugin{{.stick = {.deadzone = 0.3F}}}.Build(app);
      Plugin{{.stick = {.deadzone = 0.9F}}}.Build(app);

      CHECK_EQ(app.GetWorld().ReadResource<Settings>().stick.deadzone,
               doctest::Approx(0.3F));
    }

    SUBCASE("Update systems apply messages written before kFirst") {
      app::App app;
      Plugin{}.Build(app);
      app.Initialize();

      auto& world = app.GetWorld();
      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kA,
          .state = ButtonState::kPressed,
          .modifiers = Modifiers::kShift,
      });
      world.WriteMessages<MouseMotionMsg>().Write(
          {.delta_x = 3.0, .delta_y = -1.5});
      world.WriteMessages<GamepadButtonInputMsg>().Write({
          .id = 0,
          .button = GamepadButton::kA,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<JoystickConnectionMsg>().Write({
          .name = "Stick",
          .id = 2,
          .button_count = 4,
          .connected = true,
      });
      world.WriteMessages<JoystickButtonInputMsg>().Write({
          .id = 2,
          .button = 0,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<PenProximityMsg>().Write({
          .id = 0,
          .device_type = PenDeviceType::kDirect,
          .in_proximity = true,
      });
      world.WriteMessages<PenButtonInputMsg>().Write({
          .id = 0,
          .button = PenButton::kBarrel1,
          .state = ButtonState::kPressed,
      });

      app.Update();

      const auto& keyboard = world.ReadResource<Keyboard>();
      const auto& mouse = world.ReadResource<Mouse>();
      const auto& gamepads = world.ReadResource<Gamepads>();
      const auto& joysticks = world.ReadResource<Joysticks>();
      const auto& pens = world.ReadResource<Pens>();

      CHECK(keyboard.keys.Pressed(Key::kA));
      CHECK(keyboard.keys.JustPressed(Key::kA));
      CHECK_EQ(keyboard.modifiers, Modifiers::kShift);
      CHECK_EQ(mouse.delta_x, 3.0);
      CHECK_EQ(mouse.delta_y, -1.5);
      CHECK(gamepads.pads[0].buttons.Pressed(GamepadButton::kA));
      CHECK(gamepads.pads[0].buttons.JustPressed(GamepadButton::kA));
      CHECK(joysticks.sticks[2].connected);
      CHECK(joysticks.sticks[2].buttons.Pressed(0));
      CHECK(pens.pens[0].in_proximity);
      CHECK(pens.pens[0].buttons.Pressed(PenButton::kBarrel1));
    }

    SUBCASE("A second frame without messages clears just-pressed") {
      app::App app;
      Plugin{}.Build(app);
      app.Initialize();

      auto& world = app.GetWorld();
      world.WriteMessages<KeyboardInputMsg>().Write(KeyboardInputMsg{
          .key = Key::kW,
          .state = ButtonState::kPressed,
      });
      app.Update();
      app.Update();

      const auto& keyboard = world.ReadResource<Keyboard>();
      CHECK(keyboard.keys.Pressed(Key::kW));
      CHECK_FALSE(keyboard.keys.JustPressed(Key::kW));
    }
  }
}
