#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/manager.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/iterator.hpp>

#include <functional>
#include <span>
#include <tuple>
#include <vector>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Position&) const noexcept = default;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Velocity&) const noexcept = default;
};

struct Health {
  int value = 100;

  constexpr bool operator==(const Health&) const noexcept = default;
};

/// Creates a ComponentManager with the given entities and components, and
/// returns both the manager and the vector of matching archetype refs that
/// the iterators require.
struct QueryFixture {
  ComponentManager manager;
  std::vector<std::reference_wrapper<const Archetype>> archetypes;

  /// Add an entity with Position + Velocity to the manager.
  void AddPosVelEntity(Entity entity, Position pos, Velocity vel) {
    manager.InitEntity(entity);
    manager.AddArchetypeComponents(entity, pos);
    manager.AddArchetypeComponents(entity, vel);
  }

  /// Add an entity with only Position.
  void AddPosEntity(Entity entity, Position pos) {
    manager.InitEntity(entity);
    manager.AddArchetypeComponents(entity, pos);
  }

  /// Rebuild the archetype list from the manager's current state.
  void RebuildArchetypes() {
    archetypes.clear();
    for (const auto& ref : manager.Archetypes()) {
      if (ref.get().EntityCount() > 0) {
        archetypes.emplace_back(ref);
      }
    }
  }

  auto ArchetypeSpan() const noexcept
      -> std::span<const std::reference_wrapper<const Archetype>> {
    return archetypes;
  }
};

}  // namespace

