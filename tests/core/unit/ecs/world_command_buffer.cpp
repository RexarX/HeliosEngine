#include <doctest/doctest.h>

#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/query.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/ecs/world_command_buffer.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

using namespace helios::ecs;

namespace {

// Test components
struct Position {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Health {
  int value = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

struct Tag {};

}  // namespace

TEST_SUITE("ecs::WorldCommandBuffer") {
  TEST_CASE("WorldCmdBuffer: Basic Construction") {
    details::SystemLocalStorage local_storage;

    SUBCASE("Construct with world reference") {
      WorldCmdBuffer cmd_buffer(local_storage);
      // Should construct without issues
    }
  }

  TEST_CASE("WorldCmdBuffer: Push Function Commands") {
    World world;
    details::SystemLocalStorage local_storage;
    std::atomic<int> execution_counter = 0;

    SUBCASE("Simple function command") {
      {
        WorldCmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Push([&execution_counter](World& /*world*/) { execution_counter++; });
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(execution_counter.load(), 1);
    }

    SUBCASE("Multiple function commands") {
      std::vector<int> execution_order;

      {
        WorldCmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Push([&execution_order](World& /*world*/) { execution_order.push_back(1); });
        cmd_buffer.Push([&execution_order](World& /*world*/) { execution_order.push_back(2); });
        cmd_buffer.Push([&execution_order](World& /*world*/) { execution_order.push_back(3); });
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(execution_order.size(), 3);
      CHECK_EQ(execution_order[0], 1);
      CHECK_EQ(execution_order[1], 2);
      CHECK_EQ(execution_order[2], 3);
    }

    SUBCASE("Function command with world manipulation") {
      Entity created_entity;

      {
        WorldCmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Push([&created_entity](World& w) {
          created_entity = w.CreateEntity();
          w.AddComponent(created_entity, Position(10.0F, 20.0F));
          w.AddComponent(created_entity, Health(50));
        });
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.Exists(created_entity));
      CHECK(world.HasComponent<Position>(created_entity));
      CHECK(world.HasComponent<Health>(created_entity));
    }
  }

  TEST_CASE("WorldCmdBuffer: Destroy Single Entity") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F});
    world.AddComponent(entity, Velocity{3.0F, 4.0F});

    CHECK(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 1);

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Destroy(entity);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("WorldCmdBuffer: Destroy Multiple Entities") {
    World world;
    details::SystemLocalStorage local_storage;

    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), static_cast<float>(i)});
      entities.push_back(entity);
    }

    CHECK_EQ(world.EntityCount(), 5);

    std::vector<Entity> to_destroy = {entities[1], entities[3]};

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Destroy(to_destroy);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 3);
    CHECK(world.Exists(entities[0]));
    CHECK_FALSE(world.Exists(entities[1]));
    CHECK(world.Exists(entities[2]));
    CHECK_FALSE(world.Exists(entities[3]));
    CHECK(world.Exists(entities[4]));
  }

  TEST_CASE("WorldCmdBuffer: TryDestroy Single Entity Success") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F});

    CHECK(world.Exists(entity));

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.TryDestroy(entity);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("WorldCmdBuffer: TryDestroy Single Entity Non-existent") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.DestroyEntity(entity);

    CHECK_FALSE(world.Exists(entity));

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.TryDestroy(entity);  // Should be no-op
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("WorldCmdBuffer: TryDestroy Multiple Entities Mixed") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F});
    world.AddComponent(entity2, Position{3.0F, 4.0F});
    world.AddComponent(entity3, Position{5.0F, 6.0F});

    // Destroy one entity beforehand
    world.DestroyEntity(entity2);

    CHECK(world.Exists(entity1));
    CHECK_FALSE(world.Exists(entity2));
    CHECK(world.Exists(entity3));

    std::vector<Entity> to_try_destroy = {entity1, entity2, entity3};

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.TryDestroy(to_try_destroy);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.Exists(entity1));
    CHECK_FALSE(world.Exists(entity2));
    CHECK_FALSE(world.Exists(entity3));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("WorldCmdBuffer: Mixed Operations") {
    World world;
    details::SystemLocalStorage local_storage;

    std::vector<Entity> entities;
    for (int i = 0; i < 3; ++i) {
      entities.push_back(world.CreateEntity());
      world.AddComponent(entities[i], Position{static_cast<float>(i), 0.0F});
    }

    std::atomic<int> execution_count = 0;

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      // Function command
      cmd_buffer.Push([&execution_count](World& /*world*/) { execution_count++; });

      // Destroy entity
      cmd_buffer.Destroy(entities[0]);

      // Another function command
      cmd_buffer.Push([&execution_count, entity = entities[1]](World& w) {
        execution_count++;
        w.AddComponent(entity, Velocity{10.0F, 20.0F});
      });

      // Try destroy (might not exist after previous commands)
      cmd_buffer.TryDestroy(entities[2]);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(execution_count.load(), 2);
    CHECK_FALSE(world.Exists(entities[0]));
    CHECK(world.Exists(entities[1]));
    CHECK(world.HasComponent<Velocity>(entities[1]));
    CHECK_FALSE(world.Exists(entities[2]));
  }

  TEST_CASE("WorldCmdBuffer: Command Execution Order") {
    World world;
    details::SystemLocalStorage local_storage;

    std::vector<int> execution_order;

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      cmd_buffer.Push([&execution_order](World& /*world*/) { execution_order.push_back(1); });

      Entity entity = world.CreateEntity();
      cmd_buffer.Destroy(entity);

      cmd_buffer.Push([&execution_order](World& /*world*/) { execution_order.push_back(2); });

      cmd_buffer.Push([&execution_order](World& /*world*/) { execution_order.push_back(3); });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Commands should execute in the order they were added
    CHECK_EQ(execution_order.size(), 3);
    CHECK_EQ(execution_order[0], 1);
    CHECK_EQ(execution_order[1], 2);
    CHECK_EQ(execution_order[2], 3);
  }

  TEST_CASE("WorldCmdBuffer: Nested Entity Creation and Destruction") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity1 = world.CreateEntity();
    world.AddComponent(entity1, Position{1.0F, 2.0F});

    Entity created_entity;

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      cmd_buffer.Destroy(entity1);

      cmd_buffer.Push([&created_entity](World& w) {
        created_entity = w.CreateEntity();
        w.AddComponent(created_entity, Position{10.0F, 20.0F});
        w.AddComponent(created_entity, Velocity{5.0F, 5.0F});
      });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.Exists(entity1));
    CHECK(world.Exists(created_entity));
    CHECK(world.HasComponent<Position>(created_entity));
    CHECK(world.HasComponent<Velocity>(created_entity));
  }

  TEST_CASE("WorldCmdBuffer: Large Batch Destroy") {
    World world;
    details::SystemLocalStorage local_storage;

    constexpr size_t entity_count = 100;
    std::vector<Entity> entities;

    for (size_t i = 0; i < entity_count; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F});
      entities.push_back(entity);
    }

    CHECK_EQ(world.EntityCount(), entity_count);

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Destroy(entities);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 0);
    for (const auto& entity : entities) {
      CHECK_FALSE(world.Exists(entity));
    }
  }

  TEST_CASE("WorldCmdBuffer: Empty Command Buffer") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F});

    size_t initial_count = world.EntityCount();

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      // Don't add any commands
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), initial_count);
    CHECK(world.Exists(entity));
    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("WorldCmdBuffer: Multiple Buffer Scopes") {
    World world;
    details::SystemLocalStorage local_storage;

    std::vector<int> execution_order;

    {
      WorldCmdBuffer cmd_buffer1(local_storage);
      cmd_buffer1.Push([&execution_order](World& /*world*/) { execution_order.push_back(1); });
    }

    {
      WorldCmdBuffer cmd_buffer2(local_storage);
      cmd_buffer2.Push([&execution_order](World& /*world*/) { execution_order.push_back(2); });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(execution_order.size(), 2);
    CHECK(std::ranges::find(execution_order, 1) != execution_order.end());
    CHECK(std::ranges::find(execution_order, 2) != execution_order.end());
  }

  TEST_CASE("WorldCmdBuffer: Function with Complex World Manipulation") {
    World world;
    details::SystemLocalStorage local_storage;

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      cmd_buffer.Push([](World& w) {
        // Create multiple entities
        for (int i = 0; i < 5; ++i) {
          Entity entity = w.CreateEntity();
          w.AddComponent(entity, Position{static_cast<float>(i * 10), 0.0F});

          if (i % 2 == 0) {
            w.AddComponent(entity, Velocity{1.0F, 1.0F});
          }

          if (i == 2) {
            w.AddComponent(entity, Health{100});
          }
        }
      });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 5);

    // Verify entities have correct components
    auto query = QueryBuilder(world).Get<Position&>();
    size_t position_count = 0;
    for ([[maybe_unused]] auto&& [pos] : query) {
      position_count++;
    }
    CHECK_EQ(position_count, 5);
  }

  TEST_CASE("WorldCmdBuffer: Interleaved Commands and Direct Operations") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity1 = world.CreateEntity();
    world.AddComponent(entity1, Position{1.0F, 2.0F});

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      // Queue command
      cmd_buffer.Push([entity1](World& w) { w.AddComponent(entity1, Velocity{5.0F, 5.0F}); });
    }

    // Direct operation before flushing
    Entity entity2 = world.CreateEntity();
    world.AddComponent(entity2, Position{10.0F, 20.0F});

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity1));
    CHECK(world.Exists(entity2));
    CHECK(world.HasComponent<Velocity>(entity1));
    CHECK(world.HasComponent<Position>(entity2));
  }

  TEST_CASE("WorldCmdBuffer: Multiple Flush Cycles") {
    World world;
    details::SystemLocalStorage local_storage;

    std::atomic<int> counter = 0;

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Push([&counter](World& /*world*/) { counter++; });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK_EQ(counter.load(), 1);

    local_storage.Clear();

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Push([&counter](World& /*world*/) { counter++; });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK_EQ(counter.load(), 2);

    local_storage.Clear();

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Push([&counter](World& /*world*/) { counter++; });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK_EQ(counter.load(), 3);
  }

  TEST_CASE("WorldCmdBuffer: Destroy with Query") {
    World world;
    details::SystemLocalStorage local_storage;

    // Create entities with different component combinations
    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F});

      if (i % 2 == 0) {
        world.AddComponent(entity, Velocity{1.0F, 1.0F});
      }
    }

    CHECK_EQ(world.EntityCount(), 10);

    // Collect entities with Velocity to destroy
    std::vector<Entity> entities_to_destroy;
    {
      auto query = QueryBuilder(world).Get<const Velocity&>();
      for (auto&& [entity, velocity] : query.WithEntity()) {
        entities_to_destroy.push_back(entity);
      }
    }

    CHECK_EQ(entities_to_destroy.size(), 5);

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Destroy(entities_to_destroy);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 5);

    // Verify remaining entities don't have Velocity
    auto query = QueryBuilder(world).Get<const Velocity&>();
    CHECK_EQ(query.Count(), 0);
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents Single Type") {
    World world;
    details::SystemLocalStorage local_storage;

    // Define test event types
    struct TestEvent1 {
      int value = 0;
    };

    struct TestEvent2 {
      float data = 0.0F;
    };

    world.AddEvent<TestEvent1>();
    world.AddEvent<TestEvent2>();

    // Emit events
    auto writer1 = world.WriteEvents<TestEvent1>();
    auto writer2 = world.WriteEvents<TestEvent2>();
    writer1.Write(TestEvent1{42});
    writer1.Write(TestEvent1{100});
    writer2.Write(TestEvent2{3.14F});

    // Verify events exist
    auto reader1 = world.ReadEvents<TestEvent1>();
    auto reader2 = world.ReadEvents<TestEvent2>();
    auto events1_before = reader1.Collect();
    auto events2_before = reader2.Collect();
    CHECK_EQ(events1_before.size(), 2);
    CHECK_EQ(events2_before.size(), 1);

    // Clear only TestEvent1
    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.ClearEvents<TestEvent1>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Verify TestEvent1 cleared but TestEvent2 remains
    auto reader1_after = world.ReadEvents<TestEvent1>();
    auto reader2_after = world.ReadEvents<TestEvent2>();
    auto events1_after = reader1_after.Collect();
    auto events2_after = reader2_after.Collect();
    CHECK_EQ(events1_after.size(), 0);
    CHECK_EQ(events2_after.size(), 1);
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents All Types") {
    World world;
    details::SystemLocalStorage local_storage;

    // Define test event types
    struct TestEvent1 {
      int value = 0;
    };

    struct TestEvent2 {
      float data = 0.0F;
    };

    struct TestEvent3 {
      bool flag = false;
    };

    world.AddEvent<TestEvent1>();
    world.AddEvent<TestEvent2>();
    world.AddEvent<TestEvent3>();

    // Emit multiple events of different types
    auto writer_e1 = world.WriteEvents<TestEvent1>();
    auto writer_e2 = world.WriteEvents<TestEvent2>();
    auto writer_e3 = world.WriteEvents<TestEvent3>();
    writer_e1.Write(TestEvent1{1});
    writer_e1.Write(TestEvent1{2});
    writer_e2.Write(TestEvent2{1.5F});
    writer_e2.Write(TestEvent2{2.5F});
    writer_e3.Write(TestEvent3{true});

    // Verify events exist
    CHECK_EQ(world.ReadEvents<TestEvent1>().Count(), 2);
    CHECK_EQ(world.ReadEvents<TestEvent2>().Count(), 2);
    CHECK_EQ(world.ReadEvents<TestEvent3>().Count(), 1);

    // Clear all events
    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.ClearEvents();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Verify all events cleared
    CHECK_EQ(world.ReadEvents<TestEvent1>().Count(), 0);
    CHECK_EQ(world.ReadEvents<TestEvent2>().Count(), 0);
    CHECK_EQ(world.ReadEvents<TestEvent3>().Count(), 0);
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents Empty Queue") {
    World world;
    details::SystemLocalStorage local_storage;

    struct TestEvent {
      int value = 0;
    };

    world.AddEvent<TestEvent>();

    // Clear events when queue is empty (should be no-op)
    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.ClearEvents<TestEvent>();
      cmd_buffer.ClearEvents();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Should not crash or cause issues
    CHECK_EQ(world.ReadEvents<TestEvent>().Count(), 0);
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents Multiple Times") {
    World world;
    details::SystemLocalStorage local_storage;

    struct TestEvent {
      int value = 0;
    };

    world.AddEvent<TestEvent>();

    // Emit events
    auto test_writer = world.WriteEvents<TestEvent>();
    test_writer.Write(TestEvent{10});
    test_writer.Write(TestEvent{20});

    CHECK_EQ(world.ReadEvents<TestEvent>().Count(), 2);

    // Clear events multiple times
    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.ClearEvents<TestEvent>();
      cmd_buffer.ClearEvents<TestEvent>();
      cmd_buffer.ClearEvents();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.ReadEvents<TestEvent>().Count(), 0);
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents Mixed with Other Commands") {
    World world;
    details::SystemLocalStorage local_storage;

    struct TestEvent {
      int value = 0;
    };

    world.AddEvent<TestEvent>();

    // Emit events
    auto evt_writer = world.WriteEvents<TestEvent>();
    evt_writer.Write(TestEvent{1});
    evt_writer.Write(TestEvent{2});
    evt_writer.Write(TestEvent{3});

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F});

    std::atomic<int> counter = 0;

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      // Mix different command types
      cmd_buffer.Push([&counter](World& /*world*/) { counter++; });
      cmd_buffer.ClearEvents<TestEvent>();
      cmd_buffer.Destroy(entity);
      cmd_buffer.Push([&counter](World& /*world*/) { counter++; });
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Verify all commands executed
    CHECK_EQ(counter.load(), 2);
    CHECK_EQ(world.ReadEvents<TestEvent>().Count(), 0);
    CHECK_FALSE(world.Exists(entity));
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents Before and After Emission") {
    World world;
    details::SystemLocalStorage local_storage;

    struct TestEvent {
      int value = 0;
    };

    world.AddEvent<TestEvent>();

    // First batch of events
    auto batch1_writer = world.WriteEvents<TestEvent>();
    batch1_writer.Write(TestEvent{1});
    batch1_writer.Write(TestEvent{2});

    {
      WorldCmdBuffer cmd_buffer(local_storage);
      cmd_buffer.ClearEvents<TestEvent>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.ReadEvents<TestEvent>().Count(), 0);

    // Second batch of events after clearing
    auto batch2_writer = world.WriteEvents<TestEvent>();
    batch2_writer.Write(TestEvent{10});
    batch2_writer.Write(TestEvent{20});
    batch2_writer.Write(TestEvent{30});

    auto event_reader = world.ReadEvents<TestEvent>();
    auto events = event_reader.Collect();
    CHECK_EQ(events.size(), 3);
    CHECK_EQ(events[0].value, 10);
    CHECK_EQ(events[1].value, 20);
    CHECK_EQ(events[2].value, 30);
  }

  TEST_CASE("WorldCmdBuffer: ClearEvents Selective Type Clearing") {
    World world;
    details::SystemLocalStorage local_storage;

    struct EventA {
      int a = 0;
    };

    struct EventB {
      int b = 0;
    };

    struct EventC {
      int c = 0;
    };

    world.AddEvent<EventA>();
    world.AddEvent<EventB>();
    world.AddEvent<EventC>();

    // Emit multiple event types
    auto writer_a = world.WriteEvents<EventA>();
    auto writer_b = world.WriteEvents<EventB>();
    auto writer_c = world.WriteEvents<EventC>();
    writer_a.Write(EventA{1});
    writer_b.Write(EventB{2});
    writer_c.Write(EventC{3});
    writer_a.Write(EventA{4});
    writer_b.Write(EventB{5});

    {
      WorldCmdBuffer cmd_buffer(local_storage);

      // Clear only EventA and EventC, leave EventB
      cmd_buffer.ClearEvents<EventA>();
      cmd_buffer.ClearEvents<EventC>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Verify selective clearing
    CHECK_EQ(world.ReadEvents<EventA>().Count(), 0);
    auto reader_b = world.ReadEvents<EventB>();
    CHECK_EQ(reader_b.Count(), 2);
    CHECK_EQ(world.ReadEvents<EventC>().Count(), 0);

    // Verify EventB values using range-based for loop
    std::vector<int> b_values;
    for (const auto& event : reader_b) {
      b_values.push_back(event.b);
    }
    CHECK_EQ(b_values[0], 2);
    CHECK_EQ(b_values[1], 5);
  }

  TEST_CASE("WorldCmdBuffer: Custom Allocator Support") {
    World world;
    details::SystemLocalStorage local_storage;

    // Create entities
    Entity e1 = world.CreateEntity();
    Entity e2 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F});
    world.AddComponent(e2, Position{3.0F, 4.0F});

    CHECK_EQ(world.EntityCount(), 2);

    SUBCASE("With frame allocator") {
      helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

      using CommandAlloc =
          helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

      {
        WorldCmdBuffer<CommandAlloc> cmd_buffer(local_storage, CommandAlloc(frame_alloc));
        cmd_buffer.Destroy(e1);

        // Verify commands are buffered locally
        CHECK_EQ(cmd_buffer.Size(), 1);
        CHECK_FALSE(cmd_buffer.Empty());
      }

      // After scope ends, commands should be flushed
      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(world.EntityCount(), 1);

      // Verify frame allocator was used
      CHECK_GT(frame_alloc.Stats().total_allocated, 0);
    }

    SUBCASE("Explicit Flush") {
      helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

      using CommandAlloc =
          helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

      WorldCmdBuffer<CommandAlloc> cmd_buffer(local_storage, CommandAlloc(frame_alloc));
      cmd_buffer.Destroy(e2);

      CHECK_EQ(cmd_buffer.Size(), 1);

      // Explicit flush
      cmd_buffer.Flush();

      CHECK_EQ(cmd_buffer.Size(), 0);
      CHECK(cmd_buffer.Empty());

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(world.EntityCount(), 1);
    }
  }

  TEST_CASE("WorldCmdBuffer: Multiple Commands with Custom Allocator") {
    World world;
    details::SystemLocalStorage local_storage;

    // Create entities
    for (int i = 0; i < 5; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F});
    }

    CHECK_EQ(world.EntityCount(), 5);

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    std::atomic<int> counter = 0;

    {
      WorldCmdBuffer<CommandAlloc> cmd_buffer(local_storage, CommandAlloc(frame_alloc));

      // Add multiple commands of different types
      cmd_buffer.Push([&counter](World& /*w*/) { counter++; });
      cmd_buffer.Push([&counter](World& /*w*/) { counter++; });
      cmd_buffer.Push([&counter](World& /*w*/) { counter++; });

      CHECK_EQ(cmd_buffer.Size(), 3);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(counter.load(), 3);
  }

  TEST_CASE("WorldCmdBuffer: get_allocator") {
    details::SystemLocalStorage local_storage;

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    CommandAlloc alloc(frame_alloc);
    WorldCmdBuffer<CommandAlloc> cmd_buffer(local_storage, alloc);

    auto retrieved_alloc = cmd_buffer.get_allocator();

    // Both allocators should point to the same underlying frame allocator
    CHECK_EQ(retrieved_alloc.get_allocator(), alloc.get_allocator());
  }

  TEST_CASE("WorldCmdBuffer: Move Semantics") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F});

    CHECK_EQ(world.EntityCount(), 1);

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    {
      WorldCmdBuffer<CommandAlloc> cmd_buffer1(local_storage, CommandAlloc(frame_alloc));
      cmd_buffer1.Destroy(entity);

      CHECK_EQ(cmd_buffer1.Size(), 1);

      // Move the command buffer
      WorldCmdBuffer<CommandAlloc> cmd_buffer2 = std::move(cmd_buffer1);

      // cmd_buffer2 should have the command now
      CHECK_EQ(cmd_buffer2.Size(), 1);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 0);
  }

}  // TEST_SUITE
