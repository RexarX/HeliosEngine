#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/ecs/component.hpp>

#include <boost/unordered/concurrent_node_map.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace helios::ecs::details {

class Archetype;

/**
 * @brief Query state that caches archetype matching results.
 * @details Inspired by Bevy's QueryState, this stores pre-computed information
 * about which archetypes match a specific query signature. Each QueryState
 * corresponds to a unique combination of required and forbidden components.
 *
 * The state includes:
 * - List of matching archetypes (cached)
 * - Archetype generation counters (for invalidation detection)
 * - Component type signature (for efficient lookup)
 */
struct QueryState {
  std::vector<std::reference_wrapper<const Archetype>> matching_archetypes;  ///< Archetypes that match this query
  std::vector<size_t> archetype_generations;             ///< Generation of each matched archetype when cached
  std::vector<ComponentTypeId> with_component_types;     ///< Required component types (sorted)
  std::vector<ComponentTypeId> without_component_types;  ///< Forbidden component types (sorted)
  size_t query_generation = 0;                           ///< Generation when this state was computed
  size_t query_hash = 0;                                 ///< Hash of the query signature
  mutable std::atomic<size_t> last_access_time{0};       ///< Last access timestamp for LRU eviction

  QueryState() noexcept = default;
  QueryState(const QueryState& other);
  QueryState(QueryState&& other) noexcept;
  ~QueryState() = default;

  QueryState& operator=(const QueryState& other);
  QueryState& operator=(QueryState&& other) noexcept;
};

inline QueryState::QueryState(const QueryState& other)
    : matching_archetypes(other.matching_archetypes),
      archetype_generations(other.archetype_generations),
      with_component_types(other.with_component_types),
      without_component_types(other.without_component_types),
      query_generation(other.query_generation),
      query_hash(other.query_hash),
      last_access_time(other.last_access_time.load(std::memory_order_relaxed)) {}

inline QueryState::QueryState(QueryState&& other) noexcept
    : matching_archetypes(std::move(other.matching_archetypes)),
      archetype_generations(std::move(other.archetype_generations)),
      with_component_types(std::move(other.with_component_types)),
      without_component_types(std::move(other.without_component_types)),
      query_generation(other.query_generation),
      query_hash(other.query_hash),
      last_access_time(other.last_access_time.load(std::memory_order_relaxed)) {}

inline QueryState& QueryState::operator=(const QueryState& other) {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  matching_archetypes = other.matching_archetypes;
  archetype_generations = other.archetype_generations;
  with_component_types = other.with_component_types;
  without_component_types = other.without_component_types;
  query_generation = other.query_generation;
  query_hash = other.query_hash;
  last_access_time.store(other.last_access_time.load(std::memory_order_relaxed), std::memory_order_relaxed);

  return *this;
}

inline QueryState& QueryState::operator=(QueryState&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  matching_archetypes = std::move(other.matching_archetypes);
  archetype_generations = std::move(other.archetype_generations);
  with_component_types = std::move(other.with_component_types);
  without_component_types = std::move(other.without_component_types);
  query_generation = other.query_generation;
  query_hash = other.query_hash;
  last_access_time.store(other.last_access_time.load(std::memory_order_relaxed), std::memory_order_relaxed);

  return *this;
}

/**
 * @brief Statistics for query cache performance.
 * @details Tracks cache effectiveness metrics including hits, misses, and invalidations.
 * @note Thread-safe.
 */
struct QueryCacheStats {
  std::atomic<size_t> hit_count{0};              ///< Number of cache hits
  std::atomic<size_t> miss_count{0};             ///< Number of cache misses
  std::atomic<size_t> invalidation_count{0};     ///< Number of cache invalidations
  std::atomic<size_t> total_queries{0};          ///< Total number of queries executed
  std::atomic<size_t> archetype_changes{0};      ///< Number of archetype structural changes
  std::atomic<size_t> partial_invalidations{0};  ///< Number of partial (component-specific) invalidations

  QueryCacheStats() noexcept = default;
  QueryCacheStats(const QueryCacheStats& other);
  QueryCacheStats(QueryCacheStats&& other) noexcept;
  ~QueryCacheStats() noexcept = default;

  QueryCacheStats& operator=(const QueryCacheStats& other);
  QueryCacheStats& operator=(QueryCacheStats&& other) noexcept;

  void Reset() noexcept;

  [[nodiscard]] double HitRate() const noexcept;
};

inline QueryCacheStats::QueryCacheStats(const QueryCacheStats& other)
    : hit_count(other.hit_count.load(std::memory_order_relaxed)),
      miss_count(other.miss_count.load(std::memory_order_relaxed)),
      invalidation_count(other.invalidation_count.load(std::memory_order_relaxed)),
      total_queries(other.total_queries.load(std::memory_order_relaxed)),
      archetype_changes(other.archetype_changes.load(std::memory_order_relaxed)),
      partial_invalidations(other.partial_invalidations.load(std::memory_order_relaxed)) {}

