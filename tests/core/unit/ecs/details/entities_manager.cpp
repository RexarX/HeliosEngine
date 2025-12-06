#include <doctest/doctest.h>

#include <helios/core/ecs/details/entities_manager.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <set>
#include <thread>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

TEST_SUITE("ecs::details::EntitiesManager") {
  TEST_CASE("Entities::Entities: default construction") {
    Entities entities;

    CHECK_EQ(entities.Count(), 0);
  }

  TEST_CASE("Entities::CreateEntity: basic") {
    Entities entities;

    Entity entity = entities.CreateEntity();

    CHECK(entity.Valid());
    CHECK_EQ(entity.Index(), 0);
    CHECK_EQ(entity.Generation(), 1);
    CHECK_EQ(entities.Count(), 1);
    CHECK(entities.IsValid(entity));
  }

  TEST_CASE("Entities::CreateEntity: multiple") {
    Entities entities;

    Entity entity1 = entities.CreateEntity();
    Entity entity2 = entities.CreateEntity();
    Entity entity3 = entities.CreateEntity();

    CHECK(entity1.Valid());
    CHECK(entity2.Valid());
    CHECK(entity3.Valid());

    CHECK_EQ(entity1.Index(), 0);
    CHECK_EQ(entity2.Index(), 1);
    CHECK_EQ(entity3.Index(), 2);

    CHECK_EQ(entity1.Generation(), 1);
    CHECK_EQ(entity2.Generation(), 1);
    CHECK_EQ(entity3.Generation(), 1);

    CHECK_EQ(entities.Count(), 3);
    CHECK(entities.IsValid(entity1));
    CHECK(entities.IsValid(entity2));
    CHECK(entities.IsValid(entity3));
  }

  TEST_CASE("Entities::CreateEntities: bulk with back_inserter") {
    Entities entities;

    std::vector<Entity> created;
    created.reserve(5);
    entities.CreateEntities(5, std::back_inserter(created));

    CHECK_EQ(created.size(), 5);
    CHECK_EQ(entities.Count(), 5);

    for (size_t i = 0; i < 5; ++i) {
      CHECK(created[i].Valid());
      CHECK_EQ(created[i].Index(), static_cast<Entity::IndexType>(i));
      CHECK_EQ(created[i].Generation(), 1);
      CHECK(entities.IsValid(created[i]));
    }
  }

  TEST_CASE("Entities::CreateEntities: with pre-allocated array") {
    Entities entities;

    std::array<Entity, 10> created{};
    auto end_it = entities.CreateEntities(10, created.begin());

    CHECK_EQ(end_it, created.end());
    CHECK_EQ(entities.Count(), 10);

    for (size_t i = 0; i < 10; ++i) {
      CHECK(created[i].Valid());
      CHECK_EQ(created[i].Index(), static_cast<Entity::IndexType>(i));
      CHECK_EQ(created[i].Generation(), 1);
      CHECK(entities.IsValid(created[i]));
    }
  }

  TEST_CASE("Entities::CreateEntities: with raw array") {
    Entities entities;

    Entity buffer[5];
    auto end_it = entities.CreateEntities(5, std::begin(buffer));

    CHECK_EQ(end_it, std::end(buffer));
    CHECK_EQ(entities.Count(), 5);

    for (size_t i = 0; i < 5; ++i) {
      CHECK(buffer[i].Valid());
      CHECK_EQ(buffer[i].Index(), static_cast<Entity::IndexType>(i));
      CHECK(entities.IsValid(buffer[i]));
    }
  }

  TEST_CASE("Entities::CreateEntities: zero count") {
    Entities entities;

    std::vector<Entity> created;
    auto result = entities.CreateEntities(0, std::back_inserter(created));

    CHECK(created.empty());
    CHECK_EQ(entities.Count(), 0);
    static_cast<void>(result);  // Suppress unused variable warning
  }

  TEST_CASE("Entities::Destroy: single entity") {
    Entities entities;

    Entity entity = entities.CreateEntity();
    CHECK_EQ(entities.Count(), 1);
    CHECK(entities.IsValid(entity));

    entities.Destroy(entity);
    CHECK_EQ(entities.Count(), 0);
    CHECK_FALSE(entities.IsValid(entity));
  }

  TEST_CASE("Entities::Destroy: reuse index") {
    Entities entities;

    Entity entity1 = entities.CreateEntity();
    CHECK_EQ(entity1.Index(), 0);
    CHECK_EQ(entity1.Generation(), 1);

    entities.Destroy(entity1);
    CHECK_FALSE(entities.IsValid(entity1));

    Entity entity2 = entities.CreateEntity();
    CHECK_EQ(entity2.Index(), 0);       // Same index
    CHECK_EQ(entity2.Generation(), 2);  // Different generation
    CHECK(entities.IsValid(entity2));
    CHECK_FALSE(entities.IsValid(entity1));  // Old entity should still be invalid
  }

  TEST_CASE("Entities::Destroy: multiple entities") {
    Entities entities;

    std::vector<Entity> created;
    created.reserve(5);
    entities.CreateEntities(5, std::back_inserter(created));
    CHECK_EQ(entities.Count(), 5);

    std::vector<Entity> to_destroy = {created[1], created[3]};
    entities.Destroy(to_destroy);

    CHECK_EQ(entities.Count(), 3);
    CHECK(entities.IsValid(created[0]));
    CHECK_FALSE(entities.IsValid(created[1]));
    CHECK(entities.IsValid(created[2]));
    CHECK_FALSE(entities.IsValid(created[3]));
    CHECK(entities.IsValid(created[4]));
  }

  TEST_CASE("Entities: generation increment") {
    Entities entities;

    Entity entity1 = entities.CreateEntity();
    CHECK_EQ(entity1.Generation(), 1);

    entities.Destroy(entity1);
    Entity entity2 = entities.CreateEntity();  // Should reuse index but increment generation
    CHECK_EQ(entity2.Index(), entity1.Index());
    CHECK_EQ(entity2.Generation(), 2);

    entities.Destroy(entity2);
    Entity entity3 = entities.CreateEntity();
    CHECK_EQ(entity3.Index(), entity1.Index());
    CHECK_EQ(entity3.Generation(), 3);
  }

  TEST_CASE("Entities::IsValid: invalid entity") {
    Entities entities;

    Entity invalid_entity;
    CHECK_FALSE(entities.IsValid(invalid_entity));

    Entity nonexistent_entity(999, 1);
    CHECK_FALSE(entities.IsValid(nonexistent_entity));
  }

  TEST_CASE("Entities::IsValid: wrong generation") {
    Entities entities;

    Entity entity = entities.CreateEntity();
    CHECK(entities.IsValid(entity));

    Entity wrong_generation(entity.Index(), entity.Generation() + 1);
    CHECK_FALSE(entities.IsValid(wrong_generation));

    Entity wrong_generation2(entity.Index(), entity.Generation() - 1);
    CHECK_FALSE(entities.IsValid(wrong_generation2));
  }

  TEST_CASE("Entities::Clear") {
    Entities entities;

    std::vector<Entity> entities_vec;
    entities_vec.reserve(10);
    entities.CreateEntities(10, std::back_inserter(entities_vec));
    CHECK_EQ(entities.Count(), 10);

    entities.Clear();
    CHECK_EQ(entities.Count(), 0);

    // Should be able to create new entities after clear
    Entity entity = entities.CreateEntity();
    CHECK(entity.Valid());
    CHECK_EQ(entity.Index(), 0);
    CHECK_EQ(entity.Generation(), 1);
    CHECK_EQ(entities.Count(), 1);
  }

  TEST_CASE("Entities::Reserve") {
    Entities entities;

    entities.Reserve(1000);

    // Should not affect count
    CHECK_EQ(entities.Count(), 0);

    // Should be able to create entities normally
    std::vector<Entity> created;
    created.reserve(100);
    entities.CreateEntities(100, std::back_inserter(created));
    CHECK_EQ(created.size(), 100);
    CHECK_EQ(entities.Count(), 100);

    for (size_t i = 0; i < 100; ++i) {
      CHECK(entities.IsValid(created[i]));
    }
  }

  TEST_CASE("Entities::ReserveEntity: thread safety") {
    Entities entities;

    // Test basic reservation
    Entity reserved = entities.ReserveEntity();
    CHECK(reserved.Valid());
    CHECK_EQ(reserved.Index(), 0);
    CHECK_EQ(reserved.Generation(), 1);

    // Before flushing, the entity should not be considered valid by IsValid
    CHECK_FALSE(entities.IsValid(reserved));
    CHECK_EQ(entities.Count(), 0);

    // After flushing, it should be valid
    entities.FlushReservedEntities();
    CHECK(entities.IsValid(reserved));
    CHECK_EQ(entities.Count(), 1);
  }

  TEST_CASE("Entities::ReserveEntity: multiple") {
    Entities entities;

    constexpr int count = 5;
    std::vector<Entity> reserved;
    reserved.reserve(count);

    for (int i = 0; i < count; ++i) {
      reserved.push_back(entities.ReserveEntity());
    }

    CHECK_EQ(entities.Count(), 0);  // Count should still be 0 before flushing

    for (size_t i = 0; i < reserved.size(); ++i) {
      CHECK(reserved[i].Valid());
      CHECK_EQ(reserved[i].Index(), static_cast<Entity::IndexType>(i));
      CHECK_EQ(reserved[i].Generation(), 1);
      CHECK_FALSE(entities.IsValid(reserved[i]));  // Not valid until flushed
    }

    entities.FlushReservedEntities();
    CHECK_EQ(entities.Count(), count);

    for (const auto& entity : reserved) {
      CHECK(entities.IsValid(entity));
    }
  }

  TEST_CASE("Entities::FlushReservedEntities: multiple times") {
    Entities entities;

    Entity reserved1 = entities.ReserveEntity();
    entities.FlushReservedEntities();
    CHECK(entities.IsValid(reserved1));
    CHECK_EQ(entities.Count(), 1);

    Entity reserved2 = entities.ReserveEntity();
    entities.FlushReservedEntities();
    CHECK(entities.IsValid(reserved2));
    CHECK_EQ(entities.Count(), 2);

    // Both should still be valid
    CHECK(entities.IsValid(reserved1));
    CHECK(entities.IsValid(reserved2));
  }

  TEST_CASE("Entities: mixed reserved and direct creation") {
    Entities entities;

    Entity reserved = entities.ReserveEntity();
    Entity direct = entities.CreateEntity();

    CHECK_EQ(reserved.Index(), 0);
    CHECK_EQ(direct.Index(), 1);
    CHECK_EQ(entities.Count(), 1);  // Only direct entity counts before flush

    entities.FlushReservedEntities();
    CHECK_EQ(entities.Count(), 2);
    CHECK(entities.IsValid(reserved));
    CHECK(entities.IsValid(direct));
  }

  TEST_CASE("Entities::Destroy: with range") {
    Entities entities;

    constexpr int count = 10;
    std::vector<Entity> created;
    created.reserve(count);
    entities.CreateEntities(count, std::back_inserter(created));
    CHECK_EQ(entities.Count(), count);

    std::vector<Entity> to_destroy = {created[2], created[5], created[8]};
    entities.Destroy(to_destroy);

    CHECK_EQ(entities.Count(), 7);

    for (size_t i = 0; i < count; ++i) {
      if (i == 2 || i == 5 || i == 8) {
        CHECK_FALSE(entities.IsValid(created[i]));
      } else {
        CHECK(entities.IsValid(created[i]));
      }
    }
  }

  TEST_CASE("Entities: stress test") {
    Entities entities;
    constexpr size_t entity_count = 10000;

    // Create many entities
    std::vector<Entity> created;
    created.reserve(entity_count);
    entities.CreateEntities(entity_count, std::back_inserter(created));
    CHECK_EQ(created.size(), entity_count);
    CHECK_EQ(entities.Count(), entity_count);

    // Verify all are valid and have correct indices
    for (size_t i = 0; i < entity_count; ++i) {
      CHECK(created[i].Valid());
      CHECK_EQ(created[i].Index(), static_cast<Entity::IndexType>(i));
      CHECK_EQ(created[i].Generation(), 1);
      CHECK(entities.IsValid(created[i]));
    }

    // Destroy every other entity
    std::vector<Entity> to_destroy;
    for (size_t i = 0; i < entity_count; i += 2) {
      to_destroy.push_back(created[i]);
    }

    entities.Destroy(to_destroy);
    CHECK_EQ(entities.Count(), entity_count / 2);

    // Verify correct entities are destroyed
    for (size_t i = 0; i < entity_count; ++i) {
      if (i % 2 == 0) {
        CHECK_FALSE(entities.IsValid(created[i]));
      } else {
        CHECK(entities.IsValid(created[i]));
      }
    }
  }

  TEST_CASE("Entities: free index reuse pattern") {
    Entities entities;

    // Create 5 entities
    std::vector<Entity> created;
    created.reserve(5);
    entities.CreateEntities(5, std::back_inserter(created));

    // Destroy entities 1 and 3 (indices 1 and 3)
    entities.Destroy(created[1]);
    entities.Destroy(created[3]);
    CHECK_EQ(entities.Count(), 3);

    // Create new entities - should reuse freed indices in LIFO order
    Entity new1 = entities.CreateEntity();
    Entity new2 = entities.CreateEntity();

    // Should reuse index 3 first (last destroyed), then index 1
    CHECK_EQ(new1.Index(), 3);
    CHECK_EQ(new1.Generation(), 2);  // Generation incremented
    CHECK_EQ(new2.Index(), 1);
    CHECK_EQ(new2.Generation(), 2);  // Generation incremented

    CHECK_EQ(entities.Count(), 5);
    CHECK(entities.IsValid(new1));
    CHECK(entities.IsValid(new2));
  }

  TEST_CASE("Entities::ReserveEntity: thread safety concurrent") {
    Entities entities;
    constexpr int thread_count = 4;
    constexpr int entities_per_thread = 250;

    std::vector<std::thread> threads;
    std::vector<std::vector<Entity>> thread_entities(thread_count);

    // Launch threads that reserve entities concurrently
    for (int t = 0; t < thread_count; ++t) {
      threads.emplace_back([&entities, &thread_entities, t, entities_per_thread]() {
        for (int i = 0; i < entities_per_thread; ++i) {
          thread_entities[t].push_back(entities.ReserveEntity());
        }
      });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }

    // Flush reserved entities
    entities.FlushReservedEntities();

    // Verify all entities are unique and valid
    std::set<Entity::IndexType> used_indices;
    size_t total_entities = 0;

    for (int t = 0; t < thread_count; ++t) {
      CHECK_EQ(thread_entities[t].size(), entities_per_thread);
      total_entities += thread_entities[t].size();

      for (const auto& entity : thread_entities[t]) {
        CHECK(entity.Valid());
        CHECK(entities.IsValid(entity));
        CHECK_EQ(entity.Generation(), 1);

        // Index should be unique
        CHECK(used_indices.find(entity.Index()) == used_indices.end());
        used_indices.insert(entity.Index());
      }
    }

    CHECK_EQ(total_entities, thread_count * entities_per_thread);
    CHECK_EQ(entities.Count(), total_entities);
    CHECK_EQ(used_indices.size(), total_entities);
  }

  TEST_CASE("Entities: edge cases") {
    Entities entities;

    // Test creating zero entities
    std::vector<Entity> empty;
    entities.CreateEntities(0, std::back_inserter(empty));
    CHECK(empty.empty());
    CHECK_EQ(entities.Count(), 0);

    // Test destroying empty range
    std::vector<Entity> empty_range;
    entities.Destroy(empty_range);  // Should not crash
    CHECK_EQ(entities.Count(), 0);

    // Test multiple flushes without reservations
    entities.FlushReservedEntities();
    entities.FlushReservedEntities();
    CHECK_EQ(entities.Count(), 0);
  }

  TEST_CASE("Entities::CreateEntities: returns correct iterator") {
    Entities entities;

    std::vector<Entity> created;
    created.reserve(10);

    auto result_it = entities.CreateEntities(5, std::back_inserter(created));
    static_cast<void>(result_it);  // back_inserter returns itself

    CHECK_EQ(created.size(), 5);

    // Create more entities using the same vector
    entities.CreateEntities(3, std::back_inserter(created));
    CHECK_EQ(created.size(), 8);
    CHECK_EQ(entities.Count(), 8);
  }

  TEST_CASE("ecs::details::Entities - Lifecycle Integration") {
    Entities entities;

    // Mixed operations to test integration
    Entity reserved1 = entities.ReserveEntity();
    Entity direct1 = entities.CreateEntity();
    Entity reserved2 = entities.ReserveEntity();

    CHECK_EQ(entities.Count(), 1);  // Only direct1

    entities.FlushReservedEntities();
    CHECK_EQ(entities.Count(), 3);

    Entity direct2 = entities.CreateEntity();
    CHECK_EQ(entities.Count(), 4);

    // Destroy some entities
    entities.Destroy(direct1);
    entities.Destroy(reserved2);
    CHECK_EQ(entities.Count(), 2);

    // Create new entities to test reuse
    Entity new1 = entities.CreateEntity();
    Entity new2 = entities.CreateEntity();
    CHECK_EQ(entities.Count(), 4);

    // All remaining entities should be valid
    CHECK(entities.IsValid(reserved1));
    CHECK_FALSE(entities.IsValid(direct1));    // Destroyed
    CHECK_FALSE(entities.IsValid(reserved2));  // Destroyed
    CHECK(entities.IsValid(direct2));
    CHECK(entities.IsValid(new1));
    CHECK(entities.IsValid(new2));
  }

}  // TEST_SUITE
