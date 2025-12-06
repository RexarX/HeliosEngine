#include <doctest/doctest.h>

#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/world.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
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

struct TagComponent {};

struct Health {
  int points = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

// Test event types (must be trivially copyable)
struct EntityCreatedEvent {
  Entity entity;
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  static constexpr std::string_view GetName() noexcept { return "EntityCreatedEvent"; }
};

struct EntityDestroyedEvent {
  Entity entity;
  std::array<char, 64> reason = {};

  explicit EntityDestroyedEvent(Entity e = {}, std::string_view r = "") : entity(e) {
    const auto copy_size = std::min(r.size(), reason.size() - 1);
    std::copy_n(r.begin(), copy_size, reason.begin());
    reason[copy_size] = '\0';
  }

  static constexpr std::string_view GetName() noexcept { return "EntityDestroyedEvent"; }
};

struct ComponentAddedEvent {
  Entity entity;
  size_t component_type_id = 0;

  static constexpr std::string_view GetName() noexcept { return "ComponentAddedEvent"; }
};

struct DamageEvent {
  Entity attacker;
  Entity target;
  int damage = 0;

  static constexpr std::string_view GetName() noexcept { return "DamageEvent"; }
};

struct ScoreEvent {
  int points = 0;
  std::array<char, 32> player_name = {};

  explicit ScoreEvent(int p = 0, std::string_view name = "") : points(p) {
    const auto copy_size = std::min(name.size(), player_name.size() - 1);
    std::copy_n(name.begin(), copy_size, player_name.begin());
    player_name[copy_size] = '\0';
  }

  static constexpr std::string_view GetName() noexcept { return "ScoreEvent"; }
};

}  // namespace

