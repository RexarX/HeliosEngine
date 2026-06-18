#include <doctest/doctest.h>

#include <helios/assert.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/manager.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/system/access_policy.hpp>

#include <algorithm>
#include <memory_resource>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace helios::ecs;

using Alloc = std::allocator<ComponentTypeIndex>;

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

struct Enemy {};

struct SparseBuff {
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;
  int stacks = 0;

  constexpr bool operator==(const SparseBuff&) const noexcept = default;
};

struct SparseFlag {
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;
  int value = 0;

  constexpr bool operator==(const SparseFlag&) const noexcept = default;
};

template <QueryArg... Args>
[[nodiscard]] auto MakeQuery(ComponentManager& manager)
    -> BasicQuery<World, std::allocator<ComponentTypeIndex>, Args...> {
  return BasicQuery<World, std::allocator<ComponentTypeIndex>, Args...>(
      manager);
}

// The "const World" variant for read-only queries
template <QueryArg... Args>
[[nodiscard]] auto MakeReadOnlyQuery(const ComponentManager& manager)
    -> BasicQuery<const World, std::allocator<ComponentTypeIndex>, Args...> {
  return BasicQuery<const World, std::allocator<ComponentTypeIndex>, Args...>(
      manager);
}

// Convenience: add an entity with Position only
void AddPos(ComponentManager& mgr, Entity entity, Position pos) {
  mgr.InitEntity(entity);
  mgr.AddArchetypeComponents(entity, pos);
}

// Convenience: add an entity with Position + Velocity
void AddPosVel(ComponentManager& mgr, Entity entity, Position pos,
               Velocity vel) {
  mgr.InitEntity(entity);
  mgr.AddArchetypeComponents(entity, pos);
  mgr.AddArchetypeComponents(entity, vel);
}

// Convenience: add an entity with Position + Velocity + Health
void AddPosVelHealth(ComponentManager& mgr, Entity entity, Position pos,
                     Velocity vel, Health hp) {
  mgr.InitEntity(entity);
  mgr.AddArchetypeComponents(entity, pos);
  mgr.AddArchetypeComponents(entity, vel);
  mgr.AddArchetypeComponents(entity, hp);
}

void AddSparseBuff(ComponentManager& mgr, Entity entity, SparseBuff buff) {
  if (!mgr.Tracked(entity)) {
    mgr.InitEntity(entity);
  }
  mgr.AddSparseComponent(entity, buff);
}

void AddSparseFlag(ComponentManager& mgr, Entity entity, SparseFlag flag) {
  if (!mgr.Tracked(entity)) {
    mgr.InitEntity(entity);
  }
  mgr.AddSparseComponent(entity, flag);
}

struct AssertionCapture {
  bool called = false;
  std::string condition;
  std::string message;
};

thread_local AssertionCapture g_assert_capture;

void QueryTestAssertionHandler(std::string_view condition,
                               const std::source_location&,
                               std::string_view message) noexcept {
  g_assert_capture.called = true;
  g_assert_capture.condition = std::string(condition);
  g_assert_capture.message = std::string(message);
}

void ResetAssertionCapture() {
  g_assert_capture.called = false;
  g_assert_capture.condition.clear();
  g_assert_capture.message.clear();
}

}  // namespace

