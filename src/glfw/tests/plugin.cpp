#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/glfw.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/glfw/systems/input.hpp>
#include <helios/input/input.hpp>
#endif

#include <string_view>

namespace {

struct PumpProbeMsg {
  static constexpr std::string_view kName = "PumpProbeMsg";

  int value = 0;
};

struct WritePumpProbe {
  void operator()(helios::ecs::MessageWriter<PumpProbeMsg> writer) const {
    writer.Write({.value = 11});
  }
};

}  // namespace

TEST_SUITE("helios::glfw::Plugin") {
  TEST_CASE("helios::glfw::Plugin::Build") {
    SUBCASE("Registers backend resources and the events schedule") {
      helios::app::App app;
      helios::window::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Build(app);

      CHECK(app.GetWorld().HasResource<helios::glfw::Context>());
      CHECK(app.GetWorld().HasResource<helios::glfw::NativeWindows>());
      CHECK_NE(app.GetMainSubApp().GetScheduler().TryGetSchedule(
                   helios::window::kEvents),
               nullptr);
      CHECK(app.GetWorld().ReadResource<helios::app::MainFrameOrder>().Contains(
          helios::window::kWindowStage));
    }
  }

  TEST_CASE("helios::glfw::Plugin::Finish") {
    SUBCASE("Leaves input disabled without the input plugin") {
      helios::app::App app;
      helios::window::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Finish(app);

      CHECK_FALSE(
          app.GetWorld().ReadResource<helios::glfw::Context>().input_enabled);
#ifdef HELIOS_MODULE_INPUT_AVAILABLE
      CHECK_FALSE(app.GetWorld().HasResource<helios::input::Settings>());
#endif
      CHECK_NE(app.GetWorld().ReadResource<helios::glfw::Context>().frame_pump,
               nullptr);
    }

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
    SUBCASE("Enables input systems after the input plugin builds") {
      helios::app::App app;
      helios::glfw::Plugin{}.Build(app);
      helios::window::Plugin{}.Build(app);
      helios::input::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Finish(app);

      CHECK(app.GetWorld().HasResource<helios::input::Settings>());
      CHECK(app.GetWorld().ReadResource<helios::glfw::Context>().input_enabled);
      CHECK(app.GetWorld().HasResource<helios::glfw::GamepadCache>());
      CHECK(app.GetWorld().HasResource<helios::glfw::CursorCache>());
    }
#endif
  }

  TEST_CASE("helios::glfw::Plugin::Destroy") {
    SUBCASE("Clears the frame pump when GLFW was never initialized") {
      helios::app::App app;
      helios::window::Plugin{}.Build(app);
      helios::glfw::Plugin glfw_plugin;
      glfw_plugin.Build(app);
      glfw_plugin.Finish(app);
      glfw_plugin.Destroy(app);

      CHECK_EQ(app.GetWorld().ReadResource<helios::glfw::Context>().frame_pump,
               nullptr);
      CHECK_FALSE(
          app.GetWorld().ReadResource<helios::glfw::Context>().initialized);
    }
  }
}

TEST_SUITE("helios::glfw::WindowPlugin") {
  TEST_CASE("helios::glfw::WindowPlugin") {
    SUBCASE("Adds glfw and window plugins") {
      helios::app::App app;
      app.AddPluginGroups(helios::glfw::WindowPlugin{});
      CHECK(app.HasPlugins<helios::glfw::Plugin>());
      CHECK(app.HasPlugins<helios::window::Plugin>());
    }
  }
}

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
TEST_SUITE("helios::glfw::WindowInputPlugin") {
  TEST_CASE("helios::glfw::WindowInputPlugin") {
    SUBCASE("Adds glfw, window, and input plugins") {
      helios::app::App app;
      app.AddPluginGroups(helios::glfw::WindowInputPlugin{});
      CHECK(app.HasPlugins<helios::glfw::Plugin>());
      CHECK(app.HasPlugins<helios::window::Plugin>());
      CHECK(app.HasPlugins<helios::input::Plugin>());
    }
  }
}
#endif

TEST_SUITE("helios::glfw::Plugin") {
  TEST_CASE("helios::glfw::Plugin") {
    SUBCASE("FramePumpOrder advances message lifecycle once") {
      HELIOS_SKIP_IF_NO_GLFW();

      helios::app::App app;
      helios::window::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Finish(app);

      app.AddMessages<PumpProbeMsg>();
      app.AddSystem(helios::app::kPreUpdate, WritePumpProbe{});
      app.Initialize();
      helios::glfw::test::ScopedGlfwShutdown shutdown{app};

      app.RunFrameOrder(
          app.GetWorld().ReadResource<helios::app::FramePumpOrder>());

      auto& world = app.GetWorld();
      CHECK(world.Messages().CurrentMessages<PumpProbeMsg>().empty());
      CHECK_EQ(world.Messages().PreviousMessages<PumpProbeMsg>().size(), 1);
      CHECK_EQ(world.Messages().PreviousMessages<PumpProbeMsg>()[0].value, 11);
    }

    SUBCASE("Events schedule runs without the input plugin") {
      helios::app::App app;
      helios::window::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Finish(app);

      auto* events = app.GetMainSubApp().GetScheduler().TryGetSchedule(
          helios::window::kEvents);
      REQUIRE_NE(events, nullptr);
      CHECK(events->Build().has_value());

      helios::ecs::MainThreadExecutor executor;
      events->RunAndWait(app.GetWorld(), executor);
    }

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
    SUBCASE("Events schedule runs with input systems enabled") {
      helios::app::App app;
      helios::glfw::Plugin{}.Build(app);
      helios::window::Plugin{}.Build(app);
      helios::input::Plugin{}.Build(app);
      helios::glfw::Plugin{}.Finish(app);

      auto* events = app.GetMainSubApp().GetScheduler().TryGetSchedule(
          helios::window::kEvents);
      REQUIRE_NE(events, nullptr);
      CHECK(events->Build().has_value());

      helios::ecs::MainThreadExecutor executor;
      events->RunAndWait(app.GetWorld(), executor);
    }
#endif
  }
}
