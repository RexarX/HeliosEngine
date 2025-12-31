#include <doctest/doctest.h>

#include <helios/core/app/access_policy.hpp>
#include <helios/core/app/details/scheduler.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>

#include <atomic>
#include <string_view>
#include <utility>

using namespace helios::app;
using namespace helios::app::details;
using namespace helios::ecs;
using namespace helios::async;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
};

struct Health {
  int points = 100;
};

struct GameTime {
  float delta_time = 0.016F;
  int update_count = 0;
};

struct PhysicsSettings {
  float gravity = 9.8F;
};

struct RenderSettings {
  bool vsync = true;
};

inline std::atomic<int> execution_order_counter{0};

struct FirstSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "FirstSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Position&>(); }

  void Update(SystemContext& /*ctx*/) override { order = execution_order_counter.fetch_add(1); }

  inline static std::atomic<int> order{-1};
};

struct SecondSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "SecondSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Velocity&>(); }

  void Update(SystemContext& /*ctx*/) override { order = execution_order_counter.fetch_add(1); }

  inline static std::atomic<int> order{-1};
};

struct ThirdSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "ThirdSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Health&>(); }

  void Update(SystemContext& /*ctx*/) override { order = execution_order_counter.fetch_add(1); }

  inline static std::atomic<int> order{-1};
};

struct TimeUpdateSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "TimeUpdateSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().WriteResources<GameTime>(); }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    ++time.update_count;
  }
};

struct PhysicsSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "PhysicsSystem"; }
  static constexpr auto GetAccessPolicy() {
    return AccessPolicy().Query<Position&, const Velocity&>().ReadResources<GameTime, PhysicsSettings>();
  }

  void Update(SystemContext& /*ctx*/) override {}
};

struct RenderSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "RenderSystem"; }
  static constexpr auto GetAccessPolicy() {
    return AccessPolicy().Query<const Position&>().ReadResources<RenderSettings>();
  }
  void Update(SystemContext& /*ctx*/) override {}
};

struct ConflictingWriteSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "ConflictingWriteSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Position&>(); }

  void Update(SystemContext& /*ctx*/) override {}
};

struct EmptySystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "EmptySystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy(); }

  void Update(SystemContext& /*ctx*/) override {}
};

}  // namespace

