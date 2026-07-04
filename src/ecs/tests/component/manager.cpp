#include <doctest/doctest.h>

#include <helios/ecs/component/manager.hpp>
#include <helios/ecs/entity/entity.hpp>
#include "helios/ecs/component/component.hpp"

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Health {
  int value = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

struct SparsePosition {
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;

  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const SparsePosition& other) const noexcept =
      default;
};

struct SparseVelocity {
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;

  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const SparseVelocity& other) const noexcept =
      default;
};

struct SparseHealth {
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;

  int value = 100;

  constexpr bool operator==(const SparseHealth& other) const noexcept = default;
};

// Tag component (zero-sized, archetype storage)
struct Tag {};

// Sparse-stored component (must be declared with sparse storage trait)
struct ScriptComponent {
  int script_id = 0;

  constexpr bool operator==(const ScriptComponent& other) const noexcept =
      default;
};

}  // namespace

TEST_SUITE("helios::ecs::ComponentManager") {
  TEST_CASE("ecs::ComponentManager::ctor") {
    constexpr Entity entity{1, 0};

    SUBCASE("ComponentManager can be default constructed") {
      ComponentManager mgr;
      CHECK_EQ(mgr.TrackedEntityCount(), 0);
    }

    SUBCASE("Move constructor") {
      ComponentManager mgr1;
      mgr1.InitEntity(entity);
      ComponentManager mgr2(std::move(mgr1));

      CHECK_FALSE(mgr1.Tracked(entity));
      CHECK(mgr2.Tracked(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::operator=") {
    constexpr Entity entity{1, 0};

    SUBCASE("Move assignment") {
      ComponentManager mgr1;
      mgr1.InitEntity(entity);
      ComponentManager mgr2;
      mgr2 = std::move(mgr1);

      CHECK_FALSE(mgr1.Tracked(entity));
      CHECK(mgr2.Tracked(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::Clear") {
    SUBCASE("No entities are tracked after Clear") {
      ComponentManager mgr;

      mgr.InitEntity(Entity{1, 0});
      mgr.InitEntity(Entity{2, 0});
      mgr.Clear();

      CHECK_EQ(mgr.TrackedEntityCount(), 0);
    }

    SUBCASE("Metadata is also removed after Clear") {
      ComponentManager mgr;

      mgr.Register<Position>();
      mgr.Clear();

      CHECK_FALSE(mgr.Registered<Position>());
    }
  }

  TEST_CASE("ecs::ComponentManager::ClearData") {
    SUBCASE("No entities are tracked after ClearData") {
      ComponentManager mgr;

      mgr.InitEntity(Entity{1, 0});
      mgr.ClearData();

      CHECK_EQ(mgr.TrackedEntityCount(), 0);
    }

    SUBCASE("Metadata is retained after ClearData") {
      ComponentManager mgr;

      mgr.Register<Position>();
      mgr.ClearData();

      CHECK(mgr.Registered<Position>());
    }
  }

  TEST_CASE("ecs::ComponentManager::Register") {
    SUBCASE("Register a single component type") {
      ComponentManager mgr;
      mgr.Register<Position>();
      CHECK(mgr.Registered<Position>());
    }

    SUBCASE("Unregistered component returns false") {
      ComponentManager mgr;
      CHECK_FALSE(mgr.Registered<Position>());
    }

    SUBCASE("Re-registering the same type is idempotent") {
      ComponentManager mgr;

      mgr.Register<Position>();
      mgr.Register<Position>();

      CHECK(mgr.Registered<Position>());
    }
  }

  TEST_CASE("ecs::ComponentManager::RegisterAll") {
    SUBCASE("Register multiple component types at once") {
      ComponentManager mgr;

      mgr.RegisterAll<Position, Velocity, Health>();

      CHECK(mgr.Registered<Position>());
      CHECK(mgr.Registered<Velocity>());
      CHECK(mgr.Registered<Health>());
    }

    SUBCASE("Register multiple component duplicates") {
      ComponentManager mgr;

      mgr.RegisterAll<Position, Velocity, Position, Health, Velocity>();

      CHECK(mgr.Registered<Position>());
      CHECK(mgr.Registered<Velocity>());
      CHECK(mgr.Registered<Health>());
    }
  }

  TEST_CASE("ecs::ComponentManager::InitEntity") {
    const Entity e1{1, 0};
    const Entity e2{2, 0};
    const Entity e3{3, 0};

    SUBCASE("Entity is tracked after init") {
      ComponentManager mgr;
      mgr.InitEntity(e1);
      CHECK(mgr.Tracked(e1));
    }

    SUBCASE("Entity is not tracked before init") {
      ComponentManager mgr;
      CHECK_FALSE(mgr.Tracked(e1));
    }

    SUBCASE("Multiple entities are all tracked after init") {
      ComponentManager mgr;

      mgr.InitEntity(e1);
      mgr.InitEntity(e2);
      mgr.InitEntity(e3);

      CHECK(mgr.Tracked(e1));
      CHECK(mgr.Tracked(e2));
      CHECK(mgr.Tracked(e3));
    }

    SUBCASE("TrackedEntityCount increments per entity") {
      ComponentManager mgr;

      CHECK_EQ(mgr.TrackedEntityCount(), 0);
      mgr.InitEntity(e1);
      CHECK_EQ(mgr.TrackedEntityCount(), 1);
      mgr.InitEntity(e2);
      CHECK_EQ(mgr.TrackedEntityCount(), 2);
    }
  }

  TEST_CASE("ecs::ComponentManager::RemoveEntity") {
    constexpr Entity entity{1, 0};

    SUBCASE("Entity is no longer tracked after removal") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.RemoveEntity(entity);

      CHECK_FALSE(mgr.Tracked(entity));
    }

    SUBCASE("TrackedEntityCount decrements after removal") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      CHECK_EQ(mgr.TrackedEntityCount(), 1);
      mgr.RemoveEntity(entity);
      CHECK_EQ(mgr.TrackedEntityCount(), 0);
    }

    SUBCASE("Removing entity also removes its archetype components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{});
      mgr.RemoveEntity(entity);

      CHECK_FALSE(mgr.Tracked(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::TryRemoveEntity") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true and removes tracked entity") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      const bool removed = mgr.TryRemoveEntity(entity);

      CHECK(removed);
      CHECK_FALSE(mgr.Tracked(entity));
    }

    SUBCASE("Returns false for untracked entity") {
      ComponentManager mgr;
      const bool removed = mgr.TryRemoveEntity(entity);
      CHECK_FALSE(removed);
    }
  }

  TEST_CASE("ecs::ComponentManager::Clear/entity") {
    constexpr Entity entity{1, 0};

    SUBCASE("Entity remains tracked after Clear") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{});
      mgr.Clear(entity);

      CHECK(mgr.Tracked(entity));
    }

    SUBCASE("Archetype component is gone after Clear") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{});
      mgr.Clear(entity);

      CHECK_FALSE(mgr.Has<Position>(entity));
    }

    SUBCASE("Entity moves to empty archetype after Clear") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{});
      mgr.Clear(entity);

      const auto& archetype = mgr.EntityArchetype(entity);
      const bool empty = archetype.Empty() || archetype.Id().Empty();
      CHECK(empty);
    }
  }

  TEST_CASE("ecs::ComponentManager::AddArchetypeComponents") {
    constexpr Entity entity{1, 0};

    SUBCASE("Entity has components after add") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});

      CHECK(mgr.Has<Position>(entity));
      CHECK(mgr.Has<Velocity>(entity));
    }

    SUBCASE("Components value is stored correctly") {
      ComponentManager mgr;
      const Position pos{.x = 3.0F, .y = 7.0F};
      const Velocity vel{.x = 1.0F, .y = 2.0F};

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, pos, vel);

      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<Velocity>(entity), vel);
    }

    SUBCASE("Adding more components migrates entity to new archetype") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});
      mgr.AddArchetypeComponents(entity, Health{}, ScriptComponent{});

      CHECK(mgr.Has<Position>(entity));
      CHECK(mgr.Has<Velocity>(entity));
      CHECK(mgr.Has<Health>(entity));
      CHECK(mgr.Has<ScriptComponent>(entity));
    }

    SUBCASE("Existing components is replaced on repeated add") {
      ComponentManager mgr;
      const Position pos{.x = 9.0F, .y = 9.0F};
      const Velocity vel{.x = 5.0F, .y = 6.0F};

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});
      mgr.AddArchetypeComponents(entity, pos, vel);

      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<Velocity>(entity), vel);
    }

    SUBCASE("ArchetypeCount increases when a new archetype is needed") {
      ComponentManager mgr;
      const auto initial_count = mgr.ArchetypeCount();

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});

      CHECK_GT(mgr.ArchetypeCount(), initial_count);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryAddArchetypeComponents") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true when component is new") {
      ComponentManager mgr;

      mgr.InitEntity(entity);

      const auto added =
          mgr.TryAddArchetypeComponents(entity, Position{}, Velocity{});
      CHECK(added[0]);
      CHECK(added[1]);
    }

    SUBCASE("Returns false when entity already has the component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});

      const auto added =
          mgr.TryAddArchetypeComponents(entity, Position{}, Health{});
      CHECK_FALSE(added[0]);
      CHECK(added[1]);
    }
  }

  TEST_CASE("ecs::ComponentManager::EmplaceArchetypeComponent") {
    constexpr Entity entity{1, 0};

    SUBCASE("Component is present after emplace") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.EmplaceArchetypeComponent<Position>(entity, 4.0F, 8.0F);

      CHECK(mgr.Has<Position>(entity));
    }

    SUBCASE("Value is correct after emplace") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.EmplaceArchetypeComponent<Position>(entity, 4.0F, 8.0F);

      CHECK_EQ(mgr.Get<Position>(entity), Position{.x = 4.0F, .y = 8.0F});
    }
  }

  TEST_CASE("ecs::ComponentManager::TryEmplaceArchetypeComponent") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true when component is new") {
      ComponentManager mgr;

      mgr.InitEntity(entity);

      const bool emplaced =
          mgr.TryEmplaceArchetypeComponent<Position>(entity, 1.0F, 2.0F);
      CHECK(emplaced);
    }

    SUBCASE("Returns false when entity already has the component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.EmplaceArchetypeComponent<Position>(entity, 1.0F, 2.0F);

      const bool emplaced =
          mgr.TryEmplaceArchetypeComponent<Position>(entity, 3.0F, 4.0F);
      CHECK_FALSE(emplaced);
    }
  }

  TEST_CASE("ecs::ComponentManager::RemoveArchetypeComponents") {
    constexpr Entity entity{1, 0};

    SUBCASE("Components are absent after removal") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});
      mgr.RemoveArchetypeComponents<Position, Velocity>(entity);

      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK_FALSE(mgr.Has<Velocity>(entity));
    }

    SUBCASE("Other components survive removal of others") {
      ComponentManager mgr;
      const Position pos{.x = 1.0F, .y = 2.0F};

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, pos, Velocity{}, Health{});
      mgr.RemoveArchetypeComponents<Velocity, Health>(entity);

      CHECK_FALSE(mgr.Has<Velocity>(entity));
      CHECK_FALSE(mgr.Has<Health>(entity));
      CHECK(mgr.Has<Position>(entity));
      CHECK_EQ(mgr.Get<Position>(entity), pos);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryRemoveArchetypeComponents") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true and removes existing components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, Position{}, Velocity{});

      const auto removed =
          mgr.TryRemoveArchetypeComponents<Position, Velocity>(entity);
      CHECK(removed[0]);
      CHECK(removed[1]);
      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK_FALSE(mgr.Has<Velocity>(entity));
    }

    SUBCASE("Returns false when entity does not have the components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);

      const auto removed =
          mgr.TryRemoveArchetypeComponents<Position, Velocity>(entity);
      CHECK_FALSE(removed[0]);
      CHECK_FALSE(removed[1]);
    }
  }

  TEST_CASE("ecs::ComponentManager::AddSparseComponent") {
    constexpr Entity entity{1, 0};
    const SparsePosition pos{.x = 1.0F, .y = 2.0F};

    SUBCASE("Entity has component after add") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, pos);

      CHECK(mgr.Has<SparsePosition>(entity));
    }

    SUBCASE("Component value is stored correctly") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, pos);

      CHECK_EQ(mgr.Get<SparsePosition>(entity), pos);
    }

    SUBCASE("Existing component is replaced on repeated add") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparsePosition{.x = 9.0F, .y = 9.0F});
      mgr.AddSparseComponent(entity, pos);

      CHECK_EQ(mgr.Get<SparsePosition>(entity), pos);
    }

    SUBCASE("Sparse storage size increases when adding a new entity") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparsePosition{});
      const auto& storage = mgr.SparseStorage<SparsePosition>();

      CHECK_EQ(storage.Size(), 1);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryAddSparseComponent") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true when component is new") {
      ComponentManager mgr;

      mgr.InitEntity(entity);

      const bool added = mgr.TryAddSparseComponent(entity, SparsePosition{});
      CHECK(added);
    }

    SUBCASE("Returns false when entity already has the component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparsePosition{});

      const bool added = mgr.TryAddSparseComponent(entity, SparsePosition{});
      CHECK_FALSE(added);
    }
  }

  TEST_CASE("ecs::ComponentManager::EmplaceSparseComponent") {
    constexpr Entity entity{1, 0};

    SUBCASE("Component is present after emplace") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.EmplaceSparseComponent<SparsePosition>(entity, 4.0F, 8.0F);

      CHECK(mgr.Has<SparsePosition>(entity));
    }

    SUBCASE("Value is correct after emplace") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.EmplaceSparseComponent<SparsePosition>(entity, 4.0F, 8.0F);

      CHECK_EQ(mgr.Get<SparsePosition>(entity),
               SparsePosition{.x = 4.0F, .y = 8.0F});
    }
  }

  TEST_CASE("ecs::ComponentManager::TryEmplaceSparseComponent") {
    Entity entity{1, 0};

    SUBCASE("Returns true when component is new") {
      ComponentManager mgr;

      mgr.InitEntity(entity);

      const bool emplaced =
          mgr.TryEmplaceSparseComponent<SparsePosition>(entity, 1.0F, 2.0F);
      CHECK(emplaced);
    }

    SUBCASE("Returns false when entity already has the component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.EmplaceSparseComponent<SparsePosition>(entity, 1.0F, 2.0F);

      const bool emplaced =
          mgr.TryEmplaceSparseComponent<SparsePosition>(entity, 3.0F, 4.0F);
      CHECK_FALSE(emplaced);
    }
  }

  TEST_CASE("ecs::ComponentManager::RemoveSparseComponent") {
    constexpr Entity entity{1, 0};

    SUBCASE("Component is absent after removal") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparsePosition{});
      mgr.RemoveSparseComponent<SparsePosition>(entity);

      CHECK_FALSE(mgr.Has<SparsePosition>(entity));
    }

    SUBCASE("Other components survive removal of one") {
      ComponentManager mgr;
      const SparseVelocity vel{.x = 3.0F, .y = 4.0F};

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparsePosition{.x = 1.0F, .y = 2.0F});

      mgr.AddSparseComponent(entity, vel);
      mgr.RemoveSparseComponent<SparsePosition>(entity);

      CHECK_FALSE(mgr.Has<SparsePosition>(entity));
      CHECK(mgr.Has<SparseVelocity>(entity));
      CHECK_EQ(mgr.Get<SparseVelocity>(entity), vel);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryRemoveSparseComponent") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true and removes existing component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparsePosition{});

      const bool removed = mgr.TryRemoveSparseComponent<SparsePosition>(entity);
      CHECK(removed);
      CHECK_FALSE(mgr.Has<SparsePosition>(entity));
    }

    SUBCASE("Returns false when entity does not have the component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);

      const bool removed = mgr.TryRemoveSparseComponent<SparsePosition>(entity);
      CHECK_FALSE(removed);
    }
  }

  TEST_CASE("ecs::ComponentManager::Add") {
    constexpr Entity entity{1, 0};

    SUBCASE("Archetype components") {
      ComponentManager mgr;
      const Position pos{.x = 2.0F, .y = 4.0F};
      const Velocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, pos, vel);

      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<Velocity>(entity), vel);
      CHECK_EQ(mgr.ArchetypeCount(), 2);
    }

    SUBCASE("Sparse components") {
      ComponentManager mgr;
      const SparsePosition pos{.x = 2.0F, .y = 4.0F};
      const SparseVelocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, pos, vel);

      CHECK_EQ(mgr.Get<SparsePosition>(entity), pos);
      CHECK_EQ(mgr.Get<SparseVelocity>(entity), vel);
      const auto& pos_storage = mgr.SparseStorage<SparsePosition>();
      const auto& vel_storage = mgr.SparseStorage<SparseVelocity>();
      CHECK_EQ(pos_storage.Size(), 1);
      CHECK_EQ(vel_storage.Size(), 1);
    }

    SUBCASE("Mixed components") {
      ComponentManager mgr;
      constexpr Entity mixed_entity{1, 0};
      const Position pos{.x = 2.0F, .y = 4.0F};
      const SparseVelocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(mixed_entity);
      mgr.Add(mixed_entity, pos, vel);

      CHECK_EQ(mgr.Get<Position>(mixed_entity), pos);
      CHECK_EQ(mgr.Get<SparseVelocity>(mixed_entity), vel);
      CHECK_EQ(mgr.ArchetypeCount(), 2);
      const auto& vel_storage = mgr.SparseStorage<SparseVelocity>();
      CHECK_EQ(vel_storage.Size(), 1);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryAdd") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns correct bools for new components") {
      ComponentManager mgr;
      const Position pos{.x = 2.0F, .y = 4.0F};
      const Velocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      const auto results = mgr.TryAdd(entity, pos, vel);

      CHECK(results[0]);
      CHECK(results[1]);
      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<Velocity>(entity), vel);
    }

    SUBCASE("Returns false for already-present components") {
      ComponentManager mgr;
      const Position pos{.x = 2.0F, .y = 4.0F};
      const Velocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, pos);
      const auto results = mgr.TryAdd(entity, Position{}, vel);

      CHECK_FALSE(results[0]);
      CHECK(results[1]);
      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<Velocity>(entity), vel);
    }

    SUBCASE("Archetype components") {
      ComponentManager mgr;
      const Position pos{.x = 2.0F, .y = 4.0F};
      const Velocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      mgr.AddArchetypeComponents(entity, pos);
      const auto added = mgr.TryAdd(entity, Position{}, vel);

      CHECK_FALSE(added[0]);
      CHECK(added[1]);
      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<Velocity>(entity), vel);
      CHECK_EQ(mgr.ArchetypeCount(), 3);
    }

    SUBCASE("Sparse components") {
      ComponentManager mgr;
      const SparsePosition pos{.x = 2.0F, .y = 4.0F};
      const SparseVelocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, pos);
      const auto added = mgr.TryAdd(entity, SparsePosition{}, vel);

      CHECK_FALSE(added[0]);
      CHECK(added[1]);
      CHECK_EQ(mgr.Get<SparsePosition>(entity), pos);
      CHECK_EQ(mgr.Get<SparseVelocity>(entity), vel);
      const auto& pos_storage = mgr.SparseStorage<SparsePosition>();
      const auto& vel_storage = mgr.SparseStorage<SparseVelocity>();
      CHECK_EQ(pos_storage.Size(), 1);
      CHECK_EQ(vel_storage.Size(), 1);
    }

    SUBCASE("Mixed components") {
      ComponentManager mgr;
      const Position pos{.x = 2.0F, .y = 4.0F};
      const SparseVelocity vel{.x = 1.0F, .y = 3.0F};

      mgr.InitEntity(entity);
      mgr.AddSparseComponent(entity, SparseVelocity{});
      const auto added = mgr.TryAdd(entity, pos, vel);

      CHECK(added[0]);
      CHECK_FALSE(added[1]);
      CHECK_EQ(mgr.Get<Position>(entity), pos);
      CHECK_EQ(mgr.Get<SparseVelocity>(entity), SparseVelocity{});
      CHECK_EQ(mgr.ArchetypeCount(), 2);
      const auto& vel_storage = mgr.SparseStorage<SparseVelocity>();
      CHECK_EQ(vel_storage.Size(), 1);
    }
  }

  TEST_CASE("ecs::ComponentManager::Emplace") {
    constexpr Entity entity{1, 0};

    SUBCASE("Component is present after emplace") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Emplace<Position>(entity, 5.0F, 10.0F);

      CHECK_EQ(mgr.Get<Position>(entity), Position{.x = 5.0F, .y = 10.0F});
    }

    SUBCASE("Archetype component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Emplace<Velocity>(entity, 1.0F, 2.0F);

      CHECK_EQ(mgr.Get<Velocity>(entity), Velocity{.x = 1.0F, .y = 2.0F});
    }

    SUBCASE("Sparse component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Emplace<SparseVelocity>(entity, 1.0F, 2.0F);

      CHECK_EQ(mgr.Get<SparseVelocity>(entity),
               SparseVelocity{.x = 1.0F, .y = 2.0F});
      const auto& storage = mgr.SparseStorage<SparseVelocity>();
      CHECK_EQ(storage.Size(), 1);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryEmplace") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true when emplacing a new component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      const bool emplaced = mgr.TryEmplace<Position>(entity, 1.0F, 2.0F);

      CHECK(emplaced);
      CHECK_EQ(mgr.Get<Position>(entity), Position{.x = 1.0F, .y = 2.0F});
    }

    SUBCASE("Returns false when entity already has the component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Emplace<Position>(entity, 1.0F, 2.0F);
      const bool emplaced = mgr.TryEmplace<Position>(entity, 3.0F, 4.0F);

      CHECK_FALSE(emplaced);
      CHECK_EQ(mgr.Get<Position>(entity), Position{.x = 1.0F, .y = 2.0F});
    }
  }

  TEST_CASE("ecs::ComponentManager::Remove") {
    constexpr Entity entity{1, 0};

    SUBCASE("All specified components are removed") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{}, SparseVelocity{}, Health{});
      mgr.Remove<Position, SparseVelocity>(entity);

      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK_FALSE(mgr.Has<SparseVelocity>(entity));
      CHECK(mgr.Has<Health>(entity));
    }

    SUBCASE("Archetype components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{}, Velocity{}, Health{});
      mgr.Remove<Position, Health>(entity);

      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK(mgr.Has<Velocity>(entity));
      CHECK_FALSE(mgr.Has<Health>(entity));
    }

    SUBCASE("Sparse components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, SparsePosition{}, SparseVelocity{}, SparseHealth{});
      mgr.Remove<SparsePosition, SparseHealth>(entity);

      CHECK_FALSE(mgr.Has<SparsePosition>(entity));
      CHECK(mgr.Has<SparseVelocity>(entity));
      CHECK_FALSE(mgr.Has<SparseHealth>(entity));
    }

    SUBCASE("Mixed components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{}, Velocity{}, SparseHealth{});
      mgr.Remove<Position, SparseHealth>(entity);

      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK(mgr.Has<Velocity>(entity));
      CHECK_FALSE(mgr.Has<SparseHealth>(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::TryRemove") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns correct bools for mixed presence") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto results = mgr.TryRemove<Position, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK_FALSE(mgr.Has<SparseVelocity>(entity));
    }

    SUBCASE("Archetype components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto results = mgr.TryRemove<Position, Velocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK_FALSE(mgr.Has<Velocity>(entity));
    }

    SUBCASE("Sparse components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, SparsePosition{});
      const auto results =
          mgr.TryRemove<SparsePosition, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
      CHECK_FALSE(mgr.Has<SparsePosition>(entity));
      CHECK_FALSE(mgr.Has<SparseVelocity>(entity));
    }

    SUBCASE("Mixed components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto results = mgr.TryRemove<Position, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
      CHECK_FALSE(mgr.Has<Position>(entity));
      CHECK_FALSE(mgr.Has<SparseVelocity>(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::Get") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns mutable reference") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{.x = 1.0F, .y = 2.0F});
      auto& pos = mgr.Get<Position>(entity);
      pos.x = 99.0F;

      CHECK_EQ(mgr.Get<Position>(entity).x, 99.0F);
    }

    SUBCASE("Returns const reference via const manager") {
      ComponentManager mgr;
      const Position position{.x = 3.0F, .y = 4.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, position);
      const auto& const_mgr = mgr;
      const auto& pos = const_mgr.Get<Position>(entity);

      CHECK_EQ(pos, position);
    }

    SUBCASE("Archetype component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Velocity{.x = 1.0F, .y = 2.0F});
      auto& vel = mgr.Get<Velocity>(entity);
      vel.y = 99.0F;

      CHECK_EQ(mgr.Get<Velocity>(entity).y, 99.0F);
    }

    SUBCASE("Sparse component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, SparseVelocity{.x = 1.0F, .y = 2.0F});
      auto& vel = mgr.Get<SparseVelocity>(entity);
      vel.y = 99.0F;

      CHECK_EQ(mgr.Get<SparseVelocity>(entity).y, 99.0F);
    }
  }

  TEST_CASE("ecs::ComponentManager::TryGet") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns pointer to existing component") {
      ComponentManager mgr;
      const Position pos{.x = 5.0F, .y = 6.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, pos);
      const auto* ptr = mgr.TryGet<Position>(entity);

      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, pos);
    }

    SUBCASE("Returns nullptr when component is absent") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      const auto* ptr = mgr.TryGet<Position>(entity);

      CHECK_EQ(ptr, nullptr);
    }

    SUBCASE("Returns const pointer via const manager") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{.x = 7.0F, .y = 8.0F});
      const auto& const_mgr = mgr;
      const auto* ptr = const_mgr.TryGet<Position>(entity);

      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("Archetype component") {
      ComponentManager mgr;
      const Velocity vel{.x = 1.0F, .y = 2.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, vel);
      const auto* ptr = mgr.TryGet<Velocity>(entity);

      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, vel);
    }

    SUBCASE("Sparse component") {
      ComponentManager mgr;
      const SparseVelocity vel{.x = 1.0F, .y = 2.0F};

      mgr.InitEntity(entity);
      mgr.Add(entity, vel);
      const auto* ptr = mgr.TryGet<SparseVelocity>(entity);

      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, vel);
    }
  }

  TEST_CASE("ecs::ComponentManager::Registered") {
    SUBCASE("Registered returns true for explicitly registered type") {
      ComponentManager mgr;
      mgr.Register<Position>();
      CHECK(mgr.Registered<Position>());
    }

    SUBCASE("Registered returns true for implicitly registered type") {
      ComponentManager mgr;
      constexpr Entity entity{1, 0};

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});

      CHECK(mgr.Registered<Position>());
    }

    SUBCASE("Registered returns false for unregistered type") {
      ComponentManager mgr;
      CHECK_FALSE(mgr.Registered<Position>());
    }
  }

  TEST_CASE("ecs::ComponentManager::Tracked") {
    constexpr Entity entity{1, 0};

    SUBCASE("Tracked returns true for initialized entity") {
      ComponentManager mgr;
      mgr.InitEntity(entity);
      CHECK(mgr.Tracked(entity));
    }

    SUBCASE("Tracked returns false for uninitialized entity") {
      ComponentManager mgr;
      CHECK_FALSE(mgr.Tracked(entity));
    }

    SUBCASE("Tracked returns false after entity removal") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.RemoveEntity(entity);

      CHECK_FALSE(mgr.Tracked(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::Has") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns true after add") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});

      CHECK(mgr.Has<Position>(entity));
    }

    SUBCASE("Returns false before add") {
      ComponentManager mgr;
      mgr.InitEntity(entity);
      CHECK_FALSE(mgr.Has<Position>(entity));
    }

    SUBCASE("Returns false after removal") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      mgr.Remove<Position>(entity);

      CHECK_FALSE(mgr.Has<Position>(entity));
    }

    SUBCASE("Archetype component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Velocity{});
      mgr.Remove<Velocity>(entity);

      CHECK_FALSE(mgr.Has<Velocity>(entity));
    }

    SUBCASE("Sparse component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, SparsePosition{});
      mgr.Remove<SparsePosition>(entity);

      CHECK_FALSE(mgr.Has<SparsePosition>(entity));
    }
  }

  TEST_CASE("ecs::ComponentManager::Has/multi") {
    constexpr Entity entity{1, 0};

    SUBCASE("Returns all true when entity has all components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{}, SparseVelocity{});
      const auto results = mgr.Has<Position, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK(results[1]);
    }

    SUBCASE("Returns mixed results for partial presence") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto results = mgr.Has<Position, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
    }

    SUBCASE("Archetype components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto results = mgr.Has<Position, Velocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
    }

    SUBCASE("Sparse components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, SparsePosition{});
      const auto results = mgr.Has<SparsePosition, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
    }

    SUBCASE("Mixed components") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto results = mgr.Has<Position, SparseVelocity>(entity);

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
    }
  }

  TEST_CASE("ecs::ComponentManager::EntityArchetype") {
    constexpr Entity entity{1, 0};

    SUBCASE("New entity is in the empty archetype") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      const auto& archetype = mgr.EntityArchetype(entity);

      CHECK(archetype.Id().Empty());
    }

    SUBCASE("Archetype changes after adding a component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto& archetype = mgr.EntityArchetype(entity);

      CHECK_FALSE(archetype.Id().Empty());
    }

    SUBCASE("Archetype reverts to empty after Clear") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      mgr.Clear(entity);
      const auto& archetype = mgr.EntityArchetype(entity);

      CHECK(archetype.Id().Empty());
    }

    SUBCASE("Const EntityArchetype is accessible") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto& const_mgr = mgr;
      const auto& archetype = const_mgr.EntityArchetype(entity);

      CHECK_FALSE(archetype.Id().Empty());
    }
  }

  TEST_CASE("ecs::ComponentManager::Archetypes") {
    const Entity e1{1, 0};

    SUBCASE("Empty archetype exists from the start") {
      ComponentManager mgr;
      // Archetypes may be lazily created; force it by initialising an entity.
      mgr.InitEntity(e1);
      CHECK_GE(mgr.ArchetypeCount(), 1);
    }

    SUBCASE(
        "New archetype is added when a component combination is first seen") {
      ComponentManager mgr;

      mgr.InitEntity(e1);
      const auto count_before = mgr.ArchetypeCount();
      mgr.Add(e1, Position{});

      CHECK_GT(mgr.ArchetypeCount(), count_before);
    }

    SUBCASE("Same archetype is reused for identical component sets") {
      ComponentManager mgr;
      const Entity e2{2, 0};

      mgr.InitEntity(e1);
      mgr.InitEntity(e2);
      mgr.Add(e1, Position{});
      const auto count_after_first = mgr.ArchetypeCount();
      mgr.Add(e2, Position{});

      CHECK_EQ(mgr.ArchetypeCount(), count_after_first);
    }

    SUBCASE("Archetypes span is non-empty after entity init") {
      ComponentManager mgr;
      mgr.InitEntity(e1);
      CHECK_FALSE(mgr.Archetypes().empty());
    }

    SUBCASE("Const Archetypes span is accessible") {
      ComponentManager mgr;

      mgr.InitEntity(e1);
      const auto& const_mgr = mgr;

      CHECK_FALSE(const_mgr.Archetypes().empty());
    }
  }

  TEST_CASE("ecs::ComponentManager::StructuralVersion") {
    constexpr Entity entity{1, 0};

    SUBCASE("Version starts at zero") {
      ComponentManager mgr;
      CHECK_EQ(mgr.StructuralVersion(), 0);
    }

    SUBCASE("Version increments when a new archetype is created") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      const auto version_before = mgr.StructuralVersion();
      mgr.Add(entity, Position{});

      CHECK_GT(mgr.StructuralVersion(), version_before);
    }

    SUBCASE("Version increments on entity migration") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto version_before = mgr.StructuralVersion();
      mgr.Add(entity, Velocity{});

      CHECK_GT(mgr.StructuralVersion(), version_before);
    }

    SUBCASE("Version does not change when replacing an existing component") {
      ComponentManager mgr;

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{.x = 1.0F, .y = 2.0F});
      const auto version_after_first_add = mgr.StructuralVersion();
      mgr.Add(entity, Position{.x = 3.0F, .y = 4.0F});

      // No migration — entity stays in the same archetype.
      CHECK_EQ(mgr.StructuralVersion(), version_after_first_add);
    }
  }

  TEST_CASE("ecs::ComponentManager::Metadata") {
    SUBCASE("Metadata is accessible after explicit registration") {
      ComponentManager mgr;

      mgr.Register<Position>();
      const auto& meta = mgr.Metadata<Position>();

      CHECK_EQ(meta.size, sizeof(Position));
      CHECK_EQ(meta.alignment, alignof(Position));
    }

    SUBCASE("Metadata is accessible after implicit registration via Add") {
      ComponentManager mgr;
      constexpr Entity entity{1, 0};

      mgr.InitEntity(entity);
      mgr.Add(entity, Position{});
      const auto& meta = mgr.Metadata<Position>();

      CHECK_EQ(meta.size, sizeof(Position));
    }

    SUBCASE("MetadataByIndex returns nullptr for unknown type") {
      ComponentManager mgr;
      const auto* meta =
          mgr.MetadataByIndex(ComponentTypeIndex::From<Position>());
      CHECK_EQ(meta, nullptr);
    }

    SUBCASE("MetadataByIndex returns valid pointer after registration") {
      ComponentManager mgr;

      mgr.Register<Position>();
      const auto* meta =
          mgr.MetadataByIndex(ComponentTypeIndex::From<Position>());

      CHECK_NE(meta, nullptr);
      CHECK_EQ(meta->size, sizeof(Position));
    }
  }

  TEST_CASE("ecs::ComponentManager - multi-entity scenarios") {
    const Entity e1{1, 0};
    const Entity e2{2, 0};
    const Position pos1{.x = 1.0F, .y = 2.0F};
    const Position pos2{.x = 3.0F, .y = 4.0F};

    SUBCASE("Independent entities do not share component state") {
      ComponentManager mgr;

      mgr.InitEntity(e1);
      mgr.InitEntity(e2);
      mgr.Add(e1, pos1);
      mgr.Add(e2, pos2);

      CHECK_EQ(mgr.Get<Position>(e1), pos1);
      CHECK_EQ(mgr.Get<Position>(e2), pos2);
    }

    SUBCASE("Removing one entity does not affect another") {
      ComponentManager mgr;

      mgr.InitEntity(e1);
      mgr.InitEntity(e2);
      mgr.Add(e1, pos1);
      mgr.Add(e2, pos2);
      mgr.RemoveEntity(e1);

      CHECK_FALSE(mgr.Tracked(e1));
      CHECK(mgr.Tracked(e2));
      CHECK_EQ(mgr.Get<Position>(e2), pos2);
    }

    SUBCASE(
        "Modifying a component on one entity does not affect another in same "
        "archetype") {
      ComponentManager mgr;

      mgr.InitEntity(e1);
      mgr.InitEntity(e2);
      mgr.Add(e1, pos1);
      mgr.Add(e2, pos2);
      mgr.Get<Position>(e1).x = 99.0F;

      CHECK_EQ(mgr.Get<Position>(e1).x, 99.0F);
      CHECK_EQ(mgr.Get<Position>(e2).x, 3.0F);
    }
  }
}
