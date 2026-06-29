#include <doctest/doctest.h>

#include <helios/async/executor.hpp>
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/executor/single_threaded.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>
#include <helios/ecs/world_view.hpp>

#include <compare>
#include <memory>
#include <memory_resource>
#include <string>
#include <vector>

#include "schedule/details/schedule_fixtures.hpp"

using namespace helios::ecs;
using namespace helios::ecs::schedule_test;

TEST_SUITE("helios::ecs::ScheduleSystemId") {
  TEST_CASE("ecs::ScheduleSystemId::operator==") {
    SUBCASE("Default ids compare equal") {
      constexpr ScheduleSystemId id1;
      constexpr ScheduleSystemId id2;
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Ids with different slots compare unequal") {
      constexpr ScheduleSystemId id1{.id = SystemId::From("A"), .slot = 0};
      constexpr ScheduleSystemId id2{.id = SystemId::From("A"), .slot = 1};
      CHECK_NE(id1, id2);
    }

    SUBCASE("Ids with same fields compare equal") {
      constexpr auto sys_id = SystemId::From("Test");
      constexpr ScheduleSystemId id1{.id = sys_id, .slot = 3, .generation = 0};
      constexpr ScheduleSystemId id2{.id = sys_id, .slot = 3, .generation = 0};
      CHECK_EQ(id1, id2);
    }
  }

  TEST_CASE("ecs::ScheduleSystemId::operator<=>") {
    SUBCASE("Ids with different slots are ordered") {
      constexpr ScheduleSystemId id1{.id = SystemId::From("A"), .slot = 0};
      constexpr ScheduleSystemId id2{.id = SystemId::From("A"), .slot = 1};
      CHECK_LT(id1, id2);
    }

    SUBCASE("Equal ids have equivalent ordering") {
      constexpr ScheduleSystemId id1;
      constexpr ScheduleSystemId id2;
      CHECK_EQ(id1 <=> id2, std::strong_ordering::equal);
    }
  }
}

TEST_SUITE("helios::ecs::ScheduleSystemMetadata") {
  TEST_CASE("ecs::ScheduleSystemMetadata::AddBefore(SystemId)") {
    SUBCASE("AddBefore with SystemId adds a target") {
      ScheduleSystemMetadata metadata;
      constexpr auto sys_id = SystemId::From<SystemAlpha>();

      metadata.AddBefore(sys_id);

      CHECK_EQ(metadata.before_targets.size(), 1);
      CHECK_EQ(metadata.before_targets[0], sys_id);
    }

    SUBCASE("AddBefore with same SystemId adds it only once") {
      ScheduleSystemMetadata metadata;
      constexpr auto sys_id = SystemId::From<SystemAlpha>();

      metadata.AddBefore(sys_id);
      metadata.AddBefore(sys_id);

      CHECK_EQ(metadata.before_targets.size(), 1);
    }
  }

  TEST_CASE("ecs::ScheduleSystemMetadata::AddAfter(SystemId)") {
    SUBCASE("AddAfter with SystemId adds a target") {
      ScheduleSystemMetadata metadata;
      constexpr auto sys_id = SystemId::From<SystemAlpha>();

      metadata.AddAfter(sys_id);

      CHECK_EQ(metadata.after_targets.size(), 1);
      CHECK_EQ(metadata.after_targets[0], sys_id);
    }

    SUBCASE("AddAfter with same SystemId adds it only once") {
      ScheduleSystemMetadata metadata;
      constexpr auto sys_id = SystemId::From<SystemAlpha>();

      metadata.AddAfter(sys_id);
      metadata.AddAfter(sys_id);

      CHECK_EQ(metadata.after_targets.size(), 1);
    }
  }

  TEST_CASE("ecs::ScheduleSystemMetadata::AddBefore(SystemSetId)") {
    SUBCASE("AddBefore with SystemSetId adds a set target") {
      ScheduleSystemMetadata metadata;
      constexpr auto set_id = SystemSetId::From<SetOne>();

      metadata.AddBefore(set_id);

      CHECK_EQ(metadata.before_set_targets.size(), 1);
      CHECK_EQ(metadata.before_set_targets[0], set_id);
    }

    SUBCASE("AddBefore with same SystemSetId adds it only once") {
      ScheduleSystemMetadata metadata;
      constexpr auto set_id = SystemSetId::From<SetOne>();

      metadata.AddBefore(set_id);
      metadata.AddBefore(set_id);

      CHECK_EQ(metadata.before_set_targets.size(), 1);
    }
  }

  TEST_CASE("ecs::ScheduleSystemMetadata::AddAfter(SystemSetId)") {
    SUBCASE("AddAfter with SystemSetId adds a set target") {
      ScheduleSystemMetadata metadata;
      constexpr auto set_id = SystemSetId::From<SetOne>();

      metadata.AddAfter(set_id);

      CHECK_EQ(metadata.after_set_targets.size(), 1);
      CHECK_EQ(metadata.after_set_targets[0], set_id);
    }

    SUBCASE("AddAfter with same SystemSetId adds it only once") {
      ScheduleSystemMetadata metadata;
      constexpr auto set_id = SystemSetId::From<SetOne>();

      metadata.AddAfter(set_id);
      metadata.AddAfter(set_id);

      CHECK_EQ(metadata.after_set_targets.size(), 1);
    }
  }

  TEST_CASE("ecs::ScheduleSystemMetadata::AppendUnique") {
    SUBCASE("AppendUnique adds a value that is not yet present") {
      std::vector<int> vec;
      ScheduleSystemMetadata::AppendUnique(vec, 42);

      CHECK_EQ(vec.size(), 1);
      CHECK_EQ(vec[0], 42);
    }

    SUBCASE("AppendUnique does not duplicate an existing value") {
      std::vector<int> vec{1, 2, 3};
      ScheduleSystemMetadata::AppendUnique(vec, 2);

      CHECK_EQ(vec.size(), 3);
    }
  }
}

