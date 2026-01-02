#include <doctest/doctest.h>

#include <helios/core/ecs/query.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

using namespace helios::ecs;

namespace {

// Test component types
struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Name {
  std::string value;

  constexpr bool operator==(const Name& other) const noexcept = default;
};

struct Health {
  int points = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

struct TagComponent {};

}  // namespace

TEST_SUITE("ecs::Query") {
  TEST_CASE("QueryBuilder::QueryBuilder: basic construction") {
    World world;
    QueryBuilder builder(world);

    // Should be able to construct without issues
  }

  TEST_CASE("QueryBuilder::With: single component") {
    World world;

    // Create entities with Position
    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});

    // entity3 has no components

    auto query = QueryBuilder(world).With<Position>().Get<Position&>();
    CHECK_FALSE(query.Empty());
    CHECK_EQ(query.Count(), 2);

    std::vector<Position> positions;
    query.ForEach([&positions](Position& pos) { positions.push_back(pos); });

    CHECK_EQ(positions.size(), 2);
    CHECK(std::ranges::find(positions, Position{1.0F, 2.0F, 3.0F}) != positions.end());
    CHECK(std::ranges::find(positions, Position{4.0F, 5.0F, 6.0F}) != positions.end());
  }

  TEST_CASE("QueryBuilder::With: multiple components") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, Velocity{1.0F, 1.0F, 1.0F});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});
    // entity2 missing Velocity

    world.AddComponent(entity3, Velocity{2.0F, 2.0F, 2.0F});
    // entity3 missing Position

    auto query = QueryBuilder(world).With<Position, Velocity>().Get<Position&, Velocity&>();

    CHECK_FALSE(query.Empty());
    CHECK_EQ(query.Count(), 1);  // Only entity1 has both components

    bool found = false;
    query.ForEach([&found](Position& pos, Velocity& vel) {
      CHECK_EQ(pos, Position{1.0F, 2.0F, 3.0F});
      CHECK_EQ(vel, Velocity{1.0F, 1.0F, 1.0F});
      found = true;
    });

    CHECK(found);
  }

  TEST_CASE("QueryBuilder::Without: excludes components") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity2, Velocity{1.0F, 1.0F, 1.0F});

    world.AddComponent(entity3, Velocity{2.0F, 2.0F, 2.0F});

    // Query for entities with Position but without Velocity
    auto query = QueryBuilder(world).With<Position>().Without<Velocity>().Get<Position&>();

    CHECK_FALSE(query.Empty());
    CHECK_EQ(query.Count(), 1);  // Only entity1 matches

    bool found = false;
    query.ForEach([&found](Position& pos) {
      CHECK_EQ(pos, Position{1.0F, 2.0F, 3.0F});
      found = true;
    });

    CHECK(found);
  }

  TEST_CASE("QueryBuilder::With: mixed With and Without") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();
    Entity entity4 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, Health{100});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity2, Health{50});
    world.AddComponent(entity2, Velocity{1.0F, 1.0F, 1.0F});

    world.AddComponent(entity3, Position{7.0F, 8.0F, 9.0F});
    world.AddComponent(entity3, Name{"Entity3"});

    world.AddComponent(entity4, Health{75});
    world.AddComponent(entity4, Name{"Entity4"});

    // Query for entities with Position and Health but without Velocity
    auto query = QueryBuilder(world).With<Position, Health>().Without<Velocity>().Get<Position&, Health&>();

    CHECK_FALSE(query.Empty());
    CHECK_EQ(query.Count(), 1);  // Only entity1 matches

    bool found = false;
    query.ForEach([&found](Position& pos, Health& health) {
      CHECK_EQ(pos, Position{1.0F, 2.0F, 3.0F});
      CHECK_EQ(health, Health{100});
      found = true;
    });

    CHECK(found);
  }

  TEST_CASE("QueryBuilder::Get: empty query") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    // Query for components that don't exist
    auto query = QueryBuilder(world).Get<Velocity&>();

    CHECK(query.Empty());
    CHECK_EQ(query.Count(), 0);

    bool called = false;
    query.ForEach([&called](Velocity&) { called = true; });

    CHECK_FALSE(called);
  }

  TEST_CASE("Query::Get: Const Component Access") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, Velocity{1.0F, 1.0F, 1.0F});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity2, Velocity{2.0F, 2.0F, 2.0F});

    // Query with const access to Position, mutable access to Velocity
    auto query = QueryBuilder(world).Get<const Position&, Velocity&>();

    CHECK_EQ(query.Count(), 2);

    query.ForEach([](const Position& pos, Velocity& vel) {
      // pos is const, vel is mutable
      vel.dx += pos.x;  // This should compile
      // pos.x = 0.0F;  // This should NOT compile (const)
    });
  }

  TEST_CASE("Query::Get: Value Component Access") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position(1.0F, 2.0F, 3.0F));
    world.AddComponent(entity, Health(100));

    // Query with value access (copy)
    auto query = QueryBuilder(world).Get<Position, Health>();

    CHECK_EQ(query.Count(), 1);

    bool found = false;
    query.ForEach([&found](Position pos, Health health) {
      CHECK_EQ(pos, Position(1.0F, 2.0F, 3.0F));
      CHECK_EQ(health, Health(100));

      // Modifying copies should not affect originals
      pos.x = 999.0F;
      health.points = 0;

      found = true;
    });

    CHECK(found);
  }

  TEST_CASE("Query::Get: ForEachWithEntity") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});

    auto query = QueryBuilder(world).Get<Position&>();

    std::vector<Entity> found_entities;
    std::vector<Position> found_positions;

    query.ForEachWithEntity([&found_entities, &found_positions](Entity entity, Position& pos) {
      found_entities.push_back(entity);
      found_positions.push_back(pos);
    });

    CHECK_EQ(found_entities.size(), 2);
    CHECK_EQ(found_positions.size(), 2);

    CHECK_NE(std::ranges::find(found_entities, entity1), found_entities.end());
    CHECK_NE(std::ranges::find(found_entities, entity2), found_entities.end());
    CHECK_NE(std::ranges::find(found_positions, Position{1.0F, 2.0F, 3.0F}), found_positions.end());
    CHECK_NE(std::ranges::find(found_positions, Position{4.0F, 5.0F, 6.0F}), found_positions.end());
  }

  TEST_CASE("Query::Get: WithEntity Iterator") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});

    auto query = QueryBuilder(world).Get<Position&>();

    std::vector<Entity> found_entities;
    std::vector<Position> found_positions;

    for (auto&& result : query.WithEntity()) {
      auto [entity, pos] = result;
      found_entities.push_back(entity);
      found_positions.push_back(pos);
    }

    CHECK_EQ(found_entities.size(), 2);
    CHECK_EQ(found_positions.size(), 2);

    CHECK(std::ranges::find(found_entities, entity1) != found_entities.end());
    CHECK(std::ranges::find(found_entities, entity2) != found_entities.end());
  }

  TEST_CASE("Query::Get: Regular Iterator") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, Velocity{1.0F, 1.0F, 1.0F});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity2, Velocity{2.0F, 2.0F, 2.0F});

    auto query = QueryBuilder(world).Get<Position&, Velocity&>();

    std::vector<Position> positions;
    std::vector<Velocity> velocities;

    for (auto&& result : query) {
      auto [pos, vel] = result;
      positions.push_back(pos);
      velocities.push_back(vel);
    }

    CHECK_EQ(positions.size(), 2);
    CHECK_EQ(velocities.size(), 2);

    CHECK(std::ranges::find(positions, Position(1.0F, 2.0F, 3.0F)) != positions.end());
    CHECK(std::ranges::find(positions, Position(4.0F, 5.0F, 6.0F)) != positions.end());
  }

  TEST_CASE("Query::Get: Tag Components") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();
    Entity entity3 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, TagComponent{});

    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});
    // entity2 doesn't have TagComponent

    world.AddComponent(entity3, TagComponent{});
    // entity3 doesn't have Position

    auto query = QueryBuilder(world).With<TagComponent>().Get<Position&>();

    CHECK_EQ(query.Count(), 1);  // Only entity1 has both

    bool found = false;
    query.ForEach([&found](Position& pos) {
      CHECK_EQ(pos, Position{1.0F, 2.0F, 3.0F});
      found = true;
    });

    CHECK(found);
  }

  TEST_CASE("Query::Get: Dynamic Entity Changes") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});

    auto query = QueryBuilder(world).Get<Position&>();
    CHECK_EQ(query.Count(), 2);

    // Add component to entity2 (this might change archetype)
    world.AddComponent(entity2, Velocity{1.0F, 1.0F, 1.0F});

    // Query should still find both entities
    CHECK_EQ(query.Count(), 2);

    // Remove component from entity1
    world.RemoveComponent<Position>(entity1);

    // Query should now find only entity2
    CHECK_EQ(query.Count(), 1);

    bool found = false;
    query.ForEach([&found](Position& pos) {
      CHECK_EQ(pos, Position{4.0F, 5.0F, 6.0F});
      found = true;
    });

    CHECK(found);
  }

  TEST_CASE("Query::Get: Large Scale") {
    World world;
    constexpr size_t entity_count = 1000;

    std::vector<Entity> entities;
    entities.reserve(entity_count);

    // Create entities with different component combinations
    for (size_t i = 0; i < entity_count; ++i) {
      Entity entity = world.CreateEntity();
      entities.push_back(entity);

      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});

      if (i % 2 == 0) {
        world.AddComponent(entity, Velocity{1.0F, 0.0F, 0.0F});
      }

      if (i % 3 == 0) {
        world.AddComponent(entity, Health{100});
      }

      if (i % 5 == 0) {
        world.AddComponent(entity, Name{std::format("Entity{}", i)});
      }
    }

    // Query for entities with Position and Velocity
    auto position_velocity_query = QueryBuilder(world).With<Position, Velocity>().Get();
    CHECK_EQ(position_velocity_query.Count(), entity_count / 2);  // Every other entity

    // Query for entities with Position, Health, and Name (every 15th entity: lcm(3,5) = 15)
    auto complex_query = QueryBuilder(world).Get<Position&, Health&, Name&>();
    CHECK_EQ(complex_query.Count(), 67);  // 0, 15, 30, ..., 990 = 67 entities

    // Query for entities with Position but without Velocity
    auto position_no_velocity_query = QueryBuilder(world).With<Position>().Without<Velocity>().Get();
    CHECK_EQ(position_no_velocity_query.Count(), entity_count / 2);  // The other half

    // Verify specific values
    size_t count = 0;
    complex_query.ForEach([&count](Position& pos, Health& health, Name& name) {
      size_t index = static_cast<size_t>(pos.x);
      CHECK_EQ(index % 15, 0);  // Should be multiples of 15
      CHECK_EQ(health.points, 100);
      CHECK_EQ(name.value, std::format("Entity{}", index));
      ++count;
    });

    CHECK_EQ(count, 67);  // 0, 15, 30, ..., 990 = 67 entities
  }

  TEST_CASE("Query::Get: Iterator Consistency") {
    World world;

    std::vector<Entity> entities;
    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      entities.push_back(entity);
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query = QueryBuilder(world).Get<Position&>();

    // Multiple iterations should yield same results
    std::vector<float> x_values1;
    std::vector<float> x_values2;

    for (auto&& result : query) {
      auto [pos] = result;
      x_values1.push_back(pos.x);
    }

    for (auto&& result : query) {
      auto [pos] = result;
      x_values2.push_back(pos.x);
    }

    CHECK_EQ(x_values1.size(), 10);
    CHECK_EQ(x_values2.size(), 10);
    CHECK(std::ranges::equal(x_values1, x_values2));
  }

  TEST_CASE("Query::Get: Component Modification Through Query") {
    World world;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    world.AddComponent(entity1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity2, Position{4.0F, 5.0F, 6.0F});

    auto query = QueryBuilder(world).Get<Position&>();

    // Modify components through query
    query.ForEach([](Position& pos) { pos.x += 10.0F; });

    // Verify modifications
    std::vector<float> x_values;
    query.ForEach([&x_values](Position& pos) { x_values.push_back(pos.x); });

    CHECK_EQ(x_values.size(), 2);
    CHECK(std::ranges::find(x_values, 11.0F) != x_values.end());  // 1.0F + 10.0F
    CHECK(std::ranges::find(x_values, 14.0F) != x_values.end());  // 4.0F + 10.0F
  }

  TEST_CASE("Query::Get: Chaining: Collect") {
    World world;

    for (int i = 0; i < 5; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query = QueryBuilder(world).Get<const Position&>();

    auto results = query.Collect();

    CHECK_EQ(results.size(), 5);
  }

  TEST_CASE("Query::Get: Chaining: Take and Collect") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    std::vector<std::tuple<const Health&>> results;
    for (auto&& item : query.Take(5)) {
      results.push_back(item);
    }

    CHECK_EQ(results.size(), 5);
  }

  TEST_CASE("Query::Get: Chaining: Fold") {
    World world;

    for (int i = 1; i <= 5; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    int total = query.Fold(0, [](int sum, const Health& h) { return sum + h.points; });

    CHECK_EQ(total, 150);  // 10 + 20 + 30 + 40 + 50
  }

  TEST_CASE("Query::Get: Chaining: Find") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    auto result = query.FindFirst([](const Health& h) { return h.points == 50; });

    REQUIRE(result.has_value());
    CHECK_EQ(std::get<0>(*result).points, 50);
  }

  TEST_CASE("Query::Get: Chaining: Filter and Collect Last") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i < 5 ? 50 : i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    std::vector<std::tuple<const Health&>> results;
    for (auto&& item : query.Filter([](const Health& h) { return h.points == 50; })) {
      results.push_back(item);
    }

    REQUIRE(!results.empty());
    CHECK_EQ(std::get<0>(results.back()).points, 50);
  }

  TEST_CASE("Query::Get: Chaining: Skip and Take Nth") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    std::vector<std::tuple<const Health&>> results;
    for (auto&& item : query.Skip(3).Take(1)) {
      results.push_back(item);
    }

    REQUIRE(!results.empty());
    CHECK_EQ(std::get<0>(results[0]).points, 30);
  }

  TEST_CASE("Query::Get: Chaining: Any") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    bool has_high = query.Any([](const Health& h) { return h.points >= 80; });
    bool has_negative = query.Any([](const Health& h) { return h.points < 0; });

    CHECK(has_high);
    CHECK_FALSE(has_negative);
  }

  TEST_CASE("Query::Get: Chaining: All") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    bool all_non_negative = query.All([](const Health& h) { return h.points >= 0; });
    bool all_high = query.All([](const Health& h) { return h.points >= 80; });

    CHECK(all_non_negative);
    CHECK_FALSE(all_high);
  }

  TEST_CASE("Query::Get: Chaining: None") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    bool none_negative = query.None([](const Health& h) { return h.points < 0; });
    bool none_positive = query.None([](const Health& h) { return h.points > 0; });

    CHECK(none_negative);
    CHECK_FALSE(none_positive);
  }

  TEST_CASE("Query::Get: Chaining: CountIf") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    size_t high_count = query.CountIf([](const Health& h) { return h.points >= 50; });

    CHECK_EQ(high_count, 5);  // 50, 60, 70, 80, 90
  }

  TEST_CASE("Query::Get: Chaining: Partition") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    auto [high, low] = query.Partition([](const Health& h) { return h.points >= 50; });

    CHECK_EQ(high.size(), 5);
    CHECK_EQ(low.size(), 5);
  }

  TEST_CASE("Query::Get: Chaining: MaxBy") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    auto result = query.MaxBy([](const Health& h) { return h.points; });

    REQUIRE(result.has_value());
    CHECK_EQ(std::get<0>(*result).points, 90);
  }

  TEST_CASE("Query::Get: Chaining: MinBy") {
    World world;

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    auto result = query.MinBy([](const Health& h) { return h.points; });

    REQUIRE(result.has_value());
    CHECK_EQ(std::get<0>(*result).points, 0);
  }

  TEST_CASE("QueryWithEntity::QueryWithEntity: Chaining: CollectEntities") {
    World world;

    std::vector<Entity> created_entities;
    for (int i = 0; i < 5; ++i) {
      Entity entity = world.CreateEntity();
      created_entities.push_back(entity);
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query = QueryBuilder(world).Get<const Position&>();

    auto entities = query.WithEntity().CollectEntities();

    CHECK_EQ(entities.size(), 5);
    for (const auto& entity : entities) {
      CHECK(std::ranges::find(created_entities, entity) != created_entities.end());
    }
  }

  TEST_CASE("QueryWithEntity::QueryWithEntity: Chaining: GroupBy") {
    World world;

    struct Team {
      int id = 0;
    };

    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Team{i % 3});  // 3 teams: 0, 1, 2
    }

    auto query = QueryBuilder(world).Get<const Team&>();

    auto groups = query.WithEntity().GroupBy([](Entity, const Team& t) { return t.id; });

    CHECK_EQ(groups.size(), 3);
    CHECK_EQ(groups[0].size(), 4);  // 0, 3, 6, 9
    CHECK_EQ(groups[1].size(), 3);  // 1, 4, 7
    CHECK_EQ(groups[2].size(), 3);  // 2, 5, 8
  }

  TEST_CASE("Query::Get: Chaining: Complex Pipeline") {
    World world;

    for (int i = 0; i < 20; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
      world.AddComponent(entity, Health{i * 5});
    }

    auto query = QueryBuilder(world).Get<const Position&, const Health&>();

    // Complex chain: Filter -> Take -> Map -> Collect
    std::vector<float> result;
    for (auto&& x : query.Filter([](const Position&, const Health& h) { return h.points >= 25; })
                        .Take(5)
                        .Map([](const Position& p, const Health&) { return p.x; })) {
      result.push_back(x);
    }

    CHECK_EQ(result.size(), 5);
    for (const auto& val : result) {
      CHECK_GE(val, 5.0F);  // Corresponding to health >= 25
    }
  }

  TEST_CASE("Query::Get: Const component access") {
    World world;

    auto e1 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(e1, Velocity{0.1F, 0.2F, 0.3F});

    auto e2 = world.CreateEntity();
    world.AddComponent(e2, Position{4.0F, 5.0F, 6.0F});
    world.AddComponent(e2, Velocity{0.4F, 0.5F, 0.6F});

    // Should be able to query with const components
    auto query = QueryBuilder(world).Get<const Position&, const Velocity&>();

    size_t count = 0;
    query.ForEach([&count](const Position& pos, const Velocity& vel) {
      CHECK_GE(pos.x, 0.0F);
      CHECK_GE(vel.dx, 0.0F);
      ++count;
    });

    CHECK_EQ(count, 2);
  }

  TEST_CASE("Query::Get: Const component single access") {
    World world;

    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query = QueryBuilder(world).Get<const Position&>();

    CHECK_EQ(query.Count(), 5);

    float sum = 0.0F;
    query.ForEach([&sum](const Position& pos) { sum += pos.x; });

    CHECK_EQ(sum, 10.0F);  // 0 + 1 + 2 + 3 + 4
  }

  TEST_CASE("Query::Get: Const components with query adapters") {
    World world;

    for (int i = 0; i < 10; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
      world.AddComponent(e, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Position&, const Health&>();

    // Test Filter with const world
    auto filtered = query.Filter([](const Position& p, const Health& h) { return p.x >= 5.0F && h.points >= 50; });

    size_t count = 0;
    for (auto&& [pos, health] : filtered) {
      CHECK_GE(pos.x, 5.0F);
      CHECK_GE(health.points, 50);
      ++count;
    }

    CHECK_EQ(count, 5);  // Entities 5, 6, 7, 8, 9
  }

  TEST_CASE("QueryBuilder::QueryBuilder: With/Without modifiers with const components") {
    World world;

    auto e1 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 0.0F, 0.0F});
    world.AddComponent(e1, Velocity{0.1F, 0.0F, 0.0F});

    auto e2 = world.CreateEntity();
    world.AddComponent(e2, Position{2.0F, 0.0F, 0.0F});
    // No Velocity

    auto e3 = world.CreateEntity();
    world.AddComponent(e3, Position{3.0F, 0.0F, 0.0F});
    world.AddComponent(e3, Velocity{0.3F, 0.0F, 0.0F});

    // Query: With Position, Velocity
    auto query1 = QueryBuilder(world).With<Position, Velocity>().Get<const Position&>();
    CHECK_EQ(query1.Count(), 2);  // e1 and e3

    // Query: With Position, Without Velocity
    auto query2 = QueryBuilder(world).With<Position>().Without<Velocity>().Get<const Position&>();
    CHECK_EQ(query2.Count(), 1);  // e2 only
  }

  TEST_CASE("QueryBuilder::QueryBuilder: Construction with AccessPolicy") {
    World world;

    auto e = world.CreateEntity();
    world.AddComponent(e, Position{1.0F, 2.0F, 3.0F});

    // Create AccessPolicy (for SystemContext integration)
    helios::app::AccessPolicy policy;
    policy.Query<const Position&>();

    auto query = QueryBuilder(world, policy).Get<const Position&>();

    CHECK_EQ(query.Count(), 1);
  }

  TEST_CASE("Query::Get: Const components with CollectEntities") {
    World world;

    std::vector<Entity> expected_entities;
    for (int i = 0; i < 3; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
      expected_entities.push_back(e);
    }

    auto query = QueryBuilder(world).Get<const Position&>();

    auto entities = query.WithEntity().CollectEntities();

    CHECK_EQ(entities.size(), 3);
    for (auto entity : entities) {
      CHECK(std::ranges::find(expected_entities, entity) != expected_entities.end());
    }
  }

  TEST_CASE("Query::Get: Empty query with const components") {
    World world;

    auto query = QueryBuilder(world).Get<const Position&>();

    CHECK(query.Empty());
    CHECK_EQ(query.Count(), 0);

    // ForEach should not execute
    bool executed = false;
    query.ForEach([&executed](const Position&) { executed = true; });

    CHECK_FALSE(executed);
  }

  TEST_CASE("Query::Get: Const components with Map adapter") {
    World world;

    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i * 2), 0.0F, 0.0F});
    }

    auto query = QueryBuilder(world).Get<const Position&>();

    std::vector<float> x_values;
    for (auto x : query.Map([](const Position& p) { return p.x; })) {
      x_values.push_back(x);
    }

    CHECK_EQ(x_values.size(), 5);
    CHECK_EQ(x_values[0], 0.0F);
    CHECK_EQ(x_values[1], 2.0F);
    CHECK_EQ(x_values[2], 4.0F);
    CHECK_EQ(x_values[3], 6.0F);
    CHECK_EQ(x_values[4], 8.0F);
  }

  TEST_CASE("Query::CollectWith: Custom Allocator") {
    World world;

    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

    auto query = QueryBuilder(world).Get<const Position&>();

    using ValueType = std::tuple<const Position&>;
    using Alloc = helios::memory::STLGrowableAllocator<ValueType, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    auto collected = query.CollectWith(alloc);

    CHECK_EQ(collected.size(), 5);

    // Verify allocator was used
    CHECK_GT(frame_alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("Query::CollectWith: Empty Query") {
    World world;

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

    auto query = QueryBuilder(world).Get<const Position&>();

    using ValueType = std::tuple<const Position&>;
    using Alloc = helios::memory::STLGrowableAllocator<ValueType, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    auto collected = query.CollectWith(alloc);

    CHECK(collected.empty());
  }

  TEST_CASE("QueryWithEntity::CollectWith: Custom Allocator") {
    World world;

    for (int i = 0; i < 3; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

    auto query = QueryBuilder(world).Get<const Position&>();

    using ValueType = std::tuple<Entity, const Position&>;
    using Alloc = helios::memory::STLGrowableAllocator<ValueType, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    auto collected = query.WithEntity().CollectWith(alloc);

    CHECK_EQ(collected.size(), 3);

    // Verify allocator was used
    CHECK_GT(frame_alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("QueryWithEntity::CollectEntitiesWith: Custom Allocator") {
    World world;

    for (int i = 0; i < 4; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

    auto query = QueryBuilder(world).Get<const Position&>();

    using Alloc = helios::memory::STLGrowableAllocator<Entity, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    auto entities = query.WithEntity().CollectEntitiesWith(alloc);

    CHECK_EQ(entities.size(), 4);

    // Verify allocator was used
    CHECK_GT(frame_alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("ReadOnlyQueryBuilder: const World only allows const component access") {
    World world;

    auto e1 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 2.0F, 3.0F});

    auto e2 = world.CreateEntity();
    world.AddComponent(e2, Position{4.0F, 5.0F, 6.0F});

    // ReadOnlyQueryBuilder with const World should only allow const components
    auto query = ReadOnlyQueryBuilder(std::as_const(world)).Get<const Position&>();

    CHECK_EQ(query.Count(), 2);

    float sum = 0.0F;
    query.ForEach([&sum](const Position& pos) { sum += pos.x; });

    CHECK_EQ(sum, 5.0F);  // 1.0 + 4.0
  }

  TEST_CASE("ReadOnlyQueryBuilder: supports With and Without") {
    World world;

    auto e1 = world.CreateEntity();
    world.AddComponent(e1, Position{1.0F, 0.0F, 0.0F});

    auto e2 = world.CreateEntity();
    world.AddComponent(e2, Position{2.0F, 0.0F, 0.0F});
    world.AddComponent(e2, Velocity{0.1F, 0.0F, 0.0F});

    auto e3 = world.CreateEntity();
    world.AddComponent(e3, Position{3.0F, 0.0F, 0.0F});
    world.AddComponent(e3, Velocity{0.2F, 0.0F, 0.0F});

    // Query with Position but without Velocity
    auto query = ReadOnlyQueryBuilder(std::as_const(world)).With<Position>().Without<Velocity>().Get<const Position&>();

    CHECK_EQ(query.Count(), 1);

    query.ForEach([](const Position& pos) { CHECK_EQ(pos.x, 1.0F); });
  }

  TEST_CASE("QueryBuilder: custom allocator support") {
    World world;

    for (int i = 0; i < 5; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

    using Alloc = helios::memory::STLGrowableAllocator<ComponentTypeId, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    // Create QueryBuilder with custom allocator
    auto builder = QueryBuilder(world, alloc);
    auto query = builder.Get<const Position&>();

    CHECK_EQ(query.Count(), 5);

    // Verify allocator was used by QueryBuilder
    CHECK_GT(frame_alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("Query: move semantics work correctly") {
    World world;

    for (int i = 0; i < 3; ++i) {
      auto e = world.CreateEntity();
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query1 = QueryBuilder(world).Get<const Position&>();

    // Move query
    auto query2 = std::move(query1);

    CHECK_EQ(query2.Count(), 3);
  }

  TEST_CASE("ReadOnlyQueryBuilder: iterators work correctly") {
    World world;

    std::vector<Entity> created_entities;
    for (int i = 0; i < 5; ++i) {
      auto entity = world.CreateEntity();
      created_entities.push_back(entity);
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query = ReadOnlyQueryBuilder(std::as_const(world)).Get<const Position&>();

    // Test iteration
    std::vector<float> x_values;
    for (auto&& [pos] : query) {
      x_values.push_back(pos.x);
    }

    CHECK_EQ(x_values.size(), 5);
  }

  TEST_CASE("ReadOnlyQueryBuilder: functional adapters work") {
    World world;

    for (int i = 0; i < 10; ++i) {
      auto entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = ReadOnlyQueryBuilder(std::as_const(world)).Get<const Health&>();

    // Test Filter
    size_t high_count = 0;
    for (auto&& _ : query.Filter([](const Health& h) { return h.points >= 50; })) {
      ++high_count;
    }
    CHECK_EQ(high_count, 5);  // 50, 60, 70, 80, 90

    // Test Fold
    int total = query.Fold(0, [](int sum, const Health& h) { return sum + h.points; });
    CHECK_EQ(total, 450);  // 0 + 10 + 20 + ... + 90
  }

  TEST_CASE("QueryBuilder with AccessPolicy: custom allocator") {
    World world;

    for (int i = 0; i < 3; ++i) {
      auto entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    // Create access policy
    helios::app::AccessPolicy policy;
    policy.Query<const Position&>();

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);

    using Alloc = helios::memory::STLGrowableAllocator<ComponentTypeId, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    // Create QueryBuilder with policy and custom allocator
    auto builder = QueryBuilder(world, policy, alloc);
    auto query = builder.Get<const Position&>();

    CHECK_EQ(query.Count(), 3);
    CHECK_GT(frame_alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("Query::WithEntity: works with const World") {
    World world;

    std::vector<Entity> created;
    for (int i = 0; i < 3; ++i) {
      auto e = world.CreateEntity();
      created.push_back(e);
      world.AddComponent(e, Position{static_cast<float>(i), 0.0F, 0.0F});
    }

    auto query = ReadOnlyQueryBuilder(std::as_const(world)).Get<const Position&>();

    auto entities = query.WithEntity().CollectEntities();

    CHECK_EQ(entities.size(), 3);
    for (auto entity : entities) {
      CHECK(std::ranges::find(created, entity) != created.end());
    }
  }

}  // TEST_SUITE
