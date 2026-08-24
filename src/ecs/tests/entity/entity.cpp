#include <doctest/doctest.h>

#include <helios/ecs/entity/entity.hpp>

#include <functional>
#include <limits>

using namespace helios::ecs;

TEST_SUITE("helios::ecs::Entity") {
  TEST_CASE("helios::ecs::Entity::ctor") {
    SUBCASE("Default ctor") {
      constexpr Entity entity;

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Hash(), 0);
      CHECK_EQ(entity.Index(), Entity::kInvalidIndex);
      CHECK_EQ(entity.Generation(), Entity::kInvalidGeneration);
    }

    SUBCASE("Value ctor with valid parameters") {
      constexpr Entity entity{7, 2};

      CHECK(entity.Valid());
      CHECK_EQ(entity.Index(), 7);
      CHECK_EQ(entity.Generation(), 2);
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Value ctor with zero index and generation") {
      constexpr Entity entity{0, 0};

      CHECK(entity.Valid());
      CHECK_EQ(entity.Index(), 0);
      CHECK_EQ(entity.Generation(), 0);
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Copy ctor") {
      constexpr Entity original{42, 5};
      constexpr Entity copy(original);

      CHECK_EQ(copy, original);
      CHECK_EQ(copy.Index(), original.Index());
      CHECK_EQ(copy.Generation(), original.Generation());
      CHECK_EQ(copy.Hash(), original.Hash());
    }

    SUBCASE("Move ctor") {
      constexpr Entity original{100, 10};
      constexpr auto original_index = original.Index();
      constexpr auto original_generation = original.Generation();
      constexpr auto original_hash = original.Hash();

      constexpr Entity moved(std::move(original));

      CHECK_EQ(moved.Index(), original_index);
      CHECK_EQ(moved.Generation(), original_generation);
      CHECK_EQ(moved.Hash(), original_hash);
      CHECK(moved.Valid());
    }
  }

  TEST_CASE("helios::ecs::Entity::assignment") {
    SUBCASE("Copy assignment") {
      constexpr Entity original{50, 3};
      Entity assigned;

      assigned = original;

      CHECK_EQ(assigned, original);
      CHECK_EQ(assigned.Index(), original.Index());
      CHECK_EQ(assigned.Generation(), original.Generation());
      CHECK_EQ(assigned.Hash(), original.Hash());
    }

    SUBCASE("Move assignment") {
      constexpr Entity original{75, 8};
      constexpr auto original_index = original.Index();
      constexpr auto original_generation = original.Generation();
      constexpr auto original_hash = original.Hash();

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

  TEST_CASE("helios::ecs::Entity::operator==") {
    SUBCASE("Equal entities") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{10, 5};
      CHECK(entity1 == entity2);
    }

    SUBCASE("Different index") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{11, 5};
      CHECK_FALSE(entity1 == entity2);
    }

    SUBCASE("Different generation") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{10, 6};
      CHECK_FALSE(entity1 == entity2);
    }

    SUBCASE("Both invalid") {
      constexpr Entity entity1;
      constexpr Entity entity2;
      CHECK(entity1 == entity2);
    }
  }

  TEST_CASE("helios::ecs::Entity::operator!=") {
    SUBCASE("Equal entities") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{10, 5};
      CHECK_FALSE(entity1 != entity2);
    }

    SUBCASE("Different index") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{11, 5};
      CHECK(entity1 != entity2);
    }

    SUBCASE("Different generation") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{10, 6};
      CHECK(entity1 != entity2);
    }

    SUBCASE("Both invalid") {
      constexpr Entity entity1;
      constexpr Entity entity2;
      CHECK_FALSE(entity1 != entity2);
    }
  }

  TEST_CASE("helios::ecs::Entity::operator<") {
    SUBCASE("Equal entities") {
      constexpr Entity left{1, 1};
      constexpr Entity right{1, 1};

      CHECK_FALSE(left < right);
      CHECK_FALSE(right < left);
    }

    SUBCASE("Same index, different generation") {
      constexpr Entity left{1, 1};
      constexpr Entity right{1, 2};

      CHECK(left < right);
      CHECK_FALSE(right < left);
    }

    SUBCASE("Different indices") {
      constexpr Entity left{1, 1};
      constexpr Entity right{2, 1};

      CHECK(left < right);
      CHECK_FALSE(right < left);
    }
  }

  TEST_CASE("helios::ecs::Entity::Valid") {
    SUBCASE("Invalid entity (default constructed)") {
      constexpr Entity entity;
      CHECK_FALSE(entity.Valid());
    }

    SUBCASE("Valid entity with positive index and generation") {
      constexpr Entity entity{10, 5};
      CHECK(entity.Valid());
    }

    SUBCASE("Valid entity with zero index and generation") {
      constexpr Entity entity{0, 0};
      CHECK(entity.Valid());
    }

    SUBCASE("Invalid entity with invalid index only") {
      constexpr Entity entity{Entity::kInvalidIndex, 5};
      CHECK_FALSE(entity.Valid());
    }

    SUBCASE("Invalid entity with invalid generation only") {
      constexpr Entity entity{10, Entity::kInvalidGeneration};
      CHECK_FALSE(entity.Valid());
    }
  }

  TEST_CASE("helios::ecs::Entity::Alive") {
    SUBCASE("Default constructed entity is not alive") {
      constexpr Entity entity;
      CHECK_FALSE(entity.Alive());
    }

    SUBCASE("Initial alive generation is alive") {
      constexpr Entity entity{0, Entity::kInitialAliveGeneration};
      CHECK(entity.Valid());
      CHECK(entity.Alive());
    }

    SUBCASE("Free generation encoding is not alive") {
      constexpr Entity free{5, 2U};
      CHECK(free.Valid());
      CHECK_FALSE(free.Alive());
    }

    SUBCASE("Invalid sentinel is not alive") {
      constexpr Entity entity{1, Entity::kInvalidGeneration};
      CHECK_FALSE(entity.Valid());
      CHECK_FALSE(entity.Alive());
    }

    SUBCASE("Alive matches IsAliveGeneration on generation") {
      constexpr Entity entity{7, Entity::kAliveBit | 3U};
      CHECK_EQ(entity.Alive(), IsAliveGeneration(entity.Generation()));
    }
  }

  TEST_CASE("helios::ecs::Entity::Hash") {
    SUBCASE("Hash of invalid entity is zero") {
      constexpr Entity entity;
      CHECK_EQ(entity.Hash(), 0);
    }

    SUBCASE("Hash of valid entity is non-zero") {
      constexpr Entity entity{5, 3};
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Hash consistency - same entity produces same hash") {
      constexpr Entity entity{100, 50};

      constexpr auto hash1 = entity.Hash();
      constexpr auto hash2 = entity.Hash();

      CHECK_EQ(hash1, hash2);
    }

    SUBCASE("Hash uniqueness - different entities produce different hashes") {
      constexpr Entity entity1{10, 5};
      constexpr Entity entity2{10, 6};
      constexpr Entity entity3{11, 5};

      CHECK_NE(entity1.Hash(), entity2.Hash());
      CHECK_NE(entity1.Hash(), entity3.Hash());
      CHECK_NE(entity2.Hash(), entity3.Hash());
    }

    SUBCASE("Hash of copied entity is identical") {
      constexpr Entity original{42, 7};
      constexpr Entity copy(original);
      CHECK_EQ(original.Hash(), copy.Hash());
    }
  }

  TEST_CASE("helios::ecs::Entity::ReuseCount") {
    SUBCASE("Initial alive generation has reuse count 1") {
      constexpr Entity entity{0, Entity::kInitialAliveGeneration};
      CHECK_EQ(entity.ReuseCount(), 1U);
      CHECK_NE(entity.ReuseCount(), entity.Generation());
    }

    SUBCASE("Free generation reuse count equals packed generation") {
      constexpr Entity entity{5, 2U};
      CHECK_EQ(entity.ReuseCount(), 2U);
      CHECK_EQ(entity.ReuseCount(), entity.Generation());
    }

    SUBCASE("Alive and free encodings of the same counter match") {
      constexpr Entity alive{7, Entity::kAliveBit | 3U};
      constexpr Entity free{7, 3U};
      CHECK_EQ(alive.ReuseCount(), 3U);
      CHECK_EQ(free.ReuseCount(), 3U);
      CHECK_EQ(alive.ReuseCount(), free.ReuseCount());
      CHECK_NE(alive.Generation(), free.Generation());
    }

    SUBCASE("Zero generation has reuse count 0") {
      constexpr Entity entity{10, 0};
      CHECK_EQ(entity.ReuseCount(), 0U);
    }

    SUBCASE("Invalid entity reuse count is the counter mask") {
      constexpr Entity entity;
      CHECK_EQ(entity.ReuseCount(), Entity::kCounterMask);
      CHECK_EQ(entity.ReuseCount(), entity.Generation() & Entity::kCounterMask);
    }

    SUBCASE("Reuse count preservation after copy") {
      constexpr Entity original{100, Entity::kAliveBit | 9U};
      constexpr Entity copy(original);
      CHECK_EQ(copy.ReuseCount(), original.ReuseCount());
      CHECK_EQ(copy.ReuseCount(), 9U);
    }
  }

  TEST_CASE("helios::ecs::Entity::Index") {
    SUBCASE("Index retrieval from valid entity") {
      constexpr Entity entity{123, 1};
      CHECK_EQ(entity.Index(), 123);
    }

    SUBCASE("Index from entity with zero index") {
      constexpr Entity entity{0, 10};
      CHECK_EQ(entity.Index(), 0);
    }

    SUBCASE("Index from invalid entity") {
      constexpr Entity entity;
      CHECK_EQ(entity.Index(), Entity::kInvalidIndex);
    }

    SUBCASE("Index preservation after copy") {
      constexpr Entity original{999, 5};
      constexpr Entity copy(original);
      CHECK_EQ(copy.Index(), original.Index());
    }
  }

  TEST_CASE("helios::ecs::Entity::Generation") {
    SUBCASE("Generation retrieval from valid entity") {
      constexpr Entity entity{5, 42};
      CHECK_EQ(entity.Generation(), 42);
    }

    SUBCASE("Generation from entity with zero generation") {
      constexpr Entity entity{10, 0};
      CHECK_EQ(entity.Generation(), 0);
    }

    SUBCASE("Generation from invalid entity") {
      constexpr Entity entity;
      CHECK_EQ(entity.Generation(), Entity::kInvalidGeneration);
    }

    SUBCASE("Generation preservation after copy") {
      constexpr Entity original{100, 777};
      constexpr Entity copy(original);
      CHECK_EQ(copy.Generation(), original.Generation());
    }
  }

  TEST_CASE("helios::ecs::Entity::edge_cases") {
    SUBCASE("Maximum index value") {
      constexpr auto max_index = std::numeric_limits<Entity::IndexType>::max();
      constexpr Entity entity{max_index, 1};

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Index(), max_index);
    }

    SUBCASE("Maximum generation value") {
      constexpr auto max_gen =
          std::numeric_limits<Entity::GenerationType>::max();
      constexpr Entity entity{1, max_gen};

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Generation(), max_gen);
    }

    SUBCASE("Both index and generation at maximum") {
      constexpr auto max_index = std::numeric_limits<Entity::IndexType>::max();
      constexpr auto max_gen =
          std::numeric_limits<Entity::GenerationType>::max();
      constexpr Entity entity{max_index, max_gen};

      CHECK_FALSE(entity.Valid());
      CHECK_EQ(entity.Index(), max_index);
      CHECK_EQ(entity.Generation(), max_gen);
    }

    SUBCASE("Entity with index 0 and generation 0 is valid") {
      constexpr Entity entity{0, 0};

      CHECK(entity.Valid());
      CHECK_EQ(entity.Index(), 0);
      CHECK_EQ(entity.Generation(), 0);
      CHECK_NE(entity.Hash(), 0);
    }

    SUBCASE("Hash uniqueness with same index but different generation") {
      constexpr Entity entity1{100, 1};
      constexpr Entity entity2{100, 2};
      constexpr Entity entity3{100, 3};

      CHECK_NE(entity1.Hash(), entity2.Hash());
      CHECK_NE(entity2.Hash(), entity3.Hash());
      CHECK_NE(entity1.Hash(), entity3.Hash());
    }
  }
}