inline QueryCacheStats::QueryCacheStats(QueryCacheStats&& other) noexcept
    : hit_count(other.hit_count.load(std::memory_order_relaxed)),
      miss_count(other.miss_count.load(std::memory_order_relaxed)),
      invalidation_count(other.invalidation_count.load(std::memory_order_relaxed)),
      total_queries(other.total_queries.load(std::memory_order_relaxed)),
      archetype_changes(other.archetype_changes.load(std::memory_order_relaxed)),
      partial_invalidations(other.partial_invalidations.load(std::memory_order_relaxed)) {}

inline QueryCacheStats& QueryCacheStats::operator=(const QueryCacheStats& other) {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  hit_count.store(other.hit_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  miss_count.store(other.miss_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  invalidation_count.store(other.invalidation_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  total_queries.store(other.total_queries.load(std::memory_order_relaxed), std::memory_order_relaxed);
  archetype_changes.store(other.archetype_changes.load(std::memory_order_relaxed), std::memory_order_relaxed);
  partial_invalidations.store(other.partial_invalidations.load(std::memory_order_relaxed), std::memory_order_relaxed);

  return *this;
}

inline QueryCacheStats& QueryCacheStats::operator=(QueryCacheStats&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  hit_count.store(other.hit_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  miss_count.store(other.miss_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  invalidation_count.store(other.invalidation_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  total_queries.store(other.total_queries.load(std::memory_order_relaxed), std::memory_order_relaxed);
  archetype_changes.store(other.archetype_changes.load(std::memory_order_relaxed), std::memory_order_relaxed);
  partial_invalidations.store(other.partial_invalidations.load(std::memory_order_relaxed), std::memory_order_relaxed);

  return *this;
}

inline void QueryCacheStats::Reset() noexcept {
  hit_count.store(0, std::memory_order_relaxed);
  miss_count.store(0, std::memory_order_relaxed);
  invalidation_count.store(0, std::memory_order_relaxed);
  total_queries.store(0, std::memory_order_relaxed);
  archetype_changes.store(0, std::memory_order_relaxed);
  partial_invalidations.store(0, std::memory_order_relaxed);
}

inline double QueryCacheStats::HitRate() const noexcept {
  const size_t total = total_queries.load(std::memory_order_relaxed);
  if (total == 0) [[unlikely]] {
    return 0.0;
  }

  const size_t hits = hit_count.load(std::memory_order_relaxed);
  return static_cast<double>(hits) / static_cast<double>(total);
}

/**
 * @brief Manages query state caching with Bevy-inspired optimizations.
 * @details Provides efficient caching of archetype matching results with smart invalidation.
 *
 * Key features:
 * - Archetype generation tracking: Each archetype has a generation counter that increments
 *   on structural changes. Query states cache these generations and can detect stale data.
 * - Component-aware invalidation: Only queries using specific components are invalidated
 *   when those components are modified.
 * - Query state reuse: Pre-computed query metadata is cached and reused across frames.
 * - Efficient validation: Quick checks using generation counters avoid full re-computation.
 *
 * @note Thread-safe for read operations, single-writer for updates.
 */
class QueryCacheManager {
public:
  QueryCacheManager() = default;
  QueryCacheManager(const QueryCacheManager&) = delete;
  QueryCacheManager(QueryCacheManager&& other) noexcept;
  ~QueryCacheManager() = default;

  QueryCacheManager& operator=(const QueryCacheManager&) = delete;
  QueryCacheManager& operator=(QueryCacheManager&& other) noexcept;

  /**
   * @brief Clears all cached query states and resets statistics.
   */
  void Clear() noexcept;

  /**
   * @brief Resets cache statistics without clearing query states.
   */
  void ResetStats() noexcept { stats_.Reset(); }

  /**
   * @brief Tries to retrieve a valid cached query state.
   * @details Checks if cached state exists and validates it against current archetype generations.
   * If state is stale (archetypes changed), returns nullptr to trigger re-computation.
   * @param with_components Component types that must be present (must be sorted)
   * @param without_components Component types that must not be present (must be sorted)
   * @param current_generation Current world structural generation
   * @return Pointer to valid cached state, or nullptr if cache miss/stale
   */
  [[nodiscard]] const QueryState* TryGetCache(std::span<const ComponentTypeId> with_components,
                                              std::span<const ComponentTypeId> without_components,
                                              size_t current_generation) const;

  /**
   * @brief Stores or updates a query state in the cache.
   * @details Caches the matched archetypes along with their current generation counters.
   * This allows future queries to validate the cache using generation checks.
   * @param with_components Component types that must be present (must be sorted)
   * @param without_components Component types that must not be present (must be sorted)
   * @param matching_archetypes Archetypes that match the query criteria
   * @param current_generation Current world structural generation
   */
  void StoreCache(std::span<const ComponentTypeId> with_components, std::span<const ComponentTypeId> without_components,
                  std::vector<std::reference_wrapper<const Archetype>> matching_archetypes, size_t current_generation);

  /**
   * @brief Invalidates all cached query states.
   * @details Called when major structural changes occur that affect many queries.
   * Increments global invalidation counter.
   */
  void InvalidateAll();

  /**
   * @brief Invalidates query states that involve specific components.
   * @details Selective invalidation - only queries using the specified components
   * need to be recomputed. More efficient than full invalidation.
   * @param component_ids Component types that were modified
   */
  void InvalidateForComponents(std::span<const ComponentTypeId> component_ids);

  /**
   * @brief Notifies the cache of an archetype structural change.
   * @details Increments archetype change counter for statistics tracking.
   * Does not directly invalidate - queries validate themselves using generation checks.
   */
  void NotifyArchetypeChange() { ++stats_.archetype_changes; }

  /**
   * @brief Gets the number of cached query states.
   * @return Cache entry count
   */
  [[nodiscard]] size_t CacheSize() const noexcept { return cache_.size(); }

  /**
   * @brief Gets cache performance statistics.
   * @return Struct containing hit/miss counts and other metrics
   */
  [[nodiscard]] QueryCacheStats Stats() const noexcept { return stats_; }

  /**
   * @brief Validates a cached query state against current archetype generations.
   * @details Compares cached archetype generations with current ones to detect
   * if any archetypes have changed structure since caching.
   * @param state Query state to validate
   * @param current_generation Current world structural generation
   * @return True if state is still valid, false if stale
   */
  [[nodiscard]] static bool ValidateQueryState(const QueryState& state, size_t current_generation);

private:
  /**
   * @brief Updates access timestamp for LRU tracking.
   * @param state Query state to update
   */
  void UpdateAccessTime(QueryState& state) const noexcept {
    state.last_access_time.store(access_counter_.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);
  }

  /**
   * @brief Computes hash for a query signature.
   * @details Combines sorted component type lists into a single hash value.
   * @param with_components Required components (must be sorted)
   * @param without_components Forbidden components (must be sorted)
   * @return Hash value representing the query signature
   */
  [[nodiscard]] static size_t ComputeQueryHash(std::span<const ComponentTypeId> with_components,
                                               std::span<const ComponentTypeId> without_components);

  /**
   * @brief Checks if a query involves any of the specified components.
   * @details Used for selective invalidation - determines if a query needs
   * to be recomputed when specific components change.
   * @param state Query state to check
   * @param component_ids Component types to check against
   * @return True if query uses any of the specified components
   */
  [[nodiscard]] static bool QueryInvolvesAnyComponents(const QueryState& state,
                                                       std::span<const ComponentTypeId> component_ids);

  boost::unordered::concurrent_node_map<size_t, QueryState> cache_;  ///< Hash -> query state mapping
  mutable QueryCacheStats stats_;                                    ///< Cache performance statistics
  mutable std::atomic<size_t> access_counter_{0};                    ///< Monotonic counter for LRU tracking
};

inline QueryCacheManager::QueryCacheManager(QueryCacheManager&& other) noexcept
    : cache_(std::move(other.cache_)),
      stats_(std::move(other.stats_)),
      access_counter_(other.access_counter_.load(std::memory_order_relaxed)) {}

inline QueryCacheManager& QueryCacheManager::operator=(QueryCacheManager&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  cache_ = std::move(other.cache_);
  stats_ = other.stats_;
  access_counter_.store(other.access_counter_.load(std::memory_order_relaxed), std::memory_order_relaxed);

  return *this;
}

inline void QueryCacheManager::Clear() noexcept {
  cache_.clear();
  stats_.Reset();
  access_counter_.store(0, std::memory_order_relaxed);
}

inline const QueryState* QueryCacheManager::TryGetCache(std::span<const ComponentTypeId> with_components,
                                                        std::span<const ComponentTypeId> without_components,
                                                        size_t current_generation) const {
  const size_t hash = ComputeQueryHash(with_components, without_components);
  const QueryState* found = nullptr;

  cache_.cvisit(hash, [this, &found, current_generation](const auto& pair) {
    auto& state = const_cast<QueryState&>(pair.second);

    // Validate state against current generation
    if (!ValidateQueryState(state, current_generation)) {
      return;  // State is stale
    }

    UpdateAccessTime(state);
    found = &state;
  });

  if (found == nullptr) {
    ++stats_.miss_count;
  } else {
    ++stats_.hit_count;
  }
  ++stats_.total_queries;

  return found;
}

inline void QueryCacheManager::InvalidateAll() {
  cache_.clear();
  ++stats_.invalidation_count;
}

inline void QueryCacheManager::InvalidateForComponents(std::span<const ComponentTypeId> component_ids) {
  if (component_ids.empty()) {
    return;
  }

  // Remove cache entries that involve any of the specified components
  std::vector<size_t> keys_to_remove;
  cache_.cvisit_all([&keys_to_remove, component_ids](const auto& pair) {
    if (QueryInvolvesAnyComponents(pair.second, component_ids)) {
      keys_to_remove.push_back(pair.first);
    }
  });

  for (const size_t key : keys_to_remove) {
    cache_.erase(key);
  }

  if (!keys_to_remove.empty()) {
    stats_.partial_invalidations.fetch_add(keys_to_remove.size(), std::memory_order_relaxed);
  }
}

}  // namespace helios::ecs::details
