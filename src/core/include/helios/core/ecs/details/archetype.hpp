#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/query_cache.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Key for archetype edge transitions.
 * @details Represents a component add or remove operation for edge caching.
 */
struct ArchetypeEdgeKey {
  ComponentTypeId component_id = 0;  ///< Component type being added or removed
  bool is_add = true;                ///< True for add operation, false for remove

  constexpr bool operator==(const ArchetypeEdgeKey&) const noexcept = default;
};

/**
 * @brief Hash function for ArchetypeEdgeKey.
 */
struct ArchetypeEdgeKeyHash {
  [[nodiscard]] size_t operator()(const ArchetypeEdgeKey& key) const noexcept {
    // Combine component_id hash with is_add flag using different magic constants
    const size_t component_hash = std::hash<ComponentTypeId>{}(key.component_id);
    return key.is_add ? (component_hash ^ 0x9e3779b9UZ) : (component_hash ^ 0x517cc1b7UZ);
  }
};

// Forward declaration for edge pointers
class Archetype;

/**
 * @brief Represents a unique combination of component types.
 * @details Archetypes group entities that have the exact same set of components,
 * enabling efficient queries and batch operations. All entities in an archetype
 * have identical component signatures, allowing for optimized memory layout
 * and iteration patterns.
 *
 * The archetype maintains:
 * - A sorted list of component type IDs for the signature
 * - A list of entities belonging to this archetype
 * - Fast lookup structures for entity containment checks
 * - A cached hash value for efficient archetype lookups
 * - Edge graph for fast archetype transitions when adding/removing components
 *
 * @note Not thread-safe.
 * All operations should be performed from the main thread.
 */
class Archetype {
public:
  using ComponentTypeSet = std::vector<ComponentTypeId>;
  using EntityList = std::vector<Entity>;
  using EdgeMap = std::unordered_map<ArchetypeEdgeKey, Archetype*, ArchetypeEdgeKeyHash>;

  /**
   * @brief Constructs an archetype with the specified component types.
   * @details Component types will be sorted internally for consistent ordering.
   * @param component_types Vector of component type IDs that define this archetype
   */
  explicit Archetype(ComponentTypeSet component_types);
  Archetype(const Archetype&) = delete;
  Archetype(Archetype&&) noexcept = default;
  ~Archetype() = default;

  Archetype& operator=(const Archetype&) = delete;
  Archetype& operator=(Archetype&&) noexcept = default;

  /**
   * @brief Adds an entity to this archetype.
   * @details If the entity is already in this archetype, the operation is ignored.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add
   */
  void AddEntity(Entity entity);

  /**
   * @brief Removes an entity from this archetype.
   * @details Uses swap-and-pop technique to maintain dense packing.
   * If the entity is not in this archetype, the operation is ignored.
   * Time complexity: O(n) worst case due to linear search, but typically fast.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove
   */
  void RemoveEntity(Entity entity);

  /**
   * @brief Gets the cached edge for adding a component type.
   * @details Returns the target archetype for this component addition, if cached.
   * Time complexity: O(1) average case.
   * @param component_type Component type being added
   * @return Pointer to target archetype, or nullptr if edge not cached
   */
  [[nodiscard]] Archetype* GetAddEdge(ComponentTypeId component_type) const noexcept;

  /**
   * @brief Gets the cached edge for removing a component type.
   * @details Returns the target archetype for this component removal, if cached.
   * Time complexity: O(1) average case.
   * @param component_type Component type being removed
   * @return Pointer to target archetype, or nullptr if edge not cached
   */
  [[nodiscard]] Archetype* GetRemoveEdge(ComponentTypeId component_type) const noexcept;

  /**
   * @brief Sets the cached edge for adding a component type.
   * @details Caches the target archetype for fast future lookups.
   * @param component_type Component type being added
   * @param target Target archetype after adding the component
   */
  void SetAddEdge(ComponentTypeId component_type, Archetype* target) noexcept;

