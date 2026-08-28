#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/app/builtin/frame_limiter.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/properties.hpp>

using namespace helios;
using namespace helios::window;

TEST_SUITE("helios::window::SyncFrameLimiterRefreshRate") {
  TEST_CASE("helios::window::SyncFrameLimiterRefreshRate::operator()") {
    SUBCASE("Does nothing when FrameLimiter is absent") {
      app::App app;
      app.AddPlugins(Plugin{});
      app.Initialize();
      app.Update();
      CHECK_FALSE(app.GetWorld().HasResource<app::FrameLimiter>());
    }

    SUBCASE("Copies primary monitor refresh rate in Auto mode") {
      app::App app;
      app.AddPlugins(app::FrameLimiterPlugin{app::FrameLimiterSettings::Auto()},
                     Plugin{});
      app.Initialize();

      auto& monitors = app.GetWorld().WriteResource<Monitors>();
      monitors.monitors.push_back({
          .index = 0,
          .current = VideoMode{.refresh_rate = 144},
          .primary = true,
      });

      const auto entity = app.GetWorld().CreateEntity();
      Window window;
      window.SetMonitorIndex(0);
      app.GetWorld().AddComponents(entity, std::move(window), Primary{});

      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<app::FrameLimiter>().RefreshRate(),
               144U);
    }

    SUBCASE("Leaves refresh rate unchanged when limiter is not Auto") {
      app::App app;
      app.AddPlugins(
          app::FrameLimiterPlugin{app::FrameLimiterSettings::FromFPS(60)},
          Plugin{});
      app.Initialize();

      auto& monitors = app.GetWorld().WriteResource<Monitors>();
      monitors.monitors.push_back({
          .index = 0,
          .current = VideoMode{.refresh_rate = 75},
          .primary = true,
      });
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<app::FrameLimiter>().RefreshRate(),
               0U);
    }
  }
}
