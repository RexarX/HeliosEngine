#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/glfw.hpp>
#include <helios/glfw/state.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/glfw/systems/input.hpp>
#include <helios/input/input.hpp>
#endif

#include <string_view>

using namespace helios;
using namespace helios::glfw;

namespace {

struct PumpProbeMsg {
  static constexpr std::string_view kName = "PumpProbeMsg";

  int value = 0;
};

struct WritePumpProbe {
  void operator()(ecs::MessageWriter<PumpProbeMsg> writer) const {
    writer.Write({.value = 11});
  }
};

}  // namespace

TEST_SUITE("helios::glfw::Plugin") {
  TEST_CASE("helios::glfw::Plugin::Build") {
    SUBCASE("Registers backend resources and the events schedule") {
      app::App app;
      window::Plugin{}.Build(app);
      Plugin{}.Build(app);

      CHECK(app.GetWorld().HasResource<Context>());
      CHECK(app.GetWorld().HasResource<NativeWindows>());
      CHECK_NE(
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents),
          nullptr);
      CHECK(app.GetWorld().ReadResource<app::MainFrameOrder>().Contains(
          window::kWindowStage));
    }
  }

  TEST_CASE("helios::glfw::Plugin::Finish") {
    SUBCASE("Leaves input disabled without the input plugin") {
      app::App app;
      window::Plugin{}.Build(app);
      Plugin{}.Build(app);
      Plugin{}.Finish(app);

      CHECK_FALSE(app.GetWorld().ReadResource<Context>().input_enabled);
#ifdef HELIOS_MODULE_INPUT_AVAILABLE
      CHECK_FALSE(app.GetWorld().HasResource<input::Settings>());
#endif
      CHECK_NE(app.GetWorld().ReadResource<Context>().frame_pump, nullptr);
    }

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
    SUBCASE("Enables input systems after the input plugin builds") {
      app::App app;
      Plugin{}.Build(app);
      window::Plugin{}.Build(app);
      input::Plugin{}.Build(app);
      Plugin{}.Finish(app);

      CHECK(app.GetWorld().HasResource<input::Settings>());
      CHECK(app.GetWorld().ReadResource<Context>().input_enabled);
      CHECK(app.GetWorld().HasResource<GamepadCache>());
      CHECK(app.GetWorld().HasResource<CursorCache>());
    }
#endif
  }

  TEST_CASE("helios::glfw::Plugin::Destroy") {
    SUBCASE("Clears the frame pump when GLFW was never initialized") {
      app::App app;
      window::Plugin{}.Build(app);
      Plugin glfw_plugin;
      glfw_plugin.Build(app);
      glfw_plugin.Finish(app);
      glfw_plugin.Destroy(app);

      CHECK_EQ(app.GetWorld().ReadResource<Context>().frame_pump, nullptr);
      CHECK_FALSE(app.GetWorld().ReadResource<Context>().initialized);
    }
  }
}

TEST_SUITE("helios::glfw::WindowPlugin") {
  TEST_CASE("helios::glfw::WindowPlugin") {
    SUBCASE("Adds glfw and window plugins") {
      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      CHECK(app.HasPlugins<Plugin>());
      CHECK(app.HasPlugins<window::Plugin>());
    }
  }
}

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
TEST_SUITE("helios::glfw::WindowInputPlugin") {
  TEST_CASE("helios::glfw::WindowInputPlugin") {
    SUBCASE("Adds glfw, window, and input plugins") {
      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      CHECK(app.HasPlugins<Plugin>());
      CHECK(app.HasPlugins<window::Plugin>());
      CHECK(app.HasPlugins<input::Plugin>());
    }
  }
}
#endif

TEST_SUITE("helios::glfw::Plugin") {
  TEST_CASE("helios::glfw::Plugin") {
    SUBCASE("FramePumpOrder advances message lifecycle once") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      window::Plugin{}.Build(app);
      Plugin{}.Build(app);
      Plugin{}.Finish(app);

      app.AddMessages<PumpProbeMsg>();
      app.AddSystem(app::kPreUpdate, WritePumpProbe{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      app.RunFrameOrder(app.GetWorld().ReadResource<app::FramePumpOrder>());

      auto& world = app.GetWorld();
      CHECK(world.Messages().CurrentMessages<PumpProbeMsg>().empty());
      CHECK_EQ(world.Messages().PreviousMessages<PumpProbeMsg>().size(), 1);
      CHECK_EQ(world.Messages().PreviousMessages<PumpProbeMsg>()[0].value, 11);
    }

    SUBCASE("Events schedule runs without the input plugin") {
      app::App app;
      window::Plugin{}.Build(app);
      Plugin{}.Build(app);
      Plugin{}.Finish(app);

      auto* events =
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents);
      REQUIRE_NE(events, nullptr);
      CHECK(events->Build().has_value());

      ecs::MainThreadExecutor executor;
      events->RunAndWait(app.GetWorld(), executor);
    }

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
    SUBCASE("Events schedule runs with input systems enabled") {
      app::App app;
      Plugin{}.Build(app);
      window::Plugin{}.Build(app);
      input::Plugin{}.Build(app);
      Plugin{}.Finish(app);

      auto* events =
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents);
      REQUIRE_NE(events, nullptr);
      CHECK(events->Build().has_value());

      ecs::MainThreadExecutor executor;
      events->RunAndWait(app.GetWorld(), executor);
    }
#endif
  }
}
#endif
