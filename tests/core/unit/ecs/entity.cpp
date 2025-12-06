#include <doctest/doctest.h>

#include <helios/core/ecs/entity.hpp>

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

using namespace helios::ecs;

TEST_SUITE("ecs::Entity") {
  TEST_CASE("Entity::ctor: default construction") {
    Entity entity;

    CHECK_FALSE(entity.Valid());
    CHECK_EQ(entity.Index(), Entity::kInvalidIndex);
    CHECK_EQ(entity.Generation(), Entity::kInvalidGeneration);
    CHECK_EQ(entity.Hash(), 0);
  }

  TEST_CASE("Entity::ctor: construction with values") {
    constexpr Entity::IndexType index = 42;
    constexpr Entity::GenerationType generation = 5;

    Entity entity(index, generation);

    CHECK(entity.Valid());
    CHECK_EQ(entity.Index(), index);
    CHECK_EQ(entity.Generation(), generation);
    CHECK_NE(entity.Hash(), 0);
  }

  TEST_CASE("Entity::ctor: invalid values") {
    // Test with invalid index but valid generation
    Entity entity1(Entity::kInvalidIndex, 1);
    CHECK_FALSE(entity1.Valid());
    CHECK_EQ(entity1.Hash(), 0);

    // Test with valid index but invalid generation
    Entity entity2(42, Entity::kInvalidGeneration);
    CHECK_FALSE(entity2.Valid());
    CHECK_EQ(entity2.Hash(), 0);

    // Test with both invalid
    Entity entity3(Entity::kInvalidIndex, Entity::kInvalidGeneration);
    CHECK_FALSE(entity3.Valid());
    CHECK_EQ(entity3.Hash(), 0);
  }

  TEST_CASE("Entity::ctor: copy semantics") {
    Entity original(42, 5);

    // Copy constructor
    Entity copy(original);
    CHECK_EQ(copy.Index(), original.Index());
    CHECK_EQ(copy.Generation(), original.Generation());
    CHECK_EQ(copy.Hash(), original.Hash());
    CHECK_EQ(copy, original);

    // Copy assignment
    Entity assigned;
    assigned = original;
    CHECK_EQ(assigned.Index(), original.Index());
    CHECK_EQ(assigned.Generation(), original.Generation());
    CHECK_EQ(assigned.Hash(), original.Hash());
    CHECK_EQ(assigned, original);
  }

  TEST_CASE("Entity::ctor: move semantics") {
    constexpr Entity::IndexType index = 42;
    constexpr Entity::GenerationType generation = 5;
    Entity original(index, generation);

    // Move constructor
    Entity moved(std::move(original));
    CHECK_EQ(moved.Index(), index);
    CHECK_EQ(moved.Generation(), generation);
    CHECK(moved.Valid());

    // Move assignment
    Entity assigned;
    Entity source(100, 10);
    assigned = std::move(source);
    CHECK_EQ(assigned.Index(), 100);
    CHECK_EQ(assigned.Generation(), 10);
    CHECK(assigned.Valid());
  }

  TEST_CASE("Entity::operator==: equality comparison") {
    Entity entity1(42, 5);
    Entity entity2(42, 5);
    Entity entity3(43, 5);
    Entity entity4(42, 6);
    Entity invalid1;
    Entity invalid2;

    // Same values should be equal
    CHECK_EQ(entity1, entity2);
    CHECK_FALSE(entity1 != entity2);

    // Different index should not be equal
    CHECK_FALSE(entity1 == entity3);
    CHECK_NE(entity1, entity3);

    // Different generation should not be equal
    CHECK_FALSE(entity1 == entity4);
    CHECK_NE(entity1, entity4);

    // Invalid entities should be equal to each other
    CHECK_EQ(invalid1, invalid2);
    CHECK_FALSE(invalid1 != invalid2);

    // Valid entity should not equal invalid entity
    CHECK_FALSE(entity1 == invalid1);
    CHECK_NE(entity1, invalid1);
  }

  TEST_CASE("Entity::operator<: less than comparison") {
    Entity entity1(10, 5);
    Entity entity2(20, 5);
    Entity entity3(10, 6);
    Entity entity4(10, 5);

    // Compare by index first
    CHECK_LT(entity1, entity2);
    CHECK_FALSE(entity2 < entity1);

    // If index is same, compare by generation
    CHECK_LT(entity1, entity3);
    CHECK_FALSE(entity3 < entity1);

    // Same entity should not be less than itself
    CHECK_FALSE(entity1 < entity4);
    CHECK_FALSE(entity4 < entity1);
  }

  TEST_CASE("Entity::Hash: returns correct hash") {
    Entity entity1(42, 5);
    Entity entity2(42, 5);
    Entity entity3(43, 5);
    Entity entity4(42, 6);
    Entity invalid;

    // Same entities should have same hash
    CHECK_EQ(entity1.Hash(), entity2.Hash());

    // Different entities should have different hashes (likely)
    CHECK_NE(entity1.Hash(), entity3.Hash());
    CHECK_NE(entity1.Hash(), entity4.Hash());

    // Invalid entities should have hash of 0
    CHECK_EQ(invalid.Hash(), 0);

    // Valid entities should have non-zero hash
    CHECK_NE(entity1.Hash(), 0);
  }

  TEST_CASE("Entity::Hash: combines index and generation") {
    constexpr Entity::IndexType index = 0x12345678;
    constexpr Entity::GenerationType generation = 0x9ABCDEF0;

    Entity entity(index, generation);
    const size_t hash = entity.Hash();

    // Hash should combine generation (high 32 bits) and index (low 32 bits)
    const size_t expected = (static_cast<size_t>(generation) << (sizeof(size_t) / 2)) | static_cast<size_t>(index);
    CHECK_EQ(hash, expected);
  }

  TEST_CASE("std::hash<Entity>: standard hash specialization") {
    Entity entity1(42, 5);
    Entity entity2(42, 5);
    Entity entity3(43, 5);

    std::hash<Entity> hasher;

    // Same entities should have same hash
    CHECK_EQ(hasher(entity1), hasher(entity2));
    CHECK_EQ(hasher(entity1), static_cast<size_t>(entity1.Hash()));

    // Different entities should have different hashes (likely)
    CHECK_NE(hasher(entity1), hasher(entity3));
  }

  TEST_CASE("Entity: use in hash containers") {
    std::unordered_set<Entity> entity_set;
    std::unordered_map<Entity, int> entity_map;

    Entity entity1(42, 5);
    Entity entity2(43, 5);
    Entity entity3(42, 5);  // Same as entity1

    // Test unordered_set
    entity_set.insert(entity1);
    entity_set.insert(entity2);
    entity_set.insert(entity3);  // Should not add duplicate

    CHECK_EQ(entity_set.size(), 2);
    CHECK(entity_set.contains(entity1));
    CHECK(entity_set.contains(entity2));
    CHECK(entity_set.contains(entity3));  // Should find entity1

    // Test unordered_map
    entity_map[entity1] = 100;
    entity_map[entity2] = 200;
    entity_map[entity3] = 300;  // Should overwrite entity1's value

    CHECK_EQ(entity_map.size(), 2);
    CHECK_EQ(entity_map[entity1], 300);  // Overwritten by entity3
    CHECK_EQ(entity_map[entity2], 200);
    CHECK_EQ(entity_map[entity3], 300);  // Same as entity1
  }

  TEST_CASE("Entity: constexpr operations") {
    // Test that operations can be used in constexpr context
    constexpr Entity entity(42, 5);
    constexpr bool is_valid = entity.Valid();
    constexpr auto index = entity.Index();
    constexpr auto generation = entity.Generation();
    constexpr auto hash = entity.Hash();

    CHECK(is_valid);
    CHECK_EQ(index, 42);
    CHECK_EQ(generation, 5);
    CHECK_NE(hash, 0);

    constexpr Entity invalid;
    constexpr bool is_invalid = !invalid.Valid();
    constexpr auto invalid_hash = invalid.Hash();

    CHECK(is_invalid);
    CHECK_EQ(invalid_hash, 0);
  }

  TEST_CASE("Entity: edge cases") {
    // Test maximum values
    constexpr auto max_index = std::numeric_limits<Entity::IndexType>::max();
    constexpr auto max_generation = std::numeric_limits<Entity::GenerationType>::max();

    Entity max_entity(max_index, max_generation);
    // Should be invalid due to Entity::kInvalidIndex = std::numeric_limits<Entity::IndexType>::max()
    CHECK_FALSE(max_entity.Valid());
    CHECK_EQ(max_entity.Index(), max_index);
    CHECK_EQ(max_entity.Generation(), max_generation);

    // Test zero values (should be invalid due to Entity::kInvalidGeneration = 0)
    Entity zero_entity(0, 0);
    CHECK_FALSE(zero_entity.Valid());

    // Test minimum valid values
    Entity min_valid(0, 1);
    CHECK(min_valid.Valid());
    CHECK_EQ(min_valid.Index(), 0);
    CHECK_EQ(min_valid.Generation(), 1);
  }

  TEST_CASE("Entity: large scale operations") {
    constexpr size_t entity_count = 10000;
    std::vector<Entity> entities;
    std::unordered_set<Entity> unique_entities;

    entities.reserve(entity_count);

    // Create many entities
    for (size_t i = 0; i < entity_count; ++i) {
      Entity entity(static_cast<Entity::IndexType>(i), 1);
      entities.push_back(entity);
      unique_entities.insert(entity);
    }

    CHECK_EQ(entities.size(), entity_count);
    CHECK_EQ(unique_entities.size(), entity_count);  // All should be unique

    // Verify all entities are valid and have correct values
    for (size_t i = 0; i < entity_count; ++i) {
      CHECK(entities[i].Valid());
      CHECK_EQ(entities[i].Index(), static_cast<Entity::IndexType>(i));
      CHECK_EQ(entities[i].Generation(), 1);
      CHECK(unique_entities.contains(entities[i]));
    }
  }

}  // TEST_SUITE
