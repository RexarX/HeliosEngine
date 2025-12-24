#include <doctest/doctest.h>

#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity_command_buffer.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using namespace helios::ecs;

namespace {

// Test component types
struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Name {
  std::string value;

  constexpr bool operator==(const Name& other) const noexcept = default;
};

struct Health {
  int points = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

struct TagComponent {};

}  // namespace

TEST_SUITE("ecs::EntityCommandBuffer") {
  TEST_CASE("EntityCmdBuffer: Construction with New Entity") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();
      CHECK(entity.Valid());
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 1);
  }

  TEST_CASE("EntityCmdBuffer: Construction with Existing Entity") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    CHECK(world.Exists(entity));

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      CHECK_EQ(cmd_buffer.GetEntity(), entity);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK(world.Exists(entity));
  }

  TEST_CASE("EntityCmdBuffer: AddComponent") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.AddComponent(Velocity{4.0F, 5.0F, 6.0F});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("EntityCmdBuffer: AddComponent Move Semantics") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      Name name{"TestEntity"};
      cmd_buffer.AddComponent(std::move(name));

      CHECK(name.value.empty());  // Should be moved from
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Name>(entity));
  }

  TEST_CASE("EntityCmdBuffer: AddComponents Multiple") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      Position pos{1.0F, 2.0F, 3.0F};
      Velocity vel{4.0F, 5.0F, 6.0F};
      Health health{100};

      cmd_buffer.AddComponents(std::move(pos), std::move(vel), std::move(health));
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryAddComponent Success") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      Position pos{1.0F, 2.0F, 3.0F};
      cmd_buffer.TryAddComponent(std::move(pos));
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryAddComponent Failure") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);

      Position pos{4.0F, 5.0F, 6.0F};
      cmd_buffer.TryAddComponent(std::move(pos));  // No effect (already has Position)
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Original component should remain
    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryAddComponents Mixed") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);

      Position pos{4.0F, 5.0F, 6.0F};
      Velocity vel{7.0F, 8.0F, 9.0F};
      Health health{100};

      cmd_buffer.TryAddComponents(std::move(pos), std::move(vel), std::move(health));
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("EntityCmdBuffer: EmplaceComponent") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      cmd_buffer.EmplaceComponent<Position>(1.0F, 2.0F, 3.0F);
      cmd_buffer.EmplaceComponent<Health>(150);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryEmplaceComponent Success") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      cmd_buffer.TryEmplaceComponent<Position>(1.0F, 2.0F, 3.0F);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryEmplaceComponent Failure") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);

      cmd_buffer.TryEmplaceComponent<Position>(4.0F, 5.0F, 6.0F);  // No effect (already has Position)
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("EntityCmdBuffer: RemoveComponent") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity(4.0F, 5.0F, 6.0F));
    world.AddComponent(entity, Health(100));

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.RemoveComponent<Position>();
      cmd_buffer.RemoveComponent<Velocity>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));  // Should remain
  }

  TEST_CASE("EntityCmdBuffer: RemoveComponents Multiple") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Health{100});
    world.AddComponent(entity, Name{"TestEntity"});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.RemoveComponents<Position, Velocity>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
    CHECK(world.HasComponent<Name>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryRemoveComponent Success") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);

      cmd_buffer.TryRemoveComponent<Position>();
      cmd_buffer.TryRemoveComponent<Health>();  // No effect (doesn't exist)
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("EntityCmdBuffer: TryRemoveComponents Mixed") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);

      cmd_buffer.TryRemoveComponents<Position, Health, Velocity>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("EntityCmdBuffer: ClearComponents") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Health{100});
    world.AddComponent(entity, Name{"TestEntity"});
    world.AddComponent(entity, TagComponent{});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.ClearComponents();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));
    CHECK_FALSE(world.HasComponent<Name>(entity));
    CHECK_FALSE(world.HasComponent<TagComponent>(entity));
    CHECK(world.Exists(entity));  // Entity should still exist
  }

  TEST_CASE("EntityCmdBuffer: Destroy") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Health{100});

    CHECK(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 1);

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.Destroy();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("EntityCmdBuffer: TryDestroy Success") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.TryDestroy();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("EntityCmdBuffer: Mixed Operations") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      // Add initial components
      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.EmplaceComponent<Velocity>(4.0F, 5.0F, 6.0F);
      cmd_buffer.AddComponent(Health{100});
      cmd_buffer.AddComponent(Name{"TestEntity"});
      cmd_buffer.AddComponent(TagComponent{});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
    CHECK(world.HasComponent<Name>(entity));
    CHECK(world.HasComponent<TagComponent>(entity));

    local_storage.Clear();

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);

      // Remove some components and add others
      cmd_buffer.RemoveComponent<TagComponent>();
      cmd_buffer.RemoveComponents<Velocity, Name>();
      cmd_buffer.TryAddComponent(Name{"ModifiedEntity"});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
    CHECK(world.HasComponent<Name>(entity));
    CHECK_FALSE(world.HasComponent<TagComponent>(entity));
  }

  TEST_CASE("EntityCmdBuffer: Component Operations with Tag Components") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      cmd_buffer.AddComponent(TagComponent{});
      cmd_buffer.EmplaceComponent<Position>(1.0F, 2.0F, 3.0F);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<TagComponent>(entity));
    CHECK(world.HasComponent<Position>(entity));

    local_storage.Clear();

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.TryRemoveComponent<TagComponent>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_FALSE(world.HasComponent<TagComponent>(entity));
    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("EntityCmdBuffer: Multiple Command Buffers") {
    World world;
    details::SystemLocalStorage local_storage;

    std::vector<Entity> entities;

    // Create multiple entities using separate command buffers
    for (int i = 0; i < 5; ++i) {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      Entity entity = cmd_buffer.GetEntity();
      entities.push_back(entity);

      cmd_buffer.AddComponent(Position{static_cast<float>(i), 0.0F, 0.0F});
      cmd_buffer.EmplaceComponent<Health>(100 + i * 10);

      if (i % 2 == 0) {
        cmd_buffer.AddComponent(Velocity{1.0F, 0.0F, 0.0F});
      }
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(world.EntityCount(), 5);

    for (size_t i = 0; i < entities.size(); ++i) {
      CHECK(world.Exists(entities[i]));
      CHECK(world.HasComponent<Position>(entities[i]));
      CHECK(world.HasComponent<Health>(entities[i]));

      if (i % 2 == 0) {
        CHECK(world.HasComponent<Velocity>(entities[i]));
      } else {
        CHECK_FALSE(world.HasComponent<Velocity>(entities[i]));
      }
    }
  }

  TEST_CASE("EntityCmdBuffer: Existing Entity Operations") {
    World world;
    details::SystemLocalStorage local_storage;

    // Create entities directly
    std::vector<Entity> entities;
    for (int i = 0; i < 3; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
      entities.push_back(entity);
    }

    // Modify entities using command buffers
    {
      EntityCmdBuffer cmd_buffer1(entities[0], local_storage);
      cmd_buffer1.AddComponent(Velocity{1.0F, 0.0F, 0.0F});
      cmd_buffer1.AddComponent(Health{100});
    }

    {
      EntityCmdBuffer cmd_buffer2(entities[1], local_storage);
      cmd_buffer2.RemoveComponent<Position>();
      cmd_buffer2.AddComponent(Name{"Entity1"});
    }

    {
      EntityCmdBuffer cmd_buffer3(entities[2], local_storage);
      cmd_buffer3.ClearComponents();
      cmd_buffer3.AddComponent(TagComponent{});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Verify modifications
    CHECK(world.HasComponent<Position>(entities[0]));
    CHECK(world.HasComponent<Velocity>(entities[0]));
    CHECK(world.HasComponent<Health>(entities[0]));

    CHECK_FALSE(world.HasComponent<Position>(entities[1]));
    CHECK(world.HasComponent<Name>(entities[1]));

    CHECK_FALSE(world.HasComponent<Position>(entities[2]));
    CHECK(world.HasComponent<TagComponent>(entities[2]));
  }

  TEST_CASE("EntityCmdBuffer: GetEntity Consistency") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();

      // GetEntity should return the same entity consistently
      Entity same_entity = cmd_buffer.GetEntity();
      CHECK_EQ(entity, same_entity);
      CHECK_EQ(entity.Index(), same_entity.Index());
      CHECK_EQ(entity.Generation(), same_entity.Generation());
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
  }

  TEST_CASE("EntityCmdBuffer: Empty Command Buffer") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();
      // Don't add any commands
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    // Entity should still be created even without any component operations
    CHECK(world.Exists(entity));
    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("EntityCmdBuffer: Multiple Flush Cycles") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      EntityCmdBuffer cmd_buffer(world, local_storage);
      entity = cmd_buffer.GetEntity();
      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK(world.HasComponent<Position>(entity));

    local_storage.Clear();

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.AddComponent(Velocity{4.0F, 5.0F, 6.0F});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK(world.HasComponent<Velocity>(entity));

    local_storage.Clear();

    {
      EntityCmdBuffer cmd_buffer(entity, local_storage);
      cmd_buffer.RemoveComponent<Position>();
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("EntityCmdBuffer: Custom Allocator Support") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();

    CHECK_EQ(world.EntityCount(), 1);
    CHECK_FALSE(world.HasComponent<Position>(entity));

    SUBCASE("With frame allocator") {
      helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

      using CommandAlloc =
          helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

      {
        EntityCmdBuffer<CommandAlloc> cmd_buffer(entity, local_storage, CommandAlloc(frame_alloc));
        cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});

        // Verify commands are buffered locally
        CHECK_EQ(cmd_buffer.Size(), 1);
        CHECK_FALSE(cmd_buffer.Empty());
        CHECK_EQ(cmd_buffer.GetEntity(), entity);
      }

      // After scope ends, commands should be flushed
      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));

      // Verify frame allocator was used
      CHECK_GT(frame_alloc.Stats().total_allocated, 0);
    }

    SUBCASE("Explicit Flush") {
      helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

      using CommandAlloc =
          helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

      EntityCmdBuffer<CommandAlloc> cmd_buffer(entity, local_storage, CommandAlloc(frame_alloc));
      cmd_buffer.AddComponent(Velocity{4.0F, 5.0F, 6.0F});

      CHECK_EQ(cmd_buffer.Size(), 1);

      // Explicit flush
      cmd_buffer.Flush();

      CHECK_EQ(cmd_buffer.Size(), 0);
      CHECK(cmd_buffer.Empty());

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Velocity>(entity));
    }
  }

  TEST_CASE("EntityCmdBuffer: Multiple Commands with Custom Allocator") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    {
      EntityCmdBuffer<CommandAlloc> cmd_buffer(entity, local_storage, CommandAlloc(frame_alloc));

      // Add multiple commands
      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.AddComponent(Velocity{4.0F, 5.0F, 6.0F});
      cmd_buffer.AddComponent(Health{100});

      CHECK_EQ(cmd_buffer.Size(), 3);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("EntityCmdBuffer: get_allocator") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    CommandAlloc alloc(frame_alloc);
    EntityCmdBuffer<CommandAlloc> cmd_buffer(entity, local_storage, alloc);

    auto retrieved_alloc = cmd_buffer.get_allocator();

    // Both allocators should point to the same underlying frame allocator
    CHECK_EQ(retrieved_alloc.get_allocator(), alloc.get_allocator());
  }

  TEST_CASE("EntityCmdBuffer: Move Semantics") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    {
      EntityCmdBuffer<CommandAlloc> cmd_buffer1(entity, local_storage, CommandAlloc(frame_alloc));
      cmd_buffer1.AddComponent(Position{1.0F, 2.0F, 3.0F});

      CHECK_EQ(cmd_buffer1.Size(), 1);

      // Move the command buffer
      EntityCmdBuffer<CommandAlloc> cmd_buffer2 = std::move(cmd_buffer1);

      // cmd_buffer2 should have the command now
      CHECK_EQ(cmd_buffer2.Size(), 1);
      CHECK_EQ(cmd_buffer2.GetEntity(), entity);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("EntityCmdBuffer: Reserve Entity with Custom Allocator") {
    World world;
    details::SystemLocalStorage local_storage;

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    Entity reserved_entity;

    {
      EntityCmdBuffer<CommandAlloc> cmd_buffer(world, local_storage, CommandAlloc(frame_alloc));
      reserved_entity = cmd_buffer.GetEntity();

      cmd_buffer.AddComponent(Position{10.0F, 20.0F, 30.0F});
      cmd_buffer.AddComponent(Name{"Reserved Entity"});

      CHECK_EQ(cmd_buffer.Size(), 2);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(reserved_entity));
    CHECK(world.HasComponent<Position>(reserved_entity));
    CHECK(world.HasComponent<Name>(reserved_entity));
  }

  TEST_CASE("EntityCmdBuffer: FromWorld static factory") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity;
    {
      auto cmd_buffer = EntityCmdBuffer<>::FromWorld(world, local_storage);
      entity = cmd_buffer.GetEntity();
      CHECK(entity.Valid());

      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.AddComponent(Velocity{4.0F, 5.0F, 6.0F});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("EntityCmdBuffer: FromEntity static factory") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    CHECK(world.Exists(entity));

    {
      auto cmd_buffer = EntityCmdBuffer<>::FromEntity(entity, local_storage);
      CHECK_EQ(cmd_buffer.GetEntity(), entity);

      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.AddComponent(Health{100});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("EntityCmdBuffer: FromWorld with custom allocator") {
    World world;
    details::SystemLocalStorage local_storage;

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    Entity entity;
    {
      auto cmd_buffer = EntityCmdBuffer<CommandAlloc>::FromWorld(world, local_storage, CommandAlloc(frame_alloc));
      entity = cmd_buffer.GetEntity();
      CHECK(entity.Valid());

      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.AddComponent(Name{"FromWorld Entity"});
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Name>(entity));
  }

  TEST_CASE("EntityCmdBuffer: FromEntity with custom allocator") {
    World world;
    details::SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();

    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using CommandAlloc = helios::memory::STLGrowableAllocator<std::unique_ptr<Command>, helios::memory::FrameAllocator>;

    {
      auto cmd_buffer = EntityCmdBuffer<CommandAlloc>::FromEntity(entity, local_storage, CommandAlloc(frame_alloc));
      CHECK_EQ(cmd_buffer.GetEntity(), entity);

      cmd_buffer.AddComponent(Position{1.0F, 2.0F, 3.0F});
      cmd_buffer.AddComponent(Velocity{4.0F, 5.0F, 6.0F});
      cmd_buffer.AddComponent(Health{100});

      CHECK_EQ(cmd_buffer.Size(), 3);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

}  // TEST_SUITE