TEST_SUITE("helios::ecs::IsAliveGeneration") {
  TEST_CASE("helios::ecs::IsAliveGeneration checks") {
    SUBCASE("Initial alive generation is alive") {
      CHECK(IsAliveGeneration(Entity::kInitialAliveGeneration));
    }

    SUBCASE("Free generation encoding is not alive") {
      CHECK_FALSE(IsAliveGeneration(2U));
      CHECK_FALSE(IsAliveGeneration(0U));
    }

    SUBCASE("Invalid sentinel is not alive despite odd/top-bit shape") {
      CHECK_FALSE(IsAliveGeneration(Entity::kInvalidGeneration));
    }

    SUBCASE("Alive bit alone with zero counter is alive") {
      CHECK(IsAliveGeneration(Entity::kAliveBit));
    }
  }
}

TEST_SUITE("helios::ecs::NextGeneration") {
  TEST_CASE("helios::ecs::NextGeneration transitions") {
    SUBCASE("Alive to free increments counter and clears alive bit") {
      constexpr auto free_gen =
          NextGeneration(Entity::kInitialAliveGeneration, /*alive=*/false);
      CHECK_EQ(free_gen, 2U);
      CHECK_FALSE(IsAliveGeneration(free_gen));
    }

    SUBCASE("Free to alive preserves counter and sets alive bit") {
      constexpr auto alive_gen = NextGeneration(2U, /*alive=*/true);
      CHECK_EQ(alive_gen, Entity::kAliveBit | 2U);
      CHECK(IsAliveGeneration(alive_gen));
    }

    SUBCASE("Full reuse cycle advances counter once") {
      constexpr auto original = Entity::kInitialAliveGeneration;
      constexpr auto free_gen = NextGeneration(original, /*alive=*/false);
      constexpr auto reused = NextGeneration(free_gen, /*alive=*/true);

      CHECK_EQ(reused & Entity::kCounterMask,
               (original & Entity::kCounterMask) + 1U);
      CHECK(IsAliveGeneration(reused));
      CHECK_NE(reused, original);
    }

    SUBCASE("Counter skips reserved mask value near wrap") {
      constexpr auto near_wrap =
          (Entity::kCounterMask - 1U) | Entity::kAliveBit;
      constexpr auto free_gen = NextGeneration(near_wrap, /*alive=*/false);
      CHECK_EQ(free_gen, 0U);

      constexpr auto alive_gen = NextGeneration(free_gen, /*alive=*/true);
      CHECK_EQ(alive_gen, Entity::kAliveBit);
      CHECK(IsAliveGeneration(alive_gen));
      CHECK_NE(alive_gen, Entity::kInvalidGeneration);
    }

    SUBCASE("Invalid sentinel remains unreachable across transitions") {
      auto gen = Entity::kInitialAliveGeneration;
      for (int cycle = 0; cycle < 8; ++cycle) {
        gen = NextGeneration(gen, /*alive=*/false);
        CHECK_NE(gen, Entity::kInvalidGeneration);
        CHECK_FALSE(IsAliveGeneration(gen));

        gen = NextGeneration(gen, /*alive=*/true);
        CHECK_NE(gen, Entity::kInvalidGeneration);
        CHECK(IsAliveGeneration(gen));
      }

      constexpr auto wrap_free =
          NextGeneration((Entity::kCounterMask - 1U) | Entity::kAliveBit,
                         /*alive=*/false);
      constexpr auto wrap_alive = NextGeneration(wrap_free, /*alive=*/true);
      CHECK_NE(wrap_alive, Entity::kInvalidGeneration);
    }

    SUBCASE("Free generation never carries alive bit after increment") {
      // Counter at mask without alive bit would overflow into kAliveBit without
      // masking the return value.
      constexpr auto free_gen =
          NextGeneration(Entity::kCounterMask, /*alive=*/false);
      CHECK_EQ(free_gen, 0U);
      CHECK_FALSE(IsAliveGeneration(free_gen));
    }
  }
}

TEST_SUITE("std::hash<helios::ecs::Entity>") {
  TEST_CASE("std::hash specialization produces same value as Hash method") {
    constexpr Entity entity{15, 8};
    CHECK_EQ(std::hash<Entity>{}(entity), entity.Hash());
  }

  TEST_CASE("std::hash consistency") {
    constexpr Entity entity{25, 12};

    constexpr auto hash1 = std::hash<Entity>{}(entity);
    constexpr auto hash2 = std::hash<Entity>{}(entity);

    CHECK_EQ(hash1, hash2);
  }

  TEST_CASE("std::hash for invalid entity") {
    constexpr Entity entity;
    CHECK_EQ(std::hash<Entity>{}(entity), 0);
  }
}
