#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/window.hpp>

using namespace helios;
using namespace helios::window;

TEST_SUITE("helios::window::Plugin") {
  TEST_CASE("helios::window::Plugin::ctor") {
    SUBCASE("Default settings use default exit triggers") {
      const Plugin plugin;
      CHECK_EQ(plugin.settings_.exit_triggers, kExitTriggersDefault);
      CHECK_EQ(plugin.settings_.event_mode, EventMode::kPoll);
      CHECK_EQ(plugin.settings_.event_wait_timeout,
               Settings::kDefaultEventWaitTimeout);
    }

    SUBCASE("Stores constructor-provided settings") {
      const Plugin plugin{{.event_wait_timeout = 0.05,
                           .exit_triggers = kExitTriggersLastWindow,
                           .event_mode = EventMode::kWaitTimeout}};
      CHECK_EQ(plugin.settings_.exit_triggers, kExitTriggersLastWindow);
      CHECK_EQ(plugin.settings_.event_mode, EventMode::kWaitTimeout);
      CHECK_EQ(plugin.settings_.event_wait_timeout, doctest::Approx(0.05));
    }
  }

  TEST_CASE("helios::window::Plugin::Build") {
    SUBCASE("Inserts settings, monitors, clipboard, and messages") {
      app::App app;
      Plugin plugin;
      plugin.Build(app);

      CHECK(app.GetWorld().HasResource<Settings>());
      CHECK_EQ(app.GetWorld().ReadResource<Settings>().exit_triggers,
               kExitTriggersDefault);
      CHECK_EQ(app.GetWorld().ReadResource<Settings>().event_mode,
               EventMode::kPoll);
      CHECK(app.GetWorld().HasResource<Monitors>());
      CHECK(app.GetWorld().HasResource<Clipboard>());
      CHECK(app.GetWorld().HasMessage<CreatedMsg>());
      CHECK(app.GetWorld().HasMessage<ClosedMsg>());
      CHECK(app.GetWorld().HasMessage<ResizedMsg>());
      CHECK(app.GetWorld().HasMessage<ClientResizedMsg>());
      CHECK(app.GetWorld().HasMessage<ContentScaleChangedMsg>());
      CHECK(app.GetWorld().HasMessage<PosChangedMsg>());
      CHECK(app.GetWorld().HasMessage<ModeChangedMsg>());
      CHECK(app.GetWorld().HasMessage<CursorModeChangedMsg>());
      CHECK(app.GetWorld().HasMessage<VisibilityChangedMsg>());
      CHECK(app.GetWorld().HasMessage<FocusChangedMsg>());
      CHECK(app.GetWorld().HasMessage<MaximizedChangedMsg>());
      CHECK(app.GetWorld().HasMessage<IconChangedMsg>());
      CHECK(app.GetWorld().HasMessage<ResizableChangedMsg>());
      CHECK(app.GetWorld().HasMessage<DecoratedChangedMsg>());
      CHECK(app.GetWorld().HasMessage<OpacityChangedMsg>());
      CHECK(app.GetWorld().HasMessage<FloatingChangedMsg>());
      CHECK(app.GetWorld().HasMessage<HoverChangedMsg>());
      CHECK(app.GetWorld().HasMessage<MousePassthroughChangedMsg>());
      CHECK(app.GetWorld().HasMessage<ClipboardChangedMsg>());
      CHECK(app.GetWorld().HasMessage<DroppedFilesMsg>());
      CHECK(app.GetWorld().HasMessage<CloseRequestedMsg>());
      CHECK(app.GetWorld().HasMessage<CreationFailedMsg>());
      CHECK(app.GetWorld().HasMessage<MonitorConnectedMsg>());
      CHECK(app.GetWorld().HasMessage<MonitorDisconnectedMsg>());
    }

    SUBCASE("Inserts constructor-provided settings") {
      app::App app;
      Plugin{{.exit_triggers = kExitTriggersLastWindow}}.Build(app);

      CHECK_EQ(app.GetWorld().ReadResource<Settings>().exit_triggers,
               kExitTriggersLastWindow);
    }

    SUBCASE("Does not overwrite settings inserted by an earlier build") {
      app::App app;
      Plugin{{.exit_triggers = kExitTriggersNone}}.Build(app);
      Plugin{{.exit_triggers = kExitTriggersLastWindow}}.Build(app);

      CHECK_EQ(app.GetWorld().ReadResource<Settings>().exit_triggers,
               kExitTriggersNone);
    }
  }
}

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
      monitors.monitors.push_back(Monitor{
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
      monitors.monitors.push_back(Monitor{
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
