#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/app/builtin/frame_limiter.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/world.hpp>

#include <chrono>

using namespace helios;
using namespace helios::app;

namespace {

struct RenderSubAppLabel {};

}  // namespace

TEST_SUITE("helios::app::FrameLimiterSettings") {
  TEST_CASE("helios::app::FrameLimiterSettings::Off") {
    SUBCASE("Selects off mode") {
      constexpr auto settings = FrameLimiterSettings::Off();
      CHECK_EQ(settings.mode, FrameLimiterMode::kOff);
    }
  }

  TEST_CASE("helios::app::FrameLimiterSettings::FromFPS") {
    SUBCASE("60 FPS maps to ~16.67 ms interval") {
      constexpr auto settings = FrameLimiterSettings::FromFPS(60);
      CHECK_EQ(settings.mode, FrameLimiterMode::kManual);
      CHECK_EQ(settings.interval.count(), 16'666'666);
    }
  }

  TEST_CASE("helios::app::FrameLimiterSettings::FromHz") {
    SUBCASE("Positive Hz yields positive nanosecond interval") {
      constexpr auto settings = FrameLimiterSettings::FromHz(30.0);
      CHECK_EQ(settings.mode, FrameLimiterMode::kManual);
      CHECK_GT(settings.interval.count(), 0);
    }
  }

  TEST_CASE("helios::app::FrameLimiterSettings::FromInterval") {
    SUBCASE("Milliseconds convert to nanoseconds") {
      constexpr auto settings =
          FrameLimiterSettings::FromInterval(std::chrono::milliseconds{20});
      CHECK_EQ(settings.mode, FrameLimiterMode::kManual);
      CHECK_EQ(settings.interval.count(), 20'000'000);
    }
  }

  TEST_CASE("helios::app::FrameLimiterSettings::Auto") {
    SUBCASE("Selects auto mode") {
      constexpr auto settings = FrameLimiterSettings::Auto();
      CHECK_EQ(settings.mode, FrameLimiterMode::kAuto);
    }
  }
}

TEST_SUITE("helios::app::FrameLimiter") {
  TEST_CASE("helios::app::FrameLimiter::ctor") {
    SUBCASE("Default construction uses manual 60 FPS settings") {
      const FrameLimiter limiter;
      CHECK_EQ(limiter.Mode(), FrameLimiterMode::kManual);
      CHECK_EQ(limiter.Interval(), FrameLimiterSettings::kDefaultInterval);
      CHECK_EQ(limiter.RefreshRate(), 0U);
    }

    SUBCASE("Copy construction preserves atomic state") {
      FrameLimiter limiter{FrameLimiterSettings::FromFPS(30)};
      limiter.SetRefreshRate(120);
      const FrameLimiter copy{limiter};
      CHECK_EQ(copy.Mode(), FrameLimiterMode::kManual);
      CHECK_EQ(copy.Interval(), limiter.Interval());
      CHECK_EQ(copy.RefreshRate(), 120U);
    }
  }

  TEST_CASE("helios::app::FrameLimiter::operator=") {
    SUBCASE("Copy assignment updates mode and interval") {
      FrameLimiter limiter;
      limiter = FrameLimiter{FrameLimiterSettings::Off()};
      CHECK_EQ(limiter.Mode(), FrameLimiterMode::kOff);
    }
  }

  TEST_CASE("helios::app::FrameLimiter::Wait") {
    SUBCASE("First call does not sleep") {
      FrameLimiter limiter{
          FrameLimiterSettings::FromInterval(std::chrono::milliseconds{50})};
      const auto start = std::chrono::steady_clock::now();
      limiter.Wait();
      CHECK_LT(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{10});
    }

    SUBCASE("Off mode does not sleep and resets the deadline") {
      FrameLimiter limiter{
          FrameLimiterSettings::FromInterval(std::chrono::milliseconds{50})};
      limiter.Wait();
      limiter.SetMode(FrameLimiterMode::kOff);
      const auto start = std::chrono::steady_clock::now();
      limiter.Wait();
      CHECK_LT(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{10});
      limiter.SetMode(FrameLimiterMode::kManual);
      limiter.Wait();
      CHECK_LT(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{10});
    }

    SUBCASE("Manual mode sleeps until the configured interval") {
      FrameLimiter limiter{
          FrameLimiterSettings::FromInterval(std::chrono::milliseconds{15})};
      limiter.Wait();
      const auto start = std::chrono::steady_clock::now();
      limiter.Wait();
      CHECK_GE(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{10});
    }

    SUBCASE("Auto with zero refresh rate does not sleep") {
      FrameLimiter limiter{FrameLimiterSettings::Auto()};
      const auto start = std::chrono::steady_clock::now();
      limiter.Wait();
      limiter.Wait();
      CHECK_LT(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{10});
    }

    SUBCASE("Auto with a refresh rate paces subsequent waits") {
      FrameLimiter limiter{FrameLimiterSettings::Auto()};
      limiter.SetRefreshRate(50);
      limiter.Wait();
      const auto start = std::chrono::steady_clock::now();
      limiter.Wait();
      CHECK_GE(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{10});
    }
  }

  TEST_CASE("helios::app::FrameLimiter::SetMode") {
    SUBCASE("Stores the new mode") {
      FrameLimiter limiter;
      limiter.SetMode(FrameLimiterMode::kAuto);
      CHECK_EQ(limiter.Mode(), FrameLimiterMode::kAuto);
    }
  }

  TEST_CASE("helios::app::FrameLimiter::SetInterval") {
    SUBCASE("Stores the new interval") {
      FrameLimiter limiter;
      limiter.SetInterval(std::chrono::milliseconds{8});
      CHECK_EQ(limiter.Interval(), std::chrono::milliseconds{8});
    }
  }

  TEST_CASE("helios::app::FrameLimiter::SetRefreshRate") {
    SUBCASE("Stores the new refresh rate") {
      FrameLimiter limiter;
      limiter.SetRefreshRate(144);
      CHECK_EQ(limiter.RefreshRate(), 144U);
    }
  }

  TEST_CASE("helios::app::FrameLimiter::Mode") {
    SUBCASE("Returns the configured mode") {
      const FrameLimiter limiter{FrameLimiterSettings::Off()};
      CHECK_EQ(limiter.Mode(), FrameLimiterMode::kOff);
    }
  }

  TEST_CASE("helios::app::FrameLimiter::Interval") {
    SUBCASE("Returns the configured interval") {
      const FrameLimiter limiter{FrameLimiterSettings::FromFPS(30)};
      CHECK_EQ(limiter.Interval().count(), 33'333'333);
    }
  }

  TEST_CASE("helios::app::FrameLimiter::RefreshRate") {
    SUBCASE("Defaults to zero") {
      const FrameLimiter limiter;
      CHECK_EQ(limiter.RefreshRate(), 0U);
    }
  }
}

