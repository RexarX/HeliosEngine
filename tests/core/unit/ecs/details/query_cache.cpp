#include <doctest/doctest.h>

#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/archetype.hpp>
#include <helios/core/ecs/details/query_cache.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Helper function to create archetype and return reference wrapper
auto CreateArchetype(std::vector<ComponentTypeId> component_types) -> std::unique_ptr<Archetype> {
  return std::make_unique<Archetype>(std::move(component_types));
}

}  // namespace

TEST_SUITE("ecs::details::QueryCache") {
  TEST_CASE("QueryCacheStats::QueryCacheStats: default construction") {
    QueryCacheStats stats;

    CHECK_EQ(stats.hit_count.load(), 0);
    CHECK_EQ(stats.miss_count.load(), 0);
    CHECK_EQ(stats.invalidation_count.load(), 0);
    CHECK_EQ(stats.total_queries.load(), 0);
    CHECK_EQ(stats.archetype_changes.load(), 0);
    CHECK_EQ(stats.partial_invalidations.load(), 0);
    CHECK_EQ(stats.HitRate(), 0.0);
  }

  TEST_CASE("QueryCacheStats::QueryCacheStats: copy constructor") {
    QueryCacheStats stats1;
    stats1.hit_count.store(10);
    stats1.miss_count.store(5);
    stats1.invalidation_count.store(2);
    stats1.total_queries.store(15);
    stats1.archetype_changes.store(3);
    stats1.partial_invalidations.store(1);

    QueryCacheStats stats2(stats1);

    CHECK_EQ(stats2.hit_count.load(), 10);
    CHECK_EQ(stats2.miss_count.load(), 5);
    CHECK_EQ(stats2.invalidation_count.load(), 2);
    CHECK_EQ(stats2.total_queries.load(), 15);
    CHECK_EQ(stats2.archetype_changes.load(), 3);
    CHECK_EQ(stats2.partial_invalidations.load(), 1);
  }

  TEST_CASE("QueryCacheStats::operator=: copy assignment") {
    QueryCacheStats stats1;
    stats1.hit_count.store(20);
    stats1.miss_count.store(10);
    stats1.invalidation_count.store(3);
    stats1.total_queries.store(30);
    stats1.archetype_changes.store(5);
    stats1.partial_invalidations.store(2);

    QueryCacheStats stats2;
    stats2 = stats1;

    CHECK_EQ(stats2.hit_count.load(), 20);
    CHECK_EQ(stats2.miss_count.load(), 10);
    CHECK_EQ(stats2.invalidation_count.load(), 3);
    CHECK_EQ(stats2.total_queries.load(), 30);
    CHECK_EQ(stats2.archetype_changes.load(), 5);
    CHECK_EQ(stats2.partial_invalidations.load(), 2);
  }

  TEST_CASE("QueryCacheStats::operator=: self assignment") {
    QueryCacheStats stats;
    stats.hit_count.store(15);
    stats.miss_count.store(8);

    stats = stats;

    CHECK_EQ(stats.hit_count.load(), 15);
    CHECK_EQ(stats.miss_count.load(), 8);
  }

  TEST_CASE("QueryCacheStats::Reset") {
    QueryCacheStats stats;
    stats.hit_count.store(100);
    stats.miss_count.store(50);
    stats.invalidation_count.store(10);
    stats.total_queries.store(150);
    stats.archetype_changes.store(20);
    stats.partial_invalidations.store(5);

    stats.Reset();

    CHECK_EQ(stats.hit_count.load(), 0);
    CHECK_EQ(stats.miss_count.load(), 0);
    CHECK_EQ(stats.invalidation_count.load(), 0);
    CHECK_EQ(stats.total_queries.load(), 0);
    CHECK_EQ(stats.archetype_changes.load(), 0);
    CHECK_EQ(stats.partial_invalidations.load(), 0);
    CHECK_EQ(stats.HitRate(), 0.0);
  }

  TEST_CASE("QueryCacheStats::HitRate") {
    QueryCacheStats stats;

    SUBCASE("Zero queries returns 0.0") {
      CHECK_EQ(stats.HitRate(), 0.0);
    }

    SUBCASE("All hits") {
      stats.hit_count.store(10);
      stats.total_queries.store(10);
      CHECK_EQ(stats.HitRate(), 1.0);
    }

    SUBCASE("All misses") {
      stats.miss_count.store(10);
      stats.total_queries.store(10);
      CHECK_EQ(stats.HitRate(), 0.0);
    }

    SUBCASE("50% hit rate") {
      stats.hit_count.store(50);
      stats.miss_count.store(50);
      stats.total_queries.store(100);
      CHECK_EQ(stats.HitRate(), 0.5);
    }

    SUBCASE("75% hit rate") {
      stats.hit_count.store(75);
      stats.miss_count.store(25);
      stats.total_queries.store(100);
      CHECK_EQ(stats.HitRate(), 0.75);
    }

    SUBCASE("Fractional hit rate") {
      stats.hit_count.store(7);
      stats.miss_count.store(3);
      stats.total_queries.store(10);
      CHECK_EQ(stats.HitRate(), doctest::Approx(0.7));
    }
  }

  TEST_CASE("QueryCacheManager::QueryCacheManager: default construction") {
    QueryCacheManager cache;

    CHECK_EQ(cache.CacheSize(), 0);
    CHECK_EQ(cache.Stats().hit_count.load(), 0);
    CHECK_EQ(cache.Stats().miss_count.load(), 0);
  }

  TEST_CASE("QueryCacheManager::Clear") {
    QueryCacheManager cache;

    // Store some cache entries
    std::vector<ComponentTypeId> with_components = {100, 200};
    std::vector<ComponentTypeId> without_components = {};
    auto archetype = CreateArchetype({100, 200});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    cache.StoreCache(with_components, without_components, archetypes, 1);
    CHECK_GT(cache.CacheSize(), 0);

    cache.Clear();

    CHECK_EQ(cache.CacheSize(), 0);
    CHECK_EQ(cache.Stats().hit_count.load(), 0);
    CHECK_EQ(cache.Stats().miss_count.load(), 0);
  }

  TEST_CASE("QueryCacheManager::ResetStats") {
    QueryCacheManager cache;

    // Perform some queries to generate stats
    std::vector<ComponentTypeId> with_components = {100};
    std::vector<ComponentTypeId> without_components = {};
    const auto* result = cache.TryGetCache(with_components, without_components, 1);
    CHECK_EQ(result, nullptr);
    CHECK_GT(cache.Stats().total_queries.load(), 0);

    cache.ResetStats();

    CHECK_EQ(cache.Stats().hit_count.load(), 0);
    CHECK_EQ(cache.Stats().miss_count.load(), 0);
    CHECK_EQ(cache.Stats().total_queries.load(), 0);
    CHECK_EQ(cache.Stats().invalidation_count.load(), 0);
    CHECK_EQ(cache.Stats().archetype_changes.load(), 0);
    CHECK_EQ(cache.Stats().partial_invalidations.load(), 0);
  }

  TEST_CASE("QueryCacheManager: StoreCache and TryGetCache") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100, 200};
    std::vector<ComponentTypeId> without_components = {300};
    auto archetype1 = CreateArchetype({100, 200});
    auto archetype2 = CreateArchetype({100, 200, 400});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype1), std::cref(*archetype2)};

    SUBCASE("Store and retrieve cache entry") {
      cache.StoreCache(with_components, without_components, archetypes, 1);
      CHECK_GT(cache.CacheSize(), 0);

      const auto* result = cache.TryGetCache(with_components, without_components, 1);
      REQUIRE(result != nullptr);
      CHECK_EQ(result->matching_archetypes.size(), 2);
      CHECK_EQ(result->query_generation, 1);
      CHECK_EQ(result->with_component_types, with_components);
      CHECK_EQ(result->without_component_types, without_components);
    }

    SUBCASE("Cache miss on different query") {
      cache.StoreCache(with_components, without_components, archetypes, 1);

      std::vector<ComponentTypeId> different_with = {100, 300};
      const auto* result = cache.TryGetCache(different_with, without_components, 1);
      CHECK_EQ(result, nullptr);
    }

    SUBCASE("Cache miss on stale generation") {
      cache.StoreCache(with_components, without_components, archetypes, 1);

      const auto* result = cache.TryGetCache(with_components, without_components, 2);
      CHECK_EQ(result, nullptr);
    }

    SUBCASE("Cache hit increments hit count") {
      cache.StoreCache(with_components, without_components, archetypes, 1);

      size_t initial_hits = cache.Stats().hit_count.load();
      [[maybe_unused]] const auto* cache_ptr = cache.TryGetCache(with_components, without_components, 1);
      CHECK_EQ(cache.Stats().hit_count.load(), initial_hits + 1);
    }

    SUBCASE("Cache miss increments miss count") {
      size_t initial_misses = cache.Stats().miss_count.load();
      [[maybe_unused]] const auto* cache_ptr = cache.TryGetCache(with_components, without_components, 1);
      CHECK_EQ(cache.Stats().miss_count.load(), initial_misses + 1);
    }
  }

  TEST_CASE("QueryCacheManager: multiple cache entries") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with1 = {100};
    std::vector<ComponentTypeId> with2 = {200};
    std::vector<ComponentTypeId> with3 = {100, 200};

    auto archetype1 = CreateArchetype({100});
    auto archetype2 = CreateArchetype({200});
    auto archetype3 = CreateArchetype({100, 200});
    std::vector<std::reference_wrapper<const Archetype>> archetypes1 = {std::cref(*archetype1)};
    std::vector<std::reference_wrapper<const Archetype>> archetypes2 = {std::cref(*archetype2)};
    std::vector<std::reference_wrapper<const Archetype>> archetypes3 = {std::cref(*archetype3)};

    cache.StoreCache(with1, {}, archetypes1, 1);
    cache.StoreCache(with2, {}, archetypes2, 1);
    cache.StoreCache(with3, {}, archetypes3, 1);

    CHECK_GE(cache.CacheSize(), 3);

    // Verify each entry can be retrieved
    const auto* result1 = cache.TryGetCache(with1, {}, 1);
    const auto* result2 = cache.TryGetCache(with2, {}, 1);
    const auto* result3 = cache.TryGetCache(with3, {}, 1);

    REQUIRE(result1 != nullptr);
    REQUIRE(result2 != nullptr);
    REQUIRE(result3 != nullptr);

    CHECK_EQ(result1->matching_archetypes.size(), 1);
    CHECK_EQ(result2->matching_archetypes.size(), 1);
    CHECK_EQ(result3->matching_archetypes.size(), 1);
  }

  TEST_CASE("QueryCacheManager::InvalidateAll") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100, 200};
    auto archetype = CreateArchetype({100, 200});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    cache.StoreCache(with_components, {}, archetypes, 1);
    CHECK_GT(cache.CacheSize(), 0);

    size_t initial_invalidations = cache.Stats().invalidation_count.load();
    cache.InvalidateAll();

    CHECK_EQ(cache.CacheSize(), 0);
    CHECK_EQ(cache.Stats().invalidation_count.load(), initial_invalidations + 1);

    // Try to retrieve - should be a miss
    const auto* result = cache.TryGetCache(with_components, {}, 1);
    CHECK_EQ(result, nullptr);
  }

  TEST_CASE("QueryCacheManager::InvalidateForComponents") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with1 = {100, 200};
    std::vector<ComponentTypeId> with2 = {300, 400};
    auto archetype1 = CreateArchetype({100, 200});
    auto archetype2 = CreateArchetype({300, 400});
    std::vector<std::reference_wrapper<const Archetype>> archetypes1 = {std::cref(*archetype1)};
    std::vector<std::reference_wrapper<const Archetype>> archetypes2 = {std::cref(*archetype2)};

    cache.StoreCache(with1, {}, archetypes1, 1);
    cache.StoreCache(with2, {}, archetypes2, 1);
    CHECK_GE(cache.CacheSize(), 2);

    // Invalidate only queries involving component 100
    std::vector<ComponentTypeId> components_to_invalidate = {100};
    cache.InvalidateForComponents(components_to_invalidate);

    // First query should be invalidated
    const auto* result1 = cache.TryGetCache(with1, {}, 1);
    CHECK_EQ(result1, nullptr);

    // Second query should still be cached
    const auto* result2 = cache.TryGetCache(with2, {}, 1);
    REQUIRE(result2 != nullptr);
    CHECK_EQ(result2->matching_archetypes.size(), 1);
  }

  TEST_CASE("QueryCacheManager::NotifyArchetypeChange") {
    QueryCacheManager cache;

    size_t initial_changes = cache.Stats().archetype_changes.load();
    cache.NotifyArchetypeChange();
    CHECK_EQ(cache.Stats().archetype_changes.load(), initial_changes + 1);

    cache.NotifyArchetypeChange();
    cache.NotifyArchetypeChange();
    CHECK_EQ(cache.Stats().archetype_changes.load(), initial_changes + 3);
  }

  TEST_CASE("QueryCacheManager: query hash consistency") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with1 = {100, 200};
    std::vector<ComponentTypeId> with2 = {200, 100};  // Different order but should be sorted before hashing

    auto archetype = CreateArchetype({100, 200});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    SUBCASE("Same components in different order should match after sorting") {
      // Note: Components should be sorted by caller before passing to cache
      std::ranges::sort(with1);
      std::ranges::sort(with2);

      cache.StoreCache(with1, {}, archetypes, 1);

      // Should be able to retrieve with same sorted order
      const auto* result = cache.TryGetCache(with2, {}, 1);
      REQUIRE(result != nullptr);
      CHECK_EQ(result->matching_archetypes.size(), 1);
    }

    SUBCASE("Without components affect hash") {
      cache.StoreCache(with1, {}, archetypes, 1);

      std::vector<ComponentTypeId> without = {300};
      const auto* result = cache.TryGetCache(with1, without, 1);
      CHECK_EQ(result, nullptr);  // Different query due to without components
    }
  }

  TEST_CASE("QueryCacheManager: generation tracking") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100};
    auto archetype = CreateArchetype({100});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    SUBCASE("Query cached at generation 1") {
      cache.StoreCache(with_components, {}, archetypes, 1);

      const auto* result = cache.TryGetCache(with_components, {}, 1);
      REQUIRE(result != nullptr);
      CHECK_EQ(result->query_generation, 1);
    }

    SUBCASE("Cache miss at different generation") {
      cache.StoreCache(with_components, {}, archetypes, 1);

      const auto* result = cache.TryGetCache(with_components, {}, 2);
      CHECK_EQ(result, nullptr);
    }

    SUBCASE("Update cache to new generation") {
      cache.StoreCache(with_components, {}, archetypes, 1);
      cache.StoreCache(with_components, {}, archetypes, 2);

      const auto* result1 = cache.TryGetCache(with_components, {}, 1);
      const auto* result2 = cache.TryGetCache(with_components, {}, 2);

      CHECK_EQ(result1, nullptr);  // Old generation invalidated
      REQUIRE(result2 != nullptr);
      CHECK_EQ(result2->query_generation, 2);
    }
  }

  TEST_CASE("QueryCacheManager: empty query components") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> empty_with;
    std::vector<ComponentTypeId> empty_without;
    auto archetype = CreateArchetype({});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    SUBCASE("Cache empty query") {
      cache.StoreCache(empty_with, empty_without, archetypes, 1);

      const auto* result = cache.TryGetCache(empty_with, empty_without, 1);
      REQUIRE(result != nullptr);
      CHECK_EQ(result->matching_archetypes.size(), 1);
    }

    SUBCASE("Empty with but has without") {
      std::vector<ComponentTypeId> without = {100};
      cache.StoreCache(empty_with, without, archetypes, 1);

      const auto* result = cache.TryGetCache(empty_with, without, 1);
      REQUIRE(result != nullptr);
    }
  }

  TEST_CASE("QueryCacheManager: stats tracking") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100};
    auto archetype = CreateArchetype({100});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    SUBCASE("Total queries increments on hit") {
      cache.StoreCache(with_components, {}, archetypes, 1);

      size_t initial_total = cache.Stats().total_queries.load();
      [[maybe_unused]] const auto* cache_ptr = cache.TryGetCache(with_components, {}, 1);
      CHECK_EQ(cache.Stats().total_queries.load(), initial_total + 1);
    }

    SUBCASE("Total queries increments on miss") {
      size_t initial_total = cache.Stats().total_queries.load();
      [[maybe_unused]] const auto* cache_ptr = cache.TryGetCache(with_components, {}, 1);
      CHECK_EQ(cache.Stats().total_queries.load(), initial_total + 1);
    }

    SUBCASE("Multiple queries update stats correctly") {
      cache.StoreCache(with_components, {}, archetypes, 1);

      // 3 hits
      [[maybe_unused]] const auto* cache_ptr1 = cache.TryGetCache(with_components, {}, 1);
      [[maybe_unused]] const auto* cache_ptr2 = cache.TryGetCache(with_components, {}, 1);
      [[maybe_unused]] const auto* cache_ptr3 = cache.TryGetCache(with_components, {}, 1);

      // 2 misses (different queries)
      std::vector<ComponentTypeId> different = {200};
      [[maybe_unused]] const auto* cache_ptr4 = cache.TryGetCache(different, {}, 1);
      [[maybe_unused]] const auto* cache_ptr5 = cache.TryGetCache(different, {}, 1);

      auto stats = cache.Stats();
      CHECK_EQ(stats.hit_count.load(), 3);
      CHECK_EQ(stats.miss_count.load(), 2);
      CHECK_EQ(stats.total_queries.load(), 5);
      CHECK_EQ(stats.HitRate(), doctest::Approx(0.6));
    }
  }

  TEST_CASE("QueryCacheManager: large scale caching") {
    QueryCacheManager cache;

    constexpr size_t num_queries = 100;
    std::vector<std::unique_ptr<Archetype>> all_archetypes;

    // Store many different queries
    for (size_t i = 0; i < num_queries; ++i) {
      std::vector<ComponentTypeId> with_components = {static_cast<ComponentTypeId>(i)};
      auto archetype = CreateArchetype(with_components);
      std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};
      all_archetypes.push_back(std::move(archetype));

      cache.StoreCache(with_components, {}, archetypes, 1);
    }

    CHECK_GE(cache.CacheSize(), num_queries);

    // Verify all can be retrieved
    size_t successful_retrievals = 0;
    for (size_t i = 0; i < num_queries; ++i) {
      std::vector<ComponentTypeId> with_components = {static_cast<ComponentTypeId>(i)};
      const auto* result = cache.TryGetCache(with_components, {}, 1);
      if (result != nullptr) {
        ++successful_retrievals;
      }
    }

    CHECK_EQ(successful_retrievals, num_queries);
  }

  TEST_CASE("QueryState: structure validation") {
    auto archetype = CreateArchetype({100, 200});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    QueryState state;
    state.matching_archetypes = archetypes;
    state.with_component_types = {100, 200};
    state.without_component_types = {300};
    state.query_generation = 42;
    state.query_hash = 12345;

    CHECK_EQ(state.matching_archetypes.size(), 1);
    CHECK_EQ(state.with_component_types.size(), 2);
    CHECK_EQ(state.without_component_types.size(), 1);
    CHECK_EQ(state.query_generation, 42);
    CHECK_EQ(state.query_hash, 12345);
  }

  TEST_CASE("ecs::details::QueryCacheManager - Selective Invalidation") {
    QueryCacheManager cache;

    // Create multiple queries with different component combinations
    std::vector<ComponentTypeId> query1_with = {100, 200};
    std::vector<ComponentTypeId> query2_with = {200, 300};
    std::vector<ComponentTypeId> query3_with = {400, 500};

    auto archetype1 = CreateArchetype({100, 200});
    auto archetype2 = CreateArchetype({200, 300});
    auto archetype3 = CreateArchetype({400, 500});
    std::vector<std::reference_wrapper<const Archetype>> archetypes1 = {std::cref(*archetype1)};
    std::vector<std::reference_wrapper<const Archetype>> archetypes2 = {std::cref(*archetype2)};
    std::vector<std::reference_wrapper<const Archetype>> archetypes3 = {std::cref(*archetype3)};

    cache.StoreCache(query1_with, {}, archetypes1, 1);
    cache.StoreCache(query2_with, {}, archetypes2, 1);
    cache.StoreCache(query3_with, {}, archetypes3, 1);

    // Invalidate only queries involving component 200
    std::vector<ComponentTypeId> components_to_invalidate = {200};
    cache.InvalidateForComponents(components_to_invalidate);

    // Queries 1 and 2 should be invalidated (they use component 200)
    CHECK_EQ(cache.TryGetCache(query1_with, {}, 1), nullptr);
    CHECK_EQ(cache.TryGetCache(query2_with, {}, 1), nullptr);

    // Query 3 should still be cached (doesn't use component 200)
    const auto* result3 = cache.TryGetCache(query3_with, {}, 1);
    REQUIRE(result3 != nullptr);

    // Check partial invalidation counter
    CHECK_GT(cache.Stats().partial_invalidations.load(), 0);
  }

  TEST_CASE("ecs::details::QueryCacheManager - Access Time Tracking") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100};
    auto archetype = CreateArchetype({100});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    cache.StoreCache(with_components, {}, archetypes, 1);

    // First access
    const auto* result1 = cache.TryGetCache(with_components, {}, 1);
    REQUIRE(result1 != nullptr);
    const size_t time1 = result1->last_access_time.load();

    // Second access should update time
    const auto* result2 = cache.TryGetCache(with_components, {}, 1);
    REQUIRE(result2 != nullptr);
    const size_t time2 = result2->last_access_time.load();

    CHECK_GT(time2, time1);
  }

  TEST_CASE("ecs::details::QueryCacheManager - ValidateQueryState - Matching Generations") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100, 200};
    std::vector<ComponentTypeId> without_components = {300};
    auto archetype1 = CreateArchetype({100, 200});
    auto archetype2 = CreateArchetype({100, 200, 400});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype1), std::cref(*archetype2)};

    // Store cache with generation 1
    cache.StoreCache(with_components, without_components, archetypes, 1);

    const auto* result = cache.TryGetCache(with_components, without_components, 1);
    REQUIRE(result != nullptr);

    // Validate should pass with same generation
    CHECK(QueryCacheManager::ValidateQueryState(*result, 1));
  }

  TEST_CASE("ecs::details::QueryCacheManager - ValidateQueryState - Mismatched World Generation") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100, 200};
    std::vector<ComponentTypeId> without_components = {300};
    auto archetype = CreateArchetype({100, 200});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype)};

    // Store cache with generation 1
    cache.StoreCache(with_components, without_components, archetypes, 1);

    const auto* result = cache.TryGetCache(with_components, without_components, 1);
    REQUIRE(result != nullptr);

    // Validate should fail with different world generation
    CHECK_FALSE(QueryCacheManager::ValidateQueryState(*result, 2));
  }

  TEST_CASE("ecs::details::QueryCacheManager - ValidateQueryState - Empty State") {
    QueryState empty_state;
    empty_state.query_generation = 1;

    // Empty state should validate successfully
    CHECK(QueryCacheManager::ValidateQueryState(empty_state, 1));
  }

  TEST_CASE("ecs::details::QueryCacheManager - ValidateQueryState - Multiple Archetypes") {
    QueryCacheManager cache;

    std::vector<ComponentTypeId> with_components = {100};
    auto archetype1 = CreateArchetype({100});
    auto archetype2 = CreateArchetype({100, 200});
    auto archetype3 = CreateArchetype({100, 300});
    std::vector<std::reference_wrapper<const Archetype>> archetypes = {std::cref(*archetype1), std::cref(*archetype2),
                                                                       std::cref(*archetype3)};

    // Store cache with multiple archetypes
    cache.StoreCache(with_components, {}, archetypes, 1);

    const auto* result = cache.TryGetCache(with_components, {}, 1);
    REQUIRE(result != nullptr);

    // Should validate with matching generation
    CHECK(QueryCacheManager::ValidateQueryState(*result, 1));

    // Should validate archetype generations are stored
    CHECK_EQ(result->archetype_generations.size(), 3);
    CHECK_EQ(result->archetype_generations.size(), result->matching_archetypes.size());
  }

}  // TEST_SUITE