TEST_SUITE("app::details::Scheduler") {
  TEST_CASE("Scheduler::ctor: default construction") {
    Scheduler scheduler;

    CHECK_EQ(scheduler.SystemCount(), 0);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 0);
  }

  TEST_CASE("Scheduler::AddSystem: single system") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);

    CHECK_EQ(scheduler.SystemCount(), 1);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 1);
    CHECK(scheduler.ContainsSystem<FirstSystem>());
    CHECK(scheduler.ContainsSystem<FirstSystem>(kUpdate));
  }

  TEST_CASE("Scheduler::AddSystem: multiple systems") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.AddSystem<ThirdSystem>(kPostUpdate);

    CHECK_EQ(scheduler.SystemCount(), 3);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 2);
    CHECK_EQ(scheduler.SystemCount(kPostUpdate), 1);
    CHECK(scheduler.ContainsSystem<FirstSystem>());
    CHECK(scheduler.ContainsSystem<SecondSystem>());
    CHECK(scheduler.ContainsSystem<ThirdSystem>());
  }

  TEST_CASE("Scheduler::ContainsSystem: in different schedules") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kPostUpdate);

    CHECK(scheduler.ContainsSystem<FirstSystem>(kUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>(kPostUpdate));
    CHECK(scheduler.ContainsSystem<SecondSystem>(kPostUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<SecondSystem>(kUpdate));
  }

  TEST_CASE("Scheduler::SystemCount: across schedules") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kPreUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.AddSystem<ThirdSystem>(kPostUpdate);

    CHECK_EQ(scheduler.SystemCount(), 3);
    CHECK_EQ(scheduler.SystemCount(kPreUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kPostUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(Main{}), 0);
  }

  TEST_CASE("Scheduler::Clear: removes all systems") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.AddSystem<ThirdSystem>(kPostUpdate);

    CHECK_EQ(scheduler.SystemCount(), 3);

    scheduler.Clear();

    CHECK_EQ(scheduler.SystemCount(), 0);
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>());
    CHECK_FALSE(scheduler.ContainsSystem<SecondSystem>());
    CHECK_FALSE(scheduler.ContainsSystem<ThirdSystem>());
  }

  TEST_CASE("Scheduler::RegisterOrdering: system ordering") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);

    // Register ordering constraint: SecondSystem runs after FirstSystem
    SystemOrdering ordering;
    ordering.after.push_back(SystemTypeIdOf<FirstSystem>());
    scheduler.RegisterOrdering<SecondSystem>(kUpdate, std::move(ordering));

    CHECK_EQ(scheduler.SystemCount(kUpdate), 2);
    CHECK(scheduler.ContainsSystem<FirstSystem>());
    CHECK(scheduler.ContainsSystem<SecondSystem>());
  }

  TEST_CASE("Scheduler::BuildAllGraphs: with resource") {
    Executor executor;
    World world;
    world.InsertResource(GameTime{});

    Scheduler scheduler;
    scheduler.AddSystem<TimeUpdateSystem>(kUpdate);

    // Just test that BuildAllGraphs succeeds
    scheduler.BuildAllGraphs(world);

    CHECK_EQ(scheduler.SystemCount(kUpdate), 1);
    CHECK(scheduler.ContainsSystem<TimeUpdateSystem>(kUpdate));
  }

  TEST_CASE("Scheduler::BuildAllGraphs: multiple schedule build") {
    Executor executor;
    World world;
    world.InsertResource(GameTime{});

    Scheduler scheduler;
    scheduler.AddSystem<TimeUpdateSystem>(kPreUpdate);
    scheduler.AddSystem<TimeUpdateSystem>(kUpdate);
    scheduler.AddSystem<TimeUpdateSystem>(kPostUpdate);

    // Just test that BuildAllGraphs succeeds with multiple schedules
    scheduler.BuildAllGraphs(world);

    CHECK_EQ(scheduler.SystemCount(kPreUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kPostUpdate), 1);
  }

  TEST_CASE("Scheduler::AddSystem: same system in multiple schedules") {
    Scheduler scheduler;

    // Add the same system type to different schedules
    scheduler.AddSystem<FirstSystem>(kPreUpdate);
    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<FirstSystem>(kPostUpdate);

    // Total systems should be 3 (one per schedule)
    CHECK_EQ(scheduler.SystemCount(), 3);

    // Each schedule should have exactly one system
    CHECK_EQ(scheduler.SystemCount(kPreUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kPostUpdate), 1);

    // ContainsSystem should return true for each schedule
    CHECK(scheduler.ContainsSystem<FirstSystem>(kPreUpdate));
    CHECK(scheduler.ContainsSystem<FirstSystem>(kUpdate));
    CHECK(scheduler.ContainsSystem<FirstSystem>(kPostUpdate));

    // Global ContainsSystem should also return true
    CHECK(scheduler.ContainsSystem<FirstSystem>());
  }

  TEST_CASE("Scheduler::AddSystem: same system in multiple schedules with different systems") {
    Scheduler scheduler;

    // Add FirstSystem to multiple schedules
    scheduler.AddSystem<FirstSystem>(kPreUpdate);
    scheduler.AddSystem<FirstSystem>(kPostUpdate);

    // Add other systems to Update
    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.AddSystem<ThirdSystem>(kUpdate);

    CHECK_EQ(scheduler.SystemCount(), 4);
    CHECK_EQ(scheduler.SystemCount(kPreUpdate), 1);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 2);
    CHECK_EQ(scheduler.SystemCount(kPostUpdate), 1);

    CHECK(scheduler.ContainsSystem<FirstSystem>(kPreUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>(kUpdate));
    CHECK(scheduler.ContainsSystem<FirstSystem>(kPostUpdate));

    CHECK_FALSE(scheduler.ContainsSystem<SecondSystem>(kPreUpdate));
    CHECK(scheduler.ContainsSystem<SecondSystem>(kUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<SecondSystem>(kPostUpdate));
  }

  TEST_CASE("Scheduler::AddSystem: same system in multiple schedules builds and executes") {
    Executor executor;
    World world;
    world.InsertResource(GameTime{});

    Scheduler scheduler;
    scheduler.AddSystem<TimeUpdateSystem>(kPreUpdate);
    scheduler.AddSystem<TimeUpdateSystem>(kUpdate);
    scheduler.AddSystem<TimeUpdateSystem>(kPostUpdate);

    scheduler.BuildAllGraphs(world);

    // Execute each schedule and verify update_count increments
    scheduler.ExecuteSchedule<PreUpdate>(world, executor);
    auto& time1 = world.WriteResource<GameTime>();
    CHECK_EQ(time1.update_count, 1);

    scheduler.ExecuteSchedule<Update>(world, executor);
    auto& time2 = world.WriteResource<GameTime>();
    CHECK_EQ(time2.update_count, 2);

    scheduler.ExecuteSchedule<PostUpdate>(world, executor);
    auto& time3 = world.WriteResource<GameTime>();
    CHECK_EQ(time3.update_count, 3);
  }

  TEST_CASE("Scheduler::AddSystem: cleanup system in multiple schedules pattern") {
    // This test simulates a real-world use case: a cleanup system that runs
    // in multiple schedules (e.g., PreUpdate and PostUpdate)
    Scheduler scheduler;

    scheduler.AddSystem<EmptySystem>(kPreUpdate);
    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.AddSystem<EmptySystem>(kPostUpdate);

    CHECK_EQ(scheduler.SystemCount(), 4);
    CHECK(scheduler.ContainsSystem<EmptySystem>(kPreUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<EmptySystem>(kUpdate));
    CHECK(scheduler.ContainsSystem<EmptySystem>(kPostUpdate));
  }

  TEST_CASE("Scheduler::Clear: clears same system from multiple schedules") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kPreUpdate);
    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<FirstSystem>(kPostUpdate);

    CHECK_EQ(scheduler.SystemCount(), 3);

    scheduler.Clear();

    CHECK_EQ(scheduler.SystemCount(), 0);
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>());
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>(kPreUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>(kUpdate));
    CHECK_FALSE(scheduler.ContainsSystem<FirstSystem>(kPostUpdate));
  }

  TEST_CASE("Scheduler::GetSystemStorage: returns valid span") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);

    const auto& storage = std::as_const(scheduler).GetSystemStorage();
    CHECK_EQ(storage.size(), 2);
  }

  TEST_CASE("Scheduler::GetSystemStorage: const version") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    const auto& storage = std::as_const(scheduler).GetSystemStorage();
    CHECK_EQ(storage.size(), 1);
  }

  TEST_CASE("Scheduler::AddSystem: system with no conflicts registration") {
    Executor executor(4);
    World world;
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.AddSystem<ThirdSystem>(kUpdate);

    CHECK_EQ(scheduler.SystemCount(kUpdate), 3);
    CHECK(scheduler.ContainsSystem<FirstSystem>());
    CHECK(scheduler.ContainsSystem<SecondSystem>());
    CHECK(scheduler.ContainsSystem<ThirdSystem>());

    scheduler.BuildAllGraphs(world);
  }

  TEST_CASE("Scheduler::BuildAllGraphs: multiple times") {
    Executor executor;
    World world;
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.BuildAllGraphs(world);

    scheduler.AddSystem<SecondSystem>(kUpdate);
    scheduler.BuildAllGraphs(world);

    CHECK_EQ(scheduler.SystemCount(kUpdate), 2);
  }

  TEST_CASE("Scheduler::BuildAllGraphs: empty schedule") {
    Executor executor;
    World world;
    Scheduler scheduler;

    CHECK_EQ(scheduler.SystemCount(kUpdate), 0);

    // Should succeed even with no systems
    scheduler.BuildAllGraphs(world);
    CHECK_EQ(scheduler.SystemCount(kUpdate), 0);
  }

  TEST_CASE("Scheduler::RegisterOrdering") {
    Scheduler scheduler;

    scheduler.AddSystem<FirstSystem>(kUpdate);
    scheduler.AddSystem<SecondSystem>(kUpdate);

    SystemOrdering ordering;
    ordering.after.push_back(SystemTypeIdOf<FirstSystem>());
    scheduler.RegisterOrdering<SecondSystem>(kUpdate, ordering);

    CHECK_EQ(scheduler.SystemCount(kUpdate), 2);
  }

  TEST_CASE("Scheduler::BuildAllGraphs: complex system graph") {
    Executor executor;
    World world;
    world.InsertResource(GameTime{});
    world.InsertResource(PhysicsSettings{});
    world.InsertResource(RenderSettings{});

    Scheduler scheduler;
    scheduler.AddSystem<TimeUpdateSystem>(kUpdate);
    scheduler.AddSystem<PhysicsSystem>(kUpdate);
    scheduler.AddSystem<RenderSystem>(kUpdate);

    CHECK_EQ(scheduler.SystemCount(kUpdate), 3);
    CHECK(scheduler.ContainsSystem<TimeUpdateSystem>());
    CHECK(scheduler.ContainsSystem<PhysicsSystem>());
    CHECK(scheduler.ContainsSystem<RenderSystem>());

    scheduler.BuildAllGraphs(world);
  }
}