TEST_SUITE("helios::ecs::BasicQueryIter") {
  TEST_CASE("ecs::BasicQueryIter::Constructor") {
    SUBCASE("Begin equals end for empty archetype list") {
      ComponentManager manager;
      std::vector<std::reference_wrapper<const Archetype>> empty;
      std::span<const std::reference_wrapper<const Archetype>> span = empty;

      QueryIter<const Position&> iter{span, manager, 0, 0};
      const auto end = iter.end();

      CHECK(iter == end);
    }

    SUBCASE("Begin does not equal end when entities are present") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 2.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};

      CHECK(iter != iter.end());
    }

    SUBCASE("Constructor advances past empty archetypes automatically") {
      // An archetype with no entities should be skipped at construction time.
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 3.0F, .y = 4.0F});
      fix.RebuildArchetypes();

      // Only non-empty archetypes end up in the span (fixture ensures that),
      // so begin should immediately point at the first valid entity.
      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      const auto& [pos] = *iter;
      CHECK_EQ(pos, Position{.x = 3.0F, .y = 4.0F});
    }
  }

  TEST_CASE("ecs::BasicQueryIter::operator*") {
    SUBCASE("Returns correct component value for single component") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 5.0F, .y = 6.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      const auto& [pos] = *iter;

      CHECK_EQ(pos, Position{.x = 5.0F, .y = 6.0F});
    }

    SUBCASE("Returns correct values for multiple components") {
      QueryFixture fix;
      fix.AddPosVelEntity(Entity{1, 0}, {.x = 1.0F, .y = 2.0F},
                          {.x = 3.0F, .y = 4.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&, const Velocity&> iter{fix.ArchetypeSpan(),
                                                       fix.manager, 0, 0};
      const auto& [pos, vel] = *iter;

      CHECK_EQ(pos, Position{.x = 1.0F, .y = 2.0F});
      CHECK_EQ(vel, Velocity{.x = 3.0F, .y = 4.0F});
    }

    SUBCASE("Mutable reference allows mutation through dereference") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto&& [pos] = *iter;
      pos.x = 99.0F;

      // Re-fetch to verify mutation persisted.
      auto&& [pos2] = *iter;
      CHECK_EQ(pos2.x, 99.0F);
    }

    SUBCASE("Const iterator returns const reference") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 7.0F, .y = 8.0F});
      fix.RebuildArchetypes();

      ReadOnlyQueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager,
                                              0, 0};
      const auto& [pos] = *iter;

      CHECK_EQ(pos, Position{.x = 7.0F, .y = 8.0F});
    }

    SUBCASE("Value copy access returns independent copy") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 2.0F, .y = 3.0F});
      fix.RebuildArchetypes();

      QueryIter<Position> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto [pos_copy] = *iter;
      pos_copy.x = 999.0F;

      // Original should be unchanged.
      auto [pos_again] = *iter;
      CHECK_EQ(pos_again.x, 2.0F);
    }

    SUBCASE("Optional component returns non-null pointer when present") {
      QueryFixture fix;
      fix.AddPosVelEntity(Entity{1, 0}, {.x = 1.0F, .y = 1.0F},
                          {.x = 2.0F, .y = 2.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&, const Velocity*> iter{fix.ArchetypeSpan(),
                                                       fix.manager, 0, 0};
      auto [pos, vel_ptr] = *iter;

      REQUIRE_NE(vel_ptr, nullptr);
      CHECK_EQ(*vel_ptr, Velocity{.x = 2.0F, .y = 2.0F});
    }

    SUBCASE("Optional component returns nullptr when absent") {
      QueryFixture fix;
      // Entity only has Position, no Velocity.
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 1.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&, const Velocity*> iter{fix.ArchetypeSpan(),
                                                       fix.manager, 0, 0};
      const auto& [pos, vel_ptr] = *iter;

      CHECK_EQ(vel_ptr, nullptr);
    }
  }

  TEST_CASE("ecs::BasicQueryIter::operator++ (prefix)") {
    SUBCASE("Prefix increment advances to next entity") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;
      const auto& [pos] = *iter;

      CHECK_EQ(pos.x, 2.0F);
    }

    SUBCASE("Prefix increment returns reference to same iterator") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto& returned = ++iter;

      CHECK_EQ(&returned, &iter);
    }

    SUBCASE("Incrementing past last entity reaches end") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;

      CHECK_EQ(iter, iter.end());
    }

    SUBCASE("Incrementing across two archetypes yields all entities") {
      // Two separate archetypes: one with (Position), another with
      // (Position+Velocity). A query for Position visits both.
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 10.0F, .y = 0.0F});
      fix.AddPosVelEntity(Entity{2, 0}, {.x = 20.0F, .y = 0.0F}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};

      float sum = 0.0F;
      for (auto&& [pos] : iter) {
        sum += pos.x;
      }

      CHECK_EQ(sum, doctest::Approx(30.0F));
    }
  }

  TEST_CASE("ecs::BasicQueryIter::operator++ (postfix)") {
    SUBCASE("Postfix increment returns copy before advance") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto old = iter++;

      const auto& [old_pos] = *old;
      const auto& [new_pos] = *iter;

      CHECK_EQ(old_pos.x, 1.0F);
      CHECK_EQ(new_pos.x, 2.0F);
    }

    SUBCASE("Postfix increment advances the original iterator") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      iter++;

      CHECK_NE(iter, iter.end());
    }
  }

  TEST_CASE("ecs::BasicQueryIter::operator-- (prefix)") {
    SUBCASE("Prefix decrement moves back to previous entity") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;  // now at entity 2
      --iter;  // back at entity 1

      const auto& [pos] = *iter;
      CHECK_EQ(pos.x, 1.0F);
    }

    SUBCASE("Prefix decrement returns reference to same iterator") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;
      auto& returned = --iter;

      CHECK_EQ(&returned, &iter);
    }

    SUBCASE("Decrement at first entity is a no-op") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      --iter;  // already at index 0, should stay

      const auto& [pos] = *iter;
      CHECK_EQ(pos.x, 5.0F);
    }
  }

  TEST_CASE("ecs::BasicQueryIter::operator-- (postfix)") {
    SUBCASE("Postfix decrement returns copy before retreat") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;  // now at entity 2
      const auto& old = iter--;

      const auto& [old_pos] = *old;
      const auto& [new_pos] = *iter;

      CHECK_EQ(old_pos.x, 2.0F);
      CHECK_EQ(new_pos.x, 1.0F);
    }

    SUBCASE("Postfix decrement modifies the original iterator") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;  // at entity 2
      iter--;  // back at entity 1

      const auto& [pos] = *iter;
      CHECK_EQ(pos.x, 1.0F);
    }
  }

  TEST_CASE("ecs::BasicQueryIter::operator==") {
    SUBCASE("Two iterators at same position are equal") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> a{fix.ArchetypeSpan(), fix.manager, 0, 0};
      QueryIter<const Position&> b{fix.ArchetypeSpan(), fix.manager, 0, 0};

      CHECK(a == b);
    }

    SUBCASE("Two iterators at different positions are not equal") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> a{fix.ArchetypeSpan(), fix.manager, 0, 0};
      QueryIter<const Position&> b{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++b;

      CHECK(a != b);
    }

    SUBCASE("End iterators from the same span are equal") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      CHECK(iter.end() == iter.end());
    }

    SUBCASE("Begin equals end after iterating all entities") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      ++iter;

      CHECK(iter == iter.end());
    }
  }

  TEST_CASE("ecs::BasicQueryIter::begin / end") {
    SUBCASE("begin() returns copy pointing to first entity") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 3.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto b = iter.begin();
      const auto& [pos] = *b;

      CHECK_EQ(pos.x, 3.0F);
    }

    SUBCASE("Range-based for loop visits every entity exactly once") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> iter{fix.ArchetypeSpan(), fix.manager, 0, 0};

      int count = 0;
      float sum = 0.0F;
      for (auto&& [pos] : iter) {
        ++count;
        sum += pos.x;
      }

      CHECK_EQ(count, 3);
      CHECK_EQ(sum, doctest::Approx(6.0F));
    }

    SUBCASE("Iterating empty span produces zero iterations") {
      ComponentManager manager;
      std::vector<std::reference_wrapper<const Archetype>> empty;
      std::span<const std::reference_wrapper<const Archetype>> span = empty;

      QueryIter<const Position&> iter{span, manager, 0, 0};

      int count = 0;
      for ([[maybe_unused]] auto&& _ : iter) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::BasicQueryIter — copy semantics") {
    SUBCASE("Copied iterator is independent of the original") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> original{fix.ArchetypeSpan(), fix.manager, 0,
                                          0};
      auto copy = original;
      ++original;

      const auto& [copy_pos] = *copy;
      CHECK_EQ(copy_pos.x, 1.0F);  // copy did not advance
    }

    SUBCASE("Copy-assigned iterator behaves identically to source") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 7.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      QueryIter<const Position&> a{fix.ArchetypeSpan(), fix.manager, 0, 0};
      QueryIter<const Position&> b = a;

      CHECK(a == b);
    }
  }
}

