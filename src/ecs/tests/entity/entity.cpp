#include <doctest/doctest.h>

#include <helios/ecs/entity/entity.hpp>

#include <functional>
#include <limits>

using namespace helios::ecs;

TEST_SUITE("helios::ecs::Entity") {
  TEST_CASE("ecs::Entity::ctor") {
    SUBCASE("Default ctor") {
      const Entity entity;

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Hash(), 0);
      CHECK_EQ(entity.Index(), Entity::kInvalidIndex);
      CHECK_EQ(entity.Generation(), Entity::kInvalidGeneration);
    }

    SUBCASE("Value ctor with valid parameters") {
      const Entity entity{7, 2};

      CHECK(entity.Valid());
      CHECK_EQ(entity.Index(), 7);
      CHECK_EQ(entity.Generation(), 2);
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Value ctor with zero index and generation") {
      const Entity entity{0, 0};

      CHECK(entity.Valid());
      CHECK_EQ(entity.Index(), 0);
      CHECK_EQ(entity.Generation(), 0);
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Copy ctor") {
      const Entity original{42, 5};
      const Entity copy(original);

      CHECK_EQ(copy, original);
      CHECK_EQ(copy.Index(), original.Index());
      CHECK_EQ(copy.Generation(), original.Generation());
      CHECK_EQ(copy.Hash(), original.Hash());
    }

    SUBCASE("Move ctor") {
      Entity original{100, 10};
      const auto original_index = original.Index();
      const auto original_generation = original.Generation();
      const auto original_hash = original.Hash();

      const Entity moved(std::move(original));

      CHECK_EQ(moved.Index(), original_index);
      CHECK_EQ(moved.Generation(), original_generation);
      CHECK_EQ(moved.Hash(), original_hash);
      CHECK(moved.Valid());
    }
  }

  TEST_CASE("ecs::Entity::assignment") {
    SUBCASE("Copy assignment") {
      const Entity original{50, 3};
      Entity assigned;

      assigned = original;

      CHECK_EQ(assigned, original);
      CHECK_EQ(assigned.Index(), original.Index());
      CHECK_EQ(assigned.Generation(), original.Generation());
      CHECK_EQ(assigned.Hash(), original.Hash());
    }

    SUBCASE("Move assignment") {
      Entity original{75, 8};
      const auto original_index = original.Index();
      const auto original_generation = original.Generation();
      const auto original_hash = original.Hash();

      Entity assigned;
      assigned = std::move(original);

      CHECK_EQ(assigned.Index(), original_index);
      CHECK_EQ(assigned.Generation(), original_generation);
      CHECK_EQ(assigned.Hash(), original_hash);
    }

    SUBCASE("Self assignment") {
      Entity entity{25, 2};

      entity = entity;

      CHECK_EQ(entity.Index(), 25);
      CHECK_EQ(entity.Generation(), 2);
    }
  }

  TEST_CASE("ecs::Entity::Valid") {
    SUBCASE("Invalid entity (default constructed)") {
      const Entity entity;
      CHECK_FALSE(entity.Valid());
    }

    SUBCASE("Valid entity with positive index and generation") {
      const Entity entity{10, 5};
      CHECK(entity.Valid());
    }

    SUBCASE("Valid entity with zero index and generation") {
      const Entity entity{0, 0};
      CHECK(entity.Valid());
    }

    SUBCASE("Invalid entity with invalid index only") {
      const Entity entity{Entity::kInvalidIndex, 5};
      CHECK_FALSE(entity.Valid());
    }

    SUBCASE("Invalid entity with invalid generation only") {
      const Entity entity{10, Entity::kInvalidGeneration};
      CHECK_FALSE(entity.Valid());
    }
  }

  TEST_CASE("ecs::Entity::Index") {
    SUBCASE("Index retrieval from valid entity") {
      const Entity entity{123, 1};
      CHECK_EQ(entity.Index(), 123);
    }

    SUBCASE("Index from entity with zero index") {
      const Entity entity{0, 10};
      CHECK_EQ(entity.Index(), 0);
    }

    SUBCASE("Index from invalid entity") {
      const Entity entity;
      CHECK_EQ(entity.Index(), Entity::kInvalidIndex);
    }

    SUBCASE("Index preservation after copy") {
      const Entity original{999, 5};
      const Entity copy(original);
      CHECK_EQ(copy.Index(), original.Index());
    }
  }

  TEST_CASE("ecs::Entity::Generation") {
    SUBCASE("Generation retrieval from valid entity") {
      const Entity entity{5, 42};
      CHECK_EQ(entity.Generation(), 42);
    }

    SUBCASE("Generation from entity with zero generation") {
      const Entity entity{10, 0};
      CHECK_EQ(entity.Generation(), 0);
    }

    SUBCASE("Generation from invalid entity") {
      const Entity entity;
      CHECK_EQ(entity.Generation(), Entity::kInvalidGeneration);
    }

    SUBCASE("Generation preservation after copy") {
      const Entity original{100, 777};
      const Entity copy(original);
      CHECK_EQ(copy.Generation(), original.Generation());
    }
  }

  TEST_CASE("ecs::Entity::Hash") {
    SUBCASE("Hash of invalid entity is zero") {
      const Entity entity;
      CHECK_EQ(entity.Hash(), 0);
    }

    SUBCASE("Hash of valid entity is non-zero") {
      const Entity entity{5, 3};
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Hash consistency - same entity produces same hash") {
      const Entity entity{100, 50};

      const auto hash1 = entity.Hash();
      const auto hash2 = entity.Hash();

      CHECK_EQ(hash1, hash2);
    }

    SUBCASE("Hash uniqueness - different entities produce different hashes") {
      const Entity entity1{10, 5};
      const Entity entity2{10, 6};
      const Entity entity3{11, 5};

      CHECK_NE(entity1.Hash(), entity2.Hash());
      CHECK_NE(entity1.Hash(), entity3.Hash());
      CHECK_NE(entity2.Hash(), entity3.Hash());
    }

    SUBCASE("Hash of copied entity is identical") {
      const Entity original{42, 7};
      const Entity copy(original);
      CHECK_EQ(original.Hash(), copy.Hash());
    }
  }

  TEST_CASE("ecs::Entity::operator==") {
    SUBCASE("Equal entities") {
      const Entity entity1{10, 5};
      const Entity entity2{10, 5};
      CHECK(entity1 == entity2);
    }

    SUBCASE("Different index") {
      const Entity entity1{10, 5};
      const Entity entity2{11, 5};
      CHECK_FALSE(entity1 == entity2);
    }

    SUBCASE("Different generation") {
      const Entity entity1{10, 5};
      const Entity entity2{10, 6};
      CHECK_FALSE(entity1 == entity2);
    }

    SUBCASE("Both invalid") {
      const Entity entity1;
      const Entity entity2;
      CHECK(entity1 == entity2);
    }
  }

  TEST_CASE("ecs::Entity::operator!=") {
    SUBCASE("Equal entities") {
      const Entity entity1{10, 5};
      const Entity entity2{10, 5};
      CHECK_FALSE(entity1 != entity2);
    }

    SUBCASE("Different index") {
      const Entity entity1{10, 5};
      const Entity entity2{11, 5};
      CHECK(entity1 != entity2);
    }

    SUBCASE("Different generation") {
      const Entity entity1{10, 5};
      const Entity entity2{10, 6};
      CHECK(entity1 != entity2);
    }

    SUBCASE("Both invalid") {
      const Entity entity1;
      const Entity entity2;
      CHECK_FALSE(entity1 != entity2);
    }
  }

  TEST_CASE("ecs::Entity::operator<") {
    SUBCASE("Equal entities") {
      const Entity left{1, 1};
      const Entity right{1, 1};

      CHECK_FALSE(left < right);
      CHECK_FALSE(right < left);
    }

    SUBCASE("Same index, different generation") {
      const Entity left{1, 1};
      const Entity right{1, 2};

      CHECK(left < right);
      CHECK_FALSE(right < left);
    }

    SUBCASE("Different indices") {
      const Entity left{1, 1};
      const Entity right{2, 1};

      CHECK(left < right);
      CHECK_FALSE(right < left);
    }
  }

  TEST_CASE("ecs::Entity::edge_cases") {
    SUBCASE("Maximum index value") {
      constexpr auto max_index = std::numeric_limits<Entity::IndexType>::max();
      const Entity entity{max_index, 1};

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Index(), max_index);
    }

    SUBCASE("Maximum generation value") {
      constexpr auto max_gen =
          std::numeric_limits<Entity::GenerationType>::max();
      const Entity entity{1, max_gen};

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Generation(), max_gen);
    }

    SUBCASE("Both index and generation at maximum") {
      constexpr auto max_index = std::numeric_limits<Entity::IndexType>::max();
      constexpr auto max_gen =
          std::numeric_limits<Entity::GenerationType>::max();
      const Entity entity{max_index, max_gen};

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Index(), max_index);
      CHECK_EQ(entity.Generation(), max_gen);
    }

    SUBCASE("Entity with index 0 and generation 0 is valid") {
      const Entity entity{0, 0};

      CHECK(entity.Valid());
      CHECK_EQ(entity.Index(), 0);
      CHECK_EQ(entity.Generation(), 0);
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Hash uniqueness with same index but different generation") {
      const Entity entity1{100, 1};
      const Entity entity2{100, 2};
      const Entity entity3{100, 3};

      CHECK_NE(entity1.Hash(), entity2.Hash());
      CHECK_NE(entity2.Hash(), entity3.Hash());
      CHECK_NE(entity1.Hash(), entity3.Hash());
    }
  }
}

TEST_SUITE("std::hash<ecs::Entity>") {
  TEST_CASE("std::hash specialization produces same value as Hash method") {
    const Entity entity{15, 8};
    CHECK_EQ(std::hash<Entity>{}(entity), entity.Hash());
  }

  TEST_CASE("std::hash consistency") {
    const Entity entity{25, 12};

    const auto hash1 = std::hash<Entity>{}(entity);
    const auto hash2 = std::hash<Entity>{}(entity);

    CHECK_EQ(hash1, hash2);
  }

  TEST_CASE("std::hash for invalid entity") {
    const Entity entity;
    CHECK_EQ(std::hash<Entity>{}(entity), 0);
  }
}
