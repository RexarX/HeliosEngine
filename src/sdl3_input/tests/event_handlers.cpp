#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE

#include <helios/app/application.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/input/event_handlers.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/poll_sensors.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_touch.h>
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

    SUBCASE("Emits IME composition and candidate messages") {
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

      const char* composition = "ni";
      SDL_Event editing{};
      editing.type = SDL_EVENT_TEXT_EDITING;
      editing.edit.windowID = window_id;
      editing.edit.text = composition;
      editing.edit.start = 2;
      editing.edit.length = 0;
      dispatcher.Dispatch(editing, world);

      const char* candidate0 = "you";
      const char* candidate1 = "ni";
      const char* candidates[] = {candidate0, candidate1};
      SDL_Event list{};
      list.type = SDL_EVENT_TEXT_EDITING_CANDIDATES;
      list.edit_candidates.windowID = window_id;
      list.edit_candidates.candidates = candidates;
      list.edit_candidates.num_candidates = 2;
      list.edit_candidates.selected_candidate = 0;
      list.edit_candidates.horizontal = true;
      dispatcher.Dispatch(list, world);

      const auto edits =
          world.Messages().CurrentMessages<helios::input::TextEditingMsg>();
      REQUIRE_EQ(edits.size(), 1U);
      CHECK_EQ(edits[0].entity, entity);
      CHECK_EQ(edits[0].composition, "ni");
      CHECK_EQ(edits[0].start, 2);
      CHECK_EQ(edits[0].length, 0);

      const auto lists =
          world.Messages()
              .CurrentMessages<helios::input::TextEditingCandidatesMsg>();
      REQUIRE_EQ(lists.size(), 1U);
      CHECK_EQ(lists[0].entity, entity);
      REQUIRE_EQ(lists[0].candidates.size(), 2U);
      CHECK_EQ(lists[0].candidates[0], "you");
      CHECK_EQ(lists[0].candidates[1], "ni");
      CHECK_EQ(lists[0].selected, 0);
      CHECK(lists[0].horizontal);
    }

    SUBCASE("Emits keyboard and mouse connection messages") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      auto& dispatcher = world.WriteResource<sdl3::EventDispatcher>();

      SDL_Event keyboard{};
      keyboard.type = SDL_EVENT_KEYBOARD_ADDED;
      keyboard.kdevice.which = 42;
      dispatcher.Dispatch(keyboard, world);

      SDL_Event mouse{};
      mouse.type = SDL_EVENT_MOUSE_REMOVED;
      mouse.mdevice.which = 7;
      dispatcher.Dispatch(mouse, world);

      const auto keyboards =
          world.Messages()
              .CurrentMessages<helios::input::KeyboardConnectionMsg>();
      REQUIRE_EQ(keyboards.size(), 1U);
      CHECK_EQ(keyboards[0].id, 42);
      CHECK(keyboards[0].connected);

      const auto mice =
          world.Messages().CurrentMessages<helios::input::MouseConnectionMsg>();
      REQUIRE_EQ(mice.size(), 1U);
      CHECK_EQ(mice[0].id, 7);
      CHECK_FALSE(mice[0].connected);
    }

    SUBCASE("Converts finger events into window-pixel TouchInputMsg") {
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

      SDL_Event down{};
      down.type = SDL_EVENT_FINGER_DOWN;
      down.tfinger.windowID = window_id;
      down.tfinger.touchID = 1;
      down.tfinger.fingerID = 2;
      down.tfinger.x = 0.5F;
      down.tfinger.y = 0.25F;
      down.tfinger.dx = 0.0F;
      down.tfinger.dy = 0.0F;
      down.tfinger.pressure = 1.0F;
      dispatcher.Dispatch(down, world);

      SDL_Event up{};
      up.type = SDL_EVENT_FINGER_UP;
      up.tfinger.windowID = window_id;
      up.tfinger.touchID = 1;
      up.tfinger.fingerID = 2;
      up.tfinger.x = 0.5F;
      up.tfinger.y = 0.25F;
      up.tfinger.dx = 0.0F;
      up.tfinger.dy = 0.0F;
      up.tfinger.pressure = 0.0F;
      dispatcher.Dispatch(up, world);

      const auto touches =
          world.Messages().CurrentMessages<helios::input::TouchInputMsg>();
      REQUIRE_EQ(touches.size(), 2U);
      CHECK_EQ(touches[0].entity, entity);
      CHECK_EQ(touches[0].id, 0);
      CHECK_EQ(touches[0].phase, helios::input::TouchPhase::kStarted);
      CHECK_EQ(touches[0].x, doctest::Approx(32.0));
      CHECK_EQ(touches[0].y, doctest::Approx(16.0));
      CHECK_EQ(touches[0].pressure, doctest::Approx(1.0F));
      CHECK_EQ(touches[1].phase, helios::input::TouchPhase::kEnded);
      CHECK_FALSE(world.ReadResource<TouchCache>().slots[0].connected);
    }

    SUBCASE("Emits SensorUpdateMsg for a cached instance id") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      auto& cache = world.WriteResource<SensorCache>();
      cache.slots[0].instance_id = 9;
      cache.slots[0].type = helios::input::SensorType::kAccel;
      cache.slots[0].connected = true;

      SDL_Event event{};
      event.type = SDL_EVENT_SENSOR_UPDATE;
      event.sensor.which = 9;
      event.sensor.data[0] = 1.0F;
      event.sensor.data[1] = 2.0F;
      event.sensor.data[2] = 3.0F;
      world.WriteResource<sdl3::EventDispatcher>().Dispatch(event, world);

      const auto samples =
          world.Messages().CurrentMessages<helios::input::SensorUpdateMsg>();
      REQUIRE_EQ(samples.size(), 1U);
      CHECK_EQ(samples[0].id, 0);
      CHECK_EQ(samples[0].type, helios::input::SensorType::kAccel);
      CHECK_EQ(samples[0].value[0], doctest::Approx(1.0F));
      CHECK_EQ(samples[0].value[1], doctest::Approx(2.0F));
      CHECK_EQ(samples[0].value[2], doctest::Approx(3.0F));
    }
  }
}

#endif  // HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#endif
