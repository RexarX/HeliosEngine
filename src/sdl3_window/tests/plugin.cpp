#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <functional>
#include <helios/app/application.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/resources.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <algorithm>
#include <functional>

using namespace helios;
using namespace helios::sdl3::window;

TEST_SUITE("helios::sdl3::window::Plugin") {
  TEST_CASE("helios::sdl3::window::Plugin::Build") {
    SUBCASE("Registers backend resources and the events schedule") {
      app::App app;
      sdl3::Plugin{}.Build(app);
      window::Plugin{}.Build(app);
      Plugin{}.Build(app);

      CHECK(app.GetWorld().HasResource<Context>());
      CHECK(app.GetWorld().HasResource<NativeWindows>());
      CHECK(app.GetWorld().HasResource<WindowMap>());
      CHECK_NE(
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents),
          nullptr);
      CHECK(app.GetWorld().ReadResource<app::MainFrameOrder>().Contains(
          window::kWindowStage));
    }

    SUBCASE("Events schedule builds and runs") {
      app::App app;
      sdl3::Plugin sdl3_plugin;
      window::Plugin window_plugin;
      Plugin backend;
      sdl3_plugin.Build(app);
      window_plugin.Build(app);
      backend.Build(app);
      sdl3_plugin.Finish(app);

      auto* events =
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents);
      REQUIRE_NE(events, nullptr);
      CHECK(events->Build().has_value());

      ecs::MainThreadExecutor executor;
      events->RunAndWait(app.GetWorld(), executor);
    }
  }

  TEST_CASE("helios::sdl3::window::Plugin::Destroy") {
    SUBCASE("Leaves native windows empty when SDL was never initialized") {
      app::App app;
      sdl3::Plugin{}.Build(app);
      window::Plugin{}.Build(app);
      Plugin plugin;
      plugin.Build(app);
      plugin.Destroy(app);

      CHECK_FALSE(app.GetWorld().ReadResource<Context>().initialized);
      CHECK(app.GetWorld().ReadResource<NativeWindows>().Empty());
    }
  }
}

TEST_SUITE("helios::sdl3::window::WindowPlugin") {
  TEST_CASE("helios::sdl3::window::WindowPlugin::ctor") {
    SUBCASE("Adds sdl3, window, and window-backend plugins") {
      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      const bool all_plugins_added = std::ranges::all_of(
          app.HasPlugins<sdl3::Plugin, window::Plugin, Plugin>(),
          std::identity{});
      CHECK(all_plugins_added);
    }

    SUBCASE("Forwards window settings into the window plugin") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(
          WindowPlugin{{.exit_triggers = window::kExitTriggersNone}});
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      CHECK_EQ(app.GetWorld().ReadResource<window::Settings>().exit_triggers,
               window::kExitTriggersNone);
    }
  }
}
#endif
