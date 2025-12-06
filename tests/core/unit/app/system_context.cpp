#include <doctest/doctest.h>

#include <helios/core/app/access_policy.hpp>
#include <helios/core/app/details/system_info.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/async/sub_task_graph.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/query.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/ecs/world.hpp>

#include <string_view>
#include <vector>

using namespace helios::app;
using namespace helios::ecs;
using namespace helios::async;

namespace {

// Test components
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

// Test resources
struct GameTime {
  float delta_time = 0.016F;
  float total_time = 0.0F;
};

struct PhysicsSettings {
  float gravity = 9.8F;
  bool enabled = true;
};

struct RenderSettings {
  bool vsync = true;
  int fps_limit = 60;
};

// Test events
struct DamageEvent {
  int amount = 0;
};

struct SpawnEvent {
  float x = 0.0F;
  float y = 0.0F;
};

// Helper to create SystemInfo with AccessPolicy
helios::app::details::SystemInfo CreateSystemInfo(std::string_view name, const AccessPolicy& policy) {
  helios::app::details::SystemInfo info;
  info.name = name;
  info.type_id = 12345;  // Dummy type ID
  info.access_policy = policy;
  info.execution_count = 0;
  return info;
}

}  // namespace

TEST_SUITE("app::SystemContext") {
  TEST_CASE("SystemContext: Construction With Executor") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().WriteResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    CHECK(ctx.HasExecutor());
    CHECK_FALSE(ctx.HasSubTaskGraph());
    CHECK_EQ(ctx.GetSystemName(), "TestSystem");
  }

  TEST_CASE("SystemContext: ReserveEntity") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    Entity entity1 = ctx.ReserveEntity();
    Entity entity2 = ctx.ReserveEntity();

    CHECK(entity1.Valid());
    CHECK(entity2.Valid());
    CHECK_NE(entity1, entity2);
  }

  TEST_CASE("SystemContext: WriteResource With Valid Access") {
    World world;
    world.InsertResource(GameTime{0.016F, 0.0F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().WriteResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto& time = ctx.WriteResource<GameTime>();
    time.total_time = 1.0F;

    CHECK_EQ(time.total_time, doctest::Approx(1.0F));
  }

  TEST_CASE("SystemContext: ReadResource With Valid Access") {
    World world;
    world.InsertResource(GameTime{0.016F, 2.5F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().ReadResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    const auto& time = ctx.ReadResource<GameTime>();

    CHECK_EQ(time.delta_time, doctest::Approx(0.016F));
    CHECK_EQ(time.total_time, doctest::Approx(2.5F));
  }

  TEST_CASE("SystemContext: ReadResource With WriteAccess") {
    World world;
    world.InsertResource(GameTime{0.016F, 2.5F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().WriteResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    // Should work - write access includes read access
    const auto& time = ctx.ReadResource<GameTime>();

    CHECK_EQ(time.delta_time, doctest::Approx(0.016F));
    CHECK_EQ(time.total_time, doctest::Approx(2.5F));
  }

  TEST_CASE("SystemContext: TryWriteResource Exists") {
    World world;
    world.InsertResource(GameTime{0.016F, 0.0F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().WriteResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto* time = ctx.TryWriteResource<GameTime>();

    REQUIRE(time != nullptr);
    CHECK_EQ(time->delta_time, doctest::Approx(0.016F));
  }

  TEST_CASE("SystemContext: TryWriteResource Does Not Exist") {
    World world;

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().WriteResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto* time = ctx.TryWriteResource<GameTime>();

    CHECK_EQ(time, nullptr);
  }

  TEST_CASE("SystemContext: TryReadResource Exists") {
    World world;
    world.InsertResource(GameTime{0.016F, 5.0F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().ReadResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    const auto* time = ctx.TryReadResource<GameTime>();

    REQUIRE(time != nullptr);
    CHECK_EQ(time->total_time, doctest::Approx(5.0F));
  }

  TEST_CASE("SystemContext: TryReadResource Does Not Exist") {
    World world;

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().ReadResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    const auto* time = ctx.TryReadResource<GameTime>();

    CHECK_EQ(time, nullptr);
  }

  TEST_CASE("SystemContext: HasResource Returns True") {
    World world;
    world.InsertResource(GameTime{});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().ReadResources<GameTime>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    CHECK(ctx.HasResource<GameTime>());
  }

  TEST_CASE("SystemContext: HasResource Returns False") {
    World world;

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    CHECK_FALSE(ctx.HasResource<GameTime>());
  }

  TEST_CASE("SystemContext: EmitEvent Single") {
    World world;
    world.AddEvent<DamageEvent>();

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    DamageEvent event{50};
    ctx.EmitEvent(event);

    // Events should be in local storage
    auto& event_queue = local_storage.GetEventQueue();
    CHECK(event_queue.HasEvents<DamageEvent>());

    // Merge local events to world
    world.MergeEventQueue(event_queue);

    auto reader = world.ReadEvents<DamageEvent>();
    auto events = reader.Collect();
    REQUIRE_EQ(events.size(), 1);
    CHECK_EQ(events[0].amount, 50);
  }

  TEST_CASE("SystemContext: EmitEvent Multiple") {
    World world;
    world.AddEvent<DamageEvent>();

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    ctx.EmitEvent(DamageEvent{10});
    ctx.EmitEvent(DamageEvent{20});
    ctx.EmitEvent(DamageEvent{30});

    world.MergeEventQueue(local_storage.GetEventQueue());

    auto reader = world.ReadEvents<DamageEvent>();
    auto events = reader.Collect();
    REQUIRE_EQ(events.size(), 3);
    CHECK_EQ(events[0].amount, 10);
    CHECK_EQ(events[1].amount, 20);
    CHECK_EQ(events[2].amount, 30);
  }

  TEST_CASE("SystemContext: Read Events from World") {
    World world;
    world.AddEvent<DamageEvent>();

    auto writer = world.WriteEvents<DamageEvent>();
    writer.Write(DamageEvent{100});
    writer.Write(DamageEvent{200});
    world.Update();  // Flush events

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto reader = world.ReadEvents<DamageEvent>();
    auto events = reader.Collect();

    REQUIRE_EQ(events.size(), 2);
    CHECK_EQ(events[0].amount, 100);
    CHECK_EQ(events[1].amount, 200);
  }

  TEST_CASE("SystemContext: GetExecutor With Executor Context") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto& exec = ctx.Executor();

    CHECK_GE(exec.WorkerCount(), 0);
  }

  TEST_CASE("SystemContext: HasResource Check") {
    World world;
    world.InsertResource(GameTime{});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    CHECK(ctx.HasResource<GameTime>());
  }

  TEST_CASE("SystemContext: GetSystemInfo Returns Reference") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().WriteResources<GameTime>();
    auto info = CreateSystemInfo("CustomSystemName", policy);

    SystemContext ctx(world, info, executor, local_storage);

    const auto& sys_info = ctx.GetSystemInfo();

    CHECK_EQ(sys_info.name, "CustomSystemName");
    CHECK_EQ(sys_info.type_id, 12345);
  }

  TEST_CASE("SystemContext: GetSystemName") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("MyTestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    CHECK_EQ(ctx.GetSystemName(), "MyTestSystem");
  }

  TEST_CASE("SystemContext: Multiple Resource Access") {
    World world;
    world.InsertResource(GameTime{0.016F, 0.0F});
    world.InsertResource(PhysicsSettings{9.8F, true});
    world.InsertResource(RenderSettings{true, 60});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy =
        AccessPolicy().WriteResources<GameTime>().ReadResources<PhysicsSettings>().WriteResources<RenderSettings>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto& time = ctx.WriteResource<GameTime>();
    const auto& physics = ctx.ReadResource<PhysicsSettings>();
    auto& render = ctx.WriteResource<RenderSettings>();

    time.total_time = 10.0F;
    render.fps_limit = 120;

    CHECK_EQ(time.total_time, doctest::Approx(10.0F));
    CHECK_EQ(physics.gravity, doctest::Approx(9.8F));
    CHECK_EQ(render.fps_limit, 120);
  }

  TEST_CASE("SystemContext: Event Isolation Before Merge") {
    World world;
    world.AddEvent<DamageEvent>();

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage1;
    helios::ecs::details::SystemLocalStorage local_storage2;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("System1", policy);

    SystemContext ctx1(world, info, executor, local_storage1);
    SystemContext ctx2(world, info, executor, local_storage2);

    // Emit events in first context
    ctx1.EmitEvent(DamageEvent{100});

    // Events should be in local storage
    CHECK(local_storage1.GetEventQueue().HasEvents<DamageEvent>());

    // Merge first context's events
    world.MergeEventQueue(local_storage1.GetEventQueue());

    // Now world should have the event
    auto reader = world.ReadEvents<DamageEvent>();
    REQUIRE_EQ(reader.Count(), 1);
    auto events = reader.Collect();
    CHECK_EQ(events[0].amount, 100);
  }

  TEST_CASE("SystemContext: Commands Deferred Execution") {
    World world;

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    auto entity1 = ctx.ReserveEntity();
    auto entity2 = ctx.ReserveEntity();

    // Entities should be reserved but not yet created in world
    CHECK(entity1.Valid());
    CHECK(entity2.Valid());

    // Apply commands
    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Now entities should exist in world
    CHECK(world.Exists(entity1));
    CHECK(world.Exists(entity2));
  }

  TEST_CASE("SystemContext::ReadOnlyQuery: creates read-only QueryBuilder") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor(4);

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<const Position&>();
    info.access_policy = policy;

    // Create some entities with Position
    auto e1 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F, 3.0F});

    auto e2 = world.CreateEntity();
    world.AddComponent(e2, Position{4.0F, 5.0F, 6.0F});

    // Create SystemContext (ReadOnlyQuery works on both const and non-const)
    SystemContext ctx(world, info, executor, local_storage);

    // Should be able to create read-only query
    auto query = ctx.ReadOnlyQuery().Get<const Position&>();

    CHECK_EQ(query.Count(), 2);

    // Verify we can iterate
    size_t count = 0;
    query.ForEach([&count](const Position& pos) {
      CHECK_GE(pos.x, 0.0F);
      ++count;
    });

    CHECK_EQ(count, 2);
  }

  TEST_CASE("SystemContext::ReadOnlyQuery: with multiple components") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor(4);

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<const Position&, const Velocity&>();
    info.access_policy = policy;

    // Create entities with Position and Velocity
    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
      world.AddComponent(e, Velocity{0.1F, 0.2F, 0.3F});
    }

    SystemContext ctx(world, info, executor, local_storage);

    // Query with const components via ReadOnlyQuery
    auto query = ctx.ReadOnlyQuery().Get<const Position&, const Velocity&>();

    CHECK_EQ(query.Count(), 5);

    float total_x = 0.0F;
    query.ForEach([&total_x](const Position& pos, const Velocity& vel) {
      total_x += pos.x;
      CHECK_EQ(vel.dx, 0.1F);
    });

    CHECK_EQ(total_x, 10.0F);  // 0 + 1 + 2 + 3 + 4
  }

  TEST_CASE("SystemContext::Query: mutable and read-only query coexist") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor(4);

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<Position&>();
    info.access_policy = policy;

    auto e = world.CreateEntity();
    world.AddComponent(e, Position{1.0F, 2.0F, 3.0F});

    // Context allows both mutable and read-only access
    SystemContext ctx(world, info, executor, local_storage);
    auto mut_query = ctx.Query().Get<Position&>();

    mut_query.ForEach([](Position& pos) { pos.x = 10.0F; });

    // ReadOnlyQuery only allows const access
    auto const_query = ctx.ReadOnlyQuery().Get<const Position&>();

    const_query.ForEach([](const Position& pos) {
      CHECK_EQ(pos.x, 10.0F);  // Verify mutation from mutable query
    });
  }

  TEST_CASE("SystemContext: FrameAllocator Access") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    // Should be able to access frame allocator
    auto& alloc = ctx.FrameAllocator();

    // Allocator should start empty
    CHECK(alloc.Stats().total_allocated == 0);

    // Test allocation
    auto result = alloc.Allocate(128);
    CHECK(result.Valid());
    CHECK_GE(result.allocated_size, 128);

    // Stats should reflect allocation
    CHECK_GT(alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("SystemContext: FrameAllocator Stats") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    // Initial stats should show no allocations
    auto initial_stats = ctx.FrameAllocatorStats();
    CHECK_EQ(initial_stats.total_allocated, 0);
    CHECK_EQ(initial_stats.allocation_count, 0);

    // Make some allocations
    [[maybe_unused]] auto result1 = ctx.FrameAllocator().Allocate(256);
    [[maybe_unused]] auto result2 = ctx.FrameAllocator().Allocate(512);

    // Stats should be updated
    auto updated_stats = ctx.FrameAllocatorStats();
    CHECK_GT(updated_stats.total_allocated, 0);
    CHECK_EQ(updated_stats.allocation_count, 2);
  }

  TEST_CASE("SystemContext: MakeFrameAllocator Creates STL Allocator") {
    World world;
    auto e1 = world.CreateEntity();
    auto e2 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(e2, Position{4.0F, 5.0F, 6.0F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().Query<const Position&>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    // Create an STL allocator from the frame allocator
    auto alloc = ctx.MakeFrameAllocator<int>();

    // Use it with a vector
    std::vector<int, decltype(alloc)> temp{alloc};
    temp.push_back(42);
    temp.push_back(100);

    CHECK_EQ(temp.size(), 2);
    CHECK_EQ(temp[0], 42);
    CHECK_EQ(temp[1], 100);

    // Frame allocator should have recorded allocations
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }

  TEST_CASE("SystemContext: Query CollectWith Uses Custom Allocator") {
    World world;
    auto e1 = world.CreateEntity();
    auto e2 = world.CreateEntity();
    auto e3 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(e2, Position{4.0F, 5.0F, 6.0F});
    world.AddComponent(e3, Position{7.0F, 8.0F, 9.0F});

    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy().Query<const Position&>();
    auto info = CreateSystemInfo("TestSystem", policy);

    SystemContext ctx(world, info, executor, local_storage);

    // Use CollectWith with frame allocator
    using ValueType = std::tuple<const Position&>;
    auto alloc = ctx.MakeFrameAllocator<ValueType>();
    auto query = ctx.Query().Get<const Position&>();
    auto results = query.CollectWith(alloc);

    CHECK_EQ(results.size(), 3);

    // Verify frame allocator was used
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }

  TEST_CASE("SystemContext: Const Access To FrameAllocator") {
    World world;
    Executor executor;
    helios::ecs::details::SystemLocalStorage local_storage;
    auto policy = AccessPolicy();
    auto info = CreateSystemInfo("TestSystem", policy);

    const SystemContext ctx(world, info, executor, local_storage);

    // Const context should still allow reading allocator stats
    const auto& alloc = ctx.FrameAllocator();
    auto stats = alloc.Stats();
    CHECK_EQ(stats.total_allocated, 0);
  }

  TEST_CASE("SystemContext::ReadOnlyQuery: accessible from const context") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    auto e1 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F, 3.0F});

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<const Position&>();
    info.access_policy = policy;

    // Create const SystemContext
    const SystemContext const_ctx(world, info, executor, local_storage);

    // ReadOnlyQuery should be accessible from const context
    auto query = const_ctx.ReadOnlyQuery().Get<const Position&>();

    CHECK_EQ(query.Count(), 1);

    query.ForEach([](const Position& pos) {
      CHECK_EQ(pos.x, 1.0F);
      CHECK_EQ(pos.y, 2.0F);
      CHECK_EQ(pos.z, 3.0F);
    });
  }

  TEST_CASE("SystemContext::ReadOnlyQuery: supports functional adapters") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    for (int i = 0; i < 10; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<const Position&>();
    info.access_policy = policy;

    SystemContext ctx(world, info, executor, local_storage);

    auto query = ctx.ReadOnlyQuery().Get<const Position&>();

    // Test Filter
    size_t filtered_count = 0;
    for (auto&& _ : query.Filter([](const Position& p) { return p.x >= 5.0F; })) {
      ++filtered_count;
    }
    CHECK_EQ(filtered_count, 5);

    // Test Fold
    float sum = query.Fold(0.0F, [](float acc, const Position& p) { return acc + p.x; });
    CHECK_EQ(sum, 45.0F);  // 0 + 1 + 2 + ... + 9
  }

  TEST_CASE("SystemContext::ReadOnlyQuery: CollectWith uses custom allocator") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<const Position&>();
    info.access_policy = policy;

    SystemContext ctx(world, info, executor, local_storage);

    auto query = ctx.ReadOnlyQuery().Get<const Position&>();

    using ValueType = std::tuple<const Position&>;
    auto alloc = ctx.MakeFrameAllocator<ValueType>();
    auto results = query.CollectWith(alloc);

    CHECK_EQ(results.size(), 5);
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }

  TEST_CASE("SystemContext::Query: uses frame allocator internally") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    for (int i = 0; i < 3; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<Position&>();
    info.access_policy = policy;

    SystemContext ctx(world, info, executor, local_storage);

    // Query should use frame allocator for internal storage
    auto query = ctx.Query().Get<Position&>();

    CHECK_EQ(query.Count(), 3);

    // The frame allocator should have been used for query's internal vectors
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }

  TEST_CASE("SystemContext::Commands: uses frame allocator for command buffer") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    auto entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    CHECK_EQ(world.EntityCount(), 1);

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    info.access_policy = policy;

    SystemContext ctx(world, info, executor, local_storage);

    {
      auto cmd = ctx.Commands();
      cmd.Destroy(entity);

      // Verify command buffer has the command
      CHECK_EQ(cmd.Size(), 1);
    }

    // Commands should be flushed to local storage after scope ends
    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 0);

    // Frame allocator should have been used
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }

  TEST_CASE("SystemContext::EntityCommands: uses frame allocator for command buffer") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    auto entity = world.CreateEntity();

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    info.access_policy = policy;

    SystemContext ctx(world, info, executor, local_storage);

    {
      auto cmd = ctx.EntityCommands(entity);
      cmd.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd.AddComponent(Velocity{4.0F, 5.0F, 6.0F});

      // Verify command buffer has the commands
      CHECK_EQ(cmd.Size(), 2);
      CHECK_EQ(cmd.GetEntity(), entity);
    }

    // Commands should be flushed to local storage after scope ends
    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));

    // Frame allocator should have been used
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }

  TEST_CASE("SystemContext: Multiple Queries use frame allocator") {
    World world;
    helios::ecs::details::SystemLocalStorage local_storage;
    Executor executor;

    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
      if (i % 2 == 0) {
        world.AddComponent(e, Velocity{1.0F, 1.0F, 1.0F});
      }
    }

    helios::app::details::SystemInfo info;
    info.name = "TestSystem";
    AccessPolicy policy;
    policy.Query<const Position&>();
    policy.Query<const Velocity&>();
    info.access_policy = policy;

    SystemContext ctx(world, info, executor, local_storage);

    // Create multiple queries - all should use the same frame allocator
    auto query1 = ctx.Query().Get<const Position&>();
    auto query2 = ctx.Query().With<Velocity>().Get<const Position&>();

    CHECK_EQ(query1.Count(), 5);
    CHECK_EQ(query2.Count(), 3);

    // Multiple allocations should have been made
    CHECK_GT(ctx.FrameAllocatorStats().total_allocated, 0);
  }
}  // TEST_SUITE
