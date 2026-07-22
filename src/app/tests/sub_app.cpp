#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/runners.hpp>
#include <helios/app/schedules.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>

#include <atomic>
#include <chrono>
#include <string_view>
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

struct ExtractCounterResource {
  int value = 0;
};

struct CustomSchedule {
  static constexpr std::string_view kName = "CustomSchedule";
};
constexpr CustomSchedule kCustomSchedule{};

struct NamedSubAppLabel {
  static constexpr std::string_view kName = "RenderSubApp";
};
constexpr NamedSubAppLabel kNamedSubApp{};

struct UnnamedSubAppLabel {};
constexpr UnnamedSubAppLabel kUnnamedSubApp{};

struct OverlappingSubAppLabel {
  static constexpr bool kAllowOverlappingUpdates = true;
};
constexpr OverlappingSubAppLabel kOverlappingSubApp{};

struct LimitedSkipSubAppLabel {
  static constexpr bool kAllowOverlappingUpdates = true;
  static constexpr size_t kMaxOverlappingUpdates = 2;
};
constexpr LimitedSkipSubAppLabel kLimitedSkipSubApp{};

struct AsyncSubAppLabel {
  static constexpr bool kAsync = true;
};
constexpr AsyncSubAppLabel kAsyncSubApp{};

struct SetOne {};
constexpr SetOne kSetOne{};

}  // namespace

TEST_SUITE("helios::app::SubAppNameOf") {
  TEST_CASE("app::SubAppNameOf") {
    SUBCASE("Returns kName when SubAppWithNameTrait is satisfied") {
      CHECK_EQ(SubAppNameOf(kNamedSubApp), "RenderSubApp");
    }

    SUBCASE("Falls back to type name for unnamed labels") {
      const std::string_view inferred = SubAppNameOf(kUnnamedSubApp);
      CHECK_FALSE(inferred.empty());
    }
  }

  TEST_CASE("app::IsSubAppAllowsOverlappingUpdates") {
    SUBCASE("True when kAllowOverlappingUpdates is set") {
      CHECK(IsSubAppAllowsOverlappingUpdates(kOverlappingSubApp));
    }

    SUBCASE("False for default labels") {
      CHECK_FALSE(IsSubAppAllowsOverlappingUpdates(kNamedSubApp));
    }
  }

  TEST_CASE("app::SubAppMaxOverlappingUpdates") {
    SUBCASE("Returns kMaxOverlappingUpdates when specified on label") {
      CHECK_EQ(SubAppMaxOverlappingUpdates(kLimitedSkipSubApp), 2);
    }

    SUBCASE("Returns zero for overlapping labels without explicit budget") {
      CHECK_EQ(SubAppMaxOverlappingUpdates(kOverlappingSubApp), 0);
    }
  }

  TEST_CASE("app::IsSubAppAsync") {
    SUBCASE("True when kAsync is set on label") {
      CHECK(IsSubAppAsync(kAsyncSubApp));
    }

    SUBCASE("False for default labels") {
      CHECK_FALSE(IsSubAppAsync(kNamedSubApp));
    }
  }
}

TEST_SUITE("helios::app::SubApp::From") {
  TEST_CASE("app::SubApp::From") {
    SUBCASE("From uses kName when SubAppWithNameTrait is satisfied") {
      SubApp sub_app = SubApp::From(kNamedSubApp);
      CHECK_EQ(sub_app.GetName(), "RenderSubApp");
    }

    SUBCASE("From falls back to type name for unnamed labels") {
      SubApp sub_app = SubApp::From(kUnnamedSubApp);
      CHECK_FALSE(sub_app.GetName().empty());
    }
  }
}

