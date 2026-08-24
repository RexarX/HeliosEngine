#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>
#include <helios/sdl3/input/details/cursor_cache.hpp>
#include <helios/sdl3/input/details/input_state.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/systems/apply_cursors.hpp>
#include <helios/sdl3/input/systems/apply_raw_mouse.hpp>
#include <helios/sdl3/input/systems/init.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#include <helios/sdl3/window/window.hpp>
#endif

#include <SDL3/SDL.h>

#include <string>

using namespace helios;
using namespace helios::sdl3::input;

namespace {

[[nodiscard]] window::Properties HiddenTestProperties() {
  return {
      .title = "HeliosInput",
      .width = 64,
      .height = 64,
      .visible = false,
      .focused = false,
  };
}

void AddInputPlugins(app::App& app) {
  app.AddPlugins(sdl3::Plugin{}, window::Plugin{},
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
                 sdl3::window::Plugin{},
#endif
                 input::Plugin{}, Plugin{});
}

}  // namespace

TEST_SUITE("helios::sdl3::input::Init") {
  TEST_CASE("helios::sdl3::input::Init::operator()") {
    SUBCASE("Retains the gamepad subsystem and registers handlers") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      const auto& context = app.GetWorld().ReadResource<Context>();
      CHECK(context.gamepad_subsystem_retained);
      CHECK(context.handlers_registered);
      CHECK_FALSE(app.GetWorld()
                      .ReadResource<sdl3::EventDispatcher>()
                      .handlers.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::input::DestroyGamepadCache") {
  TEST_CASE("helios::sdl3::input::DestroyGamepadCache") {
    SUBCASE("Clears pending device lists and enumeration state") {
      GamepadCache cache;
      cache.pending_added.push_back(1);
      cache.pending_removed.push_back(2);
      cache.pending_remapped.push_back(3);
      cache.enumerated = true;
      cache.slots[0].connected = true;
      cache.slots[0].name = "pad";

      DestroyGamepadCache(cache);

      CHECK(cache.pending_added.empty());
      CHECK(cache.pending_removed.empty());
      CHECK(cache.pending_remapped.empty());
      CHECK_FALSE(cache.enumerated);
      CHECK_FALSE(cache.slots[0].connected);
      CHECK(cache.slots[0].name.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::input::DestroyCursorCache") {
  TEST_CASE("helios::sdl3::input::DestroyCursorCache") {
    SUBCASE("Clears cached cursor pointers") {
      CursorCache cache;
      cache.custom.push_back({.entity = ecs::Entity{1, 1}, .cursor = nullptr});

      DestroyCursorCache(cache);

      CHECK(cache.custom.empty());
      for (SDL_Cursor* cursor : cache.standard) {
        CHECK_EQ(cursor, nullptr);
      }
    }
  }
}

TEST_SUITE("helios::sdl3::input::ApplyGamepadMappings") {
  TEST_CASE("helios::sdl3::input::ApplyGamepadMappings::operator()") {
    SUBCASE("Consumes queued mapping lines") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& mappings = app.GetWorld().WriteResource<input::GamepadMappings>();
      mappings.Add(
          "03000000000000000000000000000000,Test "
          "Pad,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0."
          "1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,"
          "lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,"
          "righty:a4,start:b7,x:b2,y:b3,platform:Windows,");

      auto local_data = ecs::SystemLocalData::From();
      const ecs::AccessPolicy policy =
          ecs::BuildPolicyFromParams<ecs::Res<const Context>,
                                     ecs::Res<input::GamepadMappings>>();
      ApplyGamepadMappings{}(
          ecs::SystemParamTraits<ecs::Res<const Context>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<input::GamepadMappings>>::Make(
              app.GetWorld(), local_data, policy));

      CHECK_FALSE(app.GetWorld().ReadResource<input::GamepadMappings>().dirty);
      CHECK(app.GetWorld()
                .ReadResource<input::GamepadMappings>()
                .pending_lines.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::input::PollGamepads") {
  TEST_CASE("helios::sdl3::input::PollGamepads::operator()") {
    SUBCASE("Runs without connected gamepads") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      app.Update();

      CHECK(app.GetWorld().HasResource<GamepadCache>());
      CHECK(app.GetWorld()
                .Messages()
                .PreviousMessages<input::JoystickConnectionMsg>()
                .empty());
    }
  }
}

TEST_SUITE("helios::sdl3::input::ApplyGamepadOutputs") {
  TEST_CASE("helios::sdl3::input::ApplyGamepadOutputs::operator()") {
    SUBCASE("Clears dirty output flags without an open SDL gamepad") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& pads = app.GetWorld().WriteResource<input::Gamepads>();
      pads.pads[0].connected = true;
      pads.pads[0].SetRumble(1, 2, 3);
      pads.pads[0].SetLed(4, 5, 6);

      ecs::SystemLocalData local_data = ecs::SystemLocalData::From();
      const ecs::AccessPolicy policy =
          ecs::BuildPolicyFromParams<ecs::Res<const Context>,
                                     ecs::Res<input::Gamepads>,
                                     ecs::Res<GamepadCache>>();
      ApplyGamepadOutputs{}(
          ecs::SystemParamTraits<ecs::Res<const Context>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<input::Gamepads>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<GamepadCache>>::Make(
              app.GetWorld(), local_data, policy));

      CHECK_FALSE(app.GetWorld().ReadResource<input::Gamepads>().pads[0].Dirty(
          input::GamepadDirtyFlags::kRumble));
      CHECK_FALSE(app.GetWorld().ReadResource<input::Gamepads>().pads[0].Dirty(
          input::GamepadDirtyFlags::kLed));
    }
  }
}

TEST_SUITE("helios::sdl3::input::ApplyRawMouseMotion") {
  TEST_CASE("helios::sdl3::input::ApplyRawMouseMotion::operator()") {
    SUBCASE("Writes the relative mouse scale hint") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      app.GetWorld().WriteResource<input::Settings>().raw_mouse_motion = true;
      app.Update();

      const char* raw = SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE);
      REQUIRE_NE(raw, nullptr);
      CHECK_EQ(std::string(raw), "0");
      CHECK(app.GetWorld().ReadResource<Context>().raw_mouse_hint_set);
      CHECK(app.GetWorld().ReadResource<Context>().last_raw_mouse_motion);

      app.GetWorld().WriteResource<input::Settings>().raw_mouse_motion = false;
      app.Update();

      const char* scaled = SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE);
      REQUIRE_NE(scaled, nullptr);
      CHECK_EQ(std::string(scaled), "1");
      CHECK_FALSE(app.GetWorld().ReadResource<Context>().last_raw_mouse_motion);
    }
  }
}

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
TEST_SUITE("helios::sdl3::input::ApplyCursors") {
  TEST_CASE("helios::sdl3::input::ApplyCursors::operator()") {
    SUBCASE("Applies a standard cursor icon and clears dirty") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()),
          input::Cursor{});
      app.Update();

      world.WriteComponent<input::Cursor>(entity).SetIcon(
          input::CursorIcon::kIBeam);
      app.Update();

      CHECK_FALSE(world.ReadComponent<input::Cursor>(entity).dirty);
      CHECK_EQ(world.ReadComponent<input::Cursor>(entity).icon,
               input::CursorIcon::kIBeam);
    }
  }
}
#endif
