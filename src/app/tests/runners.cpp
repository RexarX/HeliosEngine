#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/runners.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>

#include <atomic>
#include <chrono>
#include <thread>

using namespace helios;
using namespace helios::app;
using namespace helios::ecs;

namespace {

struct CounterResource {
  int value = 0;
};

struct IncrementSystem {
  void operator()(Res<CounterResource> counter) const { ++counter->value; }
};

struct ExitSystem {
  ExitCode code = ExitCode::kSuccess;

  void operator()(MessageWriter<AppExit> exits) const {
    exits.Write(AppExit::From(code));
  }
};

struct RenderSubAppLabel {};

struct AsyncSoundSubAppLabel {
  static constexpr bool kAsync = true;
};

struct UpdateCountResource {
  int value = 0;
};

struct CountingUpdateSystem {
  void operator()(Res<UpdateCountResource> counter) const { ++counter->value; }
};

}  // namespace

TEST_SUITE("helios::app::FixedRunnerConfig") {
  TEST_CASE("app::FixedRunnerConfig::FromFPS") {
    SUBCASE("60 FPS maps to ~16.67 ms interval") {
      constexpr auto config = FixedRunnerConfig::FromFPS(60);
      CHECK_EQ(config.update_interval.count(), 16'666'666);
    }
  }

  TEST_CASE("app::FixedRunnerConfig::FromHz") {
    SUBCASE("Positive Hz yields positive nanosecond interval") {
      constexpr auto config = FixedRunnerConfig::FromHz(30.0);
      CHECK_GT(config.update_interval.count(), 0);
    }
  }

  TEST_CASE("app::FixedRunnerConfig::FromInterval") {
    SUBCASE("Milliseconds convert to nanoseconds") {
      constexpr auto config =
          FixedRunnerConfig::FromInterval(std::chrono::milliseconds{20});
      CHECK_EQ(config.update_interval.count(), 20'000'000);
    }
  }
}

TEST_SUITE("helios::app::RunDefault") {
  TEST_CASE("app::RunDefault") {
    SUBCASE("Loops until AppExit and returns exit code") {
      App app;
      app.AddSystem(kFirst, ExitSystem{});
      app.Initialize();

      const ExitCode result = RunDefault(app);
      CHECK_EQ(result, ExitCode::kSuccess);
    }

    SUBCASE("Runs multiple updates before exit") {
      App app;
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.AddSystem(kFirst, ExitSystem{});
      app.Initialize();

      RunDefault(app);
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }
  }
}

TEST_SUITE("helios::app::RunFixed") {
  TEST_CASE("app::RunFixed") {
    SUBCASE("Loops with fixed timestep until AppExit") {
      App app;
      app.AddSystem(kFirst, ExitSystem{});
      app.Initialize();

      const ExitCode result = RunFixed(
          app, FixedRunnerConfig::FromInterval(std::chrono::milliseconds{1}));
      CHECK_EQ(result, ExitCode::kSuccess);
    }
  }
}

TEST_SUITE("helios::app::RunOnce") {
  TEST_CASE("app::RunOnce") {
    SUBCASE("Returns success without AppExit after single frame") {
      App app;
      app.Initialize();

      const ExitCode result = RunOnce(app);
      CHECK_EQ(result, ExitCode::kSuccess);
      CHECK_FALSE(app.ShouldExit().has_value());
    }

    SUBCASE("Returns exit code when AppExit is written during frame") {
      App app;
      app.AddSystem(kFirst, ExitSystem{ExitCode::kFailure});
      app.Initialize();

      const ExitCode result = RunOnce(app);
      CHECK_EQ(result, ExitCode::kFailure);
    }
  }
}

TEST_SUITE("helios::app::RunDefaultSubApp") {
  TEST_CASE("app::RunDefaultSubApp") {
    SUBCASE("Runs update loop until owner app requests exit") {
      App app(2);

      struct CountAndExitMain {
        App* owner = nullptr;

        void operator()(Res<UpdateCountResource> counter) const {
          ++counter->value;
          if (counter->value >= 3 && owner != nullptr) {
            owner->GetWorld().WriteMessages<AppExit>().Write(
                AppExit::Success());
          }
        }
      };

      SubApp sub_app;
      sub_app.SetAsync(true);
      sub_app.SetRunner(RunDefaultSubApp);
      sub_app.InsertResources(UpdateCountResource{});
      sub_app.AddSystem(kUpdate, CountAndExitMain{.owner = &app});

      app.InsertSubApp(RenderSubAppLabel{}, std::move(sub_app));
      app.Initialize();

      std::this_thread::sleep_for(std::chrono::milliseconds{100});
      app.GetScheduler().Shutdown(app);

      CHECK_GE(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<UpdateCountResource>()
                   .value,
               3);
    }
  }
}

TEST_SUITE("helios::app::RunFixedSubApp") {
  TEST_CASE("app::RunFixedSubApp") {
    SUBCASE("Background fixed loop increments update counter") {
      App app(4);

      SubApp sub_app;
      sub_app.SetAsync(true);
      sub_app.SetRunner([](SubApp& sa, async::Executor& ex) {
        RunFixedSubApp(
            sa, ex,
            FixedRunnerConfig::FromInterval(std::chrono::milliseconds{1}));
      });
      sub_app.InsertResources(UpdateCountResource{});
      sub_app.AddSystem(kUpdate, CountingUpdateSystem{});

      app.InsertSubApp(AsyncSoundSubAppLabel{}, std::move(sub_app));
      app.Initialize();

      std::this_thread::sleep_for(std::chrono::milliseconds{50});
      app.GetScheduler().Shutdown(app);

      CHECK_GE(app.GetSubApp<AsyncSoundSubAppLabel>()
                   .GetWorld()
                   .ReadResource<UpdateCountResource>()
                   .value,
               1);
    }
  }
}

TEST_SUITE("helios::app::RunOnceSubApp") {
  TEST_CASE("app::RunOnceSubApp") {
    SUBCASE("Single update pass increments counter when invoked via runner") {
      App app(2);
      SubApp sub_app;
      sub_app.InsertResources(UpdateCountResource{});
      sub_app.AddSystem(kUpdate, CountingUpdateSystem{});
      sub_app.SetRunner(
          [](SubApp& sa, async::Executor& ex) { RunOnceSubApp(sa, ex); });
      app.InsertSubApp(RenderSubAppLabel{}, std::move(sub_app));
      app.Initialize();
      app.Update();
      app.GetScheduler().WaitForSubApps();

      CHECK_EQ(app.GetSubApp<RenderSubAppLabel>()
                   .GetWorld()
                   .ReadResource<UpdateCountResource>()
                   .value,
               1);
    }
  }
}
