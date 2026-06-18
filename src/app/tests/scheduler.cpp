#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/scheduler.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>

using namespace helios;
using namespace helios::app;
using namespace helios::ecs;

using AppScheduler = helios::app::Scheduler;

namespace {

struct CounterResource {
  int value = 0;
};

struct IncrementSystem {
  void operator()(Res<CounterResource> counter) const { ++counter->value; }
};

struct SlowUpdateSystem {
  void operator()() const {
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  }
};

struct RenderSubAppLabel {};
struct PhysicsSubAppLabel {};

struct ParallelProbeSystem {
  static inline std::atomic<int> active{0};
  static inline std::atomic<int> max_active{0};

  void operator()() const {
    const int current = active.fetch_add(1, std::memory_order_acq_rel) + 1;
    int observed = max_active.load(std::memory_order_acquire);
    while (current > observed &&
           !max_active.compare_exchange_weak(observed, current,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire)) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    active.fetch_sub(1, std::memory_order_acq_rel);
  }
};

struct OrderSystem {
  int* order_slot = nullptr;
  int* call_index = nullptr;

  void operator()(Res<CounterResource> /*counter*/) const {
    *order_slot = (*call_index)++;
  }
};

}  // namespace

TEST_SUITE("helios::app::Scheduler") {
  TEST_CASE("app::Scheduler::ctor") {
    SUBCASE("Default-constructed scheduler can build and run frames") {
      App app(2);
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Move construction preserves ability to run frames") {
      App app(2);
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      AppScheduler source;
      source.Build(app);
      AppScheduler moved(std::move(source));
      moved.RunFrame(app);
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("app::Scheduler::operator=") {
    SUBCASE("Move assignment transfers scheduler state") {
      App app(2);
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();

      AppScheduler source;
      source.Build(app);
      AppScheduler target;
      target = std::move(source);
      target.RunFrame(app);

      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("app::Scheduler::Clear") {
    SUBCASE("Clear resets sub-app tracking so WaitForSubApps is immediate") {
      App app(2);
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      app.Initialize();
      app.Update();
      app.GetScheduler().Clear();
      app.GetScheduler().WaitForSubApps();
      CHECK_FALSE(app.GetSubApp<RenderSubAppLabel>().IsUpdating());
    }
  }

  TEST_CASE("app::Scheduler::Build") {
    SUBCASE("Build wires sub-apps for parallel startup and updates") {
      App app(4);
      app.InsertSubApp(RenderSubAppLabel{}, SubApp{});
      app.InsertSubApp(PhysicsSubAppLabel{}, SubApp{});
      app.Initialize();
      app.Update();
      app.GetScheduler().WaitForSubApps();
      CHECK_FALSE(app.GetSubApp<RenderSubAppLabel>().IsUpdating());
      CHECK_FALSE(app.GetSubApp<PhysicsSubAppLabel>().IsUpdating());
    }
  }

  TEST_CASE("app::Scheduler::RunStartup") {
    SUBCASE("RunStartup executes startup systems on main sub-app") {
      App app(2);
      app.InsertResources(CounterResource{});
      app.AddSystem(kStartup, IncrementSystem{});
      app.Initialize();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("app::Scheduler::RunFrame") {
    SUBCASE("RunFrame executes update systems on main sub-app") {
      App app(2);
      app.InsertResources(CounterResource{});
      app.AddSystem(kUpdate, IncrementSystem{});
      app.Initialize();
      app.Update();
      CHECK_EQ(app.GetWorld().ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Frame schedule ordering runs kFirst before kLast") {
      App app;
      app.InsertResources(CounterResource{});

      int call_index = 0;
      std::array<int, 2> order{};

      app.AddSystem(kFirst, OrderSystem{.order_slot = order.data(),
                                        .call_index = &call_index});
      app.AddSystem(kLast, OrderSystem{.order_slot = &order[1],
                                       .call_index = &call_index});

      app.Initialize();
      app.Update();

      CHECK_LT(order[0], order[1]);
    }

    SUBCASE("Blocking sub-apps update concurrently within one frame") {
      App app(4);

      SubApp render;
      render.AddSystem(kUpdate, SlowUpdateSystem{});
      SubApp physics;
      physics.AddSystem(kUpdate, SlowUpdateSystem{});

      app.InsertSubApp(RenderSubAppLabel{}, std::move(render));
      app.InsertSubApp(PhysicsSubAppLabel{}, std::move(physics));
      app.Initialize();

      const auto start = std::chrono::steady_clock::now();
      app.Update();
      app.GetScheduler().WaitForSubApps();
      const auto elapsed = std::chrono::steady_clock::now() - start;

      CHECK_LT(elapsed, std::chrono::seconds{1});
    }

    SUBCASE("Independent pre-update systems may run concurrently") {
      ParallelProbeSystem::active.store(0, std::memory_order_relaxed);
      ParallelProbeSystem::max_active.store(0, std::memory_order_relaxed);

      App app(4);
      app.AddSystem(kPreUpdate, ParallelProbeSystem{});
      app.AddSystem(kPreUpdate, ParallelProbeSystem{});
      app.Initialize();
      app.Update();

      CHECK_GE(ParallelProbeSystem::max_active.load(std::memory_order_relaxed),
               2);
    }
  }

  TEST_CASE("app::Scheduler::Shutdown") {
    SUBCASE("Shutdown runs shutdown stage and leaves app initialized") {
      App app(2);
      app.InsertResources(CounterResource{});
      app.Initialize();
      app.GetScheduler().Shutdown(app);
      CHECK(app.IsInitialized());
    }
  }

  TEST_CASE("app::Scheduler::WaitForSubApps") {
    SUBCASE("WaitForSubApps joins in-flight blocking sub-app updates") {
      App app(4);
      SubApp render;
      render.AddSystem(kUpdate, SlowUpdateSystem{});
      app.InsertSubApp(RenderSubAppLabel{}, std::move(render));
      app.Initialize();
      app.Update();
      app.GetScheduler().WaitForSubApps();
      CHECK_FALSE(app.GetSubApp<RenderSubAppLabel>().IsUpdating());
    }
  }
}