  /**
   * @brief Sets the cached edge for removing a component type.
   * @details Caches the target archetype for fast future lookups.
   * @param component_type Component type being removed
   * @param target Target archetype after removing the component (nullptr if no components remain)
   */
  void SetRemoveEdge(ComponentTypeId component_type, Archetype* target) noexcept;

  /**
   * @brief Checks if this archetype contains the specified entity.
   * @details Performs fast hash-based lookup.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to check
   * @return True if entity is in this archetype, false otherwise
   */
  [[nodiscard]] bool Contains(Entity entity) const;

  /**
   * @brief Checks if this archetype has all specified component types.
   * @details Uses binary search on the sorted component type array.
   * Time complexity: O(k * log(n)) where k is number of types to check, n is archetype component count.
   * @param component_types Component types to check
   * @return True if archetype has all specified components, false otherwise
   */
  [[nodiscard]] bool HasComponents(std::span<const ComponentTypeId> component_types) const {
    return std::ranges::all_of(component_types, [this](ComponentTypeId type_id) {
      return std::ranges::binary_search(component_types_, type_id);
    });
  }

  /**
   * @brief Checks if this archetype has any of the specified component types.
   * @details Uses binary search on the sorted component type array.
   * Time complexity: O(k * log(n)) where k is number of types to check, n is archetype component count.
   * @param component_types Component types to check
   * @return True if archetype has any of the specified components, false otherwise
   */
  [[nodiscard]] bool HasAnyComponents(std::span<const ComponentTypeId> component_types) const {
    return std::ranges::any_of(component_types, [this](ComponentTypeId type_id) {
      return std::ranges::binary_search(component_types_, type_id);
    });
  }

  /**
   * @brief Checks if this archetype has a specific component type.
   * @details Uses binary search on the sorted component type array.
   * Time complexity: O(log(n)) where n is archetype component count.
   * @param component_type Component type to check
   * @return True if archetype has the specified component, false otherwise
   */
  [[nodiscard]] bool HasComponent(ComponentTypeId component_type) const noexcept {
    return std::ranges::binary_search(component_types_, component_type);
  }

  /**
   * @brief Checks if archetype contains no entities.
   * @details Simple check of the entity list size.
   * Time complexity: O(1).
   * @return True if no entities are in this archetype, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return entities_.empty(); }

  /**
   * @brief Gets all entities in this archetype.
   * @details Returns a read-only view of entities for safe iteration.
   * The order of entities may change when entities are removed due to swap-and-pop.
   * Time complexity: O(1).
   * @return Span of entities belonging to this archetype
   */
  [[nodiscard]] std::span<const Entity> Entities() const noexcept { return entities_; }

  /**
   * @brief Gets the component types for this archetype.
   * @details Returns the sorted component type IDs that define this archetype's signature.
   * Time complexity: O(1).
   * @return Span of component type IDs in sorted order
   */
  [[nodiscard]] std::span<const ComponentTypeId> ComponentTypes() const noexcept { return component_types_; }

  /**
   * @brief Gets the number of entities in this archetype.
   * @details Simple count of entities currently in this archetype.
   * Time complexity: O(1).
   * @return Entity count
   */
  [[nodiscard]] size_t EntityCount() const noexcept { return entities_.size(); }

  /**
   * @brief Gets the number of component types in this archetype.
   * @details Simple count of component types defining this archetype.
   * Time complexity: O(1).
   * @return Component type count
   */
  [[nodiscard]] size_t ComponentCount() const noexcept { return component_types_.size(); }

  /**
   * @brief Gets the cached hash value for this archetype.
   * @details Hash is computed once during construction based on component types.
   * Used for efficient archetype lookups in hash-based containers.
   * Time complexity: O(1).
   * @return Hash value based on component type combination
   */
  [[nodiscard]] size_t Hash() const noexcept { return hash_; }

  /**
   * @brief Gets the generation counter for this archetype.
   * @details Generation is incremented on structural changes to the archetype.
   * Used for query cache validation.
   * Time complexity: O(1).
   * @return Current generation value
   */
  [[nodiscard]] size_t GetGeneration() const noexcept { return generation_; }

