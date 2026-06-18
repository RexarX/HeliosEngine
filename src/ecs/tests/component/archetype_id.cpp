#include <doctest/doctest.h>

#include <helios/ecs/component/archetype_id.hpp>
#include <helios/ecs/component/component.hpp>

#include <algorithm>
#include <vector>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;
};

struct Health {
  int value = 100;
};

struct Tag {};

constexpr auto pos_index = ComponentTypeIndex::From<Position>();
constexpr auto vel_index = ComponentTypeIndex::From<Velocity>();
constexpr auto health_index = ComponentTypeIndex::From<Health>();
constexpr auto tag_index = ComponentTypeIndex::From<Tag>();

}  // namespace

TEST_SUITE("helios::ecs::ArchetypeId") {
  TEST_CASE("ecs::ArchetypeId::ctor") {
    SUBCASE("Default ctor creates empty archetype") {
      ArchetypeId id;

      CHECK(id.Empty());
      CHECK_EQ(id.Size(), 0);
      CHECK_EQ(id.Hash(), 0);
      CHECK(id.Types().empty());
    }

    SUBCASE("Ctor from initializer list with single component") {
      ArchetypeId id{pos_index};

      CHECK_FALSE(id.Empty());
      CHECK_EQ(id.Size(), 1);
      CHECK(id.Contains(pos_index));
    }

    SUBCASE("Ctor from initializer list with multiple components") {
      ArchetypeId id{pos_index, vel_index};

      CHECK_FALSE(id.Empty());
      CHECK_EQ(id.Size(), 2);
      CHECK(id.Contains(pos_index));
      CHECK(id.Contains(vel_index));
    }

    SUBCASE("Ctor from initializer list with duplicate components") {
      ArchetypeId id{pos_index, pos_index, pos_index};
      CHECK_EQ(id.Size(), 1);
      CHECK(id.Contains(pos_index));
    }

    SUBCASE("Ctor from vector of components") {
      std::vector<ComponentTypeIndex> types = {pos_index, vel_index,
                                               health_index};
      ArchetypeId id(std::move(types));

      CHECK_EQ(id.Size(), 3);
      CHECK(id.Contains(pos_index));
      CHECK(id.Contains(vel_index));
      CHECK(id.Contains(health_index));
    }

    SUBCASE("Ctor from vector with unsorted components") {
      std::vector<ComponentTypeIndex> unsorted = {health_index, pos_index,
                                                  vel_index};
      ArchetypeId id(std::move(unsorted));

      CHECK_EQ(id.Size(), 3);
      CHECK(id.Contains(pos_index));
      CHECK(id.Contains(vel_index));
      CHECK(id.Contains(health_index));

      const auto types = id.Types();
      CHECK(std::ranges::is_sorted(types));
    }

    SUBCASE("From<Ts...> with no components") {
      const auto id = ArchetypeId::From<>();
      CHECK(id.Empty());
      CHECK_EQ(id.Size(), 0);
    }

    SUBCASE("From<Ts...> with single component") {
      const auto id = ArchetypeId::From<Position>();

      CHECK_FALSE(id.Empty());
      CHECK_EQ(id.Size(), 1);
      CHECK(id.Contains<Position>());
    }

    SUBCASE("From<Ts...> with multiple components") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();

      CHECK_EQ(id.Size(), 3);
      CHECK(id.Contains<Position>());
      CHECK(id.Contains<Velocity>());
      CHECK(id.Contains<Health>());
    }

    SUBCASE("Copy ctor") {
      const auto original = ArchetypeId::From<Position, Velocity>();
      ArchetypeId copy(original);

      CHECK_EQ(copy, original);
      CHECK_EQ(copy.Size(), original.Size());
      CHECK_EQ(copy.Hash(), original.Hash());
    }

    SUBCASE("Move ctor") {
      auto original = ArchetypeId::From<Position, Velocity>();
      const auto original_hash = original.Hash();
      const auto original_size = original.Size();

      ArchetypeId moved(std::move(original));

      CHECK_EQ(moved.Hash(), original_hash);
      CHECK_EQ(moved.Size(), original_size);
      CHECK(moved.Contains<Position>());
      CHECK(moved.Contains<Velocity>());
    }
  }

  TEST_CASE("ecs::ArchetypeId::assignment") {
    SUBCASE("Copy assignment") {
      const auto original = ArchetypeId::From<Position, Velocity>();
      ArchetypeId assigned;

      assigned = original;

      CHECK_EQ(assigned, original);
      CHECK_EQ(assigned.Size(), original.Size());
      CHECK_EQ(assigned.Hash(), original.Hash());
    }

    SUBCASE("Move assignment") {
      auto original = ArchetypeId::From<Position, Velocity>();
      const auto original_hash = original.Hash();
      ArchetypeId assigned;

      assigned = std::move(original);

      CHECK_EQ(assigned.Hash(), original_hash);
      CHECK(assigned.Contains<Position>());
      CHECK(assigned.Contains<Velocity>());
    }

    SUBCASE("Self assignment") {
      auto id = ArchetypeId::From<Position>();

      id = id;

      CHECK(id.Contains<Position>());
      CHECK_EQ(id.Size(), 1);
    }
  }

  TEST_CASE("ecs::ArchetypeId::With") {
    SUBCASE("With template method adds new component") {
      const auto id = ArchetypeId::From<Position>();
      const auto with_velocity = id.With<Velocity>();

      CHECK(with_velocity.Contains<Position>());
      CHECK(with_velocity.Contains<Velocity>());
      CHECK_EQ(with_velocity.Size(), 2);
    }

    SUBCASE("With template method with existing component returns same id") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      const auto with_position = id.With<Position>();

      CHECK_EQ(with_position, id);
      CHECK_EQ(with_position.Size(), 2);
    }

    SUBCASE("With method with ComponentTypeIndex adds new component") {
      const auto id = ArchetypeId::From<Position>();
      const auto with_velocity = id.With(vel_index);

      CHECK(with_velocity.Contains<Position>());
      CHECK(with_velocity.Contains(vel_index));
    }

    SUBCASE("With method with ComponentTypeIndex with existing component") {
      const auto id = ArchetypeId::From<Position>();
      const auto with_position = id.With(pos_index);
      CHECK_EQ(with_position, id);
    }

    SUBCASE("With on empty archetype") {
      const ArchetypeId empty;
      const auto with_pos = empty.With<Position>();

      CHECK(with_pos.Contains<Position>());
      CHECK_EQ(with_pos.Size(), 1);
    }

    SUBCASE("With chaining") {
      const auto id =
          ArchetypeId().With<Position>().With<Velocity>().With<Health>();

      CHECK_EQ(id.Size(), 3);
      CHECK(id.Contains<Position>());
      CHECK(id.Contains<Velocity>());
      CHECK(id.Contains<Health>());
    }
  }

  TEST_CASE("ecs::ArchetypeId::Without") {
    SUBCASE("Without template method removes existing component") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      const auto without_velocity = id.Without<Velocity>();

      CHECK(without_velocity.Contains<Position>());
      CHECK_FALSE(without_velocity.Contains<Velocity>());
      CHECK_EQ(without_velocity.Size(), 1);
    }

    SUBCASE(
        "Without template method with non-existent component returns same id") {
      const auto id = ArchetypeId::From<Position>();
      const auto without_velocity = id.Without<Velocity>();

      CHECK_EQ(without_velocity, id);
      CHECK_EQ(without_velocity.Size(), 1);
    }

    SUBCASE("Without method with ComponentTypeIndex removes component") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      const auto without_velocity = id.Without(vel_index);

      CHECK(without_velocity.Contains<Position>());
      CHECK_FALSE(without_velocity.Contains(vel_index));
    }

    SUBCASE(
        "Without method with ComponentTypeIndex with non-existent component") {
      const auto id = ArchetypeId::From<Position>();
      const auto without_velocity = id.Without(vel_index);
      CHECK_EQ(without_velocity, id);
    }

    SUBCASE("Without on empty archetype") {
      const ArchetypeId empty;
      const auto without_pos = empty.Without<Position>();

      CHECK(without_pos.Empty());
      CHECK_EQ(without_pos.Size(), 0);
    }

    SUBCASE("Without removes only specified component") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();
      const auto without_vel = id.Without<Velocity>();

      CHECK(without_vel.Contains<Position>());
      CHECK_FALSE(without_vel.Contains<Velocity>());
      CHECK(without_vel.Contains<Health>());
      CHECK_EQ(without_vel.Size(), 2);
    }

    SUBCASE("Without chaining") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();
      const auto result = id.Without<Velocity>().Without<Health>();

      CHECK(result.Contains<Position>());
      CHECK_FALSE(result.Contains<Velocity>());
      CHECK_FALSE(result.Contains<Health>());
      CHECK_EQ(result.Size(), 1);
    }
  }

  TEST_CASE("ecs::ArchetypeId::Contains") {
    SUBCASE("Contains template method with existing component") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      CHECK(id.Contains<Position>());
      CHECK(id.Contains<Velocity>());
    }

    SUBCASE("Contains template method with non-existent component") {
      const auto id = ArchetypeId::From<Position>();
      CHECK_FALSE(id.Contains<Velocity>());
      CHECK_FALSE(id.Contains<Health>());
    }

    SUBCASE("Contains method with ComponentTypeIndex") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      CHECK(id.Contains(pos_index));
      CHECK_FALSE(id.Contains(health_index));
    }

    SUBCASE("Contains on empty archetype always returns false") {
      const ArchetypeId empty;
      CHECK_FALSE(empty.Contains<Position>());
    }
  }

  TEST_CASE("ecs::ArchetypeId::ContainsAll") {
    SUBCASE("ContainsAll with all components present") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();
      const std::vector<ComponentTypeIndex> subset = {pos_index, vel_index};
      CHECK(id.ContainsAll(subset));
    }

    SUBCASE("ContainsAll with missing component") {
      const auto id = ArchetypeId::From<Position>();
      const std::vector<ComponentTypeIndex> required = {vel_index,
                                                        health_index};
      CHECK_FALSE(id.ContainsAll(required));
    }

    SUBCASE("ContainsAll with empty range") {
      const auto id = ArchetypeId::From<Position>();
      const std::vector<ComponentTypeIndex> empty;
      CHECK(id.ContainsAll(empty));
    }

    SUBCASE("ContainsAll on empty archetype with non-empty range") {
      const ArchetypeId empty;
      const std::vector<ComponentTypeIndex> types = {pos_index};
      CHECK_FALSE(empty.ContainsAll(types));
    }

    SUBCASE("ContainsAll on empty archetype with empty range") {
      const ArchetypeId empty;
      const std::vector<ComponentTypeIndex> empty_types;
      CHECK(empty.ContainsAll(empty_types));
    }
  }

  TEST_CASE("ecs::ArchetypeId::ContainsAny") {
    SUBCASE("ContainsAny with some components present") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      const std::vector<ComponentTypeIndex> types = {pos_index, health_index};
      CHECK(id.ContainsAny(types));
    }

    SUBCASE("ContainsAny with no components present") {
      const auto id = ArchetypeId::From<Position>();
      const std::vector<ComponentTypeIndex> types = {vel_index, health_index};
      CHECK_FALSE(id.ContainsAny(types));
    }

    SUBCASE("ContainsAny with empty range") {
      const auto id = ArchetypeId::From<Position>();
      const std::vector<ComponentTypeIndex> empty;
      CHECK_FALSE(id.ContainsAny(empty));
    }

    SUBCASE("ContainsAny on empty archetype") {
      const ArchetypeId empty;
      const std::vector<ComponentTypeIndex> types = {pos_index};
      CHECK_FALSE(empty.ContainsAny(types));
    }
  }

  TEST_CASE("ecs::ArchetypeId::ContainsNone") {
    SUBCASE("ContainsNone with no components present") {
      const auto id = ArchetypeId::From<Position>();
      const std::vector<ComponentTypeIndex> types = {vel_index, health_index};
      CHECK(id.ContainsNone(types));
    }

    SUBCASE("ContainsNone with some components present") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      const std::vector<ComponentTypeIndex> types = {pos_index, health_index};
      CHECK_FALSE(id.ContainsNone(types));
    }

    SUBCASE("ContainsNone with all components present") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      const std::vector<ComponentTypeIndex> types = {pos_index, vel_index};
      CHECK_FALSE(id.ContainsNone(types));
    }

    SUBCASE("ContainsNone with empty range") {
      const auto id = ArchetypeId::From<Position>();
      const std::vector<ComponentTypeIndex> empty;
      CHECK(id.ContainsNone(empty));
    }

    SUBCASE("ContainsNone on empty archetype") {
      const ArchetypeId empty;
      const std::vector<ComponentTypeIndex> types = {pos_index};
      CHECK(empty.ContainsNone(types));
    }
  }

  TEST_CASE("ecs::ArchetypeId::Types") {
    SUBCASE("Types returns empty span for empty archetype") {
      const ArchetypeId empty;
      const auto types = empty.Types();

      CHECK(types.empty());
      CHECK_EQ(types.size(), 0);
    }

    SUBCASE("Types returns span with single component") {
      const auto id = ArchetypeId::From<Position>();
      const auto types = id.Types();

      CHECK_EQ(types.size(), 1);
      CHECK_EQ(types[0], pos_index);
    }

    SUBCASE("Types returns span with multiple components in sorted order") {
      const auto id = ArchetypeId::From<Health, Position, Velocity>();
      const auto types = id.Types();

      CHECK_EQ(types.size(), 3);
      CHECK(std::ranges::is_sorted(types));
    }

    SUBCASE("Types span reflects all components") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();
      const auto types = id.Types();

      CHECK_NE(std::ranges::find(types, pos_index), types.end());
      CHECK_NE(std::ranges::find(types, vel_index), types.end());
      CHECK_NE(std::ranges::find(types, health_index), types.end());
    }
  }

  TEST_CASE("ecs::ArchetypeId::Hash") {
    SUBCASE("Hash of empty archetype is zero") {
      const ArchetypeId empty;
      CHECK_EQ(empty.Hash(), 0);
    }

    SUBCASE("Hash of non-empty archetype is non-zero") {
      const auto id = ArchetypeId::From<Position>();
      CHECK_NE(id.Hash(), 0);
    }

    SUBCASE("Hash consistency - same archetype produces same hash") {
      const auto id1 = ArchetypeId::From<Position, Velocity>();
      const auto id2 = ArchetypeId::From<Position, Velocity>();
      CHECK_EQ(id1.Hash(), id2.Hash());
    }

    SUBCASE("Hash uniqueness - different archetypes produce different hashes") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Velocity>();
      CHECK_NE(id1.Hash(), id2.Hash());
    }

    SUBCASE("Hash is order-independent") {
      const ArchetypeId id1{pos_index, vel_index};
      const ArchetypeId id2{vel_index, pos_index};
      CHECK_EQ(id1.Hash(), id2.Hash());
    }

    SUBCASE("Hash changes when components added") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = id1.With<Velocity>();
      CHECK_NE(id1.Hash(), id2.Hash());
    }

    SUBCASE("Hash changes when components removed") {
      const auto id1 = ArchetypeId::From<Position, Velocity>();
      const auto id2 = id1.Without<Velocity>();
      CHECK_NE(id1.Hash(), id2.Hash());
    }
  }

  TEST_CASE("ecs::ArchetypeId::Empty") {
    SUBCASE("Empty returns true for default constructed archetype") {
      const ArchetypeId id;
      CHECK(id.Empty());
    }

    SUBCASE("Empty returns false for archetype with components") {
      const auto id = ArchetypeId::From<Position>();
      CHECK_FALSE(id.Empty());
    }

    SUBCASE("Empty returns true after removing all components") {
      const auto id = ArchetypeId::From<Position>();
      const auto empty = id.Without<Position>();
      CHECK(empty.Empty());
    }

    SUBCASE("Empty returns false for archetype with multiple components") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();
      CHECK_FALSE(id.Empty());
    }
  }

  TEST_CASE("ecs::ArchetypeId::Size") {
    SUBCASE("Size returns 0 for empty archetype") {
      const ArchetypeId empty;
      CHECK_EQ(empty.Size(), 0);
    }

    SUBCASE("Size returns 1 for archetype with one component") {
      const auto id = ArchetypeId::From<Position>();
      CHECK_EQ(id.Size(), 1);
    }

    SUBCASE("Size returns correct count for multiple components") {
      const auto id = ArchetypeId::From<Position, Velocity, Health>();
      CHECK_EQ(id.Size(), 3);
    }

    SUBCASE("Size increases when component added") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = id1.With<Velocity>();
      CHECK_EQ(id2.Size(), id1.Size() + 1);
    }

    SUBCASE("Size decreases when component removed") {
      const auto id1 = ArchetypeId::From<Position, Velocity>();
      const auto id2 = id1.Without<Velocity>();
      CHECK_EQ(id2.Size(), id1.Size() - 1);
    }

    SUBCASE("Size with duplicate components in constructor") {
      const ArchetypeId id{pos_index, pos_index, pos_index};
      CHECK_EQ(id.Size(), 1);
    }
  }

  TEST_CASE("ecs::ArchetypeId::operator==") {
    SUBCASE("Equal archetypes") {
      const auto id1 = ArchetypeId::From<Position, Velocity>();
      const auto id2 = ArchetypeId::From<Position, Velocity>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different archetypes") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Velocity>();
      CHECK_NE(id1, id2);
    }

    SUBCASE("Empty archetypes") {
      const ArchetypeId id1;
      const ArchetypeId id2;
      CHECK_EQ(id1, id2);
    }
  }

  TEST_CASE("ecs::ArchetypeId::operator!=") {
    SUBCASE("Equal archetypes") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Position>();
      CHECK_FALSE(id1 != id2);
    }

    SUBCASE("Different archetypes") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Velocity>();
      CHECK_NE(id1, id2);
    }
  }

  TEST_CASE("ecs::ArchetypeId::operator<") {
    SUBCASE("Provides consistent ordering") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Velocity>();
      CHECK_EQ(id1 < id2, !(id2 < id1));
    }

    SUBCASE("Equal archetypes") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Position>();

      CHECK_FALSE(id1 < id2);
      CHECK_FALSE(id2 < id1);
    }

    SUBCASE("Empty vs non-empty") {
      const ArchetypeId empty;
      const auto non_empty = ArchetypeId::From<Position>();

      CHECK(empty < non_empty);
      CHECK_FALSE(non_empty < empty);
    }
  }

  TEST_CASE("std::hash<ecs::ArchetypeId>") {
    SUBCASE("std::hash produces same value as Hash method") {
      const auto id = ArchetypeId::From<Position, Velocity>();
      CHECK_EQ(std::hash<ArchetypeId>{}(id), id.Hash());
    }

    SUBCASE("std::hash consistency") {
      const auto id = ArchetypeId::From<Position>();
      const auto hash1 = std::hash<ArchetypeId>{}(id);
      const auto hash2 = std::hash<ArchetypeId>{}(id);

      CHECK_EQ(hash1, hash2);
    }

    SUBCASE("std::hash uniqueness") {
      const auto id1 = ArchetypeId::From<Position>();
      const auto id2 = ArchetypeId::From<Velocity>();
      CHECK_NE(std::hash<ArchetypeId>{}(id1), std::hash<ArchetypeId>{}(id2));
    }

    SUBCASE("std::hash for empty archetype") {
      const ArchetypeId empty;
      CHECK_EQ(std::hash<ArchetypeId>{}(empty), 0);
    }
  }
}
