#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE

#include <helios/app/application.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/input/event_handlers.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>

using namespace helios;
using namespace sdl3::input;

namespace {

[[nodiscard]] window::Properties HiddenTestProperties() {
  return {
      .title = "HeliosInputEvents",
      .width = 64,
      .height = 64,
      .visible = false,
      .focused = false,
  };
}

void AddInputPlugins(app::App& app) {
  app.AddPlugins(sdl3::Plugin{}, helios::window::Plugin{},
                 sdl3::window::Plugin{}, helios::input::Plugin{}, Plugin{});
}

[[nodiscard]] SDL_WindowID WindowIdFor(ecs::World& world, ecs::Entity entity) {
  const auto* entry =
      world.ReadResource<sdl3::window::WindowMap>().TryGet(entity);
  REQUIRE_NE(entry, nullptr);
  return entry->window_id;
}

}  // namespace

TEST_SUITE("helios::sdl3::input::RegisterEventHandlers") {
  TEST_CASE("helios::sdl3::input::RegisterEventHandlers") {
    SUBCASE("Emits KeyboardInputMsg for a key-down event") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      SDL_Event event{};
      event.type = SDL_EVENT_KEY_DOWN;
      event.key.windowID = WindowIdFor(world, entity);
      event.key.scancode = SDL_SCANCODE_A;
      event.key.down = true;
      event.key.repeat = false;
      event.key.mod = SDL_KMOD_SHIFT;
      world.WriteResource<sdl3::EventDispatcher>().Dispatch(event, world);

      const auto keys =
          world.Messages().CurrentMessages<helios::input::KeyboardInputMsg>();
      REQUIRE_EQ(keys.size(), 1U);
      CHECK_EQ(keys[0].entity, entity);
      CHECK_EQ(keys[0].key, helios::input::Key::kA);
      CHECK_EQ(keys[0].state, helios::input::ButtonState::kPressed);
      CHECK_EQ(keys[0].modifiers, helios::input::Modifiers::kShift);
    }

    SUBCASE("Decodes UTF-8 text input into codepoints") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      const char* text = "Aé";
      SDL_Event event{};
      event.type = SDL_EVENT_TEXT_INPUT;
      event.text.windowID = WindowIdFor(world, entity);
      event.text.text = text;
      world.WriteResource<sdl3::EventDispatcher>().Dispatch(event, world);

      const auto chars =
          world.Messages().CurrentMessages<helios::input::TextInputMsg>();
      REQUIRE_EQ(chars.size(), 2U);
      CHECK_EQ(chars[0].codepoint, 0x41U);
      CHECK_EQ(chars[1].codepoint, 0xE9U);
    }

    SUBCASE("Emits mouse button, motion, and wheel messages") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();
      const SDL_WindowID window_id = WindowIdFor(world, entity);
      auto& dispatcher = world.WriteResource<sdl3::EventDispatcher>();

      SDL_Event button{};
      button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
      button.button.windowID = window_id;
      button.button.button = SDL_BUTTON_LEFT;
      button.button.down = true;
      dispatcher.Dispatch(button, world);

      SDL_Event motion{};
      motion.type = SDL_EVENT_MOUSE_MOTION;
      motion.motion.windowID = window_id;
      motion.motion.x = 10.0F;
      motion.motion.y = 20.0F;
      motion.motion.xrel = 1.5F;
      motion.motion.yrel = -2.0F;
      dispatcher.Dispatch(motion, world);

      SDL_Event wheel{};
      wheel.type = SDL_EVENT_MOUSE_WHEEL;
      wheel.wheel.windowID = window_id;
      wheel.wheel.x = 1.0F;
      wheel.wheel.y = -3.0F;
      wheel.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
      dispatcher.Dispatch(wheel, world);

      const auto buttons =
          world.Messages()
              .CurrentMessages<helios::input::MouseButtonInputMsg>();
      REQUIRE_EQ(buttons.size(), 1U);
      CHECK_EQ(buttons[0].button, helios::input::MouseButton::kLeft);

      const auto moved =
          world.Messages().CurrentMessages<helios::input::CursorMovedMsg>();
      REQUIRE_EQ(moved.size(), 1U);
      CHECK_EQ(moved[0].x, doctest::Approx(10.0));
      CHECK_EQ(moved[0].y, doctest::Approx(20.0));

      const auto motion_msg =
          world.Messages().CurrentMessages<helios::input::MouseMotionMsg>();
      REQUIRE_EQ(motion_msg.size(), 1U);
      CHECK_EQ(motion_msg[0].delta_x, doctest::Approx(1.5));
      CHECK_EQ(motion_msg[0].delta_y, doctest::Approx(-2.0));

      const auto wheels =
          world.Messages().CurrentMessages<helios::input::MouseWheelMsg>();
      REQUIRE_EQ(wheels.size(), 1U);
      CHECK_EQ(wheels[0].x, doctest::Approx(-1.0));
      CHECK_EQ(wheels[0].y, doctest::Approx(3.0));
    }
  }
}

#endif  // HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#endif