  /**
   * @brief Gets the number of cached edges.
   * @details Returns count of cached archetype transitions.
   * Time complexity: O(1).
   * @return Number of cached edges
   */
  [[nodiscard]] size_t EdgeCount() const noexcept { return edges_.size(); }

private:
  /**
   * @brief Computes hash value for the component type combination.
   * @details Uses a mixing hash function to combine all component type IDs.
   * @return Hash value for this archetype's component signature
   */
  [[nodiscard]] size_t ComputeHash() const noexcept;

  ComponentTypeSet component_types_;                      ///< Sorted component type IDs defining this archetype
  EntityList entities_;                                   ///< Entities belonging to this archetype (dense packed)
  std::unordered_set<Entity::IndexType> entity_indices_;  ///< Fast lookup set for entity containment checks
  EdgeMap edges_;                                         ///< Cached archetype transitions for component add/remove
  size_t hash_ = 0;                                       ///< Cached hash value for archetype lookups
  size_t generation_ = 0;                                 ///< Generation counter for structural changes
};

inline Archetype::Archetype(ComponentTypeSet component_types) : component_types_(std::move(component_types)) {
  std::ranges::sort(component_types_);
  hash_ = ComputeHash();
}

inline void Archetype::AddEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to add entity to archetype: Entity with index '{}' is invalid!",
                entity.Index());

  if (!entity_indices_.contains(entity.Index())) {
    entities_.push_back(entity);
    entity_indices_.insert(entity.Index());
  }
}

inline void Archetype::RemoveEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove entity from archetype: Entity with index '{}' is invalid!",
                entity.Index());

  const auto idx_it = entity_indices_.find(entity.Index());
  if (idx_it == entity_indices_.end()) {
    return;
  }

  std::erase_if(entities_, [entity](const Entity& entt) { return entt == entity; });
  entity_indices_.erase(idx_it);
}

inline Archetype* Archetype::GetAddEdge(ComponentTypeId component_type) const noexcept {
  const auto it = edges_.find({component_type, true});
  return it != edges_.end() ? it->second : nullptr;
}

inline Archetype* Archetype::GetRemoveEdge(ComponentTypeId component_type) const noexcept {
  const auto it = edges_.find({component_type, false});
  return it != edges_.end() ? it->second : nullptr;
}

inline void Archetype::SetAddEdge(ComponentTypeId component_type, Archetype* target) noexcept {
  edges_[{component_type, true}] = target;
}

inline void Archetype::SetRemoveEdge(ComponentTypeId component_type, Archetype* target) noexcept {
  edges_[{component_type, false}] = target;
}

inline bool Archetype::Contains(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if archetype contains entity: Entity with index '{}' is invalid!",
                entity.Index());
  return entity_indices_.contains(entity.Index());
}

