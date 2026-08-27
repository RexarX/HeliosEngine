#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/input/cursor_cache.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/window.hpp>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#endif

#include "available.hpp"

#include <algorithm>
#include <functional>

using namespace helios;
using namespace helios::sdl3::input;

TEST_SUITE("helios::sdl3::input::Plugin") {
  TEST_CASE("helios::sdl3::input::Plugin::Build") {
    SUBCASE("Inserts the input backend context") {
      app::App app;
      sdl3::Plugin{}.Build(app);
      Plugin{}.Build(app);

      CHECK(app.GetWorld().HasResource<Context>());
      CHECK_NE(
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents),
          nullptr);
    }
  }

  TEST_CASE("helios::sdl3::input::Plugin::Finish") {
    SUBCASE("Leaves input disabled without the input plugin") {
      app::App app;
      sdl3::Plugin{}.Build(app);
      Plugin{}.Build(app);
      Plugin{}.Finish(app);

      CHECK_FALSE(app.GetWorld().ReadResource<Context>().input_enabled);
    }

    SUBCASE("Enables input systems after the input plugin builds") {
      app::App app;
      sdl3::Plugin{}.Build(app);
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
      sdl3::window::Plugin{}.Build(app);
#endif
      window::Plugin{}.Build(app);
      input::Plugin{}.Build(app);
      Plugin{}.Build(app);
      Plugin{}.Finish(app);

      CHECK(app.GetWorld().ReadResource<Context>().input_enabled);
      CHECK(app.GetWorld().HasResource<GamepadCache>());
      CHECK(app.GetWorld().HasResource<PenCache>());
      CHECK(app.GetWorld().HasResource<CursorCache>());
    }

    SUBCASE("Events schedule builds with input systems enabled") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      sdl3::Plugin{}.Build(app);
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
      sdl3::window::Plugin{}.Build(app);
#endif
      window::Plugin{}.Build(app);
      input::Plugin{}.Build(app);
      Plugin{}.Build(app);
      sdl3::Plugin{}.Finish(app);
      Plugin{}.Finish(app);

      auto* events =
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents);
      REQUIRE_NE(events, nullptr);
      CHECK(events->Build().has_value());

      ecs::MainThreadExecutor executor;
      events->RunAndWait(app.GetWorld(), executor);
    }
  }

  TEST_CASE("helios::sdl3::input::Plugin::Destroy") {
    SUBCASE("Releases the gamepad subsystem after init") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      sdl3::Plugin{}.Build(app);
      window::Plugin{}.Build(app);
      input::Plugin{}.Build(app);
      Plugin{}.Build(app);
      sdl3::Plugin{}.Finish(app);
      Plugin{}.Finish(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      CHECK(app.GetWorld().ReadResource<Context>().gamepad_subsystem_retained);

      Plugin{}.Destroy(app);

      CHECK_FALSE(
          app.GetWorld().ReadResource<Context>().gamepad_subsystem_retained);
    }
  }
}

TEST_SUITE("helios::sdl3::input::InputPlugin") {
  TEST_CASE("helios::sdl3::input::InputPlugin::ctor") {
    SUBCASE("Adds sdl3, input, and input-backend plugins") {
      app::App app;
      app.AddPluginGroups(InputPlugin{});
      const bool all_plugins_added = std::ranges::all_of(
          app.HasPlugins<sdl3::Plugin, input::Plugin, Plugin>(),
          std::identity{});
      CHECK(all_plugins_added);
    }
  }
}
#endif