TEST_SUITE("helios::app::LimitFrameRate") {
  TEST_CASE("helios::app::LimitFrameRate::operator()") {
    SUBCASE("Invokes Wait on the limiter resource") {
      App app;
      app.InsertResources(FrameLimiter{
          FrameLimiterSettings::FromInterval(std::chrono::milliseconds{12})});
      app.AddSystem(kFirst, LimitFrameRate{});
      app.Initialize();

      app.Update();
      const auto start = std::chrono::steady_clock::now();
      app.Update();
      CHECK_GE(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{8});
    }
  }
}

TEST_SUITE("helios::app::InstallFrameLimiter") {
  TEST_CASE("helios::app::InstallFrameLimiter") {
    SUBCASE("Installs resource and prepends FramePace to MainFrameOrder") {
      App app;
      InstallFrameLimiter(app, FrameLimiterSettings::FromFPS(60));
      app.Initialize();

      CHECK(app.GetWorld().HasResource<FrameLimiter>());
      CHECK_NE(app.GetMainSubApp().GetScheduler().TryGetSchedule(kFramePace),
               nullptr);
      CHECK_EQ(app.GetMainSubApp()
                   .GetScheduler()
                   .TryGetSchedule(kFramePace)
                   ->Settings()
                   .executor_kind,
               ecs::ExecutorKind::kMainThread);

      const auto& order = app.GetWorld().ReadResource<MainFrameOrder>();
      CHECK(order.Contains(kFramePaceStage));
      CHECK_EQ(order.Labels().front().Hash(),
               ecs::StageTypeIndex::From(kFramePaceStage).Hash());
      CHECK_FALSE(app.GetWorld().ReadResource<FramePumpOrder>().Contains(
          kFramePaceStage));
    }

    SUBCASE("Nested FramePumpOrder does not run the limiter") {
      App app;
      InstallFrameLimiter(
          app, FrameLimiterSettings::FromInterval(std::chrono::seconds{2}));
      app.Initialize();

      app.Update();
      const auto start = std::chrono::steady_clock::now();
      app.RunFrameOrder(app.GetWorld().ReadResource<FramePumpOrder>());
      CHECK_LT(std::chrono::steady_clock::now() - start,
               std::chrono::milliseconds{100});
    }

    SUBCASE("Installs on a blocking sub-app before kFirst") {
      App app;
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      InstallFrameLimiter(app.GetSubApp(RenderSubAppLabel{}),
                          FrameLimiterSettings::FromFPS(120));

      CHECK(app.GetSubApp(RenderSubAppLabel{})
                .GetWorld()
                .HasResource<FrameLimiter>());
      CHECK_NE(app.GetSubApp(RenderSubAppLabel{})
                   .GetScheduler()
                   .TryGetSchedule(kFramePace),
               nullptr);
    }
  }
}

TEST_SUITE("helios::app::FrameLimiterPlugin") {
  TEST_CASE("helios::app::FrameLimiterPlugin::Build") {
    SUBCASE("Adds FrameLimiter on initialize") {
      App app;
      app.AddPlugins(FrameLimiterPlugin{FrameLimiterSettings::FromFPS(60)});
      app.Initialize();

      CHECK(app.GetWorld().HasResource<FrameLimiter>());
      CHECK_EQ(app.GetWorld().ReadResource<FrameLimiter>().Mode(),
               FrameLimiterMode::kManual);
    }
  }
}
