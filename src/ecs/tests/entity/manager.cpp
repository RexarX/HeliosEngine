#include <doctest/doctest.h>

#include <helios/ecs/entity/manager.hpp>

#include <algorithm>
#include <array>
#include <iterator>
#include <vector>

using namespace helios::ecs;

TEST_SUITE("helios::ecs::EntityManager") {
  TEST_CASE("ecs::EntityManager::ctor") {
    SUBCASE("Default ctor") {
      const EntityManager manager;
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Copy ctor") {
      EntityManager original;
      [[maybe_unused]] auto e1 = original.Create();
      [[maybe_unused]] auto e2 = original.Create();

      const EntityManager copy(original);

      CHECK_EQ(copy.Count(), original.Count());
    }

    SUBCASE("Move ctor") {
      EntityManager original;
      const auto entity1 = original.Create();
      const auto entity2 = original.Create();
      const auto original_count = original.Count();

      const EntityManager moved(std::move(original));

      CHECK_EQ(moved.Count(), original_count);
      CHECK(moved.Validate(entity1));
      CHECK(moved.Validate(entity2));
    }
  }

  TEST_CASE("ecs::EntityManager::assignment") {
    SUBCASE("Copy assignment") {
      EntityManager original;
      [[maybe_unused]] auto e1 = original.Create();
      [[maybe_unused]] auto e2 = original.Create();

      EntityManager assigned;
      assigned = original;

      CHECK_EQ(assigned.Count(), original.Count());
    }

    SUBCASE("Move assignment") {
      EntityManager original;
      const auto entity = original.Create();
      const auto original_count = original.Count();

      EntityManager assigned;
      assigned = std::move(original);

      CHECK_EQ(assigned.Count(), original_count);
      CHECK(assigned.Validate(entity));
    }

    SUBCASE("Self assignment") {
      EntityManager manager;
      [[maybe_unused]] auto _ = manager.Create();

      manager = manager;

      CHECK_EQ(manager.Count(), 1);
    }
  }

  TEST_CASE("ecs::EntityManager::Create") {
    SUBCASE("Create single entity") {
      EntityManager manager;

      const auto entity = manager.Create();

      CHECK(entity.Valid());
      CHECK_EQ(manager.Count(), 1);
      CHECK(manager.Validate(entity));
    }

    SUBCASE("Create multiple entities") {
      EntityManager manager;

      const auto entity1 = manager.Create();
      const auto entity2 = manager.Create();
      const auto entity3 = manager.Create();

      CHECK(entity1.Valid());
      CHECK(entity2.Valid());
      CHECK(entity3.Valid());

      CHECK_NE(entity1, entity2);
      CHECK_NE(entity2, entity3);
      CHECK_NE(entity1, entity3);

      CHECK_EQ(manager.Count(), 3);
    }

    SUBCASE("Created entities have unique indices") {
      EntityManager manager;

      std::vector<Entity> entities(100);
      for (auto& entity : entities) {
        entity = manager.Create();
      }

      std::vector<Entity::IndexType> indices;
      indices.reserve(100);
      for (const auto& entity : entities) {
        indices.push_back(entity.Index());
      }

      std::ranges::sort(indices);
      const auto last = std::ranges::unique(indices);
      CHECK_EQ(last.begin(), indices.end());
    }

    SUBCASE("Created entities are alive") {
      EntityManager manager;
      const auto entity = manager.Create();
      CHECK(manager.Validate(entity));
    }
  }

  TEST_CASE("ecs::EntityManager::Create/batch") {
    SUBCASE("Create multiple entities with back_inserter") {
      EntityManager manager;
      std::vector<Entity> entities;

      manager.Create(10, std::back_inserter(entities));

      CHECK_EQ(entities.size(), 10);
      CHECK_EQ(manager.Count(), 10);

      const bool all_valid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return entity.Valid() && manager.Validate(entity);
          });
      CHECK(all_valid);
    }

    SUBCASE("Create entities into array") {
      EntityManager manager;
      std::array<Entity, 5> entities = {};

      manager.Create(5, entities.begin());

      CHECK_EQ(manager.Count(), 5);

      const bool all_valid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return entity.Valid() && manager.Validate(entity);
          });
      CHECK(all_valid);
    }

    SUBCASE("Create zero entities") {
      EntityManager manager;
      std::vector<Entity> entities;

      manager.Create(0, std::back_inserter(entities));

      CHECK(entities.empty());
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Create large batch of entities") {
      EntityManager manager;
      std::vector<Entity> entities;

      manager.Create(1000, std::back_inserter(entities));

      CHECK_EQ(entities.size(), 1000);
      CHECK_EQ(manager.Count(), 1000);

      const bool all_valid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return entity.Valid() && manager.Validate(entity);
          });
      CHECK(all_valid);
    }
  }

  TEST_CASE("ecs::EntityManager::Destroy") {
    SUBCASE("Destroy single entity") {
      EntityManager manager;
      const auto entity = manager.Create();

      manager.Destroy(entity);

      CHECK_FALSE(manager.Validate(entity));
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Destroy multiple entities") {
      EntityManager manager;
      const auto entity1 = manager.Create();
      const auto entity2 = manager.Create();
      const auto entity3 = manager.Create();

      manager.Destroy(entity1);
      manager.Destroy(entity2);

      CHECK_FALSE(manager.Validate(entity1));
      CHECK_FALSE(manager.Validate(entity2));
      CHECK(manager.Validate(entity3));
      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Destroy all entities") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);
      manager.Create(10, std::back_inserter(entities));

      for (const auto& entity : entities) {
        manager.Destroy(entity);
      }

      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Destroy and recreate entity") {
      EntityManager manager;

      const auto entity1 = manager.Create();
      manager.Destroy(entity1);
      const auto entity2 = manager.Create();

      CHECK(entity2.Valid());
      CHECK_NE(entity1, entity2);
      CHECK_EQ(entity1.Index(), entity2.Index());
      CHECK_NE(entity1.Generation(), entity2.Generation());
    }

    SUBCASE("Destroyed entity is not alive") {
      EntityManager manager;
      const auto entity = manager.Create();

      manager.Destroy(entity);

      CHECK_FALSE(manager.Validate(entity));
    }

    SUBCASE("Destroy range of entities") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);
      manager.Create(10, std::back_inserter(entities));

      manager.Destroy(entities);

      CHECK_EQ(manager.Count(), 0);

      for (const auto& entity : entities) {
        CHECK_FALSE(manager.Validate(entity));
      }
    }

    SUBCASE("Destroy partial range") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);
      manager.Create(10, std::back_inserter(entities));

      std::vector<Entity> to_destroy(entities.begin(), entities.begin() + 5);
      manager.Destroy(to_destroy);

      CHECK_EQ(manager.Count(), 5);

      const bool not_valid = std::all_of(entities.begin(), entities.begin() + 5,
                                         [&manager](const Entity& entity) {
                                           return !manager.Validate(entity);
                                         });
      CHECK(not_valid);
      const bool valid = std::all_of(entities.begin() + 5, entities.end(),
                                     [&manager](const Entity& entity) {
                                       return manager.Validate(entity);
                                     });
      CHECK(valid);
    }
  }

  TEST_CASE("ecs::EntityManager::Validate") {
    SUBCASE("Validate created entity") {
      EntityManager manager;
      const auto entity = manager.Create();
      CHECK(manager.Validate(entity));
    }

    SUBCASE("Validate destroyed entity") {
      EntityManager manager;

      const auto entity = manager.Create();
      manager.Destroy(entity);

      CHECK_FALSE(manager.Validate(entity));
    }

    SUBCASE("Validate invalid entity") {
      const EntityManager manager;
      const Entity invalid;
      CHECK_FALSE(manager.Validate(invalid));
    }

    SUBCASE("Validate entity from different manager") {
      EntityManager manager1;
      const EntityManager manager2;

      const auto entity = manager1.Create();

      CHECK(manager1.Validate(entity));
      CHECK_FALSE(manager2.Validate(entity));
    }
  }

  TEST_CASE("ecs::EntityManager::Count") {
    SUBCASE("Count is zero initially") {
      const EntityManager manager;
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Count increases with creation") {
      EntityManager manager;

      [[maybe_unused]] auto e1 = manager.Create();
      CHECK_EQ(manager.Count(), 1);

      [[maybe_unused]] auto e2 = manager.Create();
      CHECK_EQ(manager.Count(), 2);

      [[maybe_unused]] auto e3 = manager.Create();
      CHECK_EQ(manager.Count(), 3);
    }

    SUBCASE("Count decreases with destruction") {
      EntityManager manager;
      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      const auto e3 = manager.Create();

      CHECK_EQ(manager.Count(), 3);

      manager.Destroy(e1);
      CHECK_EQ(manager.Count(), 2);

      manager.Destroy(e2);
      CHECK_EQ(manager.Count(), 1);

      manager.Destroy(e3);
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Count with batch operations") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);

      manager.Create(10, std::back_inserter(entities));
      CHECK_EQ(manager.Count(), 10);

      manager.Destroy(entities);
      CHECK_EQ(manager.Count(), 0);
    }
  }

  TEST_CASE("ecs::EntityManager::Clear") {
    SUBCASE("Clear with no entities") {
      EntityManager manager;
      manager.Clear();
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Clear with entities") {
      EntityManager manager;
      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      const auto e3 = manager.Create();

      manager.Clear();

      CHECK_EQ(manager.Count(), 0);
      CHECK_FALSE(manager.Validate(e1));
      CHECK_FALSE(manager.Validate(e2));
      CHECK_FALSE(manager.Validate(e3));
    }

    SUBCASE("Clear and recreate") {
      EntityManager manager;
      [[maybe_unused]] auto e1 = manager.Create();
      [[maybe_unused]] auto e2 = manager.Create();

      manager.Clear();
      const auto entity = manager.Create();

      CHECK_EQ(manager.Count(), 1);
      CHECK(manager.Validate(entity));
    }
  }

  TEST_CASE("ecs::EntityManager::Reserve") {
    SUBCASE("Reserve capacity") {
      EntityManager manager;
      manager.Reserve(100);
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Reserve and create") {
      EntityManager manager;
      manager.Reserve(100);

      for (int i = 0; i < 100; ++i) {
        [[maybe_unused]] auto _ = manager.Create();
      }

      CHECK_EQ(manager.Count(), 100);
    }
  }

  TEST_CASE("ecs::EntityManager::ReserveEntity") {
    SUBCASE("Reserve single entity") {
      EntityManager manager;
      const auto reserved = manager.ReserveEntity();
      CHECK(reserved.Valid());
    }

    SUBCASE("Reserve multiple entities") {
      EntityManager manager;

      const auto r1 = manager.ReserveEntity();
      const auto r2 = manager.ReserveEntity();
      const auto r3 = manager.ReserveEntity();

      CHECK(r1.Valid());
      CHECK(r2.Valid());
      CHECK(r3.Valid());

      CHECK_NE(r1, r2);
      CHECK_NE(r2, r3);
    }

    SUBCASE("Flush makes reserved entities valid") {
      EntityManager manager;

      const auto reserved = manager.ReserveEntity();
      manager.Flush();

      CHECK(manager.Validate(reserved));
    }
  }

  TEST_CASE("ecs::EntityManager::generation") {
    SUBCASE("Entity generation increases after destroy and recreate") {
      EntityManager manager;

      const auto entity1 = manager.Create();
      const auto gen1 = entity1.Generation();

      manager.Destroy(entity1);

      const auto entity2 = manager.Create();
      const auto gen2 = entity2.Generation();

      CHECK_EQ(entity1.Index(), entity2.Index());
      CHECK_GT(gen2, gen1);
    }

    SUBCASE("Different entities have independent generations") {
      EntityManager manager;

      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      const auto e3 = manager.Create();

      manager.Destroy(e2);

      const auto e4 = manager.Create();

      CHECK_EQ(e4.Index(), e2.Index());
      CHECK_NE(e4.Generation(), e2.Generation());
    }
  }

  TEST_CASE("ecs::EntityManager::edge_cases") {
    SUBCASE("Create and destroy many entities") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(100);
      manager.Create(100, std::back_inserter(entities));

      CHECK_EQ(manager.Count(), 100);

      for (const auto& entity : entities) {
        manager.Destroy(entity);
      }

      CHECK_EQ(manager.Count(), 0);
      const bool all_invalid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return !manager.Validate(entity);
          });
      CHECK(all_invalid);
    }

    SUBCASE("Interleaved create and destroy") {
      EntityManager manager;

      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      manager.Destroy(e1);
      const auto e3 = manager.Create();
      manager.Destroy(e2);
      const auto e4 = manager.Create();

      CHECK_EQ(manager.Count(), 2);
      CHECK_FALSE(manager.Validate(e1));
      CHECK_FALSE(manager.Validate(e2));
      CHECK(manager.Validate(e3));
      CHECK(manager.Validate(e4));
    }

    SUBCASE("Reuse of entity indices") {
      EntityManager manager;

      const auto e1 = manager.Create();
      const auto index1 = e1.Index();
      manager.Destroy(e1);

      const auto e2 = manager.Create();
      const auto index2 = e2.Index();

      CHECK_EQ(index1, index2);
      CHECK_NE(e1.Generation(), e2.Generation());
    }

    SUBCASE("Copy preserves all entities") {
      EntityManager original;
      std::vector<Entity> entities;
      entities.reserve(10);
      original.Create(10, std::back_inserter(entities));

      EntityManager copy(original);

      CHECK_EQ(copy.Count(), original.Count());
      const bool all_valid = std::ranges::all_of(
          entities,
          [&copy](const Entity& entity) { return copy.Validate(entity); });
      CHECK(all_valid);
    }
  }
}
