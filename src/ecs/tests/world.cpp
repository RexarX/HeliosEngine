#include <doctest/doctest.h>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/world.hpp>

#include <memory_resource>
#include <utility>
#include <vector>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Position&) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;

  constexpr bool operator==(const Velocity&) const noexcept = default;
};

struct Tag {
  constexpr bool operator==(const Tag&) const noexcept = default;
};

using PositionBundle = ComponentBundle<Position>;
using MovementBundle = ComponentBundle<Position, Velocity>;
using TaggedMovementBundle = ComponentBundle<Tag, MovementBundle>;

struct DeltaTime {
  float value = 0.0F;

  constexpr bool operator==(const DeltaTime&) const noexcept = default;
};

struct Config {
  int max_entities = 0;

  constexpr bool operator==(const Config&) const noexcept = default;
};

struct GameMsg {
  int value = 0;
};

struct ManualGameMsg {
  static constexpr auto kClearPolicy = MessageClearPolicy::kManual;
  int value = 0;
};

struct AsyncGameMsg {
  static constexpr bool kAsync = true;

  int value = 0;
};

struct AddEntityCmd {
  static void Execute(World& world) {
    [[maybe_unused]] auto _ = world.CreateEntity();
  }
};

struct DestroyEntityCmd {
  Entity entity;

  void Execute(World& world) const { world.DestroyEntity(entity); }
};

}  // namespace

