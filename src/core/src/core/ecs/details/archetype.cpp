#include <helios/core/ecs/details/archetype.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/entity.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace helios::ecs::details {

void Archetypes::UpdateEntityArchetype(Entity entity, std::span<const ComponentTypeId> component_types) {
  HELIOS_ASSERT(entity.Valid(), "Failed to update entity archetype: Entity with index '{}' is invalid!",
                entity.Index());

  // Remove entity from current archetype
  const auto current_it = entity_to_archetype_.find(entity.Index());
  if (current_it != entity_to_archetype_.end()) {
    current_it->second->RemoveEntity(entity);
    entity_to_archetype_.erase(current_it);
  }

  // If entity has no components, don't add to any archetype
  if (component_types.empty()) {
    return;
  }

  // Get or create archetype for new component combination
  auto& archetype = GetOrCreateArchetype(component_types);
  const bool is_new_archetype = archetype.EntityCount() == 0;

  archetype.AddEntity(entity);
  entity_to_archetype_[entity.Index()] = &archetype;

  // Notify query cache of structural changes
  if (is_new_archetype) {
    InvalidateQueryCache();
  } else {
    NotifyArchetypeStructuralChange();
  }
}

void Archetypes::MoveEntityOnComponentAdd(Entity entity, ComponentTypeId added_component,
                                          std::span<const ComponentTypeId> new_component_types) {
  HELIOS_ASSERT(entity.Valid(), "Failed to move entity on component add: Entity with index '{}' is invalid!",
                entity.Index());

  Archetype* current_archetype = GetEntityArchetypeMutable(entity);

  // If entity has no current archetype (first component), use standard path
  if (current_archetype == nullptr) {
    auto& target_archetype = GetOrCreateArchetype(new_component_types);
    const bool is_new_archetype = target_archetype.EntityCount() == 0;

    target_archetype.AddEntity(entity);
    entity_to_archetype_[entity.Index()] = &target_archetype;

    if (is_new_archetype) {
      InvalidateQueryCache();
    } else {
      NotifyArchetypeStructuralChange();
    }
    return;
  }

  // Try cached edge first
  Archetype* cached_target = current_archetype->GetAddEdge(added_component);
  if (cached_target != nullptr) {
    // Fast path: use cached edge
    current_archetype->RemoveEntity(entity);
    cached_target->AddEntity(entity);
    entity_to_archetype_[entity.Index()] = cached_target;
    NotifyArchetypeStructuralChange();
    return;
  }

  // Cache miss: find or create target archetype
  auto& target_archetype = GetOrCreateArchetype(new_component_types);
  const bool is_new_archetype = target_archetype.EntityCount() == 0;

  // Cache the edge for future use
  current_archetype->SetAddEdge(added_component, &target_archetype);

  // Move entity
  current_archetype->RemoveEntity(entity);
  target_archetype.AddEntity(entity);
  entity_to_archetype_[entity.Index()] = &target_archetype;

  if (is_new_archetype) {
    InvalidateQueryCache();
  } else {
    NotifyArchetypeStructuralChange();
  }
}

void Archetypes::MoveEntityOnComponentRemove(Entity entity, ComponentTypeId removed_component,
                                             std::span<const ComponentTypeId> new_component_types) {
  HELIOS_ASSERT(entity.Valid(), "Failed to move entity on component remove: Entity with index '{}' is invalid!",
                entity.Index());

  Archetype* current_archetype = GetEntityArchetypeMutable(entity);

  // If entity has no current archetype, nothing to remove from
  if (current_archetype == nullptr) {
    return;
  }

  // Remove entity from current archetype
  current_archetype->RemoveEntity(entity);
  entity_to_archetype_.erase(entity.Index());

  // If no components remain, entity has no archetype
  if (new_component_types.empty()) {
    // Cache edge to nullptr (no archetype)
    current_archetype->SetRemoveEdge(removed_component, nullptr);
    return;
  }

  // Try cached edge first
  Archetype* cached_target = current_archetype->GetRemoveEdge(removed_component);
  if (cached_target != nullptr) {
    // Fast path: use cached edge
    cached_target->AddEntity(entity);
    entity_to_archetype_[entity.Index()] = cached_target;
    NotifyArchetypeStructuralChange();
    return;
  }

  // Cache miss: find or create target archetype
  auto& target_archetype = GetOrCreateArchetype(new_component_types);
  const bool is_new_archetype = target_archetype.EntityCount() == 0;

  // Cache the edge for future use
  current_archetype->SetRemoveEdge(removed_component, &target_archetype);

  // Move entity
  target_archetype.AddEntity(entity);
  entity_to_archetype_[entity.Index()] = &target_archetype;

  if (is_new_archetype) {
    InvalidateQueryCache();
  } else {
    NotifyArchetypeStructuralChange();
  }
}

auto Archetypes::FindMatchingArchetypes(std::span<const ComponentTypeId> with_components,
                                        std::span<const ComponentTypeId> without_components) const
    -> std::vector<std::reference_wrapper<const Archetype>> {
  // Try to get cached result if caching is enabled
  if (use_query_cache_) {
    const auto* cached = query_cache_.TryGetCache(with_components, without_components, structural_version_);
    if (cached != nullptr) {
      return cached->matching_archetypes;
    }
  }

  // Cache miss or caching disabled - perform search
  std::vector<std::reference_wrapper<const Archetype>> matching;
  matching.reserve(archetypes_.size());  // Reserve worst-case size

  for (const auto& [_, archetype] : archetypes_) {
    if (!archetype->Empty() && (without_components.empty() || !archetype->HasAnyComponents(without_components)) &&
        (with_components.empty() || archetype->HasComponents(with_components))) {
      matching.emplace_back(*archetype);
    }
  }

  matching.shrink_to_fit();  // Reduce to actual size

  // Store in cache if enabled
  if (use_query_cache_) {
    query_cache_.StoreCache(with_components, without_components, matching, structural_version_);
  }

  return matching;
}

Archetype& Archetypes::GetOrCreateArchetype(std::span<const ComponentTypeId> component_types) {
  const size_t hash = HashComponentTypes(component_types);
  const auto [it, inserted] = archetypes_.try_emplace(hash, nullptr);
  if (inserted) {
    std::vector<ComponentTypeId> sorted_types(component_types.begin(), component_types.end());
    it->second = std::make_unique<Archetype>(std::move(sorted_types));
  }

  return *it->second;
}

size_t Archetypes::HashComponentTypes(std::span<const ComponentTypeId> component_types) {
  // Create a sorted copy for consistent hashing
  std::vector<ComponentTypeId> sorted_types(component_types.begin(), component_types.end());
  std::ranges::sort(sorted_types);

  size_t hash = 0;
  for (const auto& type_id : sorted_types) {
    hash ^= std::hash<ComponentTypeId>{}(type_id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }
  return hash;
}

void Archetypes::NotifyArchetypeStructuralChange() noexcept {
  if (use_query_cache_) {
    query_cache_.NotifyArchetypeChange();
    ++structural_version_;
  }
}

}  // namespace helios::ecs::details