struct NamedScheduleType {
  static constexpr std::string_view kName = "UpdateSchedule";
};
constexpr NamedScheduleType kNamedSchedule{};

struct UnnamedScheduleType {};
constexpr UnnamedScheduleType kUnnamedSchedule{};

TEST_SUITE("ecs::Schedule") {
  TEST_CASE("ecs::Schedule::ctor") {
    SUBCASE("Default-constructed schedule is dirty with empty name") {
      const Schedule schedule;
      CHECK(schedule.IsDirty());
      CHECK(schedule.GetName().empty());
    }

    SUBCASE("Named schedule stores the provided name") {
      const Schedule schedule("RenderSchedule");
      CHECK_EQ(schedule.GetName(), "RenderSchedule");
      CHECK(schedule.IsDirty());
    }

    SUBCASE("Move-constructed schedule transfers state and name") {
      Schedule source("RenderSchedule");
      source.Add(IncrementSystem{});

      const Schedule moved(std::move(source));

      CHECK_EQ(moved.GetName(), "RenderSchedule");
      CHECK(moved.IsDirty());
    }
  }

  TEST_CASE("ecs::Schedule::From") {
    SUBCASE("From uses kName when ScheduleWithNameTrait is satisfied") {
      const Schedule schedule = Schedule::From(kNamedSchedule);
      CHECK_EQ(schedule.GetName(), "UpdateSchedule");
    }

    SUBCASE("From falls back to type name for unnamed labels") {
      const Schedule schedule = Schedule::From(kUnnamedSchedule);
      CHECK_FALSE(schedule.GetName().empty());
    }
  }

  TEST_CASE("ecs::Schedule::SetName") {
    Schedule schedule;
    schedule.SetName("PostProcess");
    CHECK_EQ(schedule.GetName(), "PostProcess");
  }

  TEST_CASE("ecs::Schedule::operator=") {
    SUBCASE("Move assignment transfers state and name") {
      Schedule source("RenderSchedule");
      source.Add(IncrementSystem{});

      Schedule target;
      target = std::move(source);

      CHECK_EQ(target.GetName(), "RenderSchedule");
      CHECK(target.IsDirty());
    }
  }

  TEST_CASE("ecs::Schedule::Run") {
    SUBCASE("Run with explicit executor executes systems") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});
      const auto build_result = schedule.Build();
      REQUIRE(build_result.has_value());

      World world;
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.Run(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Run with stored executor executes systems") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      const auto build_result = schedule.Build();
      REQUIRE(build_result.has_value());

      World world;
      world.InsertResources(CounterResource{0});
      schedule.Run(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Run with Wait and ApplyDeferred executes deferred commands") {
      helios::async::Executor async_executor;

      Schedule schedule;
      schedule.Add(FlushCommandSystem{});
      schedule.SetExecutor(
          std::make_unique<SingleThreadedExecutor>(async_executor));
      const auto build_result = schedule.Build();
      REQUIRE(build_result.has_value());

      World world;
      CHECK_EQ(world.EntityCount(), 0);

      SingleThreadedExecutor executor(async_executor);
      schedule.Run(world, executor);
      executor.Wait();
      schedule.ApplyDeferred(world);

      CHECK_EQ(world.EntityCount(), 1);
      CHECK_FALSE(schedule.HasPendingLocalData());
    }
  }

  TEST_CASE("ecs::Schedule::RunAndWait") {
    SUBCASE("RunAndWait with explicit executor executes systems") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("RunAndWait with stored executor executes systems") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{0});
      schedule.RunAndWait(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("ecs::Schedule::Build") {
    SUBCASE("Build on empty schedule succeeds") {
      Schedule schedule;

      const auto result = schedule.Build();

      CHECK(result.has_value());
      CHECK_FALSE(schedule.IsDirty());
    }

    SUBCASE("Build with a single system succeeds") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
      CHECK_FALSE(schedule.IsDirty());
    }

    SUBCASE("Build with ordered systems succeeds") {
      Schedule schedule;
      auto first = schedule.Add(IncrementSystem{});
      auto second = schedule.Add(ReadOnlySystem{});

      first.Before(second);

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }

    SUBCASE("Build with system sets succeeds") {
      Schedule schedule;
      schedule.Add(IncrementSystem{}).InSet(SetOne{});
      [[maybe_unused]] const auto handle = schedule.Set(SetOne{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }
  }

  TEST_CASE("Clear") {
    SUBCASE("Clear removes systems and marks schedule dirty") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});
      CHECK(schedule.Build().has_value());

      schedule.Clear();

      CHECK(schedule.IsDirty());
      CHECK_EQ(schedule.Settings().executor_kind, ExecutorKind::kMultiThreaded);

      schedule.Add(IncrementSystem{});
      const auto result = schedule.Build();
      CHECK(result.has_value());
    }
  }

  TEST_CASE("ecs::Schedule::Add") {
    SUBCASE("Adding a param-style system returns a handle") {
      Schedule schedule;

      const auto handle = schedule.Add(IncrementSystem{});

      CHECK_GE(handle.Id().slot, 0);
      CHECK(schedule.IsDirty());
    }

    SUBCASE("Adding a callable system with name returns a handle") {
      Schedule schedule;

      const auto handle = schedule.Add(
          "MySystem", [](World& /*world*/, SystemLocalData& /*data*/) {},
          AccessPolicy{});

      CHECK_EQ(handle.Id().slot, 0);
      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::Schedule::Set") {
    SUBCASE("Set creates a set and returns a handle") {
      Schedule schedule;
      constexpr auto set_id = SystemSetId::From<SetOne>();

      const auto handle = schedule.Set(set_id);

      CHECK_EQ(handle.Id(), set_id);
      CHECK(schedule.IsDirty());
    }

    SUBCASE("Set with template type creates a set and returns a handle") {
      Schedule schedule;

      const auto handle = schedule.Set<SetOne>();

      CHECK_EQ(handle.Id(), SystemSetId::From<SetOne>());
    }
  }

  TEST_CASE("ecs::Schedule::SetExecutor") {
    SUBCASE("New schedule has null executor") {
      const Schedule schedule;
      CHECK_EQ(schedule.GetExecutor(), nullptr);
    }

    SUBCASE("SetExecutor stores the executor") {
      Schedule schedule;
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());

      CHECK_NE(schedule.GetExecutor(), nullptr);
    }
  }

  TEST_CASE("ecs::Schedule::IsDirty") {
    SUBCASE("Newly constructed schedule is dirty") {
      Schedule schedule;
      CHECK(schedule.IsDirty());
    }

    SUBCASE("After Build, schedule is clean") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
      CHECK_FALSE(schedule.IsDirty());
    }

    SUBCASE("Adding a system after Build makes it dirty again") {
      Schedule schedule;
      schedule.Add(IncrementSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      schedule.Add(ReadOnlySystem{});

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::Schedule::Settings") {
    SUBCASE("Default settings use kMultiThreaded executor") {
      const Schedule schedule;
      const auto& settings = schedule.Settings();

      CHECK_EQ(settings.executor_kind, ExecutorKind::kMultiThreaded);
    }

    SUBCASE("Settings can be modified") {
      Schedule schedule;

      schedule.Settings().executor_kind = ExecutorKind::kMainThread;

      CHECK_EQ(schedule.Settings().executor_kind, ExecutorKind::kMainThread);
    }

    SUBCASE("Const Settings returns the correct value") {
      Schedule schedule;
      const Schedule& const_schedule = schedule;

      CHECK_EQ(const_schedule.Settings().executor_kind,
               ExecutorKind::kMultiThreaded);
    }
  }

  TEST_CASE("ecs::Schedule::GetExecutor") {
    SUBCASE("Mutable GetExecutor returns stored executor") {
      Schedule schedule;
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());

      Executor* executor = schedule.GetExecutor();

      CHECK_NE(executor, nullptr);
    }

    SUBCASE("Const GetExecutor returns stored executor") {
      Schedule schedule;
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());

      const Schedule& const_schedule = schedule;
      const Executor* executor = const_schedule.GetExecutor();

      CHECK_NE(executor, nullptr);
    }
  }

  TEST_CASE("ecs::Schedule::integration_ordering") {
    SUBCASE("Before constraint enforces execution order") {
      std::array<int, 3> order{};
      int call_index = 0;

      Schedule schedule;
      auto handle_a =
          schedule.Add(OrderSystemA{.slot = &order[0], .index = &call_index});
      auto handle_b =
          schedule.Add(OrderSystemB{.slot = &order[1], .index = &call_index});
      auto handle_c =
          schedule.Add(OrderSystemC{.slot = &order[2], .index = &call_index});

      handle_a.Before(handle_b);
      handle_b.Before(handle_c);
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_LT(order[0], order[1]);
      CHECK_LT(order[1], order[2]);
    }

    SUBCASE("After constraint enforces execution order") {
      std::array<int, 3> order{};
      int call_index = 0;

      Schedule schedule;
      auto handle_a =
          schedule.Add(OrderSystemA{.slot = &order[0], .index = &call_index});
      auto handle_b =
          schedule.Add(OrderSystemB{.slot = &order[1], .index = &call_index});
      auto handle_c =
          schedule.Add(OrderSystemC{.slot = &order[2], .index = &call_index});

      handle_c.After(handle_b);
      handle_b.After(handle_a);
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_LT(order[0], order[1]);
      CHECK_LT(order[1], order[2]);
    }
  }

  TEST_CASE("ecs::Schedule: run conditions") {
    SUBCASE("System with passing run condition executes") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.RunIf(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }));
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("System with failing run condition is skipped") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.RunIf(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return false;
          }));
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 0);
    }
  }

  TEST_CASE("ecs::Schedule: resources") {
    SUBCASE("Systems with read-only resources run correctly") {
      Schedule schedule;
      schedule.Add(ReadOnlySystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{42});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 42);
    }

    SUBCASE("Systems with conflicting write access are unordered") {
      Schedule schedule;
      schedule.Add(
          "WriterA", [](World& /*world*/, SystemLocalData& /*data*/) {},
          AccessPolicyBuilder().WriteResources<CounterResource>().Build());
      schedule.Add(
          "WriterB", [](World& /*world*/, SystemLocalData& /*data*/) {},
          AccessPolicyBuilder().WriteResources<CounterResource>().Build());

      [[maybe_unused]] const auto result = schedule.Build();

      CHECK(result.has_value());
    }
  }

  TEST_CASE("ecs::Schedule: query") {
    SUBCASE(
        "Systems with read-only Query on same component execute without "
        "ordering") {
      Schedule schedule;
      schedule.Add(ReadPositionSystem{});
      schedule.Add(ReadOtherPositionSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{5.0F, 0.0F});
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 2);
    }

    SUBCASE("System with write Query correctly modifies components") {
      Schedule schedule;
      schedule.Add(WritePositionSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{10.0F, 0.0F});
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      const auto& pos = world.ReadComponent<Position>(entity);
      CHECK_EQ(doctest::Approx(pos.x), 11.0F);
      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE(
        "Systems with conflicting write-write access on same component build "
        "successfully") {
      Schedule schedule;
      schedule.Add(WritePositionSystem{});
      schedule.Add(WriteOtherPositionSystem{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }

    SUBCASE(
        "Systems with read-write access on same component build successfully") {
      Schedule schedule;
      schedule.Add(ReadPositionSystem{});
      schedule.Add(WritePositionSystem{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }

    SUBCASE(
        "Systems writing different components have no access conflict and "
        "execute correctly") {
      Schedule schedule;
      schedule.Add(WritePositionSystem{});
      schedule.Add(WriteVelocitySystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{0.0F, 0.0F});
      world.AddComponents(entity, Velocity{0.0F, 0.0F});
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      const auto& pos = world.ReadComponent<Position>(entity);
      const auto& vel = world.ReadComponent<Velocity>(entity);
      CHECK_EQ(doctest::Approx(pos.x), 1.0F);
      CHECK_EQ(doctest::Approx(vel.x), 1.0F);
      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("ecs::Schedule: async resource") {
    SUBCASE("Systems with AsyncRes on same resource build without conflicts") {
      Schedule schedule;
      schedule.Add(AsyncResWriter{});
      schedule.Add(AsyncResOtherWriter{});

      [[maybe_unused]] const auto result = schedule.Build();

      CHECK(result.has_value());
    }

    SUBCASE("AsyncRes systems execute and modify the resource correctly") {
      Schedule schedule;
      schedule.Add(AsyncResWriter{});
      schedule.Add(AsyncResOtherWriter{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(ThreadSafeData{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ThreadSafeData>().value, 2);
    }

    SUBCASE("AsyncRes reader and writer coexist and build successfully") {
      Schedule schedule;
      schedule.Add(AsyncResWriter{});
      schedule.Add(AsyncResReader{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }
  }

  TEST_CASE("ecs::Schedule: world view") {
    SUBCASE("WorldView system runs and accesses world data") {
      Schedule schedule;
      schedule.Add(WorldViewSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      [[maybe_unused]] const Entity entity_a = world.CreateEntity();
      [[maybe_unused]] const Entity entity_b = world.CreateEntity();
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 2);
    }

    SUBCASE("WorldView system coexists with Query system without conflict") {
      Schedule schedule;
      schedule.Add(WorldViewWithQuerySystem{});
      schedule.Add(ReadPositionSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 0.0F});
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 2);
    }
  }

  TEST_CASE("ecs::Schedule: messages") {
    SUBCASE("Regular messages flow from writer to reader across Update") {
      World world;
      world.InsertResources(CounterResource{0});
      world.AddMessage<TestMessage>();

      world.WriteMessages<TestMessage>().Write(TestMessage{42});
      world.Update();

      Schedule read_schedule;
      read_schedule.Add(TestMsgReaderSystem{});
      read_schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = read_schedule.Build();
      read_schedule.RunAndWait(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 42);
    }

    SUBCASE("Async messages flow from writer to reader") {
      World world;
      world.InsertResources(CounterResource{0});
      world.AddMessage<AsyncTestEvent>();

      {
        Schedule write_schedule;
        write_schedule.Add(AsyncTestMsgWriterSystem{});
        write_schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
        [[maybe_unused]] const auto build_result = write_schedule.Build();
        write_schedule.RunAndWait(world);
      }

      {
        Schedule read_schedule;
        read_schedule.Add(AsyncTestMsgReaderSystem{});
        read_schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
        [[maybe_unused]] const auto build_result = read_schedule.Build();
        read_schedule.RunAndWait(world);
      }

      CHECK_EQ(world.ReadResource<CounterResource>().value, 42);
    }

    SUBCASE("Message systems do not create access conflicts") {
      Schedule schedule;
      schedule.Add(TestMsgWriterSystem{});
      schedule.Add(TestMsgReaderSystem{});

      [[maybe_unused]] const auto result = schedule.Build();

      CHECK(result.has_value());
    }
  }

  TEST_CASE("ecs::Schedule: combined") {
    SUBCASE("Query with Res system executes correctly") {
      Schedule schedule;
      schedule.Add(QueryAndResSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{0.0F, 0.0F});
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      const auto& pos = world.ReadComponent<Position>(entity);
      CHECK_EQ(doctest::Approx(pos.x), 1.0F);
      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Query with MessageWriter system builds") {
      Schedule schedule;
      schedule.Add(QueryAndMsgWriterSystem{});

      [[maybe_unused]] const auto build_result = schedule.Build();

      CHECK(build_result.has_value());
    }

    SUBCASE("Query with AsyncRes system executes correctly") {
      Schedule schedule;
      schedule.Add(QueryAndAsyncResSystem{});
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{0.0F, 0.0F});
      world.InsertResources(ThreadSafeData{0});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      const auto& pos = world.ReadComponent<Position>(entity);
      CHECK_EQ(doctest::Approx(pos.x), 1.0F);
      CHECK_EQ(world.ReadResource<ThreadSafeData>().value, 1);
    }

    SUBCASE(
        "Systems with Query write conflict and Res conflict build "
        "successfully") {
      Schedule schedule;
      schedule.Add(QueryAndResSystem{});
      schedule.Add(WritePositionSystem{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }

    SUBCASE(
        "Systems with Query and AsyncRes on same resource build without "
        "conflict") {
      Schedule schedule;
      schedule.Add(QueryAndAsyncResSystem{});
      schedule.Add(AsyncResWriter{});

      const auto result = schedule.Build();

      CHECK(result.has_value());
    }
  }

  TEST_CASE("ecs::Schedule: run conditions persist across rebuilds") {
    struct SetA {};

    SUBCASE("System run condition survives double Build") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});
      handle.RunIf(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }));
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      CHECK(schedule.Build().has_value());
      CHECK(schedule.Build().has_value());

      World world;
      world.InsertResources(CounterResource{0});
      schedule.RunAndWait(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Set RunIf conditions apply to all members after double Build") {
      struct AlwaysTrue {
        bool operator()() const { return true; }
      };

      Schedule schedule;
      schedule.Set<SetA>().RunIf(AlwaysTrue{});

      auto first = schedule.Add(IncrementSystem{});
      auto second = schedule.Add(IncrementSystem{});
      first.InSet<SetA>();
      second.InSet<SetA>();

      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      CHECK(schedule.Build().has_value());
      CHECK(schedule.Build().has_value());

      World world;
      world.InsertResources(CounterResource{0});
      schedule.RunAndWait(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 2);
    }
  }

  TEST_CASE("ecs::Schedule: system-local commands flushed") {
    SUBCASE(
        "Commands enqueued via Commands param are executed after RunAndWait") {
      Schedule schedule;
      schedule.Add(FlushCommandSystem{});
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      CHECK_EQ(world.EntityCount(), 0);
      schedule.RunAndWait(world);

      CHECK_EQ(world.EntityCount(), 1);
    }

    SUBCASE(
        "Multiple systems enqueueing commands all execute after RunAndWait") {
      Schedule schedule;
      schedule.Add(FlushCommandSystem{});
      schedule.Add(FlushCommandSystem{});
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      schedule.RunAndWait(world);

      CHECK_EQ(world.EntityCount(), 2);
    }

    SUBCASE("Spawn with AddComponents creates entity with components") {
      struct Enemy {};
      struct Lifetime {
        float remaining = 5.0F;
      };

      struct SpawnEnemy {
        void operator()(Commands commands, Res<CounterResource> counter) const {
          const Entity entity =
              commands.Spawn().AddComponents(Enemy{}, Lifetime{}).GetEntity();
          counter->value = static_cast<int>(entity.Index());
        }
      };

      Schedule schedule;
      schedule.Add(SpawnEnemy{});
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = schedule.Build();

      World world;
      world.InsertResources(CounterResource{});
      schedule.RunAndWait(world);

      const Entity entity{static_cast<Entity::IndexType>(
                              world.ReadResource<CounterResource>().value),
                          1};
      CHECK_EQ(world.EntityCount(), 1);
      CHECK(world.HasComponent<Enemy>(entity));
      CHECK_EQ(world.ReadComponent<Lifetime>(entity).remaining,
               doctest::Approx(5.0F));
    }
  }

  TEST_CASE("ecs::Schedule: system-local messages flushed") {
    SUBCASE(
        "Messages written via MessageWriter are in world after RunAndWait") {
      World world;
      world.AddMessage<TestMessage>();

      Schedule schedule;
      schedule.Add(FlushMessageSystem{});
      schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = schedule.Build();

      schedule.RunAndWait(world);

      const auto curr = world.Messages().CurrentMessages<TestMessage>();
      CHECK_EQ(curr.size(), 1);
      CHECK_EQ(curr[0].value, 99);
    }

    SUBCASE(
        "Messages written via MessageWriter are readable after world.Update") {
      World world;
      world.AddMessage<TestMessage>();
      world.InsertResources(CounterResource{0});

      Schedule write_schedule;
      write_schedule.Add(FlushMessageSystem{});
      write_schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto build_result = write_schedule.Build();
      write_schedule.RunAndWait(world);
      world.Update();

      Schedule read_schedule;
      read_schedule.Add(FlushReadMessageSystem{});
      read_schedule.SetExecutor(std::make_unique<MainThreadExecutor>());
      [[maybe_unused]] const auto read_result = read_schedule.Build();
      read_schedule.RunAndWait(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 99);
    }
  }
}
