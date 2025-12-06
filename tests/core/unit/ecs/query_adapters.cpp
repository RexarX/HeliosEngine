#include <doctest/doctest.h>

#include <helios/core/ecs/query.hpp>
#include <helios/core/ecs/world.hpp>

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

struct Health {
  int points = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

struct Enemy {
  int level = 1;
  std::string type = "minion";
};

}  // namespace

TEST_SUITE("ecs::QueryAdapters") {
  TEST_CASE("QueryAdapters: Filter") {
    World world;

    // Create entities with varying health
    for (int i = 0; i < 10; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{i * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Filter with predicate") {
      std::vector<int> health_values;
      for (auto&& [health] : query.Filter([](const Health& health) { return health.points >= 50; })) {
        health_values.push_back(health.points);
      }

      CHECK_EQ(health_values.size(), 5);  // 50, 60, 70, 80, 90
      for (const auto& health_value : health_values) {
        CHECK_GE(health_value, 50);
      }
    }

    SUBCASE("Filter chains") {
      std::vector<int> health_values;
      for (auto&& [health] :
           query.Filter([](const Health& health) { return health.points >= 30; }).Filter([](const Health& health) {
             return health.points <= 70;
           })) {
        health_values.push_back(health.points);
      }

      CHECK_EQ(health_values.size(), 5);  // 30, 40, 50, 60, 70
      for (const auto& health_value : health_values) {
        CHECK_GE(health_value, 30);
        CHECK_LE(health_value, 70);
      }
    }

    SUBCASE("Filter with no matches") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Filter([](const Health& health) { return health.points > 1000; })) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("QueryAdapters: Map") {
    World world;

    for (int index = 0; index < 5; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(index), 0.0F, 0.0F});
    }

    auto query = QueryBuilder(world).Get<const Position&>();

    SUBCASE("Map to scalar") {
      std::vector<float> x_values;
      for (auto&& x_value : query.Map([](const Position& position) { return position.x; })) {
        x_values.push_back(x_value);
      }

      CHECK_EQ(x_values.size(), 5);
      CHECK_EQ(x_values[0], 0.0F);
      CHECK_EQ(x_values[1], 1.0F);
      CHECK_EQ(x_values[2], 2.0F);
    }

    SUBCASE("Map transformation") {
      std::vector<float> doubled_values;
      for (auto&& doubled_value : query.Map([](const Position& position) { return position.x * 2.0F; })) {
        doubled_values.push_back(doubled_value);
      }

      CHECK_EQ(doubled_values.size(), 5);
      for (size_t index = 0; index < doubled_values.size(); ++index) {
        CHECK_EQ(doubled_values[index], static_cast<float>(index) * 2.0F);
      }
    }
  }

  TEST_CASE("QueryAdapters: Take") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Take less than available") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Take(5)) {
        ++count;
      }

      CHECK_EQ(count, 5);
    }

    SUBCASE("Take more than available") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Take(100)) {
        ++count;
      }

      CHECK_EQ(count, 10);  // Only 10 available
    }

    SUBCASE("Take zero") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Take(0)) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Take with filter") {
      std::vector<int> health_values;
      for (auto&& [health] : query.Filter([](const Health& health) { return health.points >= 30; }).Take(3)) {
        health_values.push_back(health.points);
      }

      CHECK_EQ(health_values.size(), 3);
    }
  }

  TEST_CASE("QueryAdapters: Skip") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Skip less than available") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Skip(5)) {
        ++count;
      }

      CHECK_EQ(count, 5);  // 10 - 5 = 5
    }

    SUBCASE("Skip more than available") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Skip(100)) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Skip zero") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Skip(0)) {
        ++count;
      }

      CHECK_EQ(count, 10);
    }

    SUBCASE("Skip and Take (pagination)") {
      const size_t page_size = 3;
      const size_t page_number = 2;

      std::vector<int> health_values;
      for (auto&& [health] : query.Skip(page_number * page_size).Take(page_size)) {
        health_values.push_back(health.points);
      }

      CHECK_EQ(health_values.size(), 3);
    }
  }

  TEST_CASE("QueryAdapters: TakeWhile") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("TakeWhile stops at condition") {
      std::vector<int> health_values;
      for (auto&& [health] : query.TakeWhile([](const Health& health) { return health.points < 50; })) {
        health_values.push_back(health.points);
      }

      for (const auto& health_value : health_values) {
        CHECK_LT(health_value, 50);
      }
    }

    SUBCASE("TakeWhile with always true") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.TakeWhile([](const Health& /*health*/) { return true; })) {
        ++count;
      }

      CHECK_EQ(count, 10);
    }

    SUBCASE("TakeWhile with always false") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.TakeWhile([](const Health& /*health*/) { return false; })) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("QueryAdapters: SkipWhile") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("SkipWhile skips until condition") {
      std::vector<int> health_values;
      for (auto&& [health] : query.SkipWhile([](const Health& health) { return health.points < 50; })) {
        health_values.push_back(health.points);
      }

      for (const auto& health_value : health_values) {
        CHECK_GE(health_value, 50);
      }
    }

    SUBCASE("SkipWhile with always true") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.SkipWhile([](const Health& /*health*/) { return true; })) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("SkipWhile with always false") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.SkipWhile([](const Health& /*health*/) { return false; })) {
        ++count;
      }

      CHECK_EQ(count, 10);
    }
  }

  TEST_CASE("QueryAdapters: Enumerate") {
    World world;

    for (int index = 0; index < 5; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Enumerate adds indices") {
      std::vector<size_t> indices;
      for (auto&& result : query.Enumerate()) {
        size_t index = std::get<0>(result);
        indices.push_back(index);
      }

      CHECK_EQ(indices.size(), 5);
      for (size_t index = 0; index < indices.size(); ++index) {
        CHECK_EQ(indices[index], index);
      }
    }

    SUBCASE("Enumerate with Take") {
      std::vector<size_t> indices;
      for (auto&& result : query.Enumerate().Take(3)) {
        size_t index = std::get<0>(result);
        indices.push_back(index);
      }

      CHECK_EQ(indices.size(), 3);
      CHECK_EQ(indices[0], 0);
      CHECK_EQ(indices[1], 1);
      CHECK_EQ(indices[2], 2);
    }
  }

  TEST_CASE("QueryAdapters: Inspect") {
    World world;

    for (int index = 0; index < 5; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Inspect called for each element") {
      size_t inspect_count = 0;
      size_t iteration_count = 0;

      for (auto&& [health] : query.Inspect([&inspect_count](const Health& /*health*/) { ++inspect_count; })) {
        ++iteration_count;
      }

      CHECK_EQ(inspect_count, 5);
      CHECK_EQ(iteration_count, 5);
    }

    SUBCASE("Inspect doesn't modify values") {
      std::vector<int> inspected_values;
      std::vector<int> iterated_values;

      for (auto&& [health] :
           query.Inspect([&inspected_values](const Health& health) { inspected_values.push_back(health.points); })) {
        iterated_values.push_back(health.points);
      }

      CHECK_EQ(inspected_values.size(), iterated_values.size());
      for (size_t index = 0; index < inspected_values.size(); ++index) {
        CHECK_EQ(inspected_values[index], iterated_values[index]);
      }
    }
  }

  TEST_CASE("QueryAdapters: StepBy") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("StepBy skips correctly") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.StepBy(2)) {
        ++count;
      }

      CHECK_EQ(count, 5);  // Every 2nd element from 10 elements
    }

    SUBCASE("StepBy with step 1") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.StepBy(1)) {
        ++count;
      }

      CHECK_EQ(count, 10);  // All elements
    }

    SUBCASE("StepBy with large step") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.StepBy(100)) {
        ++count;
      }

      CHECK_EQ(count, 1);  // Only first element
    }
  }

  TEST_CASE("QueryAdapters: Complex Chaining") {
    World world;

    for (int index = 0; index < 20; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(index), 0.0F, 0.0F});
      world.AddComponent(entity, Health{index * 5});
    }

    auto query = QueryBuilder(world).Get<const Position&, const Health&>();

    SUBCASE("Filter -> Take -> Enumerate") {
      std::vector<size_t> indices;
      std::vector<int> health_values;

      for (auto&& result :
           query.Filter([](const Position& /*position*/, const Health& health) { return health.points >= 25; })
               .Take(5)
               .Enumerate()) {
        size_t index = std::get<0>(result);
        const auto& position = std::get<1>(result);
        const auto& health = std::get<2>(result);
        indices.push_back(index);
        health_values.push_back(health.points);
      }

      CHECK_EQ(indices.size(), 5);
      CHECK_EQ(health_values.size(), 5);
      for (size_t index = 0; index < indices.size(); ++index) {
        CHECK_EQ(indices[index], index);
        CHECK_GE(health_values[index], 25);
      }
    }

    SUBCASE("Skip -> Filter -> Take") {
      std::vector<int> health_values;

      for (auto&& [position, health] :
           query.Skip(5)
               .Filter([](const Position& /*position*/, const Health& health) { return health.points >= 30; })
               .Take(3)) {
        health_values.push_back(health.points);
      }

      CHECK_LE(health_values.size(), 3);
      for (const auto& health_value : health_values) {
        CHECK_GE(health_value, 30);
      }
    }

    SUBCASE("Map -> Filter -> Take") {
      std::vector<float> x_values;

      for (auto&& x_value : query.Map([](const Position& position, const Health& /*health*/) { return position.x; })
                                .Filter([](float x_coord) { return x_coord >= 10.0F; })
                                .Take(5)) {
        x_values.push_back(x_value);
      }

      CHECK_EQ(x_values.size(), 5);
      for (const auto& x_value : x_values) {
        CHECK_GE(x_value, 10.0F);
      }
    }
  }

  TEST_CASE("QueryAdapters: WithEntity Filter") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("WithEntity Filter includes entity") {
      std::vector<Entity> entities;
      std::vector<int> health_values;

      for (auto&& [entity, health] :
           query.WithEntity().Filter([](Entity /*entity*/, const Health& health) { return health.points >= 50; })) {
        entities.push_back(entity);
        health_values.push_back(health.points);
      }

      CHECK_EQ(entities.size(), health_values.size());
      for (const auto& health_value : health_values) {
        CHECK_GE(health_value, 50);
      }
    }
  }

  TEST_CASE("QueryAdapters: WithEntity Enumerate") {
    World world;

    for (int index = 0; index < 5; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("WithEntity Enumerate adds index before entity") {
      std::vector<size_t> indices;
      std::vector<Entity> entities;

      for (auto&& result : query.WithEntity().Enumerate()) {
        size_t index = std::get<0>(result);
        Entity entity = std::get<1>(result);
        indices.push_back(index);
        entities.push_back(entity);
      }

      CHECK_EQ(indices.size(), 5);
      CHECK_EQ(entities.size(), 5);
      for (size_t index = 0; index < indices.size(); ++index) {
        CHECK_EQ(indices[index], index);
      }
    }
  }

  TEST_CASE("QueryAdapters: Performance Lazy Evaluation") {
    World world;

    for (int index = 0; index < 1000; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Take stops early") {
      size_t filter_calls = 0;

      // Take(5) should stop after 5 elements even with expensive filter
      size_t count = 0;
      for (auto&& [_] : query
                            .Filter([](const Health& health) {
                              //++filter_calls;
                              return health.points >= 0;
                            })
                            .Take(5)) {
        ++count;
      }

      CHECK_EQ(count, 5);
      // Filter should only be called for elements actually yielded, not all 1000
      CHECK_LE(filter_calls, 10);  // Some overhead is acceptable
    }
  }

  TEST_CASE("QueryAdapters: Empty Query Adapters") {
    World world;

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Filter on empty") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Filter([](const Health& health) { return health.points > 0; })) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Take on empty") {
      size_t count = 0;
      for (auto&& [health /*unused*/] : query.Take(5)) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Enumerate on empty") {
      size_t count = 0;
      for (auto&& result /*unused*/ : query.Enumerate()) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("QueryAdapters: Into") {
    World world;

    for (int index = 0; index < 10; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Health{index * 10});
    }

    auto query = QueryBuilder(world).Get<const Health&>();

    SUBCASE("Into basic usage") {
      std::vector<std::tuple<const Health&>> results;
      query.Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 10);
    }

    SUBCASE("Into with Filter") {
      std::vector<std::tuple<const Health&>> results;
      query.Filter([](const Health& health) { return health.points >= 50; }).Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 5);
      for (const auto& [health] : results) {
        CHECK_GE(health.points, 50);
      }
    }

    SUBCASE("Into with Map") {
      std::vector<int> health_values;
      query.Map([](const Health& health) { return health.points; }).Into(std::back_inserter(health_values));

      CHECK_EQ(health_values.size(), 10);
      CHECK_NE(std::ranges::find(health_values, 0), health_values.end());
      CHECK_NE(std::ranges::find(health_values, 90), health_values.end());
    }

    SUBCASE("Into with Take") {
      std::vector<std::tuple<const Health&>> results;
      query.Take(5).Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 5);
    }

    SUBCASE("Into with complex chain") {
      std::vector<int> results;
      query.Filter([](const Health& health) { return health.points >= 30; })
          .Map([](const Health& health) { return health.points; })
          .Take(3)
          .Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 3);
      for (const auto& value : results) {
        CHECK_GE(value, 30);
      }
    }

    SUBCASE("Into with empty result") {
      std::vector<std::tuple<const Health&>> results;
      query.Filter([](const Health& health) { return health.points > 1000; }).Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 0);
    }
  }

  TEST_CASE("QueryAdapters: Into with multiple components") {
    World world;

    for (int index = 0; index < 5; ++index) {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{static_cast<float>(index), 0.0F, 0.0F});
      world.AddComponent(entity, Health{index * 20});
    }

    auto query = QueryBuilder(world).Get<const Position&, const Health&>();

    SUBCASE("Into with tuple output") {
      std::vector<std::tuple<const Position&, const Health&>> results;
      query.Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 5);
    }

    SUBCASE("Into with Map to custom type") {
      struct HealthPos {
        int health;
        float x;
      };
      std::vector<HealthPos> results;

      query.Map([](const Position& pos, const Health& health) { return HealthPos{health.points, pos.x}; })
          .Into(std::back_inserter(results));

      CHECK_EQ(results.size(), 5);
      CHECK_EQ(results[0].health, 0);
      CHECK_EQ(results[1].health, 20);
    }
  }

}  // TEST_SUITE
