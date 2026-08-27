#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/params.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/schedules.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <string_view>

using namespace helios;
using namespace helios::sdl3;

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

TEST_SUITE("helios::sdl3::Plugin") {
  TEST_CASE("helios::sdl3::Plugin::Build") {
    SUBCASE("Inserts runtime resources and the events schedule") {
      app::App app;
      Plugin{}.Build(app);

      CHECK(app.GetWorld().HasResource<Context>());
      CHECK(app.GetWorld().HasResource<EventDispatcher>());
      CHECK_NE(
          app.GetMainSubApp().GetScheduler().TryGetSchedule(window::kEvents),
          nullptr);
      CHECK(app.GetWorld().ReadResource<app::MainFrameOrder>().Contains(
          window::kWindowStage));
    }

    SUBCASE("Events schedule builds and runs without a window backend") {
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
  }

  TEST_CASE("helios::sdl3::Plugin::Finish") {
    SUBCASE("Sets the SDL frame pump callback") {
      app::App app;
      Plugin plugin;
      plugin.Build(app);
      plugin.Finish(app);

      CHECK_NE(app.GetWorld().ReadResource<Context>().frame_pump, nullptr);
      CHECK_EQ(app.GetWorld().ReadResource<Context>().frame_pump_user_data,
               &app);
    }

    SUBCASE("Frame pump advances the message lifecycle once") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      window::Plugin{}.Build(app);
      Plugin plugin;
      plugin.Build(app);
      plugin.Finish(app);

      app.AddMessages<PumpProbeMsg>();
      app.AddSystem(app::kPreUpdate, WritePumpProbe{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      test::ScopedRetain retain{SDL_INIT_VIDEO};

      app.RunFrameOrder(app.GetWorld().ReadResource<app::FramePumpOrder>());

      auto& world = app.GetWorld();
      CHECK(world.Messages().CurrentMessages<PumpProbeMsg>().empty());
      CHECK_EQ(world.Messages().PreviousMessages<PumpProbeMsg>().size(), 1);
      CHECK_EQ(world.Messages().PreviousMessages<PumpProbeMsg>()[0].value, 11);
    }
  }

  TEST_CASE("helios::sdl3::Plugin::Destroy") {
    SUBCASE("Clears the frame pump when SDL was never initialized") {
      app::App app;
      Plugin plugin;
      plugin.Build(app);
      plugin.Finish(app);
      plugin.Destroy(app);

      CHECK_EQ(app.GetWorld().ReadResource<Context>().frame_pump, nullptr);
      CHECK_EQ(app.GetWorld().ReadResource<Context>().world, nullptr);
    }
  }
}
#endif
