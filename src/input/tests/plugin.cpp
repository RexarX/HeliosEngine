#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/input/input.hpp>

using namespace helios::input;

TEST_SUITE("helios::input::Plugin") {
  TEST_CASE("helios::input::Plugin::ctor") {
    SUBCASE("Default settings use stick and trigger filters") {
      const Plugin plugin;
      CHECK_EQ(plugin.settings.stick.deadzone,
               doctest::Approx(AxisFilter::kDefaultDeadzone));
      CHECK_EQ(plugin.settings.trigger.deadzone,
               doctest::Approx(AxisFilter::kDefaultTriggerDeadzone));
      CHECK_EQ(plugin.settings.rest_frames, Settings::kDefaultRestFrames);
      CHECK(plugin.settings.auto_calibrate);
      CHECK_FALSE(plugin.settings.raw_mouse_motion);
    }

    SUBCASE("Stores constructor-provided settings") {
      const Plugin plugin{{.stick = {.deadzone = 0.25F},
                           .auto_calibrate = false,
                           .raw_mouse_motion = true}};
      CHECK_EQ(plugin.settings.stick.deadzone, doctest::Approx(0.25F));
      CHECK_FALSE(plugin.settings.auto_calibrate);
      CHECK(plugin.settings.raw_mouse_motion);
    }
  }

  TEST_CASE("helios::input::Plugin::Build") {
    SUBCASE("Inserts resources and registers messages") {
      helios::app::App app;
      Plugin plugin;
      plugin.Build(app);

      CHECK(app.GetWorld().HasResource<Settings>());
      CHECK(app.GetWorld().HasResource<Keyboard>());
      CHECK(app.GetWorld().HasResource<Mouse>());
      CHECK(app.GetWorld().HasResource<Gamepads>());

      CHECK(app.GetWorld().HasMessage<KeyboardInputMsg>());
      CHECK(app.GetWorld().HasMessage<TextInputMsg>());
      CHECK(app.GetWorld().HasMessage<MouseButtonInputMsg>());
      CHECK(app.GetWorld().HasMessage<CursorMovedMsg>());
      CHECK(app.GetWorld().HasMessage<MouseMotionMsg>());
      CHECK(app.GetWorld().HasMessage<MouseWheelMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadConnectionMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadButtonInputMsg>());
      CHECK(app.GetWorld().HasMessage<GamepadAxisChangedMsg>());
    }

    SUBCASE("Inserts constructor-provided settings") {
      helios::app::App app;
      Plugin{{.stick = {.deadzone = 0.25F}, .raw_mouse_motion = true}}.Build(
          app);

      const auto& settings = app.GetWorld().ReadResource<Settings>();
      CHECK_EQ(settings.stick.deadzone, doctest::Approx(0.25F));
      CHECK(settings.raw_mouse_motion);
    }

    SUBCASE("Does not overwrite settings inserted by an earlier build") {
      helios::app::App app;
      Plugin{{.stick = {.deadzone = 0.3F}}}.Build(app);
      Plugin{{.stick = {.deadzone = 0.9F}}}.Build(app);

      CHECK_EQ(app.GetWorld().ReadResource<Settings>().stick.deadzone,
               doctest::Approx(0.3F));
    }

    SUBCASE("Update systems apply messages written before kFirst") {
      helios::app::App app;
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

      app.Update();

      const auto& keyboard = world.ReadResource<Keyboard>();
      const auto& mouse = world.ReadResource<Mouse>();
      const auto& gamepads = world.ReadResource<Gamepads>();

      CHECK(keyboard.keys.Pressed(Key::kA));
      CHECK(keyboard.keys.JustPressed(Key::kA));
      CHECK_EQ(keyboard.modifiers, Modifiers::kShift);
      CHECK_EQ(mouse.delta_x, 3.0);
      CHECK_EQ(mouse.delta_y, -1.5);
      CHECK(gamepads.pads[0].buttons.Pressed(GamepadButton::kA));
      CHECK(gamepads.pads[0].buttons.JustPressed(GamepadButton::kA));
    }

    SUBCASE("A second frame without messages clears just-pressed") {
      helios::app::App app;
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