inline size_t Archetype::ComputeHash() const noexcept {
  size_t hash = 0;
  for (const auto& type_id : component_types_) {
    hash ^= std::hash<ComponentTypeId>{}(type_id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }
  return hash;
}

/**
 * @brief Manages archetypes and entity-archetype relationships.
 * @details The archetype manager maintains a registry of all unique component combinations
 * and tracks which entities belong to which archetypes. This enables efficient queries
 * by allowing systems to iterate only over entities with specific component combinations.
 *
 * Key responsibilities:
 * - Create and cache archetypes for unique component combinations
 * - Track entity-to-archetype mappings
 * - Provide efficient archetype lookup for queries
 * - Handle entity movement between archetypes when components change
 * - Cache archetype transitions (edges) for fast component add/remove operations
 *
 * @note Not thread-safe.
 * All operations should be performed from the main thread.
 */
class Archetypes {
public:
  Archetypes() = default;
  Archetypes(const Archetypes&) = delete;
  Archetypes(Archetypes&&) noexcept = default;
  ~Archetypes() = default;

  Archetypes& operator=(const Archetypes&) = delete;
  Archetypes& operator=(Archetypes&&) noexcept = default;

  /**
   * @brief Clears all archetypes and entity mappings.
   * @details Removes all cached archetypes and resets entity-archetype relationships.
   * Should be called when clearing the world.
   * Time complexity: O(1).
   */
  void Clear() noexcept;

  /**
   * @brief Resets query cache statistics.
   */
  void ResetCacheStats() noexcept { query_cache_.ResetStats(); }

  /**
   * @brief Updates entity's archetype based on its current components.
   * @details Removes entity from its current archetype (if any) and adds it to the
   * archetype matching its new component combination. Creates new archetypes as needed.
   * Time complexity: O(log(A) + C*log(C)) where A is archetype count, C is component count.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to update
   * @param component_types Current component types for the entity
   */
  void UpdateEntityArchetype(Entity entity, std::span<const ComponentTypeId> component_types);

  /**
   * @brief Moves entity to a new archetype after adding a component.
   * @details Uses cached edge graph for O(1) lookup when edge is cached.
   * Falls back to full archetype lookup if edge is not cached, then caches the result.
   * Time complexity: O(1) amortized with caching, O(C*log(C)) cache miss.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to move
   * @param added_component Component type being added
   * @param new_component_types Complete set of component types after addition
   */
  void MoveEntityOnComponentAdd(Entity entity, ComponentTypeId added_component,
                                std::span<const ComponentTypeId> new_component_types);

  /**
   * @brief Moves entity to a new archetype after removing a component.
   * @details Uses cached edge graph for O(1) lookup when edge is cached.
   * Falls back to full archetype lookup if edge is not cached, then caches the result.
   * Time complexity: O(1) amortized with caching, O(C*log(C)) cache miss.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to move
   * @param removed_component Component type being removed
   * @param new_component_types Complete set of component types after removal
   */
  void MoveEntityOnComponentRemove(Entity entity, ComponentTypeId removed_component,
                                   std::span<const ComponentTypeId> new_component_types);

  /**
   * @brief Removes entity from its current archetype.
   * @details If the entity is not in any archetype, the operation is ignored.
   * Time complexity: O(1) average case for lookup, O(n) for archetype removal.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove
   */
  void RemoveEntity(Entity entity);

  /**
   * @brief Gets archetype containing the specified entity.
   * @details Performs fast hash-based lookup of entity-to-archetype mapping.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to find
   * @return Pointer to archetype containing the entity, or nullptr if not found
   */
  [[nodiscard]] const Archetype* GetEntityArchetype(Entity entity) const;

  /**
   * @brief Gets mutable archetype containing the specified entity.
   * @details Performs fast hash-based lookup of entity-to-archetype mapping.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to find
   * @return Pointer to archetype containing the entity, or nullptr if not found
   */
  [[nodiscard]] Archetype* GetEntityArchetypeMutable(Entity entity);

  /**
   * @brief Finds archetypes that match the specified component requirements.
   * @details Searches all archetypes for those that have all required components
   * and none of the forbidden components. Only returns non-empty archetypes.
   * Time complexity: O(A * (W*log(C) + X*log(C))) where A is archetype count,
   * W is with-component count, X is without-component count, C is avg components per archetype.
   *
   * @param with_components Component types that must be present
   * @param without_components Component types that must not be present
   * @return Vector of references to matching non-empty archetypes
   */
  [[nodiscard]] auto FindMatchingArchetypes(std::span<const ComponentTypeId> with_components,
                                            std::span<const ComponentTypeId> without_components = {}) const
      -> std::vector<std::reference_wrapper<const Archetype>>;

  /**
   * @brief Enables or disables query caching.
   * @param enable True to enable caching, false to disable
   */
  void EnableQueryCache(bool enable) noexcept { use_query_cache_ = enable; }

  /**
   * @brief Checks if query caching is enabled.
   * @return True if caching is enabled, false otherwise
   */
  [[nodiscard]] bool IsQueryCacheEnabled() const noexcept { return use_query_cache_; }

  /**
   * @brief Gets query cache statistics.
   * @return Cache statistics
   */
  [[nodiscard]] QueryCacheStats CacheStats() const noexcept { return query_cache_.Stats(); }

  /**
   * @brief Gets the total number of archetypes.
   * @details Returns count of unique component combinations currently cached.
   * Time complexity: O(1).
   * @return Total archetype count
   */
  [[nodiscard]] size_t ArchetypeCount() const noexcept { return archetypes_.size(); }

  /**
   * @brief Gets total number of cached edges across all archetypes.
   * @details Returns count of all cached archetype transitions.
   * Time complexity: O(A) where A is archetype count.
   * @return Total edge count
   */
  [[nodiscard]] size_t TotalEdgeCount() const noexcept;

private:
  /**
   * @brief Finds or creates archetype for the specified component types.
   * @details Uses hash-based lookup to find existing archetypes, or creates new ones.
   * Component types are sorted before hashing for consistency.
   * Time complexity: O(C*log(C)) for new archetypes, O(1) average for existing.
   *
   * @param component_types Component types for the archetype
   * @return Reference to the archetype (existing or newly created)
   */
  [[nodiscard]] Archetype& GetOrCreateArchetype(std::span<const ComponentTypeId> component_types);

  /**
   * @brief Generates hash for component type combination.
   * @details Creates a sorted copy of component types and hashes them consistently.
   * Time complexity: O(C*log(C)) where C is component count.
   *
   * @param component_types Component types to hash
   * @return Hash value for the component combination
   */
  [[nodiscard]] static size_t HashComponentTypes(std::span<const ComponentTypeId> component_types);

  /**
   * @brief Invalidates query cache when archetypes change.
   */
  void InvalidateQueryCache() noexcept;

  /**
   * @brief Notifies query cache of archetype structural changes.
   */
  void NotifyArchetypeStructuralChange() noexcept;

  std::unordered_map<size_t, std::unique_ptr<Archetype>> archetypes_;      ///< Hash -> Archetype mapping
  std::unordered_map<Entity::IndexType, Archetype*> entity_to_archetype_;  ///< Entity index -> Archetype mapping

  mutable QueryCacheManager query_cache_;  ///< Query result cache
  size_t structural_version_ = 0;          ///< Incremented on structural changes
  bool use_query_cache_ = true;            ///< Whether to use query caching
};

inline void Archetypes::Clear() noexcept {
  archetypes_.clear();
  entity_to_archetype_.clear();
  query_cache_.Clear();
  structural_version_ = 0;
}

inline void Archetypes::RemoveEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove entity from archetypes: Entity with index '{}' is invalid!",
                entity.Index());

  const auto it = entity_to_archetype_.find(entity.Index());
  if (it == entity_to_archetype_.end()) {
    return;
  }

  it->second->RemoveEntity(entity);
  entity_to_archetype_.erase(it);
  // Note: We don't invalidate cache here since entity removal doesn't change archetype structure
}

inline const Archetype* Archetypes::GetEntityArchetype(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to get entity archetype: Entity with index '{}' is invalid!", entity.Index());

  const auto it = entity_to_archetype_.find(entity.Index());
  if (it != entity_to_archetype_.end()) [[likely]] {
    return it->second;
  }

  return nullptr;
}

inline Archetype* Archetypes::GetEntityArchetypeMutable(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to get entity archetype: Entity with index '{}' is invalid!", entity.Index());

  const auto it = entity_to_archetype_.find(entity.Index());
  if (it != entity_to_archetype_.end()) [[likely]] {
    return it->second;
  }

  return nullptr;
}

inline void Archetypes::InvalidateQueryCache() noexcept {
  if (use_query_cache_) {
    query_cache_.InvalidateAll();
    ++structural_version_;
  }
}

inline size_t Archetypes::TotalEdgeCount() const noexcept {
  size_t count = 0;
  for (const auto& [_, archetype] : archetypes_) {
    count += archetype->EdgeCount();
  }
  return count;
}

}  // namespace helios::ecs::details
