#include <helios/core/ecs/details/query_cache.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/archetype.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace helios::ecs::details {

size_t QueryCacheManager::ComputeQueryHash(std::span<const ComponentTypeId> with_components,
                                           std::span<const ComponentTypeId> without_components) {
  // Components should already be sorted by caller
  size_t hash = 0;

  // Hash "with" components
  for (const auto& id : with_components) {
    hash ^= std::hash<ComponentTypeId>{}(id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }

  // Hash "without" components with different mixing to distinguish from "with"
  for (const auto& id : without_components) {
    hash ^= std::hash<ComponentTypeId>{}(id) + 0x517cc1b7 + (hash << 7) + (hash >> 3);
  }

  return hash;
}

bool QueryCacheManager::QueryInvolvesAnyComponents(const QueryState& state,
                                                   std::span<const ComponentTypeId> component_ids) {
  // Check if any of the component_ids appear in the query's with or without lists
  return std::ranges::any_of(component_ids, [&state](const ComponentTypeId id) {
    return std::ranges::binary_search(state.with_component_types, id) ||
           std::ranges::binary_search(state.without_component_types, id);
  });
}

void QueryCacheManager::StoreCache(std::span<const ComponentTypeId> with_components,
                                   std::span<const ComponentTypeId> without_components,
                                   std::vector<std::reference_wrapper<const Archetype>> matching_archetypes,
                                   size_t current_generation) {
  const size_t hash = ComputeQueryHash(with_components, without_components);

  QueryState state;
  state.matching_archetypes = std::move(matching_archetypes);
  state.archetype_generations.reserve(state.matching_archetypes.size());

  // Store each archetype's current generation for validation
  for (const auto& archetype_ref : state.matching_archetypes) {
    state.archetype_generations.push_back(archetype_ref.get().GetGeneration());
  }

  state.with_component_types.assign(with_components.begin(), with_components.end());
  state.without_component_types.assign(without_components.begin(), without_components.end());
  state.query_generation = current_generation;
  state.query_hash = hash;
  state.last_access_time.store(access_counter_.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);

  cache_.insert_or_assign(hash, std::move(state));
}

bool QueryCacheManager::ValidateQueryState(const QueryState& state, size_t current_generation) {
  // Quick check: world generation mismatch
  if (state.query_generation != current_generation) {
    return false;
  }

  // Validate each cached archetype's generation
  HELIOS_ASSERT(state.matching_archetypes.size() == state.archetype_generations.size(),
                "Archetype count mismatch in query state! Expected {}, got {}", state.matching_archetypes.size(),
                state.archetype_generations.size());

  for (size_t i = 0; i < state.matching_archetypes.size(); ++i) {
    const Archetype& archetype = state.matching_archetypes[i].get();
    const size_t cached_generation = state.archetype_generations[i];

    // Check if archetype structure has changed
    if (archetype.GetGeneration() != cached_generation) {
      return false;
    }
  }

  return true;
}

}  // namespace helios::ecs::details