TEST_SUITE("helios::ecs::World") {
  TEST_CASE("ecs::World::Update") {
    SUBCASE("Update executes pending commands") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      CHECK_EQ(world.EntityCount(), 0);
      world.Update();
      CHECK_EQ(world.EntityCount(), 1);
    }

    SUBCASE("Update flushes reserved entities") {
      World world;
      const Entity reserved = world.ReserveEntity();
      CHECK_FALSE(world.Exists(reserved));
      world.Update();
      CHECK(world.Exists(reserved));
    }

    SUBCASE("Update clears the command queue") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      CHECK_EQ(world.CommandCount(), 0);
    }

    SUBCASE("Multiple Update calls each flush their own commands") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      CHECK_EQ(world.EntityCount(), 2);
    }
  }

  TEST_CASE("ecs::World::Flush") {
    SUBCASE("Flush executes pending commands") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      CHECK_EQ(world.EntityCount(), 0);
      world.Flush();
      CHECK_EQ(world.EntityCount(), 1);
    }

    SUBCASE("Flush does not advance message lifecycle") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{42});
      CHECK_EQ(world.Messages().CurrentMessages<GameMsg>().size(), 1);
      world.Flush();
      CHECK_EQ(world.Messages().CurrentMessages<GameMsg>().size(), 1);
      CHECK(world.Messages().PreviousMessages<GameMsg>().empty());
    }

    SUBCASE("Flush clears the command queue") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Flush();
      CHECK_EQ(world.CommandCount(), 0);
    }

    SUBCASE("Flush flushes reserved entities") {
      World world;
      const Entity reserved = world.ReserveEntity();
      CHECK_FALSE(world.Exists(reserved));
      world.Flush();
      CHECK(world.Exists(reserved));
    }
  }

  TEST_CASE("ecs::World::Clear") {
    SUBCASE("Clear removes all entities") {
      World world;
      [[maybe_unused]] const auto e1 = world.CreateEntity();
      [[maybe_unused]] const auto e2 = world.CreateEntity();
      world.Clear();
      CHECK_EQ(world.EntityCount(), 0);
    }

    SUBCASE("Clear removes all resources") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      world.Clear();
      CHECK_FALSE(world.HasResource<DeltaTime>());
    }

    SUBCASE("Clear removes pending commands") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Clear();
      CHECK_EQ(world.CommandCount(), 0);
    }

    SUBCASE("World is usable after Clear") {
      World world;
      [[maybe_unused]] const auto entt = world.CreateEntity();
      world.Clear();
      const Entity entity = world.CreateEntity();
      CHECK(world.Exists(entity));
    }
  }

  TEST_CASE("ecs::World::ClearEntities") {
    SUBCASE("ClearEntities removes all entities") {
      World world;
      [[maybe_unused]] const auto e1 = world.CreateEntity();
      [[maybe_unused]] const auto e2 = world.CreateEntity();
      world.ClearEntities();
      CHECK_EQ(world.EntityCount(), 0);
    }

    SUBCASE("ClearEntities preserves resources") {
      World world;
      world.InsertResources(DeltaTime{2.0F});
      [[maybe_unused]] const auto entity = world.CreateEntity();
      world.ClearEntities();
      CHECK(world.HasResource<DeltaTime>());
    }

    SUBCASE("World is usable after ClearEntities") {
      World world;
      world.ClearEntities();
      const Entity entity = world.CreateEntity();
      CHECK(world.Exists(entity));
    }
  }

  TEST_CASE("ecs::World::CreateEntity") {
    SUBCASE("CreateEntity returns a valid entity") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK(entity.Valid());
    }

    SUBCASE("Created entity exists in the world") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK(world.Exists(entity));
    }

    SUBCASE("Each call returns a distinct entity") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      CHECK_NE(e1, e2);
    }

    SUBCASE("EntityCount increases with each created entity") {
      World world;
      [[maybe_unused]] const auto e1 = world.CreateEntity();
      [[maybe_unused]] const auto e2 = world.CreateEntity();
      CHECK_EQ(world.EntityCount(), 2);
    }
  }

  TEST_CASE("ecs::World::ReserveEntity") {
    SUBCASE("ReserveEntity returns a valid entity handle") {
      World world;
      const Entity entity = world.ReserveEntity();
      CHECK(entity.Valid());
    }

    SUBCASE("Reserved entity does not exist until Update is called") {
      World world;
      const Entity entity = world.ReserveEntity();
      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Reserved entity exists after Update") {
      World world;
      const Entity entity = world.ReserveEntity();
      world.Update();
      CHECK(world.Exists(entity));
    }

    SUBCASE("Multiple reserved entities all become alive after Update") {
      World world;
      const Entity e1 = world.ReserveEntity();
      const Entity e2 = world.ReserveEntity();
      world.Update();
      CHECK(world.Exists(e1));
      CHECK(world.Exists(e2));
    }
  }

  TEST_CASE("ecs::World::DestroyEntity") {
    SUBCASE("DestroyEntity removes the entity from the world") {
      World world;
      const Entity entity = world.CreateEntity();
      world.DestroyEntity(entity);
      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("EntityCount decreases after DestroyEntity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.DestroyEntity(entity);
      CHECK_EQ(world.EntityCount(), 0);
    }

    SUBCASE("DestroyEntity removes all components from the entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      world.DestroyEntity(entity);
      CHECK_EQ(world.EntityCount(), 0);
    }
  }

  TEST_CASE("ecs::World::TryDestroyEntity") {
    SUBCASE("TryDestroyEntity removes an existing entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.TryDestroyEntity(entity);
      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("TryDestroyEntity is a no-op for a non-existing entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.DestroyEntity(entity);
      // The entity no longer exists; second call should not assert or crash
      world.TryDestroyEntity(entity);
      CHECK_EQ(world.EntityCount(), 0);
    }
  }

  TEST_CASE("ecs::World::DestroyEntities") {
    SUBCASE("DestroyEntities removes all provided entities") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      const std::vector<Entity> to_destroy = {e1, e2};
      world.DestroyEntities(to_destroy);
      CHECK_FALSE(world.Exists(e1));
      CHECK_FALSE(world.Exists(e2));
    }

    SUBCASE("DestroyEntities with an empty range is a no-op") {
      World world;
      [[maybe_unused]] const auto entity = world.CreateEntity();
      const std::vector<Entity> empty;
      world.DestroyEntities(empty);
      CHECK_EQ(world.EntityCount(), 1);
    }

    SUBCASE("EntityCount is updated correctly after DestroyEntities") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      [[maybe_unused]] const auto entity = world.CreateEntity();
      const std::vector<Entity> to_destroy = {e1, e2};
      world.DestroyEntities(to_destroy);
      CHECK_EQ(world.EntityCount(), 1);
    }
  }

  TEST_CASE("ecs::World::TryDestroyEntities") {
    SUBCASE("TryDestroyEntities removes existing entities") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      const std::vector<Entity> to_destroy = {e1, e2};
      world.TryDestroyEntities(to_destroy);
      CHECK_FALSE(world.Exists(e1));
      CHECK_FALSE(world.Exists(e2));
    }

    SUBCASE(
        "TryDestroyEntities skips non-existing entities without asserting") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      world.DestroyEntity(e2);
      const std::vector<Entity> to_destroy = {e1, e2};
      world.TryDestroyEntities(to_destroy);
      CHECK_EQ(world.EntityCount(), 0);
    }

    SUBCASE("TryDestroyEntities with empty range is a no-op") {
      World world;
      [[maybe_unused]] const auto entity = world.CreateEntity();
      const std::vector<Entity> empty;
      world.TryDestroyEntities(empty);
      CHECK_EQ(world.EntityCount(), 1);
    }
  }

  TEST_CASE("ecs::World::AddComponents") {
    SUBCASE("AddComponents adds all provided components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F});
      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("AddComponents stores correct values for each component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{5.0F, 6.0F}, Velocity{7.0F, 8.0F});
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{5.0F, 6.0F}));
      CHECK_EQ(world.ReadComponent<Velocity>(entity), (Velocity{7.0F, 8.0F}));
    }

    SUBCASE("AddComponents replaces existing components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 1.0F}, Velocity{1.0F, 1.0F});
      world.AddComponents(entity, Position{2.0F, 2.0F}, Velocity{2.0F, 2.0F});
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{2.0F, 2.0F}));
      CHECK_EQ(world.ReadComponent<Velocity>(entity), (Velocity{2.0F, 2.0F}));
    }

    SUBCASE("AddComponents with one component passed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 0.0F});
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 0.0F}));
    }

    SUBCASE("AddComponents accepts lvalue and const lvalue components") {
      World world;
      const Entity entity = world.CreateEntity();
      constexpr Position position{3.0F, 4.0F};
      constexpr Velocity velocity{5.0F, 6.0F};
      world.AddComponents(entity, position, velocity);
      CHECK_EQ(world.ReadComponent<Position>(entity), position);
      CHECK_EQ(world.ReadComponent<Velocity>(entity), velocity);
    }
  }

  TEST_CASE("ecs::World::TryAddComponents") {
    SUBCASE("TryAddComponents returns true for each component not present") {
      World world;
      const Entity entity = world.CreateEntity();
      const auto result = world.TryAddComponents(entity, Position{1.0F, 0.0F},
                                                 Velocity{0.0F, 1.0F});
      CHECK(result[0]);
      CHECK(result[1]);
    }

    SUBCASE(
        "TryAddComponents returns false for components already present and "
        "does not replace") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      const auto result = world.TryAddComponents(entity, Position{9.0F, 9.0F},
                                                 Velocity{3.0F, 4.0F});
      CHECK_FALSE(result[0]);
      CHECK(result[1]);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 2.0F}));
    }

    SUBCASE("TryAddComponents with one component passed") {
      World world;
      const Entity entity = world.CreateEntity();
      const auto result = world.TryAddComponents(entity, Position{1.0F, 0.0F});
      CHECK(result);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 0.0F}));
    }

    SUBCASE("TryAddComponents accepts lvalue and const lvalue components") {
      World world;
      const Entity entity = world.CreateEntity();
      constexpr Position position{3.0F, 4.0F};
      constexpr Velocity velocity{5.0F, 6.0F};
      const auto result = world.TryAddComponents(entity, position, velocity);
      CHECK(result[0]);
      CHECK(result[1]);
      CHECK_EQ(world.ReadComponent<Position>(entity), position);
      CHECK_EQ(world.ReadComponent<Velocity>(entity), velocity);
    }
  }

  TEST_CASE("ecs::World::AddBundle") {
    SUBCASE("AddBundle adds a flat bundle and stores its values") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddBundle(
          entity, MovementBundle{Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F}});
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 2.0F}));
      CHECK_EQ(world.ReadComponent<Velocity>(entity), (Velocity{3.0F, 4.0F}));
    }

    SUBCASE("AddBundle flattens nested bundles") {
      World world;
      const Entity entity = world.CreateEntity();
      constexpr TaggedMovementBundle bundle{
          Tag{}, MovementBundle{Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F}}};
      world.AddBundle(entity, bundle);
      CHECK(world.HasComponent<Tag>(entity));
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 2.0F}));
      CHECK_EQ(world.ReadComponent<Velocity>(entity), (Velocity{3.0F, 4.0F}));
    }

    SUBCASE("AddBundle replaces existing leaf components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      world.AddBundle(
          entity, MovementBundle{Position{5.0F, 6.0F}, Velocity{7.0F, 8.0F}});
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{5.0F, 6.0F}));
      CHECK_EQ(world.ReadComponent<Velocity>(entity), (Velocity{7.0F, 8.0F}));
    }

    SUBCASE("AddBundle emits a message for each leaf component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddBundle(entity, MovementBundle{});
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentAddedMsg<Position>>()
                   .size(),
               1);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentAddedMsg<Velocity>>()
                   .size(),
               1);
    }
  }

  TEST_CASE("ecs::World::TryAddBundle") {
    SUBCASE("TryAddBundle returns results in flattened declaration order") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{9.0F, 9.0F});
      const auto result = world.TryAddBundle(
          entity,
          TaggedMovementBundle{Tag{}, MovementBundle{Position{1.0F, 2.0F},
                                                     Velocity{3.0F, 4.0F}}});
      CHECK(result[0]);
      CHECK_FALSE(result[1]);
      CHECK(result[2]);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{9.0F, 9.0F}));
    }

    SUBCASE("TryAddBundle returns bool for a single leaf") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK(world.TryAddBundle(entity, PositionBundle{Position{1.0F, 2.0F}}));
      CHECK_FALSE(
          world.TryAddBundle(entity, PositionBundle{Position{3.0F, 4.0F}}));
    }

    SUBCASE("TryAddBundle emits messages only for added leaf components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.Update();
      const auto result = world.TryAddBundle(entity, MovementBundle{});
      CHECK_FALSE(result[0]);
      CHECK(result[1]);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentAddedMsg<Position>>()
                   .size(),
               0);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentAddedMsg<Velocity>>()
                   .size(),
               1);
    }
  }

  TEST_CASE("ecs::World::EmplaceComponent") {
    SUBCASE("EmplaceComponent adds a component constructed in-place") {
      World world;
      const Entity entity = world.CreateEntity();
      world.EmplaceComponent<Position>(entity, 1.0F, 2.0F);
      CHECK(world.HasComponent<Position>(entity));
    }

    SUBCASE("EmplaceComponent stores the correct field values") {
      World world;
      const Entity entity = world.CreateEntity();
      world.EmplaceComponent<Position>(entity, 3.0F, 4.0F);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{3.0F, 4.0F}));
    }

    SUBCASE("EmplaceComponent replaces an existing component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.EmplaceComponent<Position>(entity, 1.0F, 0.0F);
      world.EmplaceComponent<Position>(entity, 9.0F, 9.0F);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{9.0F, 9.0F}));
    }

    SUBCASE("EmplaceComponent with no args default-constructs the component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.EmplaceComponent<Position>(entity);
      CHECK_EQ(world.ReadComponent<Position>(entity), Position{});
    }
  }

  TEST_CASE("ecs::World::TryEmplaceComponent") {
    SUBCASE("TryEmplaceComponent returns true and emplaces when absent") {
      World world;
      const Entity entity = world.CreateEntity();
      const bool added =
          world.TryEmplaceComponent<Position>(entity, 1.0F, 2.0F);
      CHECK(added);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 2.0F}));
    }

    SUBCASE("TryEmplaceComponent returns false when component already exists") {
      World world;
      const Entity entity = world.CreateEntity();
      world.EmplaceComponent<Position>(entity);
      const bool added =
          world.TryEmplaceComponent<Position>(entity, 9.0F, 9.0F);
      CHECK_FALSE(added);
    }

    SUBCASE("TryEmplaceComponent does not replace an existing component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.EmplaceComponent<Position>(entity, 1.0F, 2.0F);
      world.TryEmplaceComponent<Position>(entity, 9.0F, 9.0F);
      CHECK_EQ(world.ReadComponent<Position>(entity), (Position{1.0F, 2.0F}));
    }
  }

  TEST_CASE("ecs::World::RemoveComponents") {
    SUBCASE("RemoveComponents removes all specified components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      world.RemoveComponents<Position, Velocity>(entity);
      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("RemoveComponents does not affect components not in the list") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{}, Tag{});
      world.RemoveComponents<Position, Velocity>(entity);
      CHECK(world.HasComponent<Tag>(entity));
    }

    SUBCASE("RemoveComponents with one component passed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.RemoveComponents<Position>(entity);
      CHECK_FALSE(world.HasComponent<Position>(entity));
    }
  }

  TEST_CASE("ecs::World::TryRemoveComponents") {
    SUBCASE(
        "TryRemoveComponents returns true for each component that was "
        "present") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      const auto result = world.TryRemoveComponents<Position, Velocity>(entity);
      CHECK(result[0]);
      CHECK(result[1]);
      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }

    SUBCASE(
        "TryRemoveComponents returns false for each component that was "
        "absent") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      const auto result = world.TryRemoveComponents<Position, Velocity>(entity);
      CHECK(result[0]);
      CHECK_FALSE(result[1]);
    }

    SUBCASE("TryRemoveComponents with one component passed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      const auto result = world.TryRemoveComponents<Position>(entity);
      CHECK(result);
      CHECK_FALSE(world.HasComponent<Position>(entity));
    }
  }

  TEST_CASE("ecs::World::RemoveBundle") {
    SUBCASE("RemoveBundle removes every flattened leaf component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddBundle(entity, TaggedMovementBundle{});
      world.RemoveBundle<TaggedMovementBundle>(entity);
      CHECK_FALSE(world.HasComponent<Tag>(entity));
      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("RemoveBundle preserves unrelated components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddBundle(entity, MovementBundle{});
      world.AddComponents(entity, DeltaTime{1.0F});
      world.RemoveBundle<MovementBundle>(entity);
      CHECK(world.HasComponent<DeltaTime>(entity));
    }

    SUBCASE("RemoveBundle emits a message for each leaf component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddBundle(entity, MovementBundle{});
      world.RemoveBundle<MovementBundle>(entity);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentRemovedMsg<Position>>()
                   .size(),
               1);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentRemovedMsg<Velocity>>()
                   .size(),
               1);
    }
  }

  TEST_CASE("ecs::World::TryRemoveBundle") {
    SUBCASE("TryRemoveBundle returns results in flattened declaration order") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      const auto result = world.TryRemoveBundle<TaggedMovementBundle>(entity);
      CHECK_FALSE(result[0]);
      CHECK(result[1]);
      CHECK_FALSE(result[2]);
    }

    SUBCASE("TryRemoveBundle returns bool for a single leaf") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      CHECK(world.TryRemoveBundle<PositionBundle>(entity));
      CHECK_FALSE(world.TryRemoveBundle<PositionBundle>(entity));
    }

    SUBCASE("TryRemoveBundle emits messages only for removed leaf components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      const auto result = world.TryRemoveBundle<MovementBundle>(entity);
      CHECK(result[0]);
      CHECK_FALSE(result[1]);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentRemovedMsg<Position>>()
                   .size(),
               1);
      CHECK_EQ(world.Messages()
                   .CurrentMessages<ComponentRemovedMsg<Velocity>>()
                   .size(),
               0);
    }
  }

  TEST_CASE("ecs::World::ClearComponents") {
    SUBCASE("ClearComponents removes all components from the entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{}, Tag{});
      world.ClearComponents(entity);
      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
      CHECK_FALSE(world.HasComponent<Tag>(entity));
    }

    SUBCASE("ClearComponents on an entity with no components is a no-op") {
      World world;
      const Entity entity = world.CreateEntity();
      world.ClearComponents(entity);
      CHECK(world.Exists(entity));
    }

    SUBCASE("ClearComponents does not affect other entities") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      world.AddComponents(e1, Position{});
      world.AddComponents(e2, Position{});
      world.ClearComponents(e1);
      CHECK(world.HasComponent<Position>(e2));
    }
  }

  TEST_CASE("ecs::World::Query") {
    SUBCASE("Query with default allocator is valid") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});

      const auto query = world.Query<Position&>();
      int count = 0;
      for (auto&& [pos] : query) {
        ++count;
        CHECK_EQ(pos, (Position{1.0F, 2.0F}));
      }
      CHECK_EQ(count, 1);
    }

    SUBCASE("Query with pmr resource returns a valid builder") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{3.0F, 4.0F});

      auto* resource = std::pmr::get_default_resource();

      const auto query = world.Query<Position>(resource);
      int count = 0;
      for ([[maybe_unused]] auto&& [pos] : query) {
        ++count;
      }
      CHECK_EQ(count, 1);
    }

    SUBCASE("Query With and Without filters entities correctly") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      world.AddComponents(e1, Position{}, Tag{});
      world.AddComponents(e2, Position{});

      // Verify components were added correctly
      CHECK(world.HasComponent<Position>(e1));
      CHECK(world.HasComponent<Tag>(e1));
      CHECK(world.HasComponent<Position>(e2));
      CHECK(!world.HasComponent<Tag>(e2));

      const auto query = world.Query<Position, Without<Tag>>();
      int count = 0;
      for ([[maybe_unused]] auto&& [pos] : query) {
        ++count;
      }
      CHECK_EQ(count, 1);
    }
  }

  TEST_CASE("ecs::World::ReadOnlyQuery") {
    SUBCASE("ReadOnlyQuery with default allocator returns a valid builder") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{5.0F, 6.0F});

      const auto query = world.ReadOnlyQuery<Position>();
      int count = 0;
      for (auto&& [pos] : query) {
        ++count;
        CHECK_EQ(pos, (Position{5.0F, 6.0F}));
      }
      CHECK_EQ(count, 1);
    }

    SUBCASE("ReadOnlyQuery with pmr resource returns a valid builder") {
      World world;
      [[maybe_unused]] const auto entity = world.CreateEntity();

      auto* resource = std::pmr::get_default_resource();
      // Should compile and run without issues.
      auto builder = world.ReadOnlyQuery(resource);
    }
  }

  TEST_CASE("ecs::World::WriteComponent") {
    SUBCASE("WriteComponent returns a mutable reference to the component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      world.WriteComponent<Position>(entity).x = 99.0F;
      CHECK_EQ(world.ReadComponent<Position>(entity).x, 99.0F);
    }
  }

  TEST_CASE("ecs::World::ReadComponent") {
    SUBCASE("ReadComponent returns a const reference to the component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{7.0F, 8.0F});
      CHECK_EQ(std::as_const(world).ReadComponent<Position>(entity),
               (Position{7.0F, 8.0F}));
    }
  }

  TEST_CASE("ecs::World::TryWriteComponent") {
    SUBCASE(
        "TryWriteComponent returns non-null pointer when component exists") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      auto* ptr = world.TryWriteComponent<Position>(entity);
      REQUIRE_NE(ptr, nullptr);
      CHECK_EQ(*ptr, (Position{1.0F, 2.0F}));
    }

    SUBCASE("TryWriteComponent returns nullptr when component is absent") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK_EQ(world.TryWriteComponent<Position>(entity), nullptr);
    }

    SUBCASE(
        "Modifying through TryWriteComponent pointer updates the component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.TryWriteComponent<Position>(entity)->x = 42.0F;
      CHECK_EQ(world.ReadComponent<Position>(entity).x, 42.0F);
    }
  }

  TEST_CASE("ecs::World::TryReadComponent") {
    SUBCASE(
        "TryReadComponent returns non-null const pointer when component "
        "exists") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{3.0F, 4.0F});
      const auto* ptr = std::as_const(world).TryReadComponent<Position>(entity);
      REQUIRE_NE(ptr, nullptr);
      CHECK_EQ(*ptr, (Position{3.0F, 4.0F}));
    }

    SUBCASE("TryReadComponent returns nullptr when component is absent") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK_EQ(std::as_const(world).TryReadComponent<Position>(entity),
               nullptr);
    }
  }

  TEST_CASE("ecs::World::InsertResources") {
    SUBCASE("InsertResources adds a new resource") {
      World world;
      world.InsertResources(DeltaTime{0.016F});
      CHECK(world.HasResource<DeltaTime>());
    }

    SUBCASE("InsertResources stores the correct value") {
      World world;
      world.InsertResources(DeltaTime{0.033F});
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 0.033F);
    }

    SUBCASE("InsertResources replaces an existing resource") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      world.InsertResources(DeltaTime{2.0F});
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 2.0F);
    }

    SUBCASE("InsertResources with multiple resources") {
      World world;
      world.InsertResources(DeltaTime{1.0F}, Config{1});
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 1.0F);
      CHECK_EQ(world.ReadResource<Config>().max_entities, 1);
    }
  }

  TEST_CASE("ecs::World::TryInsertResources") {
    SUBCASE("TryInsertResources returns true and inserts when absent") {
      World world;
      const bool inserted = world.TryInsertResources(DeltaTime{1.0F});
      CHECK(inserted);
      CHECK(world.HasResource<DeltaTime>());
    }

    SUBCASE("TryInsertResources returns false when resource already exists") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      const bool inserted = world.TryInsertResources(DeltaTime{2.0F});
      CHECK_FALSE(inserted);
    }

    SUBCASE("TryInsertResources does not replace an existing resource") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      world.TryInsertResources(DeltaTime{9.0F});
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 1.0F);
    }

    SUBCASE("TryInsertResources with multiple resources") {
      World world;
      const auto result = world.TryInsertResources(DeltaTime{1.0F}, Config{1});
      CHECK(result[0]);
      CHECK(result[1]);
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 1.0F);
      CHECK_EQ(world.ReadResource<Config>().max_entities, 1);
    }
  }

  TEST_CASE("ecs::World::EmplaceResource") {
    SUBCASE("EmplaceResource adds a resource constructed in-place") {
      World world;
      world.EmplaceResource<DeltaTime>(0.016F);
      CHECK(world.HasResource<DeltaTime>());
    }

    SUBCASE("EmplaceResource stores the correct value") {
      World world;
      world.EmplaceResource<DeltaTime>(0.032F);
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 0.032F);
    }

    SUBCASE("EmplaceResource replaces an existing resource") {
      World world;
      world.EmplaceResource<DeltaTime>(1.0F);
      world.EmplaceResource<DeltaTime>(2.0F);
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 2.0F);
    }
  }

  TEST_CASE("ecs::World::TryEmplaceResource") {
    SUBCASE("TryEmplaceResource returns true and emplaces when absent") {
      World world;
      const bool emplaced = world.TryEmplaceResource<DeltaTime>(0.5F);
      CHECK(emplaced);
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 0.5F);
    }

    SUBCASE("TryEmplaceResource returns false when resource already exists") {
      World world;
      world.EmplaceResource<DeltaTime>(1.0F);
      const bool emplaced = world.TryEmplaceResource<DeltaTime>(9.0F);
      CHECK_FALSE(emplaced);
    }

    SUBCASE("TryEmplaceResource does not replace existing resource") {
      World world;
      world.EmplaceResource<DeltaTime>(1.0F);
      world.TryEmplaceResource<DeltaTime>(9.0F);
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 1.0F);
    }
  }

  TEST_CASE("ecs::World::RemoveResource") {
    SUBCASE("RemoveResources removes the resource") {
      World world;
      world.InsertResources(DeltaTime{});
      world.RemoveResources<DeltaTime>();
      CHECK_FALSE(world.HasResource<DeltaTime>());
    }

    SUBCASE("RemoveResources does not affect other resources") {
      World world;
      world.InsertResources(DeltaTime{}, Config{});
      world.RemoveResources<DeltaTime>();
      CHECK(world.HasResource<Config>());
    }
  }

  TEST_CASE("ecs::World::TryRemoveResources") {
    SUBCASE(
        "TryRemoveResources returns true and removes an existing resource") {
      World world;
      world.InsertResources(DeltaTime{});
      const bool removed = world.TryRemoveResources<DeltaTime>();
      CHECK(removed);
      CHECK_FALSE(world.HasResource<DeltaTime>());
    }

    SUBCASE("TryRemoveResource returns false when resource does not exist") {
      World world;
      const bool removed = world.TryRemoveResources<DeltaTime>();
      CHECK_FALSE(removed);
    }

    SUBCASE("TryRemoveResources returns true and removes multiple resources") {
      World world;
      world.InsertResources(DeltaTime{}, Config{});
      const auto removed = world.TryRemoveResources<DeltaTime, Config>();
      CHECK(removed[0]);
      CHECK(removed[1]);
      CHECK_FALSE(world.HasResource<DeltaTime>());
      CHECK_FALSE(world.HasResource<Config>());
    }
  }

  TEST_CASE("ecs::World::WriteResource") {
    SUBCASE("WriteResource returns a mutable reference to the resource") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      world.WriteResource<DeltaTime>().value = 99.0F;
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 99.0F);
    }
  }

  TEST_CASE("ecs::World::ReadResource") {
    SUBCASE("ReadResource returns the stored resource value") {
      World world;
      world.InsertResources(DeltaTime{0.5F});
      CHECK_EQ(std::as_const(world).ReadResource<DeltaTime>().value, 0.5F);
    }
  }

  TEST_CASE("ecs::World::TryWriteResource") {
    SUBCASE("TryWriteResource returns non-null pointer when resource exists") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      auto* ptr = world.TryWriteResource<DeltaTime>();
      REQUIRE_NE(ptr, nullptr);
      CHECK_EQ(ptr->value, 1.0F);
    }

    SUBCASE("TryWriteResource returns nullptr when resource does not exist") {
      World world;
      CHECK_EQ(world.TryWriteResource<DeltaTime>(), nullptr);
    }

    SUBCASE("Modifying through TryWriteResource pointer updates the resource") {
      World world;
      world.InsertResources(DeltaTime{});
      world.TryWriteResource<DeltaTime>()->value = 7.0F;
      CHECK_EQ(world.ReadResource<DeltaTime>().value, 7.0F);
    }
  }

  TEST_CASE("ecs::World::TryReadResource") {
    SUBCASE(
        "TryReadResource returns non-null const pointer when resource "
        "exists") {
      World world;
      world.InsertResources(DeltaTime{2.0F});
      const auto* ptr = std::as_const(world).TryReadResource<DeltaTime>();
      REQUIRE_NE(ptr, nullptr);
      CHECK_EQ(ptr->value, 2.0F);
    }

    SUBCASE("TryReadResource returns nullptr when resource does not exist") {
      World world;
      CHECK_EQ(std::as_const(world).TryReadResource<DeltaTime>(), nullptr);
    }
  }

  TEST_CASE("ecs::World::AddMessage") {
    SUBCASE("AddMessage registers a message type") {
      World world;
      world.AddMessage<GameMsg>();
      CHECK(world.HasMessage<GameMsg>());
    }

    SUBCASE(
        "AddMessage allows writing and reading messages after registration") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{42});
      world.Update();
      int count = 0;
      for (const auto msg : world.ReadMessages<GameMsg>()) {
        CHECK_EQ(msg->value, 42);
        ++count;
      }
      CHECK_EQ(count, 1);
    }
  }

  TEST_CASE("ecs::World::AddMessages") {
    SUBCASE("AddMessages registers all provided message types") {
      World world;
      world.AddMessages<GameMsg, AsyncGameMsg>();
      CHECK(world.HasMessage<GameMsg>());
      CHECK(world.HasMessage<AsyncGameMsg>());
    }
  }

  TEST_CASE("ecs::World::ClearMessages (all queues)") {
    SUBCASE("ClearMessages clears all queued messages without removing types") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{1});
      world.Update();
      world.ClearMessages();
      CHECK_FALSE(world.HasMessages<GameMsg>());
      CHECK(world.HasMessage<GameMsg>());
    }
  }

  TEST_CASE("ecs::World::ClearMessages<T>") {
    SUBCASE("ClearMessages<T> clears only the specified message type") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{1});
      world.Update();
      world.ClearMessages<GameMsg>();
      CHECK_FALSE(world.HasMessages<GameMsg>());
    }
  }

  TEST_CASE("ecs::World::ReadMessages") {
    SUBCASE(
        "ReadMessages returns all messages written in the previous update") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{10});
      world.WriteMessages<GameMsg>().Write(GameMsg{20});
      world.Update();

      std::vector<int> values;
      for (const auto msg : world.ReadMessages<GameMsg>()) {
        values.push_back(msg->value);
      }
      REQUIRE_EQ(values.size(), 2);
      CHECK_EQ(values[0], 10);
      CHECK_EQ(values[1], 20);
    }

    SUBCASE("ReadMessages returns empty range when no messages were written") {
      World world;
      world.AddMessage<GameMsg>();
      world.Update();

      int count = 0;
      for ([[maybe_unused]] const auto& _ : world.ReadMessages<GameMsg>()) {
        ++count;
      }
      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::World::WriteMessages") {
    SUBCASE("WriteMessages returns a writer that enqueues messages") {
      World world;
      world.AddMessage<GameMsg>();
      const auto writer = world.WriteMessages<GameMsg>();
      writer.Write(GameMsg{5});
      world.Update();
      CHECK(world.HasMessages<GameMsg>());
    }

    SUBCASE("Multiple writers all enqueue into the same message stream") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{1});
      world.WriteMessages<GameMsg>().Write(GameMsg{2});
      world.Update();

      int count = 0;
      for ([[maybe_unused]] const auto& _ : world.ReadMessages<GameMsg>()) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::World::ReadAsyncMessages") {
    SUBCASE("ReadAsyncMessages returns a reader for the async queue") {
      World world;
      world.AddMessage<AsyncGameMsg>();
      auto writer = world.WriteAsyncMessages<AsyncGameMsg>();
      writer.Write(AsyncGameMsg{7});

      const auto reader = world.ReadAsyncMessages<AsyncGameMsg>();
      CHECK_FALSE(reader.Empty());
    }
  }

  TEST_CASE("ecs::World::WriteAsyncMessages") {
    SUBCASE("WriteAsyncMessages returns a writer for the async queue") {
      World world;
      world.AddMessage<AsyncGameMsg>();
      auto writer = world.WriteAsyncMessages<AsyncGameMsg>();
      writer.Write(AsyncGameMsg{3});

      AsyncGameMsg msg;
      const auto reader = world.ReadAsyncMessages<AsyncGameMsg>();
      const bool dequeued = reader.Dequeue(msg);
      CHECK(dequeued);
      CHECK_EQ(msg.value, 3);
    }
  }

  TEST_CASE("ecs::World::ReserveCommands") {
    SUBCASE("ReserveCommands does not change CommandCount") {
      World world;
      world.ReserveCommands(100);
      CHECK_EQ(world.CommandCount(), 0);
    }

    SUBCASE("ReserveCommands does not prevent commands from being enqueued") {
      World world;
      world.ReserveCommands(10);
      world.EnqueueCommand(AddEntityCmd{});
      CHECK_EQ(world.CommandCount(), 1);
    }
  }

  TEST_CASE("ecs::World::EnqueueCommand") {
    SUBCASE("EnqueueCommand adds a command to the queue") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      CHECK_EQ(world.CommandCount(), 1);
    }

    SUBCASE("Enqueued command is executed during Update") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      CHECK_EQ(world.EntityCount(), 1);
    }

    SUBCASE("Multiple enqueued commands execute in order") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      CHECK_EQ(world.EntityCount(), 2);
    }
  }

  TEST_CASE("ecs::World::EnqueueCommandBulk") {
    SUBCASE("EnqueueCommandBulk enqueues all commands in the range") {
      World world;
      std::vector<AddEntityCmd> cmds(3);
      world.EnqueueCommandBulk(cmds);
      CHECK_EQ(world.CommandCount(), 3);
    }

    SUBCASE("EnqueueCommandBulk with empty range is a no-op") {
      World world;
      const std::vector<AddEntityCmd> empty;
      world.EnqueueCommandBulk(empty);
      CHECK_EQ(world.CommandCount(), 0);
    }

    SUBCASE("All bulk-enqueued commands are executed during Update") {
      World world;
      std::vector<AddEntityCmd> cmds(4);
      world.EnqueueCommandBulk(cmds);
      world.Update();
      CHECK_EQ(world.EntityCount(), 4);
    }
  }

  TEST_CASE("ecs::World::Exists") {
    SUBCASE("Exists returns true for a living entity") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK(world.Exists(entity));
    }

    SUBCASE("Exists returns false after entity is destroyed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.DestroyEntity(entity);
      CHECK_FALSE(world.Exists(entity));
    }
  }

  TEST_CASE("ecs::World::HasComponent") {
    SUBCASE("HasComponent returns true when entity has the component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      CHECK(world.HasComponent<Position>(entity));
    }

    SUBCASE(
        "HasComponent returns false when entity does not have the "
        "component") {
      World world;
      const Entity entity = world.CreateEntity();
      CHECK_FALSE(world.HasComponent<Position>(entity));
    }

    SUBCASE("HasComponent returns false after the component is removed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.RemoveComponents<Position>(entity);
      CHECK_FALSE(world.HasComponent<Position>(entity));
    }
  }

  TEST_CASE("ecs::World::HasComponents") {
    SUBCASE(
        "HasComponents returns correct flags for present and absent types") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      const auto result = world.HasComponents<Position, Velocity>(entity);
      CHECK(result[0]);
      CHECK_FALSE(result[1]);
    }

    SUBCASE("HasComponents returns all true when entity has all components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      const auto result = world.HasComponents<Position, Velocity>(entity);
      CHECK(result[0]);
      CHECK(result[1]);
    }
  }

  TEST_CASE("ecs::World::HasResource") {
    SUBCASE("HasResource returns true when resource exists") {
      World world;
      world.InsertResources(DeltaTime{});
      CHECK(world.HasResource<DeltaTime>());
    }

    SUBCASE("HasResource returns false when resource does not exist") {
      World world;
      CHECK_FALSE(world.HasResource<DeltaTime>());
    }

    SUBCASE("HasResource returns false after resource is removed") {
      World world;
      world.InsertResources(DeltaTime{});
      world.RemoveResources<DeltaTime>();
      CHECK_FALSE(world.HasResource<DeltaTime>());
    }
  }

  TEST_CASE("ecs::World::HasMessage") {
    SUBCASE("HasMessage returns true for a registered message type") {
      World world;
      world.AddMessage<GameMsg>();
      CHECK(world.HasMessage<GameMsg>());
    }

    SUBCASE("HasMessage returns false for an unregistered message type") {
      World world;
      CHECK_FALSE(world.HasMessage<GameMsg>());
    }
  }

  TEST_CASE("ecs::World::HasMessages") {
    SUBCASE("HasMessages returns true when messages are present in the queue") {
      World world;
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{1});
      world.Update();
      CHECK(world.HasMessages<GameMsg>());
    }

    SUBCASE("HasMessages returns false when no messages have been written") {
      World world;
      world.AddMessage<GameMsg>();
      world.Update();
      CHECK_FALSE(world.HasMessages<GameMsg>());
    }
  }

  TEST_CASE("ecs::World::HasCommands") {
    SUBCASE("HasCommands returns false when queue is empty") {
      World world;
      CHECK_FALSE(world.HasCommands());
    }

    SUBCASE("HasCommands returns true after a command is enqueued") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      CHECK(world.HasCommands());
    }

    SUBCASE("HasCommands returns false after Update flushes the queue") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      CHECK_FALSE(world.HasCommands());
    }
  }

  TEST_CASE("ecs::World::EntityCount") {
    SUBCASE("EntityCount returns 0 for a fresh world") {
      World world;
      CHECK_EQ(world.EntityCount(), 0);
    }

    SUBCASE("EntityCount increases with each entity creation") {
      World world;
      [[maybe_unused]] const auto e1 = world.CreateEntity();
      [[maybe_unused]] const auto e2 = world.CreateEntity();
      CHECK_EQ(world.EntityCount(), 2);
    }

    SUBCASE("EntityCount decreases after entity destruction") {
      World world;
      const Entity entity = world.CreateEntity();
      world.DestroyEntity(entity);
      CHECK_EQ(world.EntityCount(), 0);
    }
  }

  TEST_CASE("ecs::World::CommandCount") {
    SUBCASE("CommandCount returns 0 for a fresh world") {
      World world;
      CHECK_EQ(world.CommandCount(), 0);
    }

    SUBCASE("CommandCount increases with each enqueued command") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.EnqueueCommand(AddEntityCmd{});
      CHECK_EQ(world.CommandCount(), 2);
    }

    SUBCASE("CommandCount returns 0 after Update executes all commands") {
      World world;
      world.EnqueueCommand(AddEntityCmd{});
      world.Update();
      CHECK_EQ(world.CommandCount(), 0);
    }
  }

  TEST_CASE("ecs::World::Entities") {
    SUBCASE("Entities returns a mutable reference to the entity manager") {
      World world;
      [[maybe_unused]] const auto entity = world.CreateEntity();
      CHECK_EQ(world.Entities().Count(), 1);
    }

    SUBCASE(
        "Entities const overload returns a const reference to the entity "
        "manager") {
      World world;
      [[maybe_unused]] const auto entity = world.CreateEntity();
      CHECK_EQ(std::as_const(world).Entities().Count(), 1);
    }
  }

  TEST_CASE("ecs::World::Components") {
    SUBCASE("Components returns a mutable reference to the component manager") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      CHECK(world.Components().Has<Position>(entity));
    }

    SUBCASE(
        "Components const overload returns a const reference to the "
        "component "
        "manager") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      CHECK(std::as_const(world).Components().Has<Position>(entity));
    }
  }

  TEST_CASE("ecs::World::Resources") {
    SUBCASE("Resources returns a mutable reference to the resource manager") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      CHECK(world.Resources().Has<DeltaTime>());
    }

    SUBCASE(
        "Resources const overload returns a const reference to the resource "
        "manager") {
      World world;
      world.InsertResources(DeltaTime{2.0F});
      CHECK(std::as_const(world).Resources().Has<DeltaTime>());
    }
  }

  TEST_CASE("ecs::World: message lifecycle") {
    SUBCASE(
        "Messages written in Update N-1 are readable in N and cleared in "
        "N+1") {
      World world;
      world.AddMessage<GameMsg>();

      world.WriteMessages<GameMsg>().Write(GameMsg{10});
      world.Update();

      CHECK(world.HasMessages<GameMsg>());

      world.Update();
      CHECK_FALSE(world.HasMessages<GameMsg>());
    }

    SUBCASE("Manual policy messages survive multiple Update calls") {
      World world;
      world.AddMessage<ManualGameMsg>();

      world.WriteMessages<ManualGameMsg>().Write(ManualGameMsg{5});
      world.Update();
      CHECK(world.HasMessages<ManualGameMsg>());

      world.Update();
      world.Update();
      CHECK(world.HasMessages<ManualGameMsg>());
    }
  }
}  // TEST_SUITE("ecs::World")