TEST_SUITE("helios::ecs::BasicQueryWithEntityIter") {
  TEST_CASE("ecs::BasicQueryWithEntityIter::Constructor") {
    SUBCASE("Begin equals end for empty archetype list") {
      ComponentManager manager;
      std::vector<std::reference_wrapper<const Archetype>> empty;
      std::span<const std::reference_wrapper<const Archetype>> span = empty;

      BasicQueryWithEntityIter<false, const Position&> iter{span, manager, 0,
                                                            0};
      CHECK_EQ(iter, iter.end());
    }

    SUBCASE("Begin does not equal end when entities are present") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      CHECK_NE(iter, iter.end());
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::operator*") {
    SUBCASE("First element of tuple is the correct Entity") {
      const Entity e1{1, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 1.0F, .y = 2.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      auto&& [entity, pos] = *iter;

      CHECK_EQ(entity, e1);
    }

    SUBCASE("Component values are correct alongside entity") {
      const Entity e1{5, 0};
      QueryFixture fix;
      fix.AddPosVelEntity(e1, {.x = 3.0F, .y = 4.0F}, {.x = 5.0F, .y = 6.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&, const Velocity&> iter{
          fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto&& [entity, pos, vel] = *iter;

      CHECK_EQ(entity, e1);
      CHECK_EQ(pos, Position{.x = 3.0F, .y = 4.0F});
      CHECK_EQ(vel, Velocity{.x = 5.0F, .y = 6.0F});
    }

    SUBCASE("Const variant (IsConst=true) returns correct entity and data") {
      const Entity e1{2, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 9.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<true, const Position&> iter{fix.ArchetypeSpan(),
                                                           fix.manager, 0, 0};
      auto&& [entity, pos] = *iter;

      CHECK_EQ(entity, e1);
      CHECK_EQ(pos.x, 9.0F);
    }

    SUBCASE("Optional component returns nullptr when absent") {
      const Entity e1{3, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 1.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&, const Velocity*> iter{
          fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto&& [entity, pos, vel_ptr] = *iter;

      CHECK_EQ(entity, e1);
      CHECK_EQ(vel_ptr, nullptr);
    }

    SUBCASE("Optional component returns non-null pointer when present") {
      const Entity e1{4, 0};
      QueryFixture fix;
      fix.AddPosVelEntity(e1, {.x = 1.0F, .y = 0.0F}, {.x = 7.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&, const Velocity*> iter{
          fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto&& [entity, pos, vel_ptr] = *iter;

      REQUIRE_NE(vel_ptr, nullptr);
      CHECK_EQ(vel_ptr->x, 7.0F);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::operator++ (prefix)") {
    SUBCASE("Prefix increment advances to next entity") {
      const Entity e1{1, 0};
      const Entity e2{2, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(e2, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      ++iter;
      auto&& [entity, pos] = *iter;

      CHECK_EQ(entity, e2);
    }

    SUBCASE("Prefix increment returns reference to itself") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      auto& returned = ++iter;

      CHECK_EQ(&returned, &iter);
    }

    SUBCASE("Incrementing past last entity reaches end") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      ++iter;

      CHECK(iter == iter.end());
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::operator++ (postfix)") {
    SUBCASE("Postfix increment returns copy before advance") {
      const Entity e1{1, 0};
      const Entity e2{2, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 10.0F, .y = 0.0F});
      fix.AddPosEntity(e2, {.x = 20.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      auto old = iter++;

      auto&& [old_entity, old_pos] = *old;
      auto&& [new_entity, new_pos] = *iter;

      CHECK_EQ(old_entity, e1);
      CHECK_EQ(new_entity, e2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::operator-- (prefix)") {
    SUBCASE("Prefix decrement moves back to previous entity") {
      const Entity e1{1, 0};
      const Entity e2{2, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(e2, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      ++iter;  // at e2
      --iter;  // back at e1

      auto&& [entity, pos] = *iter;
      CHECK_EQ(entity, e1);
    }

    SUBCASE("Prefix decrement at first entity is a no-op") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      --iter;  // already at beginning

      auto&& [entity, pos] = *iter;
      CHECK_EQ(pos.x, 5.0F);
    }

    SUBCASE("Prefix decrement returns reference to itself") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      ++iter;
      auto& returned = --iter;

      CHECK_EQ(&returned, &iter);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::operator-- (postfix)") {
    SUBCASE("Postfix decrement returns copy before retreat") {
      const Entity e1{1, 0};
      const Entity e2{2, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(e2, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};
      ++iter;             // at e2
      auto old = iter--;  // returns copy pointing at e2, iter goes to e1

      auto&& [old_entity, old_pos] = *old;
      auto&& [new_entity, new_pos] = *iter;

      CHECK_EQ(old_entity, e2);
      CHECK_EQ(new_entity, e1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::operator==") {
    SUBCASE("Two iterators at same position are equal") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> a{fix.ArchetypeSpan(),
                                                         fix.manager, 0, 0};
      BasicQueryWithEntityIter<false, const Position&> b{fix.ArchetypeSpan(),
                                                         fix.manager, 0, 0};
      CHECK(a == b);
    }

    SUBCASE("Iterators at different positions are not equal") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {});
      fix.AddPosEntity(Entity{2, 0}, {});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> a{fix.ArchetypeSpan(),
                                                         fix.manager, 0, 0};
      auto b = a;
      ++b;

      CHECK(a != b);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter::begin / end") {
    SUBCASE("Range-based for loop visits every entity with correct entity id") {
      const Entity e1{1, 0};
      const Entity e2{2, 0};
      const Entity e3{3, 0};
      QueryFixture fix;
      fix.AddPosEntity(e1, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(e2, {.x = 2.0F, .y = 0.0F});
      fix.AddPosEntity(e3, {.x = 3.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> iter{fix.ArchetypeSpan(),
                                                            fix.manager, 0, 0};

      int count = 0;
      for (auto&& [entity, pos] : iter) {
        ++count;
        // Entity index must be one of the registered ones.
        const bool valid_entity = entity == e1 || entity == e2 || entity == e3;
        CHECK(valid_entity);
      }
      CHECK_EQ(count, 3);
    }

    SUBCASE("Iterating empty span produces zero iterations") {
      ComponentManager manager;
      std::vector<std::reference_wrapper<const Archetype>> empty;
      std::span<const std::reference_wrapper<const Archetype>> span = empty;

      BasicQueryWithEntityIter<false, const Position&> iter{span, manager, 0,
                                                            0};
      int count = 0;
      for ([[maybe_unused]] auto&& _ : iter) {
        ++count;
      }
      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntityIter — copy semantics") {
    SUBCASE("Copied iterator is independent of the original") {
      QueryFixture fix;
      fix.AddPosEntity(Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      fix.AddPosEntity(Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      fix.RebuildArchetypes();

      BasicQueryWithEntityIter<false, const Position&> original{
          fix.ArchetypeSpan(), fix.manager, 0, 0};
      auto copy = original;
      ++original;

      auto&& [copy_entity, copy_pos] = *copy;
      CHECK_EQ(copy_pos.x, 1.0F);
    }
  }
}