TEST_SUITE("helios::ecs::BasicQuery") {
  TEST_CASE("ecs::BasicQuery::Empty") {
    SUBCASE("Returns true for empty manager") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.Empty());
    }

    SUBCASE("Returns true when no entities match the with-filter") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      // Query requires Velocity — no entity has it
      const auto query = MakeQuery<const Velocity&>(mgr);
      CHECK(query.Empty());
    }

    SUBCASE("Returns false when at least one entity matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(query.Empty());
    }

    SUBCASE(
        "Returns true when all matching entities are excluded by "
        "without-filter") {
      ComponentManager mgr;
      // Entity has both Position and Velocity; query excludes Velocity
      AddPosVel(mgr, Entity{1, 0}, {}, {});
      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK(query.Empty());
    }
  }

  TEST_CASE("ecs::BasicQuery::Count") {
    SUBCASE("Returns 0 for empty manager") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 0);
    }

    SUBCASE("Returns correct count for single matching entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 1);
    }

    SUBCASE(
        "Returns correct count across multiple entities in same archetype") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 3);
    }

    SUBCASE("Returns correct count across multiple archetypes") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPosVel(mgr, Entity{2, 0}, {}, {});
      // Query has no with-filter — visits all archetypes that have entities
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 2);
    }

    SUBCASE("Excludes entities filtered by without-types") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});         // matches
      AddPosVel(mgr, Entity{2, 0}, {}, {});  // excluded (has Velocity)
      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 1);
    }

    SUBCASE("Count reflects state after entity addition") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 0);

      AddPos(mgr, Entity{1, 0}, {});
      CHECK_EQ(query.Count(), 1);

      AddPos(mgr, Entity{2, 0}, {});
      CHECK_EQ(query.Count(), 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::begin / end") {
    SUBCASE("begin equals end for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.begin() == query.end());
    }

    SUBCASE("begin does not equal end when entities match") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.begin() != query.end());
    }

    SUBCASE("Range-based for visits every matching entity exactly once") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      float sum = 0.0F;
      for (auto&& [pos] : query) {
        ++count;
        sum += pos.x;
      }
      CHECK_EQ(count, 3);
      CHECK_EQ(sum, doctest::Approx(6.0F));
    }

    SUBCASE("Iterates across multiple archetypes") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 10.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 20.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&>(mgr);
      float sum = 0.0F;
      for (auto&& [pos] : query) {
        sum += pos.x;
      }
      CHECK_EQ(sum, doctest::Approx(30.0F));
    }

    SUBCASE("Mutable reference dereference allows component mutation") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<Position&>(mgr);

      for (auto&& [pos] : query) {
        pos.x = 99.0F;
      }

      // Re-query to verify mutation persisted
      for (auto&& [pos] : query) {
        CHECK_EQ(pos.x, 99.0F);
      }
    }
  }

  TEST_CASE("ecs::BasicQuery::Collect") {
    SUBCASE("Returns empty vector when no entities match") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.Collect().empty());
    }

    SUBCASE("Returns one tuple per matching entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 2.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 3.0F, .y = 4.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto result = query.Collect();
      CHECK_EQ(result.size(), 2);
    }

    SUBCASE("Collected values match stored component data") {
      ComponentManager mgr;
      const Position pos{.x = 5.0F, .y = 6.0F};
      AddPos(mgr, Entity{1, 0}, pos);
      const auto query = MakeQuery<const Position&>(mgr);
      const auto result = query.Collect();
      REQUIRE_EQ(result.size(), 1);
      CHECK_EQ(std::get<0>(result[0]), pos);
    }

    SUBCASE("Collect with multiple components returns full tuple") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 1.0F, .y = 2.0F},
                {.x = 3.0F, .y = 4.0F});
      const auto query = MakeQuery<const Position&, const Velocity&>(mgr);
      const auto result = query.Collect();
      REQUIRE_EQ(result.size(), 1);
      CHECK_EQ(std::get<0>(result[0]), (Position{.x = 1.0F, .y = 2.0F}));
      CHECK_EQ(std::get<1>(result[0]), (Velocity{.x = 3.0F, .y = 4.0F}));
    }
  }

  TEST_CASE("ecs::BasicQuery::CollectWith") {
    SUBCASE("Returns empty container when no entities match") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      const auto result =
          query.CollectWith(std::allocator<std::tuple<const Position&>>{});
      CHECK(result.empty());
    }

    SUBCASE("Collected values match those from default Collect") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 7.0F, .y = 8.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      const auto default_result = query.Collect();
      const auto custom_result =
          query.CollectWith(std::allocator<std::tuple<const Position&>>{});

      REQUIRE_EQ(custom_result.size(), default_result.size());
      CHECK_EQ(std::get<0>(custom_result[0]), std::get<0>(default_result[0]));
    }

    SUBCASE("Works with PMR allocator") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      auto* resource = std::pmr::get_default_resource();

      const auto query = MakeQuery<const Position&>(mgr);
      const auto result = query.CollectWith(resource);

      CHECK_EQ(result.size(), 1);
    }
  }

  TEST_CASE("ecs::BasicQuery::Into") {
    SUBCASE("Writes nothing for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      std::vector<std::tuple<const Position&>> out;
      query.Into(std::back_inserter(out));
      CHECK(out.empty());
    }

    SUBCASE("Writes all results to output iterator") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      std::vector<std::tuple<const Position&>> out;
      query.Into(std::back_inserter(out));
      CHECK_EQ(out.size(), 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::ForEach") {
    SUBCASE("Not called for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);

      int calls = 0;
      query.ForEach([&calls](const Position& /*pos*/) { ++calls; });
      CHECK_EQ(calls, 0);
    }

    SUBCASE("Called once per matching entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int calls = 0;
      query.ForEach([&calls](const Position& /*pos*/) { ++calls; });
      CHECK_EQ(calls, 2);
    }

    SUBCASE("Receives correct component values") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 42.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      float captured = 0.0F;
      query.ForEach([&captured](const Position& pos) { captured = pos.x; });
      CHECK_EQ(captured, doctest::Approx(42.0F));
    }

    SUBCASE("Mutable ForEach can modify components") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<Position&>(mgr);

      query.ForEach([](Position& pos) { pos.x = 99.0F; });

      float result = 0.0F;
      query.ForEach([&result](Position& pos) { result = pos.x; });
      CHECK_EQ(result, doctest::Approx(99.0F));
    }

    SUBCASE("Tuple-style lambda receives all components") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 3.0F, .y = 0.0F},
                {.x = 5.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&, const Velocity&>(mgr);

      float pos_x = 0.0F;
      float vel_x = 0.0F;
      query.ForEach([&pos_x, &vel_x](const Position& pos, const Velocity& vel) {
        pos_x = pos.x;
        vel_x = vel.x;
      });

      CHECK_EQ(pos_x, doctest::Approx(3.0F));
      CHECK_EQ(vel_x, doctest::Approx(5.0F));
    }
  }

  TEST_CASE("ecs::BasicQuery::ForEachWithEntity") {
    SUBCASE("Not called for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);

      int calls = 0;
      query.ForEachWithEntity(
          [&calls](Entity /*entity*/, const Position& /*pos*/) { ++calls; });
      CHECK_EQ(calls, 0);
    }

    SUBCASE("Provides correct entity to the action") {
      const Entity e1{7, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {});
      const auto query = MakeQuery<const Position&>(mgr);

      Entity captured;
      query.ForEachWithEntity(
          [&captured](Entity entity, const Position& /*pos*/) {
            captured = entity;
          });
      CHECK_EQ(captured, e1);
    }

    SUBCASE("Called once per matching entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int calls = 0;
      query.ForEachWithEntity(
          [&calls](Entity /*entity*/, const Position& /*pos*/) { ++calls; });
      CHECK_EQ(calls, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Find") {
    SUBCASE("Returns nullopt when no entities match") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      const auto found =
          query.Find([](const Position& /*pos*/) { return true; });
      CHECK_FALSE(found.has_value());
    }

    SUBCASE("Returns nullopt when predicate never matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto found =
          query.Find([](const Position& pos) { return pos.x > 100.0F; });
      CHECK_FALSE(found.has_value());
    }

    SUBCASE("Returns first matching element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 50.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      const auto found =
          query.Find([](const Position& pos) { return pos.x > 10.0F; });
      REQUIRE(found.has_value());
      CHECK_EQ(std::get<0>(*found).x, doctest::Approx(50.0F));
    }

    SUBCASE("Returns first match, not all matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 10.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int call_count = 0;
      const auto _ = query.Find([&call_count](const Position& pos) {
        ++call_count;
        return pos.x > 0.0F;  // all match, but Find should stop after first
      });
      CHECK_EQ(call_count, 1);
    }
  }

  TEST_CASE("ecs::BasicQuery::Any") {
    SUBCASE("Returns false for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(query.Any([](const Position& /*pos*/) { return true; }));
    }

    SUBCASE("Returns false when predicate never matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(
          query.Any([](const Position& pos) { return pos.x > 100.0F; }));
    }

    SUBCASE("Returns true when at least one entity matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 200.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.Any([](const Position& pos) { return pos.x > 100.0F; }));
    }
  }

  TEST_CASE("ecs::BasicQuery::All") {
    SUBCASE("Returns true for empty result set (vacuous truth)") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.All([](const Position& /*pos*/) { return false; }));
    }

    SUBCASE("Returns true when all entities satisfy predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 10.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.All([](const Position& pos) { return pos.x > 0.0F; }));
    }

    SUBCASE("Returns false when any entity fails predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = -1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(query.All([](const Position& pos) { return pos.x > 0.0F; }));
    }
  }

  TEST_CASE("ecs::BasicQuery::None") {
    SUBCASE("Returns true for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.None([](const Position& /*pos*/) { return true; }));
    }

    SUBCASE("Returns true when predicate matches no entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK(query.None([](const Position& pos) { return pos.x > 100.0F; }));
    }

    SUBCASE("Returns false when predicate matches at least one entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 200.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(
          query.None([](const Position& pos) { return pos.x > 100.0F; }));
    }
  }

  TEST_CASE("ecs::BasicQuery::CountIf") {
    SUBCASE("Returns 0 for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.CountIf([](const Position& /*pos*/) { return true; }), 0);
    }

    SUBCASE("Returns 0 when no entity satisfies predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(
          query.CountIf([](const Position& pos) { return pos.x > 100.0F; }), 0);
    }

    SUBCASE("Returns correct count when subset satisfies predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 200.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 300.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(
          query.CountIf([](const Position& pos) { return pos.x > 100.0F; }), 2);
    }

    SUBCASE("Returns total count when all satisfy predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.CountIf([](const Position& /*pos*/) { return true; }), 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Fold") {
    SUBCASE("Returns init value for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      const float result = query.Fold(
          0.0F, [](float acc, const Position& pos) { return acc + pos.x; });
      CHECK_EQ(result, doctest::Approx(0.0F));
    }

    SUBCASE("Accumulates correctly over all matching entities") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      const float sum = query.Fold(
          0.0F, [](float acc, const Position& pos) { return acc + pos.x; });
      CHECK_EQ(sum, doctest::Approx(6.0F));
    }

    SUBCASE("Init value is used as base for accumulation") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      const float result = query.Fold(
          10.0F, [](float acc, const Position& pos) { return acc + pos.x; });
      CHECK_EQ(result, doctest::Approx(11.0F));
    }
  }

  TEST_CASE("ecs::BasicQuery::Partition") {
    SUBCASE("Both partitions empty for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      const auto [yes, no] =
          query.Partition([](const Position& /*pos*/) { return true; });

      CHECK(yes.empty());
      CHECK(no.empty());
    }

    SUBCASE("All in first partition when predicate always true") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto [yes, no] =
          query.Partition([](const Position& /*pos*/) { return true; });
      CHECK_EQ(yes.size(), 2);
      CHECK(no.empty());
    }

    SUBCASE("All in second partition when predicate always false") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto [yes, no] =
          query.Partition([](const Position& /*pos*/) { return false; });
      CHECK(yes.empty());
      CHECK_EQ(no.size(), 2);
    }

    SUBCASE("Splits correctly on mixed data") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 15.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 25.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto [yes, no] =
          query.Partition([](const Position& pos) { return pos.x > 10.0F; });
      CHECK_EQ(yes.size(), 2);
      CHECK_EQ(no.size(), 1);
    }
  }

  TEST_CASE("ecs::BasicQuery::MaxBy") {
    SUBCASE("Returns nullopt for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(
          query.MaxBy([](const Position& pos) { return pos.x; }).has_value());
    }

    SUBCASE("Returns the single element when only one exists") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 7.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto result =
          query.MaxBy([](const Position& pos) { return pos.x; });
      REQUIRE(result.has_value());
      CHECK_EQ(std::get<0>(*result).x, doctest::Approx(7.0F));
    }

    SUBCASE("Returns element with largest key") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 99.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 50.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto result =
          query.MaxBy([](const Position& pos) { return pos.x; });
      REQUIRE(result.has_value());
      CHECK_EQ(std::get<0>(*result).x, doctest::Approx(99.0F));
    }
  }

  TEST_CASE("ecs::BasicQuery::MinBy") {
    SUBCASE("Returns nullopt for empty result set") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_FALSE(
          query.MinBy([](const Position& pos) { return pos.x; }).has_value());
    }

    SUBCASE("Returns element with smallest key") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 10.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 50.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);
      const auto result =
          query.MinBy([](const Position& pos) { return pos.x; });
      REQUIRE(result.has_value());
      CHECK_EQ(std::get<0>(*result).x, doctest::Approx(1.0F));
    }
  }

  TEST_CASE("ecs::BasicQuery::Filter") {
    SUBCASE("Produces no elements when predicate never matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ :
           query.Filter([](const Position& pos) { return pos.x > 100.0F; })) {
        ++count;
      }
      CHECK_EQ(count, 0);
    }

    SUBCASE("Produces only elements satisfying the predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 200.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 300.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ :
           query.Filter([](const Position& pos) { return pos.x > 100.0F; })) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Map") {
    SUBCASE("Transforms each element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 3.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 4.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      float sum = 0.0F;
      for (float val :
           query.Map([](const Position& pos) { return pos.x * 2.0F; })) {
        sum += val;
      }
      CHECK_EQ(sum, doctest::Approx(14.0F));
    }
  }

  TEST_CASE("ecs::BasicQuery::Take") {
    SUBCASE("Take(0) yields no elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.Take(0)) {
        ++count;
      }
      CHECK_EQ(count, 0);
    }

    SUBCASE("Take(N) yields at most N elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.Take(2)) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }

    SUBCASE("Take(N) where N >= count yields all elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.Take(100)) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Skip") {
    SUBCASE("Skip(0) yields all elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.Skip(0)) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }

    SUBCASE("Skip(N) skips exactly N elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.Skip(2)) {
        ++count;
      }
      CHECK_EQ(count, 1);
    }

    SUBCASE("Skip(N) where N >= count yields no elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.Skip(100)) {
        ++count;
      }
      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::BasicQuery::TakeWhile") {
    SUBCASE("Stops at first element failing predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      // Iteration order is not guaranteed, but TakeWhile must stop as soon as
      // the predicate fails for the first encountered element.
      float sum = 0.0F;
      int count = 0;
      for (auto&& [pos] :
           query.TakeWhile([](const Position& pos) { return pos.x < 3.0F; })) {
        sum += pos.x;
        ++count;
      }
      CHECK_LE(count, 2);
    }

    SUBCASE("Produces all elements when predicate always true") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ :
           query.TakeWhile([](const Position& /*pos*/) { return true; })) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::SkipWhile") {
    SUBCASE("Skips leading elements satisfying predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ :
           query.SkipWhile([](const Position& pos) { return pos.x < 3.0F; })) {
        ++count;
      }
      CHECK_GE(count, 1);
    }

    SUBCASE("Produces all elements when predicate immediately false") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ :
           query.SkipWhile([](const Position& /*pos*/) { return false; })) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Enumerate") {
    SUBCASE("Index starts at 0") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      size_t first_index = 999;
      for (auto&& [idx, tuple] : query.Enumerate()) {
        first_index = idx;
        break;
      }
      CHECK_EQ(first_index, 0);
    }

    SUBCASE("Index increments for each element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      size_t last_index = 0;
      size_t count = 0;
      for (auto&& [idx, tuple] : query.Enumerate()) {
        last_index = idx;
        ++count;
      }
      CHECK_EQ(count, 3);
      CHECK_EQ(last_index, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::StepBy") {
    SUBCASE("StepBy(1) yields all elements") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.StepBy(1)) {
        ++count;
      }
      CHECK_EQ(count, 3);
    }

    SUBCASE("StepBy(2) yields every other element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      AddPos(mgr, Entity{4, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);

      int count = 0;
      for ([[maybe_unused]] auto&& _ : query.StepBy(2)) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Inspect") {
    SUBCASE(
        "Inspector is called for every element without altering iteration") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      int inspect_calls = 0;
      int iter_count = 0;
      for ([[maybe_unused]] auto&& _ :
           query.Inspect([&inspect_calls](const Position& /*pos*/) {
             ++inspect_calls;
           })) {
        ++iter_count;
      }
      CHECK_EQ(inspect_calls, 2);
      CHECK_EQ(iter_count, 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::Reverse") {
    SUBCASE("Produces elements in reverse order within a single archetype") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&>(mgr);

      std::vector<float> forward_xs;
      for (auto&& [pos] : query) {
        forward_xs.push_back(pos.x);
      }
      std::vector<float> reverse_xs;
      for (auto&& [pos] : query.Reverse()) {
        reverse_xs.push_back(pos.x);
      }

      CHECK_EQ(reverse_xs.size(), forward_xs.size());
      std::ranges::reverse(forward_xs);
      CHECK_EQ(reverse_xs, forward_xs);
    }
  }

  TEST_CASE("ecs::BasicQuery: Optional component") {
    SUBCASE("Optional component is nullptr when entity lacks it") {
      ComponentManager mgr;
      // Entity has only Position, no Velocity
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&, const Velocity*>(mgr);

      int null_count = 0;
      for (auto&& [pos, vel_ptr] : query) {
        if (vel_ptr == nullptr) {
          ++null_count;
        }
      }
      CHECK_EQ(null_count, 1);
    }

    SUBCASE("Optional component is non-null when entity has it") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F},
                {.x = 5.0F, .y = 0.0F});
      const auto query = MakeQuery<const Position&, const Velocity*>(mgr);

      const Velocity* captured = nullptr;
      for (auto&& [pos, vel_ptr] : query) {
        captured = vel_ptr;
      }
      REQUIRE_NE(captured, nullptr);
      CHECK_EQ(captured->x, doctest::Approx(5.0F));
    }

    SUBCASE(
        "Query with optional component visits entities regardless of optional "
        "presence") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});         // no Velocity
      AddPosVel(mgr, Entity{2, 0}, {}, {});  // has Velocity
      const auto query = MakeQuery<const Position&, const Velocity*>(mgr);
      CHECK_EQ(query.Count(), 2);
    }
  }

  TEST_CASE("ecs::BasicQuery — without-filter") {
    SUBCASE("Excludes entities that have the unwanted component") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});         // Position only — included
      AddPosVel(mgr, Entity{2, 0}, {}, {});  // Position + Velocity — excluded
      AddPosVel(mgr, Entity{3, 0}, {}, {});  // Position + Velocity — excluded

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 1);
    }

    SUBCASE("All entities pass when without-filter matches nothing") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});

      // Exclude Velocity — but no entity has Velocity
      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 2);
    }
  }

  TEST_CASE("ecs::BasicQuery — read-only (const World)") {
    SUBCASE("ReadOnlyQuery iterates const data correctly") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 9.0F, .y = 0.0F});

      const auto& const_mgr = mgr;
      const auto query = MakeReadOnlyQuery<const Position&>(const_mgr);

      float captured = 0.0F;
      for (auto&& [pos] : query) {
        captured = pos.x;
      }
      CHECK_EQ(captured, doctest::Approx(9.0F));
    }

    SUBCASE("ReadOnlyQuery Count returns correct value") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});

      const auto& const_mgr = mgr;
      const auto query = MakeReadOnlyQuery<const Position&>(const_mgr);
      CHECK_EQ(query.Count(), 2);
    }
  }

  TEST_CASE("ecs::BasicQuery::WithEntity") {
    SUBCASE("Returns BasicQueryWithEntity wrapping this query") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);

      // WithEntity wrapper iterates and includes the entity
      const auto we = query.WithEntity();
      int count = 0;
      for ([[maybe_unused]] auto&& _ : we) {
        ++count;
      }
      CHECK_EQ(count, 1);
    }
  }

  TEST_CASE("ecs::BasicQuery::Get") {
    SUBCASE("Returns sparse + archetype components for matching entity") {
      ComponentManager mgr;
      constexpr Entity entity{10, 0};
      AddPos(mgr, entity, {.x = 1.0F, .y = 2.0F});
      AddSparseBuff(mgr, entity, {.stacks = 7});

      const auto query = MakeQuery<const Position&, const SparseBuff&>(mgr);
      const auto& [pos, buff] = query.Get(entity);

      CHECK_EQ(pos.x, doctest::Approx(1.0F));
      CHECK_EQ(buff.stacks, 7);
    }
  }

  TEST_CASE("ecs::BasicQuery::TryGet") {
    SUBCASE("Returns value for sparse + archetype components") {
      ComponentManager mgr;
      constexpr Entity entity{11, 0};
      AddPos(mgr, entity, {.x = 3.0F, .y = 4.0F});
      AddSparseBuff(mgr, entity, {.stacks = 5});

      const auto query = MakeQuery<const Position&, const SparseBuff&>(mgr);
      const auto got = query.TryGet(entity);

      REQUIRE(got.has_value());
      CHECK_EQ(std::get<0>(*got).x, doctest::Approx(3.0F));
      CHECK_EQ(std::get<1>(*got).stacks, 5);
    }

    SUBCASE("Ignores query filters and only checks requested components") {
      ComponentManager mgr;
      constexpr Entity entity{12, 0};
      AddPos(mgr, entity, {.x = 8.0F, .y = 9.0F});

      // Query requires SparseBuff through with-filter, but requested component
      // set is only Position.
      const auto query =
          MakeQuery<const Position&, With<SparseBuff>, Without<SparseFlag>>(
              mgr);

      const auto got = query.TryGet(entity);
      REQUIRE(got.has_value());
      CHECK_EQ(std::get<0>(*got).x, doctest::Approx(8.0F));
    }
  }

  TEST_CASE("ecs::BasicQuery::TryGetFiltered") {
    SUBCASE("Applies sparse with-filter") {
      ComponentManager mgr;
      constexpr Entity e_no_sparse{13, 0};
      constexpr Entity e_with_sparse{14, 0};
      AddPos(mgr, e_no_sparse, {});
      AddPos(mgr, e_with_sparse, {});
      AddSparseBuff(mgr, e_with_sparse, {.stacks = 1});

      const auto query = MakeQuery<const Position&, With<SparseBuff>>(mgr);

      CHECK_FALSE(query.TryGetFiltered(e_no_sparse).has_value());
      CHECK(query.TryGetFiltered(e_with_sparse).has_value());
    }

    SUBCASE("Applies sparse without-filter") {
      ComponentManager mgr;
      constexpr Entity e_clean{15, 0};
      constexpr Entity e_excluded{16, 0};
      AddPos(mgr, e_clean, {});
      AddPos(mgr, e_excluded, {});
      AddSparseFlag(mgr, e_excluded, {.value = 1});

      const auto query = MakeQuery<const Position&, Without<SparseFlag>>(mgr);

      CHECK(query.TryGetFiltered(e_clean).has_value());
      CHECK_FALSE(query.TryGetFiltered(e_excluded).has_value());
    }
  }

  TEST_CASE("ecs::BasicQuery — dynamic archetype refresh") {
    SUBCASE("Query reflects entities added after construction") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 0);

      AddPos(mgr, Entity{1, 0}, {});
      CHECK_EQ(query.Count(), 1);

      AddPos(mgr, Entity{2, 0}, {});
      CHECK_EQ(query.Count(), 2);
    }

    SUBCASE("Query reflects entities removed after construction") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      const auto query = MakeQuery<const Position&>(mgr);
      CHECK_EQ(query.Count(), 2);

      mgr.RemoveEntity(Entity{1, 0});
      CHECK_EQ(query.Count(), 1);
    }
  }
}