TEST_SUITE("helios::app::SubApp") {
  TEST_CASE("app::SubApp::ctor") {
    SUBCASE(
        "Default-constructed sub-app has empty name and built-in schedules") {
      SubApp sub_app;
      CHECK(sub_app.GetName().empty());
      CHECK_NE(sub_app.TryGetSchedule(kUpdate), nullptr);
      CHECK_FALSE(sub_app.IsUpdating());
      CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
      CHECK_FALSE(sub_app.IsAsync());
      CHECK_EQ(sub_app.MaxExtractionSkips(), 0);
    }

    SUBCASE("Named sub-app stores name and built-in schedules") {
      SubApp sub_app("RenderSubApp");
      CHECK_EQ(sub_app.GetName(), "RenderSubApp");
      CHECK_NE(sub_app.TryGetSchedule(kUpdate), nullptr);
      CHECK_FALSE(sub_app.IsUpdating());
      CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
      CHECK_FALSE(sub_app.IsAsync());
      CHECK_EQ(sub_app.MaxExtractionSkips(), 0);
    }

    SUBCASE("Move construction transfers world resources") {
      SubApp source("RenderSubApp");
      source.InsertResources(CounterResource{7});
      SubApp moved{std::move(source)};
      CHECK_EQ(moved.GetName(), "RenderSubApp");
      CHECK_EQ(moved.GetWorld().ReadResource<CounterResource>().value, 7);
    }
  }

  TEST_CASE("app::SubApp::operator=") {
    SUBCASE("Move assignment transfers world resources") {
      SubApp source("RenderSubApp");
      source.InsertResources(CounterResource{3});
      SubApp target;
      target = std::move(source);
      CHECK_EQ(target.GetWorld().ReadResource<CounterResource>().value, 3);
    }
  }

  TEST_CASE("app::SubApp::Clear") {
    SUBCASE("Clear removes resources and restores built-in schedules") {
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{5});
      sub_app.Clear();
      CHECK_FALSE(sub_app.GetWorld().HasResource<CounterResource>());
      CHECK_NE(sub_app.TryGetSchedule(kUpdate), nullptr);
    }
  }

  TEST_CASE("app::SubApp::Extract") {
    SUBCASE("Extract invokes configured function and copies main-world data") {
      App app;
      app.InsertResources(CounterResource{42});

      SubApp sub_app("RenderSubApp");
      sub_app.TryInsertResources(ExtractCounterResource{0});
      sub_app.SetExtractFunction([](const World& main, World& sub) {
        sub.WriteResource<ExtractCounterResource>().value =
            main.ReadResource<CounterResource>().value;
      });

      sub_app.Extract(app.GetWorld());
      CHECK_EQ(sub_app.GetWorld().ReadResource<ExtractCounterResource>().value,
               42);
    }

    SUBCASE("Extract with no function is a no-op") {
      SubApp sub_app("RenderSubApp");
      sub_app.TryInsertResources(ExtractCounterResource{5});
      App app;
      sub_app.Extract(app.GetWorld());
      CHECK_EQ(sub_app.GetWorld().ReadResource<ExtractCounterResource>().value,
               5);
    }
  }

  TEST_CASE("app::SubApp::WaitUntilFullyIdle") {
    SUBCASE("Returns immediately when not updating") {
      SubApp sub_app("RenderSubApp");
      sub_app.WaitUntilFullyIdle();
      CHECK_FALSE(sub_app.IsUpdating());
    }
  }

  TEST_CASE("app::SubApp::BuildScheduler") {
    SUBCASE("BuildScheduler clears dirty flag after build") {
      SubApp sub_app("RenderSubApp");
      sub_app.AddSystem(kUpdate, IncrementSystem{});
      async::Executor executor{2};
      sub_app.BuildScheduler(executor);
      CHECK_FALSE(sub_app.GetScheduler().IsDirty());
    }
  }

  TEST_CASE("app::SubApp::AddSchedule") {
    SUBCASE("AddSchedule registers custom schedule") {
      SubApp sub_app("RenderSubApp");
      ecs::Schedule custom;
      [[maybe_unused]] const auto ordering =
          sub_app.AddSchedule(kCustomSchedule, std::move(custom));
      CHECK_NE(sub_app.TryGetSchedule(kCustomSchedule), nullptr);
    }
  }

  TEST_CASE("app::SubApp::InitSchedule") {
    SUBCASE("InitSchedule creates schedule when missing") {
      SubApp sub_app("RenderSubApp");
      sub_app.InitSchedule(kCustomSchedule);
      CHECK_NE(sub_app.TryGetSchedule(kCustomSchedule), nullptr);
    }

    SUBCASE("InitSchedule names the schedule from the label") {
      SubApp sub_app("RenderSubApp");
      sub_app.InitSchedule(kCustomSchedule);
      CHECK_EQ(sub_app.TryGetSchedule(kCustomSchedule)->GetName(),
               "CustomSchedule");
    }
  }

  TEST_CASE("app::SubApp::EditSchedule") {
    SUBCASE("EditSchedule adds systems and marks scheduler dirty") {
      SubApp sub_app("RenderSubApp");
      sub_app.InitSchedule(kCustomSchedule);
      sub_app.EditSchedule(kCustomSchedule, [](Schedule& schedule) {
        schedule.Add(IncrementSystem{});
      });
      CHECK(sub_app.GetScheduler().IsDirty());
    }
  }

  TEST_CASE("app::SubApp::AddSystem") {
    SUBCASE("AddSystem marks scheduler dirty") {
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystem(kUpdate, IncrementSystem{});
      CHECK(sub_app.GetScheduler().IsDirty());
    }

    SUBCASE("AddSystem runs when sub-app is updated through app frame") {
      App app(2);
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystem(kUpdate, IncrementSystem{});
      app.InsertSubApp(kNamedSubApp, std::move(sub_app));
      app.Initialize();
      app.Update();

      CHECK_EQ(app.GetSubApp(kNamedSubApp)
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               1);
    }
  }

  TEST_CASE("app::SubApp::AddSystems") {
    SUBCASE("AddSystems marks scheduler dirty") {
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystems(kUpdate, IncrementSystem{}, IncrementSystem{});
      CHECK(sub_app.GetScheduler().IsDirty());
    }

    SUBCASE("AddSystems with InSet runs both systems") {
      App app(2);
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystems(kUpdate, IncrementSystem{}, IncrementSystem{})
          .InSet(kSetOne);
      app.InsertSubApp(kNamedSubApp, std::move(sub_app));
      app.Initialize();
      app.Update();

      CHECK_EQ(app.GetSubApp(kNamedSubApp)
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               2);
    }
  }

  TEST_CASE("app::SubApp::ConfigureSet") {
    SUBCASE("ConfigureSet marks scheduler dirty") {
      SubApp sub_app("RenderSubApp");
      [[maybe_unused]] const auto set = sub_app.ConfigureSet(kUpdate, kSetOne);
      CHECK(sub_app.GetScheduler().IsDirty());
    }
  }

  TEST_CASE("app::SubApp::InsertResources") {
    SUBCASE("InsertResources stores value in sub-app world") {
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{9});
      CHECK_EQ(sub_app.GetWorld().ReadResource<CounterResource>().value, 9);
    }
  }

  TEST_CASE("app::SubApp::TryInsertResources") {
    SUBCASE("TryInsertResources keeps first value when resource exists") {
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{1});
      sub_app.TryInsertResources(CounterResource{99});
      CHECK_EQ(sub_app.GetWorld().ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("app::SubApp::AddMessages") {
    SUBCASE("AddMessages registers message on sub-app world") {
      SubApp sub_app("RenderSubApp");
      sub_app.AddMessages<AppExit>();
      CHECK(sub_app.GetWorld().HasMessage<AppExit>());
    }
  }

  TEST_CASE("app::SubApp::SetRunner") {
    SUBCASE("Custom runner is invoked during app frame update") {
      App app(2);
      std::atomic<int> runner_calls{0};

      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystem(kUpdate, IncrementSystem{});
      sub_app.SetRunner([&runner_calls](SubApp& sa, async::Executor& ex) {
        ++runner_calls;
        RunOnceSubApp(sa, ex);
      });

      app.InsertSubApp(kNamedSubApp, std::move(sub_app));
      app.Initialize();
      app.Update();

      CHECK_GE(runner_calls.load(), 1);
      CHECK_EQ(app.GetSubApp(kNamedSubApp)
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               1);
    }
  }

  TEST_CASE("app::SubApp::SetExtractFunction") {
    SUBCASE("SetExtractFunction is used by Extract") {
      App app;
      app.InsertResources(CounterResource{8});

      SubApp sub_app("RenderSubApp");
      sub_app.TryInsertResources(ExtractCounterResource{});
      sub_app.SetExtractFunction([](const World& main, World& sub) {
        sub.WriteResource<ExtractCounterResource>().value =
            main.ReadResource<CounterResource>().value;
      });
      sub_app.Extract(app.GetWorld());

      CHECK_EQ(sub_app.GetWorld().ReadResource<ExtractCounterResource>().value,
               8);
    }
  }

  TEST_CASE("app::SubApp::SetName") {
    SUBCASE("SetName updates GetName") {
      SubApp sub_app;
      sub_app.SetName("PhysicsSubApp");
      CHECK_EQ(sub_app.GetName(), "PhysicsSubApp");
    }
  }

  TEST_CASE("app::SubApp::SetAllowOverlappingUpdates") {
    SUBCASE("SetAllowOverlappingUpdates updates AllowsOverlappingUpdates") {
      SubApp sub_app("RenderSubApp");
      sub_app.SetAllowOverlappingUpdates(true);
      CHECK(sub_app.AllowsOverlappingUpdates());
      sub_app.SetAllowOverlappingUpdates(false);
      CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
    }

    SUBCASE("AllowsOverlappingUpdates reflects label trait from InsertSubApp") {
      App app;
      app.InsertSubApp(kOverlappingSubApp, SubApp{});
      CHECK(app.GetSubApp(kOverlappingSubApp).AllowsOverlappingUpdates());
    }
  }

  TEST_CASE("app::SubApp::SetMaxExtractionSkips") {
    SUBCASE("SetMaxExtractionSkips updates MaxExtractionSkips") {
      SubApp sub_app("RenderSubApp");
      sub_app.SetMaxExtractionSkips(2);
      CHECK_EQ(sub_app.MaxExtractionSkips(), 2);
    }

    SUBCASE("MaxExtractionSkips reflects label trait from InsertSubApp") {
      App app;
      app.InsertSubApp(kLimitedSkipSubApp, SubApp{});
      CHECK_EQ(app.GetSubApp(kLimitedSkipSubApp).MaxExtractionSkips(), 2);
    }
  }

  TEST_CASE("app::SubApp::SetAsync") {
    SUBCASE("SetAsync updates IsAsync") {
      SubApp sub_app("RenderSubApp");
      sub_app.SetAsync(true);
      CHECK(sub_app.IsAsync());
      sub_app.SetAsync(false);
      CHECK_FALSE(sub_app.IsAsync());
    }

    SUBCASE("IsAsync reflects label trait from InsertSubApp") {
      App app;
      app.InsertSubApp(kAsyncSubApp, SubApp{});
      CHECK(app.GetSubApp(kAsyncSubApp).IsAsync());
    }
  }

  TEST_CASE("app::SubApp::SetUpdateStage") {
    SUBCASE("SetUpdateStage changes which stage Update runs") {
      SubApp sub_app("RenderSubApp");
      sub_app.SetUpdateStage(kUpdateStage);
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystem(kUpdate, IncrementSystem{});

      App app(2);
      app.InsertSubApp(kUnnamedSubApp, std::move(sub_app));
      app.Initialize();
      app.Update();

      CHECK_EQ(app.GetSubApp(kUnnamedSubApp)
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               1);
    }
  }

  TEST_CASE("app::SubApp::TryGetSchedule") {
    SUBCASE("TryGetSchedule returns built-in update schedule") {
      SubApp sub_app("RenderSubApp");
      CHECK_NE(sub_app.TryGetSchedule(kUpdate), nullptr);
      CHECK_NE(std::as_const(sub_app).TryGetSchedule(kUpdate), nullptr);
    }
  }

  TEST_CASE("app::SubApp::ShouldExit") {
    SUBCASE("Returns false without owner app") {
      SubApp sub_app("RenderSubApp");
      CHECK_FALSE(sub_app.ShouldExit());
    }

    SUBCASE("Propagates AppExit from owner app") {
      struct ExitOnFirst {
        void operator()(MessageWriter<AppExit> exits) const {
          exits.Write(AppExit::Success());
        }
      };

      App app(2);
      app.AddSystem(kFirst, ExitOnFirst{});

      SubApp sub_app("RenderSubApp");
      sub_app.SetAsync(true);
      sub_app.SetRunner(RunDefaultSubApp);
      app.InsertSubApp(kAsyncSubApp, std::move(sub_app));
      app.Initialize();

      std::this_thread::sleep_for(std::chrono::milliseconds{50});
      app.GetScheduler().Shutdown(app);

      CHECK_FALSE(app.GetSubApp(kAsyncSubApp).IsUpdating());
    }
  }

  TEST_CASE("app::SubApp::IsUpdating") {
    SUBCASE("False when idle after frame completes") {
      App app(2);
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{});
      sub_app.AddSystem(kUpdate, IncrementSystem{});
      app.InsertSubApp(kNamedSubApp, std::move(sub_app));
      app.Initialize();
      app.Update();
      app.GetScheduler().WaitForSubApps();

      CHECK_FALSE(app.GetSubApp(kNamedSubApp).IsUpdating());
    }
  }

  TEST_CASE("app::SubApp::GetWorld") {
    SUBCASE("GetWorld returns mutable and const views of same world") {
      SubApp sub_app("RenderSubApp");
      sub_app.InsertResources(CounterResource{2});
      CHECK_EQ(&sub_app.GetWorld(), &sub_app.GetWorld());
      CHECK_EQ(std::as_const(sub_app)
                   .GetWorld()
                   .ReadResource<CounterResource>()
                   .value,
               2);
    }
  }

  TEST_CASE("app::SubApp::GetScheduler") {
    SUBCASE("GetScheduler returns ECS scheduler for this sub-app") {
      SubApp sub_app("RenderSubApp");
      CHECK_EQ(&sub_app.GetScheduler(), &sub_app.GetScheduler());
      CHECK_EQ(&std::as_const(sub_app).GetScheduler(), &sub_app.GetScheduler());
    }
  }
}