TEST_SUITE("ecs::World") {
  TEST_CASE("World: Default Construction") {
    World world;

    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("World: CreateEntity") {
    World world;

    Entity entity = world.CreateEntity();

    CHECK(entity.Valid());
    CHECK_EQ(world.EntityCount(), 1);
    CHECK(world.Exists(entity));
  }

  TEST_CASE("World: CreateEntity Multiple") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();

    CHECK_EQ(world.EntityCount(), 3);
    CHECK(world.Exists(entity1));
    CHECK(world.Exists(entity2));
    CHECK(world.Exists(entity3));

    CHECK_NE(entity1.Index(), entity2.Index());
    CHECK_NE(entity2.Index(), entity3.Index());
    CHECK_NE(entity1.Index(), entity3.Index());
  }

  TEST_CASE("World: ReserveEntity") {
    World world;

    Entity reserved = world.ReserveEntity();
    CHECK(reserved.Valid());
    CHECK_EQ(world.EntityCount(), 0);     // Not counted until flushed
    CHECK_FALSE(world.Exists(reserved));  // Not exists until flushed

    world.Update();  // Flush reserved entities
    CHECK_EQ(world.EntityCount(), 1);
    CHECK(world.Exists(reserved));
  }

  TEST_CASE("World: DestroyEntity") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK_EQ(world.EntityCount(), 1);
    CHECK(world.Exists(entity));

    world.DestroyEntity(entity);
    CHECK_EQ(world.EntityCount(), 0);
    CHECK_FALSE(world.Exists(entity));
  }

  TEST_CASE("World: DestroyEntities Range") {
    World world;

    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
      entities.push_back(world.CreateEntity());
    }
    CHECK_EQ(world.EntityCount(), 5);

    std::vector<Entity> to_destroy = {entities[1], entities[3]};
    world.DestroyEntities(to_destroy);

    CHECK_EQ(world.EntityCount(), 3);
    CHECK(world.Exists(entities[0]));
    CHECK_FALSE(world.Exists(entities[1]));
    CHECK(world.Exists(entities[2]));
    CHECK_FALSE(world.Exists(entities[3]));
    CHECK(world.Exists(entities[4]));
  }

  TEST_CASE("World: AddComponent") {
    World world;
    Entity entity = world.CreateEntity();

    Position pos{1.0F, 2.0F, 3.0F};
    world.AddComponent(entity, std::move(pos));

    CHECK(world.HasComponent<Position>(entity));
    // pos should be moved from, but we can't reliably test this
  }

  TEST_CASE("World: AddComponent Copy") {
    World world;
    Entity entity = world.CreateEntity();

    Position pos{1.0F, 2.0F, 3.0F};
    world.AddComponent(entity, pos);

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("World: AddComponents Multiple") {
    World world;
    Entity entity = world.CreateEntity();

    Position pos{1.0F, 2.0F, 3.0F};
    Velocity vel{4.0F, 5.0F, 6.0F};
    Name name{"TestEntity"};

    world.AddComponents(entity, std::move(pos), std::move(vel), std::move(name));

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Name>(entity));
  }

  TEST_CASE("World: TryAddComponent") {
    World world;
    Entity entity = world.CreateEntity();

    Position pos1{1.0F, 2.0F, 3.0F};
    bool added1 = world.TryAddComponent(entity, std::move(pos1));
    CHECK(added1);
    CHECK(world.HasComponent<Position>(entity));

    Position pos2{4.0F, 5.0F, 6.0F};
    world.TryAddComponent(entity, std::move(pos2));  // No-op if already has component
    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("World: TryAddComponents Multiple") {
    World world;
    Entity entity = world.CreateEntity();

    // Add one component first
    world.AddComponent(entity, Position(1.0F, 2.0F, 3.0F));

    Position pos{4.0F, 5.0F, 6.0F};
    Velocity vel{7.0F, 8.0F, 9.0F};
    world.TryAddComponents(entity, std::move(pos), std::move(vel));

    // Validate by component presence: Position remains, Velocity should be present.

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("World: EmplaceComponent") {
    World world;
    Entity entity = world.CreateEntity();

    world.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("World: TryEmplaceComponent") {
    World world;
    Entity entity = world.CreateEntity();

    bool emplaced1 = world.TryEmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);
    CHECK(emplaced1);
    CHECK(world.HasComponent<Position>(entity));

    bool emplaced2 = world.TryEmplaceComponent<Position>(entity, 4.0F, 5.0F, 6.0F);
    CHECK_FALSE(emplaced2);  // Already has Position component
  }

  TEST_CASE("World: RemoveComponent") {
    World world;
    Entity entity = world.CreateEntity();

    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));

    world.RemoveComponent<Position>(entity);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("World: RemoveComponents Multiple") {
    World world;
    Entity entity = world.CreateEntity();

    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Name{"TestEntity"});

    world.RemoveComponents<Position, Velocity>(entity);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Name>(entity));
  }

  TEST_CASE("World: TryRemoveComponent") {
    World world;
    Entity entity = world.CreateEntity();

    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    bool removed1 = world.TryRemoveComponent<Position>(entity);
    CHECK(removed1);
    CHECK_FALSE(world.HasComponent<Position>(entity));

    bool removed2 = world.TryRemoveComponent<Position>(entity);
    CHECK_FALSE(removed2);  // No longer has Position component
  }

  TEST_CASE("World: ClearComponents") {
    World world;
    Entity entity = world.CreateEntity();

    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Name{"TestEntity"});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Name>(entity));

    world.ClearComponents(entity);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Name>(entity));
    CHECK(world.Exists(entity));  // Entity should still exist
  }

  TEST_CASE("World: HasComponent") {
    World world;
    Entity entity = world.CreateEntity();

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));

    world.AddComponent(entity, Position(1.0F, 2.0F, 3.0F));

    CHECK(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("World: HasComponents Multiple") {
    World world;
    Entity entity = world.CreateEntity();

    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Name("TestEntity"));

    auto result = world.HasComponents<Position, Velocity, Name>(entity);

    CHECK_EQ(result.size(), 3);
    CHECK(result[0]);        // Has Position
    CHECK_FALSE(result[1]);  // Doesn't have Velocity
    CHECK(result[2]);        // Has Name
  }

  TEST_CASE("World: Clear") {
    World world;

    // Create entities and add components
    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity2, Velocity{4.0F, 5.0F, 6.0F});

    CHECK_EQ(world.EntityCount(), 2);
    CHECK(world.HasComponent<Position>(entity1));
    CHECK(world.HasComponent<Velocity>(entity2));

    world.Clear();

    CHECK_EQ(world.EntityCount(), 0);
    CHECK_FALSE(world.Exists(entity1));
    CHECK_FALSE(world.Exists(entity2));

    // Should be able to create new entities after clear
    Entity new_entity = world.CreateEntity();
    CHECK(new_entity.Valid());
    CHECK_EQ(world.EntityCount(), 1);
  }

  TEST_CASE("World: ClearEntities") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity2, Velocity{4.0F, 5.0F, 6.0F});

    CHECK_EQ(world.EntityCount(), 2);

    world.ClearEntities();

    CHECK_EQ(world.EntityCount(), 0);
    CHECK_FALSE(world.Exists(entity1));
    CHECK_FALSE(world.Exists(entity2));
  }

  TEST_CASE("World: Update with Commands") {
    World world;

    // Create reserved entity
    Entity reserved = world.ReserveEntity();
    CHECK_EQ(world.EntityCount(), 0);
    CHECK_FALSE(world.Exists(reserved));

    world.Update();

    CHECK_EQ(world.EntityCount(), 1);
    CHECK(world.Exists(reserved));
  }

  TEST_CASE("World: Component Replacement") {
    World world;
    Entity entity = world.CreateEntity();

    world.AddComponent(entity, Position(1.0F, 2.0F, 3.0F));
    CHECK(world.HasComponent<Position>(entity));

    // Replace with new component
    world.AddComponent(entity, Position{4.0F, 5.0F, 6.0F});
    CHECK(world.HasComponent<Position>(entity));

    // Should still only have one Position component
    // (Can't directly test the value without GetComponent, which isn't in the public API)
  }

  TEST_CASE("World: Tag Components") {
    World world;
    Entity entity = world.CreateEntity();

    world.EmplaceComponent<TagComponent>(entity);

    CHECK(world.HasComponent<TagComponent>(entity));

    world.RemoveComponent<TagComponent>(entity);

    CHECK_FALSE(world.HasComponent<TagComponent>(entity));
  }

  TEST_CASE("World: Large Scale Operations") {
    World world;
    constexpr size_t entity_count = 1000;

    std::vector<Entity> entities;
    entities.reserve(entity_count);

    // Create many entities
    for (size_t i = 0; i < entity_count; ++i) {
      entities.push_back(world.CreateEntity());
    }

    CHECK_EQ(world.EntityCount(), entity_count);

    // Add components to all entities
    for (size_t i = 0; i < entity_count; ++i) {
      world.EmplaceComponent<Position>(entities[i], static_cast<float>(i), static_cast<float>(i * 2),
                                       static_cast<float>(i * 3));

      if (i % 2 == 0) {
        world.EmplaceComponent<Velocity>(entities[i], static_cast<float>(i), static_cast<float>(i),
                                         static_cast<float>(i));
      }

      if (i % 3 == 0) {
        world.EmplaceComponent<Name>(entities[i], std::format("Entity{}", i));
      }
    }

    // Verify components
    for (size_t i = 0; i < entity_count; ++i) {
      CHECK(world.HasComponent<Position>(entities[i]));
      CHECK_EQ(world.HasComponent<Velocity>(entities[i]), (i % 2 == 0));
      CHECK_EQ(world.HasComponent<Name>(entities[i]), (i % 3 == 0));
    }

    // Remove half the entities
    std::vector<Entity> to_remove;
    for (size_t i = 0; i < entity_count / 2; ++i) {
      to_remove.push_back(entities[i]);
    }

    world.DestroyEntities(to_remove);
    CHECK_EQ(world.EntityCount(), entity_count - entity_count / 2);

    // Verify remaining entities
    for (size_t i = 0; i < entity_count; ++i) {
      if (i < entity_count / 2) {
        CHECK_FALSE(world.Exists(entities[i]));
      } else {
        CHECK(world.Exists(entities[i]));
        CHECK(world.HasComponent<Position>(entities[i]));
      }
    }
  }

  TEST_CASE("World: Mixed Reserved and Direct Entity Creation") {
    World world;

    Entity reserved1 = world.ReserveEntity();
    Entity direct1 = world.CreateEntity();
    Entity reserved2 = world.ReserveEntity();
    Entity direct2 = world.CreateEntity();

    CHECK_EQ(world.EntityCount(), 2);  // Only direct entities count before flush
    CHECK(world.Exists(direct1));
    CHECK(world.Exists(direct2));
    CHECK_FALSE(world.Exists(reserved1));
    CHECK_FALSE(world.Exists(reserved2));

    world.Update();  // Flush reserved entities

    CHECK_EQ(world.EntityCount(), 4);
    CHECK(world.Exists(direct1));
    CHECK(world.Exists(direct2));
    CHECK(world.Exists(reserved1));
    CHECK(world.Exists(reserved2));
  }

  TEST_CASE("World: Component Operations with Different Entity Types") {
    World world;

    Entity reserved = world.ReserveEntity();
    Entity direct = world.CreateEntity();

    world.Update();  // Flush reserved entity

    // Both entities should support component operations
    world.AddComponent(reserved, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(direct, Position{4.0F, 5.0F, 6.0F});

    CHECK(world.HasComponent<Position>(reserved));
    CHECK(world.HasComponent<Position>(direct));

    world.AddComponent(reserved, Velocity{7.0F, 8.0F, 9.0F});
    world.AddComponent(direct, Name{"DirectEntity"});

    CHECK(world.HasComponent<Velocity>(reserved));
    CHECK_FALSE(world.HasComponent<Velocity>(direct));
    CHECK_FALSE(world.HasComponent<Name>(reserved));
    CHECK(world.HasComponent<Name>(direct));
  }

  TEST_CASE("World: Entity Recycling") {
    World world;

    Entity entity1 = world.CreateEntity();
    auto index1 = entity1.Index();
    auto generation1 = entity1.Generation();

    world.DestroyEntity(entity1);
    CHECK_FALSE(world.Exists(entity1));

    Entity entity2 = world.CreateEntity();
    auto index2 = entity2.Index();
    auto generation2 = entity2.Generation();

    // Should reuse index but increment generation
    CHECK_EQ(index2, index1);
    CHECK_EQ(generation2, generation1 + 1);

    // Old entity should still be invalid
    CHECK_FALSE(world.Exists(entity1));
    CHECK(world.Exists(entity2));
  }

  TEST_CASE("World: Error Conditions") {
    World world;
    Entity invalid_entity;  // Default invalid entity
    Entity nonexistent_entity(999, 1);

    // Operations on invalid entities should trigger assertions in debug builds
    // In release builds, behavior is undefined but shouldn't crash

    // Test with valid entity that exists
    Entity valid_entity = world.CreateEntity();
    CHECK(world.Exists(valid_entity));
    CHECK_FALSE(world.HasComponent<Position>(valid_entity));

    world.AddComponent(valid_entity, Position(1.0F, 2.0F, 3.0F));
    CHECK(world.HasComponent<Position>(valid_entity));
  }

  TEST_CASE("World: TryDestroyEntity Non-existent") {
    World world;

    Entity e = world.CreateEntity();
    CHECK(world.Exists(e));
    world.DestroyEntity(e);
    CHECK_FALSE(world.Exists(e));

    // Should be no-op (entity handle still structurally valid but no longer exists in world)
    world.TryDestroyEntity(e);

    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("World: TryDestroyEntity Existing") {
    World world;

    Entity e = world.CreateEntity();
    world.AddComponent(e, Position{1.0F, 2.0F, 3.0F});
    CHECK(world.Exists(e));

    world.TryDestroyEntity(e);

    CHECK_FALSE(world.Exists(e));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("World: TryDestroyEntities Mixed Existing / Non-existing") {
    World world;

    Entity a = world.CreateEntity();
    Entity b = world.CreateEntity();
    Entity c = world.CreateEntity();

    world.AddComponent(a, Position{});
    world.AddComponent(b, Position{});
    world.AddComponent(c, Position{});

    // Destroy one beforehand to simulate a non-existent (stale) handle
    world.DestroyEntity(b);
    CHECK_FALSE(world.Exists(b));
    CHECK(world.Exists(a));
    CHECK(world.Exists(c));

    std::vector<Entity> batch = {a, b, c};
    world.TryDestroyEntities(batch);

    CHECK_FALSE(world.Exists(a));
    CHECK_FALSE(world.Exists(b));
    CHECK_FALSE(world.Exists(c));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("World: Events Basic Operations") {
    World world;
    world.AddEvent<EntityCreatedEvent>();
    world.AddEvent<ScoreEvent>();
    world.AddEvent<DamageEvent>();

    SUBCASE("Write and Read Single Event") {
      auto writer = world.WriteEvents<EntityCreatedEvent>();
      Entity entity = world.CreateEntity();
      writer.Write(EntityCreatedEvent{entity, 1.0F, 2.0F, 3.0F});

      // Read events
      auto reader = world.ReadEvents<EntityCreatedEvent>();
      auto events = reader.Collect();
      REQUIRE_EQ(events.size(), 1);
      CHECK_EQ(events[0].entity, entity);
      CHECK_EQ(events[0].x, 1.0F);
      CHECK_EQ(events[0].y, 2.0F);
      CHECK_EQ(events[0].z, 3.0F);
    }

    SUBCASE("Write and Read Multiple Events") {
      auto writer = world.WriteEvents<EntityCreatedEvent>();
      Entity e1 = world.CreateEntity();
      Entity e2 = world.CreateEntity();
      Entity e3 = world.CreateEntity();

      writer.Write(EntityCreatedEvent{e1, 1.0F, 1.0F, 1.0F});
      writer.Write(EntityCreatedEvent{e2, 2.0F, 2.0F, 2.0F});
      writer.Write(EntityCreatedEvent{e3, 3.0F, 3.0F, 3.0F});

      auto reader = world.ReadEvents<EntityCreatedEvent>();
      auto events = reader.Collect();
      REQUIRE_EQ(events.size(), 3);
      CHECK_EQ(events[0].entity, e1);
      CHECK_EQ(events[1].entity, e2);
      CHECK_EQ(events[2].entity, e3);
    }

    SUBCASE("Write Events in Bulk") {
      auto writer = world.WriteEvents<ScoreEvent>();
      std::vector<ScoreEvent> score_events;
      score_events.push_back(ScoreEvent{100, "Player1"});
      score_events.push_back(ScoreEvent{200, "Player2"});
      score_events.push_back(ScoreEvent{300, "Player3"});

      writer.WriteBulk(std::span<const ScoreEvent>(score_events));

      auto reader = world.ReadEvents<ScoreEvent>();
      auto events = reader.Collect();
      REQUIRE_EQ(events.size(), 3);
      CHECK_EQ(events[0].points, 100);
      CHECK_EQ(std::string_view(events[0].player_name.data()), "Player1");
      CHECK_EQ(events[1].points, 200);
      CHECK_EQ(std::string_view(events[1].player_name.data()), "Player2");
      CHECK_EQ(events[2].points, 300);
      CHECK_EQ(std::string_view(events[2].player_name.data()), "Player3");
    }

    SUBCASE("Clear Specific Event Type") {
      auto writer_created = world.WriteEvents<EntityCreatedEvent>();
      auto writer_score = world.WriteEvents<ScoreEvent>();
      Entity e1 = world.CreateEntity();
      writer_created.Write(EntityCreatedEvent{e1, 1.0F, 2.0F, 3.0F});
      writer_score.Write(ScoreEvent{100, "TestPlayer"});

      // Verify both event types exist
      CHECK_EQ(world.ReadEvents<EntityCreatedEvent>().Count(), 1);
      CHECK_EQ(world.ReadEvents<ScoreEvent>().Count(), 1);

      // Clear only EntityCreatedEvent
      world.ClearEvents<EntityCreatedEvent>();

      // Verify EntityCreatedEvent is cleared but ScoreEvent remains
      CHECK_EQ(world.ReadEvents<EntityCreatedEvent>().Count(), 0);
      CHECK_EQ(world.ReadEvents<ScoreEvent>().Count(), 1);
    }

    SUBCASE("Clear All Events") {
      auto writer_damage = world.WriteEvents<DamageEvent>();
      auto writer_created = world.WriteEvents<EntityCreatedEvent>();
      auto writer_score = world.WriteEvents<ScoreEvent>();
      Entity e1 = world.CreateEntity();
      writer_created.Write(EntityCreatedEvent{e1, 1.0F, 2.0F, 3.0F});
      writer_score.Write(ScoreEvent{100, "TestPlayer"});
      writer_damage.Write(DamageEvent{e1, e1, 50});

      // Verify events exist
      CHECK_EQ(world.ReadEvents<EntityCreatedEvent>().Count(), 1);
      CHECK_EQ(world.ReadEvents<ScoreEvent>().Count(), 1);
      CHECK_EQ(world.ReadEvents<DamageEvent>().Count(), 1);

      // Clear all events
      world.ClearAllEventQueues();

      // Verify all events are cleared
      CHECK_EQ(world.ReadEvents<EntityCreatedEvent>().Count(), 0);
      CHECK_EQ(world.ReadEvents<ScoreEvent>().Count(), 0);
      CHECK_EQ(world.ReadEvents<DamageEvent>().Count(), 0);
    }

    SUBCASE("Read Events Into Iterator") {
      auto writer = world.WriteEvents<ScoreEvent>();
      writer.Write(ScoreEvent{100, "Player1"});
      writer.Write(ScoreEvent{200, "Player2"});
      writer.Write(ScoreEvent{300, "Player3"});

      auto reader = world.ReadEvents<ScoreEvent>();
      std::vector<ScoreEvent> events;
      reader.ReadInto(std::back_inserter(events));

      REQUIRE_EQ(events.size(), 3);
      CHECK_EQ(events[0].points, 100);
      CHECK_EQ(events[1].points, 200);
      CHECK_EQ(events[2].points, 300);
    }

    SUBCASE("Events Persist Across Multiple Reads") {
      auto writer = world.WriteEvents<EntityCreatedEvent>();
      Entity e1 = world.CreateEntity();
      writer.Write(EntityCreatedEvent{e1, 1.0F, 2.0F, 3.0F});

      // Read events multiple times
      auto reader1 = world.ReadEvents<EntityCreatedEvent>();
      auto events1 = reader1.Collect();
      auto reader2 = world.ReadEvents<EntityCreatedEvent>();
      auto events2 = reader2.Collect();

      CHECK_EQ(events1.size(), 1);
      CHECK_EQ(events2.size(), 1);
      CHECK_EQ(events1[0].entity, events2[0].entity);
    }

    SUBCASE("No Events Returns Empty Vector") {
      auto reader = world.ReadEvents<EntityCreatedEvent>();
      auto events = reader.Collect();
      CHECK(events.empty());
    }
  }

  TEST_CASE("World: Events Multiple Types") {
    World world;
    world.AddEvent<EntityCreatedEvent>();
    world.AddEvent<DamageEvent>();
    world.AddEvent<ScoreEvent>();
    world.AddEvent<EntityDestroyedEvent>();

    Entity player = world.CreateEntity();
    Entity enemy = world.CreateEntity();

    auto writer_created = world.WriteEvents<EntityCreatedEvent>();
    auto writer_damage = world.WriteEvents<DamageEvent>();
    auto writer_score = world.WriteEvents<ScoreEvent>();
    auto writer_destroyed = world.WriteEvents<EntityDestroyedEvent>();

    // Write multiple event types
    writer_created.Write(EntityCreatedEvent{player, 0.0F, 0.0F, 0.0F});
    writer_created.Write(EntityCreatedEvent{enemy, 10.0F, 0.0F, 0.0F});
    writer_damage.Write(DamageEvent{player, enemy, 25});
    writer_score.Write(ScoreEvent{100, "PlayerOne"});
    writer_destroyed.Write(EntityDestroyedEvent{enemy, "killed"});

    // Read each event type
    auto reader_entitycreatedevent = world.ReadEvents<EntityCreatedEvent>();
    auto created_events = reader_entitycreatedevent.Collect();
    auto reader_damageevent = world.ReadEvents<DamageEvent>();
    auto damage_events = reader_damageevent.Collect();
    auto reader_scoreevent = world.ReadEvents<ScoreEvent>();
    auto score_events = reader_scoreevent.Collect();
    auto reader_entitydestroyedevent = world.ReadEvents<EntityDestroyedEvent>();
    auto destroyed_events = reader_entitydestroyedevent.Collect();

    CHECK_EQ(created_events.size(), 2);
    CHECK_EQ(damage_events.size(), 1);
    CHECK_EQ(score_events.size(), 1);
    CHECK_EQ(destroyed_events.size(), 1);

    // Verify event contents
    CHECK_EQ(created_events[0].entity, player);
    CHECK_EQ(created_events[1].entity, enemy);
    CHECK_EQ(damage_events[0].attacker, player);
    CHECK_EQ(damage_events[0].target, enemy);
    CHECK_EQ(damage_events[0].damage, 25);
    CHECK_EQ(score_events[0].points, 100);
    CHECK_EQ(destroyed_events[0].entity, enemy);
    CHECK_EQ(std::string_view(destroyed_events[0].reason.data()), "killed");
  }

  TEST_CASE("World: Events Merge from Multiple Local Storages") {
    World world;
    world.AddEvent<EntityCreatedEvent>();
    world.AddEvent<ScoreEvent>();

    helios::ecs::details::SystemLocalStorage storage1;
    helios::ecs::details::SystemLocalStorage storage2;
    helios::ecs::details::SystemLocalStorage storage3;

    Entity e1 = world.CreateEntity();
    Entity e2 = world.CreateEntity();
    Entity e3 = world.CreateEntity();

    storage1.WriteEvent(EntityCreatedEvent{e1, 1.0F, 0.0F, 0.0F});
    storage2.WriteEvent(EntityCreatedEvent{e2, 2.0F, 0.0F, 0.0F});
    storage3.WriteEvent(EntityCreatedEvent{e3, 3.0F, 0.0F, 0.0F});

    storage1.WriteEvent(ScoreEvent{100, "System1"});
    storage2.WriteEvent(ScoreEvent{200, "System2"});

    // Merge all event queues
    world.MergeEventQueue(storage1.GetEventQueue());
    world.MergeEventQueue(storage2.GetEventQueue());
    world.MergeEventQueue(storage3.GetEventQueue());
    world.Update();

    auto reader_entitycreatedevent = world.ReadEvents<EntityCreatedEvent>();
    auto created_events = reader_entitycreatedevent.Collect();
    auto reader_scoreevent = world.ReadEvents<ScoreEvent>();
    auto score_events = reader_scoreevent.Collect();

    CHECK_EQ(created_events.size(), 3);
    CHECK_EQ(score_events.size(), 2);
  }

  TEST_CASE("World: Events Large Scale") {
    World world;
    world.AddEvent<EntityCreatedEvent>();

    constexpr size_t event_count = 10000;

    auto writer = world.WriteEvents<EntityCreatedEvent>();

    // Write many events
    for (size_t i = 0; i < event_count; ++i) {
      Entity entity = world.CreateEntity();
      writer.Write(EntityCreatedEvent{entity, static_cast<float>(i), 0.0F, 0.0F});
    }

    auto reader = world.ReadEvents<EntityCreatedEvent>();
    auto events = reader.Collect();
    REQUIRE_EQ(events.size(), event_count);

    // Verify all events
    for (size_t i = 0; i < event_count; ++i) {
      CHECK_EQ(events[i].x, static_cast<float>(i));
    }

    // Clear and verify
    world.ClearEvents<EntityCreatedEvent>();
    CHECK(world.ReadEvents<EntityCreatedEvent>().Empty());
  }

  TEST_CASE("World: Events with Entity Lifecycle") {
    World world;
    world.AddEvent<EntityCreatedEvent>();
    world.AddEvent<EntityDestroyedEvent>();

    SUBCASE("Events Remain After Entity Destruction") {
      auto writer = world.WriteEvents<EntityCreatedEvent>();
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

      // Write event about entity
      writer.Write(EntityCreatedEvent{entity, 1.0F, 2.0F, 3.0F});

      // Destroy entity
      world.DestroyEntity(entity);
      CHECK_FALSE(world.Exists(entity));

      // Event should still be readable (contains stale entity reference)
      auto reader = world.ReadEvents<EntityCreatedEvent>();
      auto events = reader.Collect();
      REQUIRE_EQ(events.size(), 1);
      CHECK_EQ(events[0].entity, entity);  // Entity ID remains in event
    }

    SUBCASE("Track Entity Creation and Destruction") {
      auto writer_created = world.WriteEvents<EntityCreatedEvent>();
      auto writer_destroyed = world.WriteEvents<EntityDestroyedEvent>();

      std::vector<Entity> entities;
      for (int i = 0; i < 10; ++i) {
        Entity e = world.CreateEntity();
        entities.push_back(e);
        writer_created.Write(EntityCreatedEvent{e, static_cast<float>(i), 0.0F, 0.0F});
      }

      // Destroy half the entities
      for (int i = 0; i < 5; ++i) {
        world.DestroyEntity(entities[i]);
        writer_destroyed.Write(EntityDestroyedEvent{entities[i], "test_cleanup"});
      }

      auto reader_created = world.ReadEvents<EntityCreatedEvent>();
      auto created = reader_created.Collect();
      auto reader_destroyed = world.ReadEvents<EntityDestroyedEvent>();
      auto destroyed = reader_destroyed.Collect();

      CHECK_EQ(created.size(), 10);
      CHECK_EQ(destroyed.size(), 5);
      CHECK_EQ(world.EntityCount(), 5);
    }
  }

  TEST_CASE("World: Events Clear After World Clear") {
    World world;
    world.AddEvent<EntityCreatedEvent>();
    world.AddEvent<ScoreEvent>();

    auto writer_created = world.WriteEvents<EntityCreatedEvent>();
    auto writer_score = world.WriteEvents<ScoreEvent>();

    Entity e1 = world.CreateEntity();
    writer_created.Write(EntityCreatedEvent{e1, 1.0F, 2.0F, 3.0F});
    writer_score.Write(ScoreEvent{100, "Player"});

    // Verify events exist
    CHECK_EQ(world.ReadEvents<EntityCreatedEvent>().Count(), 1);
    CHECK_EQ(world.ReadEvents<ScoreEvent>().Count(), 1);

    // Clear world (clears everything including event registration)
    world.Clear();

    // After Clear(), world should be empty (no entities, no events, no registration)
    CHECK_EQ(world.EntityCount(), 0);

    // Events can be registered again after Clear()
    world.AddEvent<EntityCreatedEvent>();
    world.AddEvent<ScoreEvent>();
    CHECK(world.ReadEvents<EntityCreatedEvent>().Empty());
    CHECK(world.ReadEvents<ScoreEvent>().Empty());
  }

  TEST_CASE("World: HasEvent") {
    World world;

    SUBCASE("HasEvent Returns False Before Event Registration") {
      CHECK_FALSE(world.HasEvent<EntityCreatedEvent>());
      CHECK_FALSE(world.HasEvent<ScoreEvent>());
      CHECK_FALSE(world.HasEvent<DamageEvent>());
    }

    SUBCASE("HasEvent Returns True After Event Registration") {
      world.AddEvent<EntityCreatedEvent>();
      CHECK(world.HasEvent<EntityCreatedEvent>());
      CHECK_FALSE(world.HasEvent<ScoreEvent>());
      CHECK_FALSE(world.HasEvent<DamageEvent>());

      world.AddEvent<ScoreEvent>();
      CHECK(world.HasEvent<EntityCreatedEvent>());
      CHECK(world.HasEvent<ScoreEvent>());
      CHECK_FALSE(world.HasEvent<DamageEvent>());

      world.AddEvent<DamageEvent>();
      CHECK(world.HasEvent<EntityCreatedEvent>());
      CHECK(world.HasEvent<ScoreEvent>());
      CHECK(world.HasEvent<DamageEvent>());
    }

    SUBCASE("HasEvent After Clear") {
      world.AddEvent<EntityCreatedEvent>();
      world.AddEvent<ScoreEvent>();
      CHECK(world.HasEvent<EntityCreatedEvent>());
      CHECK(world.HasEvent<ScoreEvent>());

      world.Clear();
      CHECK_FALSE(world.HasEvent<EntityCreatedEvent>());
      CHECK_FALSE(world.HasEvent<ScoreEvent>());
    }

    SUBCASE("HasEvent Persistent After ClearAllEventQueues") {
      world.AddEvent<EntityCreatedEvent>();
      world.AddEvent<ScoreEvent>();

      auto writer = world.WriteEvents<EntityCreatedEvent>();
      Entity e = world.CreateEntity();
      writer.Write(EntityCreatedEvent{e, 1.0F, 2.0F, 3.0F});

      world.ClearAllEventQueues();

      // Event registration should persist
      CHECK(world.HasEvent<EntityCreatedEvent>());
      CHECK(world.HasEvent<ScoreEvent>());

      // But queues should be empty
      CHECK(world.ReadEvents<EntityCreatedEvent>().Empty());
    }
  }

}  // TEST_SUITE