TEST_SUITE("helios::ecs::BasicQueryWithEntity") {
  TEST_CASE("ecs::BasicQueryWithEntity::Collect") {
    SUBCASE("Returns empty vector when no entities match") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK(we.Collect().empty());
    }

    SUBCASE("Each element begins with the correct Entity") {
      constexpr Entity e1{3, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {.x = 7.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto result = we.Collect();
      REQUIRE_EQ(result.size(), 1);
      CHECK_EQ(std::get<0>(result[0]), e1);
      CHECK_EQ(std::get<1>(result[0]).x, doctest::Approx(7.0F));
    }

    SUBCASE("Collect returns one tuple per matching entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      CHECK_EQ(we.Collect().size(), 2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::CollectWith") {
    SUBCASE("Returns empty container when no entities match") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      const auto result = we.CollectWith(
          std::allocator<std::tuple<Entity /*entity*/, const Position&>>{});
      CHECK(result.empty());
    }

    SUBCASE("Collected values match those from default Collect") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 7.0F, .y = 8.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto default_result = we.Collect();
      const auto custom_result = we.CollectWith(
          std::allocator<std::tuple<Entity /*entity*/, const Position&>>{});

      REQUIRE_EQ(custom_result.size(), default_result.size());
      CHECK_EQ(std::get<0>(custom_result[0]), std::get<0>(default_result[0]));
      CHECK_EQ(std::get<1>(custom_result[0]).x,
               std::get<1>(default_result[0]).x);
    }

    SUBCASE("Works with PMR allocator") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 6.0F});
      auto* resource = std::pmr::get_default_resource();

      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      auto result = we.CollectWith(resource);

      REQUIRE_EQ(result.size(), 1);
      CHECK_EQ(std::get<0>(result[0]), Entity{1, 0});
      CHECK_EQ(std::get<1>(result[0]).x, doctest::Approx(5.0F));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::CollectEntities") {
    SUBCASE("Returns empty vector when no entities match") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK(we.CollectEntities().empty());
    }

    SUBCASE("Returns all matching entity IDs") {
      constexpr Entity e1{1, 0};
      constexpr Entity e2{2, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {});
      AddPos(mgr, e2, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto entities = we.CollectEntities();
      REQUIRE_EQ(entities.size(), 2);

      const bool has_e1 = std::ranges::find(entities, e1) != entities.end();
      const bool has_e2 = std::ranges::find(entities, e2) != entities.end();
      CHECK(has_e1);
      CHECK(has_e2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::CollectEntitiesWith") {
    SUBCASE("Works with PMR allocator") {
      constexpr Entity e1{4, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {});
      auto* resource = std::pmr::get_default_resource();

      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      const auto entities = we.CollectEntitiesWith(resource);

      REQUIRE_EQ(entities.size(), 1);
      CHECK_EQ(entities[0], e1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Into") {
    SUBCASE("Writes nothing for empty result set") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      auto we = query.WithEntity();

      using ValueType = std::tuple<Entity /*entity*/, const Position&>;
      std::vector<ValueType> out;
      we.Into(std::back_inserter(out));
      CHECK(out.empty());
    }

    SUBCASE("Writes all results including entity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      auto we = query.WithEntity();

      using ValueType = std::tuple<Entity /*entity*/, const Position&>;
      std::vector<ValueType> out;
      we.Into(std::back_inserter(out));
      CHECK_EQ(out.size(), 2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::ForEach") {
    SUBCASE("Not called for empty result set") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      int calls = 0;
      we.ForEach(
          [&calls](Entity /*entity*/, const Position& /*pos*/) { ++calls; });
      CHECK_EQ(calls, 0);
    }

    SUBCASE("Called once per matching entity with correct Entity argument") {
      constexpr Entity e1{5, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      Entity captured;
      we.ForEach([&captured](Entity entity, const Position& /*pos*/) {
        captured = entity;
      });
      CHECK_EQ(captured, e1);
    }

    SUBCASE("Tuple-style lambda receives entity and all components") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 3.0F, .y = 0.0F},
                {.x = 5.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&, const Velocity&>(mgr);
      const auto we = query.WithEntity();

      float pos_x = 0.0F, vel_x = 0.0F;
      we.ForEach([&pos_x, &vel_x](Entity /*entity*/, const Position& pos,
                                  const Velocity& vel) {
        pos_x = pos.x;
        vel_x = vel.x;
      });
      CHECK_EQ(pos_x, doctest::Approx(3.0F));
      CHECK_EQ(vel_x, doctest::Approx(5.0F));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Find") {
    SUBCASE("Returns nullopt when no entities match") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_FALSE(we.Find([](Entity /*entity*/, const Position& /*pos*/) {
                      return true;
                    }).has_value());
    }

    SUBCASE("Returns first matching element with entity") {
      constexpr Entity e1{1, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {.x = 50.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto found = we.Find(
          [](Entity /*entity*/, const Position& pos) { return pos.x > 10.0F; });
      REQUIRE(found.has_value());
      CHECK_EQ(std::get<0>(*found), e1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Any") {
    SUBCASE("Returns false for empty result set") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_FALSE(we.Any(
          [](Entity /*entity*/, const Position& /*pos*/) { return true; }));
    }

    SUBCASE("Returns true when predicate matches at least one result") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 200.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK(we.Any([](Entity /*entity*/, const Position& pos) {
        return pos.x > 100.0F;
      }));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::All") {
    SUBCASE("Returns true for empty result set") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK(we.All(
          [](Entity /*entity*/, const Position& /*pos*/) { return false; }));
    }

    SUBCASE("Returns false when any result fails predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = -1.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_FALSE(we.All(
          [](Entity /*entity*/, const Position& pos) { return pos.x > 0.0F; }));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::None") {
    SUBCASE("Returns true when no result matches predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK(we.None([](Entity /*entity*/, const Position& pos) {
        return pos.x > 100.0F;
      }));
    }

    SUBCASE("Returns false when at least one result matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 200.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_FALSE(we.None([](Entity /*entity*/, const Position& pos) {
        return pos.x > 100.0F;
      }));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::CountIf") {
    SUBCASE("Returns 0 when no result matches") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_EQ(we.CountIf([](Entity /*entity*/, const Position& pos) {
        return pos.x > 100.0F;
      }),
               0);
    }

    SUBCASE("Returns correct count of matching results") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 200.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 300.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_EQ(we.CountIf([](Entity /*entity*/, const Position& pos) {
        return pos.x > 100.0F;
      }),
               2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Fold") {
    SUBCASE("Returns init value for empty result set") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      const int result =
          we.Fold(0, [](int acc, Entity /*entity*/, const Position& /*pos*/) {
            return acc + 1;
          });
      CHECK_EQ(result, 0);
    }

    SUBCASE("Accumulates entity count correctly") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const int count =
          we.Fold(0, [](int acc, Entity /*entity*/, const Position& /*pos*/) {
            return acc + 1;
          });
      CHECK_EQ(count, 3);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::MaxBy") {
    SUBCASE("Returns nullopt for empty result set") {
      ComponentManager mgr;
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();
      CHECK_FALSE(we.MaxBy([](Entity /*entity*/, const Position& pos) {
                      return pos.x;
                    }).has_value());
    }

    SUBCASE("Returns result with largest key") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 99.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto result = we.MaxBy(
          [](Entity /*entity*/, const Position& pos) { return pos.x; });
      REQUIRE(result.has_value());
      CHECK_EQ(std::get<1>(*result).x, doctest::Approx(99.0F));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::MinBy") {
    SUBCASE("Returns result with smallest key") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 10.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 1.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto result = we.MinBy(
          [](Entity /*entity*/, const Position& pos) { return pos.x; });
      REQUIRE(result.has_value());
      CHECK_EQ(std::get<1>(*result).x, doctest::Approx(1.0F));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Partition") {
    SUBCASE("Splits correctly on entity-aware predicate") {
      constexpr Entity e1{1, 0};
      constexpr Entity e2{2, 0};
      ComponentManager mgr;
      AddPos(mgr, e1, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, e2, {.x = 50.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      const auto [yes, no] = we.Partition(
          [](Entity /*entity*/, const Position& pos) { return pos.x > 10.0F; });
      CHECK_EQ(yes.size(), 1);
      CHECK_EQ(no.size(), 1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Filter") {
    SUBCASE("Produces only elements matching predicate") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 200.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      int count = 0;
      for ([[maybe_unused]] auto&& _ :
           we.Filter([](Entity /*entity*/, const Position& pos) {
             return pos.x > 100.0F;
           })) {
        ++count;
      }
      CHECK_EQ(count, 1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Map") {
    SUBCASE("Transforms each element to a derived value") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 3.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 4.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      float sum = 0.0F;
      for (float val : we.Map([](Entity /*entity*/, const Position& pos) {
             return pos.x * 2.0F;
           })) {
        sum += val;
      }
      CHECK_EQ(sum, doctest::Approx(14.0F));
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Take") {
    SUBCASE("Take(1) yields only the first element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      int count = 0;
      for ([[maybe_unused]] auto&& _ : we.Take(1)) {
        ++count;
      }
      CHECK_EQ(count, 1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Skip") {
    SUBCASE("Skip(1) skips first element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      int count = 0;
      for ([[maybe_unused]] auto&& _ : we.Skip(1)) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Enumerate") {
    SUBCASE("Index starts at 0 and increments per element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      size_t last_idx = 999;
      size_t count = 0;
      for (auto&& [idx, entity, pos] : we.Enumerate()) {
        last_idx = idx;
        ++count;
      }
      CHECK_EQ(count, 2);
      CHECK_EQ(last_idx, 1);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::StepBy") {
    SUBCASE("StepBy(2) yields every other element") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      AddPos(mgr, Entity{3, 0}, {});
      AddPos(mgr, Entity{4, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      int count = 0;
      for ([[maybe_unused]] auto&& _ : we.StepBy(2)) {
        ++count;
      }
      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::Inspect") {
    SUBCASE("Inspector called for every element without disrupting iteration") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      int inspect_count = 0;
      int iter_count = 0;
      for ([[maybe_unused]] auto&& _ : we.Inspect(
               [&inspect_count](Entity /*entity*/, const Position& /*pos*/) {
                 ++inspect_count;
               })) {
        ++iter_count;
      }
      CHECK_EQ(inspect_count, 2);
      CHECK_EQ(iter_count, 2);
    }
  }

  TEST_CASE("ecs::BasicQueryWithEntity::GroupBy") {
    SUBCASE("Groups entities correctly by integer bucket") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 1.0F, .y = 0.0F});
      AddPos(mgr, Entity{3, 0}, {.x = 2.0F, .y = 0.0F});
      auto query = MakeQuery<const Position&>(mgr);
      const auto we = query.WithEntity();

      // Group by integer part of x
      const auto groups =
          we.GroupBy([](Entity /*entity*/, const Position& pos) {
            return static_cast<int>(pos.x);
          });

      CHECK_EQ(groups.size(), 2);
      const bool has_group_1 = groups.contains(1);
      const bool has_group_2 = groups.contains(2);
      CHECK(has_group_1);
      CHECK(has_group_2);
      if (has_group_1) {
        CHECK_EQ(groups.at(1).size(), 2);
      }
      if (has_group_2) {
        CHECK_EQ(groups.at(2).size(), 1);
      }
    }
  }

  TEST_CASE("ecs::BasicQuery - With filter") {
    SUBCASE("With<T> includes only entities that also have T") {
      ComponentManager mgr;
      // Entity 1: Position only — must be excluded
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      // Entity 2: Position + Velocity — must be included
      AddPosVel(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 1);

      float sum = 0.0F;
      query.ForEach([&sum](const Position& pos) { sum += pos.x; });
      CHECK_EQ(sum, doctest::Approx(2.0F));
    }

    SUBCASE("With<T> on empty manager returns empty query") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      CHECK(query.Empty());
    }

    SUBCASE("With<T> where no entity owns T returns empty query") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      CHECK(query.Empty());
      CHECK_EQ(query.Count(), 0);
    }

    SUBCASE("With<T> where all entities own T returns all entities") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {}, {});
      AddPosVel(mgr, Entity{2, 0}, {}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 2);
    }

    SUBCASE("With<T, U> requires both T and U to be present") {
      ComponentManager mgr;
      // Entity 1: Position + Velocity (no Health) — excluded
      AddPosVel(mgr, Entity{1, 0}, {}, {});
      // Entity 2: Position + Velocity + Health — included
      AddPosVelHealth(mgr, Entity{2, 0}, {.x = 5.0F, .y = 0.0F}, {}, {});

      const auto query =
          MakeQuery<const Position&, With<Velocity, Health>>(mgr);
      CHECK_EQ(query.Count(), 1);

      float sum = 0.0F;
      query.ForEach([&sum](const Position& pos) { sum += pos.x; });
      CHECK_EQ(sum, doctest::Approx(5.0F));
    }

    SUBCASE("With<T> does not expose T in the iteration tuple") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 7.0F, .y = 0.0F},
                {.x = 3.0F, .y = 0.0F});

      // Only Position& is in the component list; Velocity is a filter
      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      float captured = 0.0F;
      query.ForEach([&captured](const Position& pos) { captured = pos.x; });
      CHECK_EQ(captured, doctest::Approx(7.0F));
    }

    SUBCASE("With<T> is respected by range-based for loop") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {});
      AddPosVel(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      int count = 0;
      float sum = 0.0F;
      for (auto&& [pos] : query) {
        ++count;
        sum += pos.x;
      }
      CHECK_EQ(count, 2);
      CHECK_EQ(sum, doctest::Approx(5.0F));
    }

    SUBCASE("With<T> is respected by ForEachWithEntity") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPosVel(mgr, Entity{2, 0}, {}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);

      int calls = 0;
      query.ForEachWithEntity(
          [&calls](Entity /*entity*/, const Position& /*pos*/) { ++calls; });
      CHECK_EQ(calls, 1);
    }

    SUBCASE("With<T> is respected by Collect") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPosVel(mgr, Entity{2, 0}, {.x = 9.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      const auto result = query.Collect();
      REQUIRE_EQ(result.size(), 1);
      CHECK_EQ(std::get<0>(result[0]).x, doctest::Approx(9.0F));
    }

    SUBCASE("With<T> is respected by Any") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 999.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 1.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      // Only entity 2 passes the filter; its x is 1.0, not > 100
      CHECK_FALSE(
          query.Any([](const Position& pos) { return pos.x > 100.0F; }));
    }

    SUBCASE("With<T> is respected by CountIf") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F}, {});
      AddPosVel(mgr, Entity{2, 0}, {.x = 50.0F, .y = 0.0F}, {});
      AddPos(mgr, Entity{3, 0}, {.x = 200.0F, .y = 0.0F});  // excluded

      const auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      CHECK_EQ(query.CountIf([](const Position& pos) { return pos.x > 10.0F; }),
               1);
    }

    SUBCASE("With<T> combined with WithEntity respects filter") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPosVel(mgr, Entity{2, 0}, {}, {});

      auto query = MakeQuery<const Position&, With<Velocity>>(mgr);
      const auto we = query.WithEntity();

      int count = 0;
      we.ForEach(
          [&count](Entity /*entity*/, const Position& /*pos*/) { ++count; });
      CHECK_EQ(count, 1);
    }
  }

  TEST_CASE("ecs::BasicQuery - Without filter") {
    SUBCASE("Without<T> excludes entities that have T") {
      ComponentManager mgr;
      // Entity 1: Position only — included
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      // Entity 2: Position + Velocity — excluded
      AddPosVel(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 1);

      float sum = 0.0F;
      query.ForEach([&sum](const Position& pos) { sum += pos.x; });
      CHECK_EQ(sum, doctest::Approx(1.0F));
    }

    SUBCASE("Without<T> on empty manager returns empty query") {
      ComponentManager mgr;
      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK(query.Empty());
    }

    SUBCASE("Without<T> where all entities own T returns empty query") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {}, {});
      AddPosVel(mgr, Entity{2, 0}, {}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK(query.Empty());
      CHECK_EQ(query.Count(), 0);
    }

    SUBCASE("Without<T> where no entity owns T returns all entities") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {});
      AddPos(mgr, Entity{2, 0}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK_EQ(query.Count(), 2);
    }

    SUBCASE("Without<T, U> excludes entities owning either T or U") {
      ComponentManager mgr;
      // Entity 1: Position only — included
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      // Entity 2: Position + Velocity — excluded (has Velocity)
      AddPosVel(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {});
      // Entity 3: Position + Velocity + Health — excluded (has both)
      AddPosVelHealth(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F}, {}, {});

      const auto query =
          MakeQuery<const Position&, Without<Velocity, Health>>(mgr);
      CHECK_EQ(query.Count(), 1);

      float sum = 0.0F;
      query.ForEach([&sum](const Position& pos) { sum += pos.x; });
      CHECK_EQ(sum, doctest::Approx(1.0F));
    }

    SUBCASE("Without<T> is respected by range-based for loop") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {});
      AddPosVel(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      int count = 0;
      float sum = 0.0F;
      for (auto&& [pos] : query) {
        ++count;
        sum += pos.x;
      }
      CHECK_EQ(count, 1);
      CHECK_EQ(sum, doctest::Approx(1.0F));
    }

    SUBCASE("Without<T> is respected by ForEachWithEntity") {
      ComponentManager mgr;
      constexpr Entity e1{1, 0};
      AddPos(mgr, e1, {});
      AddPosVel(mgr, Entity{2, 0}, {}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);

      Entity captured{0, 0};
      int calls = 0;
      query.ForEachWithEntity(
          [&captured, &calls](Entity entity, const Position& /*pos*/) {
            captured = entity;
            ++calls;
          });
      CHECK_EQ(calls, 1);
      CHECK_EQ(captured, e1);
    }

    SUBCASE("Without<T> is respected by Collect") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 42.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 0.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      const auto result = query.Collect();
      REQUIRE_EQ(result.size(), 1);
      CHECK_EQ(std::get<0>(result[0]).x, doctest::Approx(42.0F));
    }

    SUBCASE("Without<T> is respected by Any") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 999.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 1.0F, .y = 0.0F}, {});

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      // Only entity 1 passes; its x > 100
      CHECK(query.Any([](const Position& pos) { return pos.x > 100.0F; }));
    }

    SUBCASE("Without<T> is respected by All") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 10.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{3, 0}, {.x = -1.0F, .y = 0.0F}, {});  // excluded

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      // Entities 1 and 2 both have x > 0; entity 3 is excluded by the filter
      CHECK(query.All([](const Position& pos) { return pos.x > 0.0F; }));
    }

    SUBCASE("Without<T> is respected by None") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 999.0F, .y = 0.0F}, {});  // excluded

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      // Entity 2 (x=999) is excluded; only entity 1 (x=1) remains
      CHECK(query.None([](const Position& pos) { return pos.x > 100.0F; }));
    }

    SUBCASE("Without<T> is respected by CountIf") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPos(mgr, Entity{2, 0}, {.x = 50.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{3, 0}, {.x = 200.0F, .y = 0.0F}, {});  // excluded

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      CHECK_EQ(query.CountIf([](const Position& pos) { return pos.x > 10.0F; }),
               1);
    }

    SUBCASE("Without<T> is respected by Find") {
      ComponentManager mgr;
      AddPos(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F});
      AddPosVel(mgr, Entity{2, 0}, {.x = 999.0F, .y = 0.0F}, {});  // excluded

      const auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      // The 999.0 entity is excluded; Find over x > 100 should yield nullopt
      CHECK_FALSE(query.Find([](const Position& pos) { return pos.x > 100.0F; })
                      .has_value());
    }

    SUBCASE("Without<T> combined with WithEntity respects filter") {
      ComponentManager mgr;
      constexpr Entity e1{10, 0};
      AddPos(mgr, e1, {});
      AddPosVel(mgr, Entity{11, 0}, {}, {});

      auto query = MakeQuery<const Position&, Without<Velocity>>(mgr);
      const auto we = query.WithEntity();

      std::vector<Entity> entities;
      we.ForEach([&entities](Entity entity, const Position& /*pos*/) {
        entities.push_back(entity);
      });

      REQUIRE_EQ(entities.size(), 1);
      CHECK_EQ(entities[0], e1);
    }
  }

  TEST_CASE("ecs::BasicQuery - With and Without combined") {
    SUBCASE("With<T> and Without<U> applied together") {
      ComponentManager mgr;
      // Entity 1: Position only — excluded (no Velocity)
      AddPos(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F});
      // Entity 2: Position + Velocity — included (has Velocity, no Health)
      AddPosVel(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {});
      // Entity 3: Position + Velocity + Health — excluded (has Health)
      AddPosVelHealth(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F}, {}, {});

      const auto query =
          MakeQuery<const Position&, With<Velocity>, Without<Health>>(mgr);
      CHECK_EQ(query.Count(), 1);

      float sum = 0.0F;
      query.ForEach([&sum](const Position& pos) { sum += pos.x; });
      CHECK_EQ(sum, doctest::Approx(2.0F));
    }

    SUBCASE("With<T> and Without<U> where nothing satisfies both is empty") {
      ComponentManager mgr;
      // Only entity with Velocity also has Health
      AddPosVelHealth(mgr, Entity{1, 0}, {}, {}, {});

      const auto query =
          MakeQuery<const Position&, With<Velocity>, Without<Health>>(mgr);
      CHECK(query.Empty());
    }

    SUBCASE("Multiple With and Without tags are all respected") {
      ComponentManager mgr;
      // Entity 1: Position + Velocity — excluded (no Health, so With<Health>
      // fails)
      AddPosVel(mgr, Entity{1, 0}, {.x = 1.0F, .y = 0.0F}, {});
      // Entity 2: Position + Velocity + Health — included
      AddPosVelHealth(mgr, Entity{2, 0}, {.x = 2.0F, .y = 0.0F}, {}, {});
      // Entity 3: Position + Velocity + Health + Enemy (sparse) — excluded
      AddPosVelHealth(mgr, Entity{3, 0}, {.x = 3.0F, .y = 0.0F}, {}, {});
      AddSparseFlag(mgr, Entity{3, 0}, {});

      const auto query = MakeQuery<const Position&, With<Velocity, Health>,
                                   Without<SparseFlag>>(mgr);
      CHECK_EQ(query.Count(), 1);

      float sum = 0.0F;
      query.ForEach([&sum](const Position& pos) { sum += pos.x; });
      CHECK_EQ(sum, doctest::Approx(2.0F));
    }

    SUBCASE("With and Without combined with Fold") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 10.0F, .y = 0.0F}, {});
      AddPosVel(mgr, Entity{2, 0}, {.x = 20.0F, .y = 0.0F}, {});
      AddPosVelHealth(mgr, Entity{3, 0}, {.x = 100.0F, .y = 0.0F}, {},
                      {});  // excluded by Without<Health>

      const auto query =
          MakeQuery<const Position&, With<Velocity>, Without<Health>>(mgr);
      const float sum = query.Fold(
          0.0F, [](float acc, const Position& pos) { return acc + pos.x; });
      CHECK_EQ(sum, doctest::Approx(30.0F));
    }

    SUBCASE("With and Without combined with Partition") {
      ComponentManager mgr;
      AddPosVel(mgr, Entity{1, 0}, {.x = 5.0F, .y = 0.0F}, {});
      AddPosVel(mgr, Entity{2, 0}, {.x = 50.0F, .y = 0.0F}, {});
      AddPosVelHealth(mgr, Entity{3, 0}, {.x = 500.0F, .y = 0.0F}, {}, {});

      const auto query =
          MakeQuery<const Position&, With<Velocity>, Without<Health>>(mgr);
      const auto [yes, no] =
          query.Partition([](const Position& pos) { return pos.x > 10.0F; });
      CHECK_EQ(yes.size(), 1);
      CHECK_EQ(no.size(), 1);
    }

    SUBCASE("With and Without combined with WithEntity iteration") {
      ComponentManager mgr;
      constexpr Entity included{5, 0};
      constexpr Entity excluded_no_vel{6, 0};
      constexpr Entity excluded_has_health{7, 0};

      AddPosVel(mgr, included, {.x = 1.0F, .y = 0.0F}, {});
      AddPos(mgr, excluded_no_vel, {.x = 2.0F, .y = 0.0F});
      AddPosVelHealth(mgr, excluded_has_health, {.x = 3.0F, .y = 0.0F}, {}, {});

      auto query =
          MakeQuery<const Position&, With<Velocity>, Without<Health>>(mgr);
      const auto we = query.WithEntity();

      std::vector<Entity> seen;
      we.ForEach([&seen](Entity entity, const Position& /*pos*/) {
        seen.push_back(entity);
      });

      REQUIRE_EQ(seen.size(), 1);
      CHECK_EQ(seen[0], included);
    }
  }
}
