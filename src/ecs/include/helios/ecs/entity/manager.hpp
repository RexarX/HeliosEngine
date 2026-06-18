#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/entity/entity.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace helios::ecs {

/**
 * @brief Entity manager responsible for entity creation, destruction, and
 * validation.
 * @details Manages entity lifecycle with generation counters to handle entity
 * recycling safely.
 * @note Partially thread-safe.
 */
class EntityManager {
public:
  EntityManager() = default;
  EntityManager(const EntityManager& other);
  EntityManager(EntityManager&& other) noexcept;
  ~EntityManager() = default;

  EntityManager& operator=(const EntityManager& other);
  EntityManager& operator=(EntityManager&& other) noexcept;

  /**
   * @brief Clears all entities.
   * @details Destroys all entities and resets the manager state.
   * @note Not thread-safe.
   */
  void Clear() noexcept;

  /**
   * @brief Creates reserved entities in the metadata.
   * @details Processes all reserved entity IDs and creates their metadata.
   * @note Not thread-safe.
   */
  void Flush() {
    Flush([](Entity /*entity*/) {});
  }

  /**
   * @brief Creates reserved entities and invokes callback for each flushed
   * entity.
   * @details Processes all reserved entity IDs, initializes their metadata,
   * and invokes `callback` exactly once per newly flushed entity.
   * @note Not thread-safe.
   * @tparam F Callable type invocable with `Entity`
   * @param callback Callback invoked for each newly flushed entity
   */
  template <typename F>
    requires std::invocable<F&, Entity>
  void Flush(const F& callback);

  /**
   * @brief Reserves space for entities to minimize allocations.
   * @details Pre-allocates storage for the specified number of entities.
   * @note Not thread-safe.
   * @param count Number of entities to reserve space for
   */
  void Reserve(size_t count);

  /**
   * @brief Reserves an entity ID that can be used immediately.
   * @details The actual entity creation is deferred until
   * `Flush()` is called.
   * @note Thread-safe.
   * @return Reserved entity with valid index and generation
   */
  [[nodiscard]] Entity ReserveEntity();

  /**
   * @brief Creates a new entity.
   * @details Reuses dead entity slots when available, otherwise creates new
   * ones.
   * @note Not thread-safe.
   * @return Newly created entity with valid index and generation
   */
  [[nodiscard]] Entity Create();

  /**
   * @brief Creates multiple entities at once and outputs them via an output
   * iterator.
   * @details Batch creation is more efficient than individual calls.
   * Entities are written to the provided output iterator, avoiding internal
   * allocations.
   * @note Not thread-safe.
   * @tparam OutputIt Output iterator type that accepts Entity values
   * @param count Number of entities to create
   * @param out Output iterator to write created entities to
   * @return Output iterator pointing past the last written entity
   *
   * @code
   * std::vector<Entity> entities;
   * entities.reserve(100);
   * manager.Create(100, std::back_inserter(entities));
   *
   * // Or with pre-allocated array:
   * std::array<Entity, 10> arr;
   * manager.Create(10, arr.begin());
   *
   * // Or with span output:
   * Entity buffer[50];
   * manager.Create(50, std::begin(buffer));
   * @endcode
   */
  template <typename OutputIt>
    requires std::output_iterator<OutputIt, Entity>
  OutputIt Create(size_t count, OutputIt&& out);

  /**
   * @brief Destroys an entity by incrementing its generation.
   * @details Marks entity as dead and adds its index to the free list for
   * reuse. Entities that do not exist or are already destroyed are ignored.
   * @note Not thread-safe.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to destroy
   */
  void Destroy(Entity entity);

  /**
   * @brief Destroys an entity by incrementing its generation.
   * @details Marks entity as dead and adds its index to the free list for
   * reuse. Entities that do not exist or are already destroyed are ignored.
   * @note Not thread-safe.
   * @warning Triggers assertion if any entity is invalid.
   * @tparam R Range type containing Entity elements
   * @param entities Entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void Destroy(const R& entities);

  /**
   * @brief Checks if an entity exists and is valid.
   * @details Validates both the entity structure and its current generation.
   * @note Thread-safe for read operations.
   * @param entity Entity to validate
   * @return True if entity exists and is valid, false otherwise
   */
  [[nodiscard]] bool Validate(Entity entity) const noexcept;

  /**
   * @brief Gets the current number of living entities.
   * @details Returns count of entities that are currently alive.
   * @note Thread-safe.
   * @return Number of living entities
   */
  [[nodiscard]] size_t Count() const noexcept {
    return entity_count_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns current generation value for a given index (or
   * `kInvalidGeneration` if out of range).
   * @warning Triggers assertion if index is invalid.
   */
  [[nodiscard]] Entity::GenerationType GetGeneration(
      Entity::IndexType index) const noexcept;

private:
  [[nodiscard]] Entity CreateEntityWithId(Entity::IndexType index,
                                          Entity::GenerationType generation);

  /// Generation counter for each entity index
  std::vector<Entity::GenerationType> generations_;
  std::vector<Entity::IndexType> free_indices_;  ///< Recycled entity indices
  std::atomic<size_t> entity_count_{0};          ///< Number of living entities

  /// Next available index (thread-safe)
  std::atomic<Entity::IndexType> next_index_{0};

  /// Cursor for free list (negative means reserved entities)
  std::atomic<int64_t> free_cursor_{0};
};

inline EntityManager::EntityManager(const EntityManager& other)
    : generations_(other.generations_),
      free_indices_(other.free_indices_),
      entity_count_(other.entity_count_.load(std::memory_order_relaxed)),
      next_index_(other.next_index_.load(std::memory_order_relaxed)),
      free_cursor_(other.free_cursor_.load(std::memory_order_relaxed)) {}

inline EntityManager::EntityManager(EntityManager&& other) noexcept
    : generations_(std::move(other.generations_)),
      free_indices_(std::move(other.free_indices_)),
      entity_count_(other.entity_count_.load(std::memory_order_relaxed)),
      next_index_(other.next_index_.load(std::memory_order_relaxed)),
      free_cursor_(other.free_cursor_.load(std::memory_order_relaxed)) {
  other.entity_count_.store(0, std::memory_order_relaxed);
  other.next_index_.store(0, std::memory_order_relaxed);
  other.free_cursor_.store(0, std::memory_order_relaxed);
}

inline EntityManager& EntityManager::operator=(const EntityManager& other) {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  generations_ = other.generations_;
  free_indices_ = other.free_indices_;
  entity_count_.store(other.entity_count_.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
  next_index_.store(other.next_index_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
  free_cursor_.store(other.free_cursor_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);

  return *this;
}

inline EntityManager& EntityManager::operator=(EntityManager&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  generations_ = std::move(other.generations_);
  free_indices_ = std::move(other.free_indices_);
  entity_count_.store(other.entity_count_.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
  next_index_.store(other.next_index_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
  free_cursor_.store(other.free_cursor_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);

  other.entity_count_.store(0, std::memory_order_relaxed);
  other.next_index_.store(0, std::memory_order_relaxed);
  other.free_cursor_.store(0, std::memory_order_relaxed);

  return *this;
}

inline void EntityManager::Clear() noexcept {
  std::ranges::fill(generations_, Entity::kInvalidGeneration);
  free_indices_.clear();
  next_index_.store(0, std::memory_order_relaxed);
  free_cursor_.store(0, std::memory_order_relaxed);
  entity_count_.store(0, std::memory_order_relaxed);
}

template <typename F>
  requires std::invocable<F&, Entity>
inline void EntityManager::Flush(const F& callback) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::EntityManager::Flush");

  const auto current_next = next_index_.load(std::memory_order_relaxed);
  if (current_next == 0) {
    return;
  }

  // Ensure generations array covers all reserved indices.
  if (current_next > generations_.size()) {
    generations_.resize(current_next, Entity::kInvalidGeneration);
  }

  // Initialize any reserved-but-unflushed entries.
  // Reserved entities get generation 1 (as returned by ReserveEntity).
  // Entries that were already created via Create
  // will already have a valid generation (!= kInvalidGeneration), so we skip
  // them.
  size_t new_entities_count = 0;
  for (Entity::IndexType i = 0; i < current_next; ++i) {
    if (generations_[i] == Entity::kInvalidGeneration) {
      generations_[i] = 1;
      ++new_entities_count;
      callback(Entity{i, 1});
    }
  }

  if (new_entities_count > 0) {
    entity_count_.fetch_add(new_entities_count, std::memory_order_relaxed);
  }

  HELIOS_ECS_PROFILE_ZONE_VALUE(new_entities_count);
}

inline void EntityManager::Reserve(size_t count) {
  if (count > generations_.size()) {
    generations_.resize(count, Entity::kInvalidGeneration);
  }
  free_indices_.reserve(count);
}

inline Entity EntityManager::ReserveEntity() {
  // Atomically reserve an index by incrementing the next available index.
  // NOTE: Do NOT mutate metadata (e.g. `generations_` or `entity_count_`) here
  // because this function is thread-safe and may be called concurrently. The
  // actual metadata initialization for reserved indices is performed in
  // `Flush()` which must be called from the main thread.
  const Entity::IndexType index =
      next_index_.fetch_add(1, std::memory_order_relaxed);

  // Return a placeholder entity with generation 1.
  return {index, 1};
}

inline void EntityManager::Destroy(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (!Validate(entity)) [[unlikely]] {
    return;
  }

  const Entity::IndexType index = entity.Index();
  ++generations_[index];  // Invalidate entity
  free_indices_.push_back(index);

  free_cursor_.store(static_cast<int64_t>(free_indices_.size()),
                     std::memory_order_relaxed);
  entity_count_.fetch_sub(1, std::memory_order_relaxed);
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void EntityManager::Destroy(const R& entities) {
  if constexpr (std::ranges::sized_range<R>) {
    const size_t incoming = std::ranges::size(entities);
    free_indices_.reserve(free_indices_.size() + incoming);
  }

  // Process each entity: validate, increment generation, collect index.
  // We increment generation immediately so behaviour matches the single-entity
  // Destroy (i.e. duplicates in the input will fail the second validation).
  for (const auto& entity : entities) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
    if (!Validate(entity)) [[unlikely]] {
      // Skip entities already destroyed / wrong generation
      continue;
    }

    const Entity::IndexType index = entity.Index();
    ++generations_[index];  // Invalidate entity
    free_indices_.push_back(index);
    entity_count_.fetch_sub(1, std::memory_order_relaxed);
  }
}

template <typename OutputIt>
  requires std::output_iterator<OutputIt, Entity>
inline OutputIt EntityManager::Create(size_t count, OutputIt&& out) {
  if (count == 0) [[unlikely]] {
    return out;
  }

  // Try to satisfy as many as possible from the free list first
  size_t remaining = count;
  int64_t cursor = free_cursor_.load(std::memory_order_relaxed);
  // Use std::max with explicit signed type to avoid non-standard integer
  // literal suffix
  const size_t available_free =
      static_cast<size_t>(std::max<int64_t>(int64_t{0}, cursor));
  const size_t from_free_list = std::min(remaining, available_free);

  if (from_free_list > 0) {
    const int64_t new_cursor = cursor - static_cast<int64_t>(from_free_list);
    if (free_cursor_.compare_exchange_strong(cursor, new_cursor,
                                             std::memory_order_relaxed)) {
      // Successfully claimed indices from free list
      for (size_t i = 0; i < from_free_list; ++i) {
        const size_t free_index = static_cast<size_t>(new_cursor) + i;
        if (free_index >= free_indices_.size()) {
          continue;
        }

        const Entity::IndexType index = free_indices_[free_index];
        if (index >= generations_.size()) {
          continue;
        }

        const Entity::GenerationType generation = generations_[index];
        *out = CreateEntityWithId(index, generation);
        ++out;
      }
      remaining -= from_free_list;
    }
  }

  // Create new entities for remaining count
  if (remaining > 0) {
    const Entity::IndexType start_index = next_index_.fetch_add(
        static_cast<Entity::IndexType>(remaining), std::memory_order_relaxed);
    const Entity::IndexType end_index =
        start_index + static_cast<Entity::IndexType>(remaining);

    // Ensure generations array is large enough
    if (end_index > generations_.size()) {
      generations_.resize(end_index, Entity::kInvalidGeneration);
    }

    // Create entities with generation 1
    for (Entity::IndexType index = start_index; index < end_index; ++index) {
      generations_[index] = 1;
      *out = CreateEntityWithId(index, 1);
      ++out;
    }
  }

  return out;
}

inline bool EntityManager::Validate(Entity entity) const noexcept {
  if (!entity.Valid()) [[unlikely]] {
    return false;
  }

  const Entity::IndexType index = entity.Index();
  return index < generations_.size() &&
         generations_[index] == entity.Generation() &&
         generations_[index] != Entity::kInvalidGeneration;
}

inline Entity::GenerationType EntityManager::GetGeneration(
    Entity::IndexType index) const noexcept {
  HELIOS_ASSERT(index != Entity::kInvalidIndex, "Provided index is invalid!");
  return index < generations_.size() ? generations_[index]
                                     : Entity::kInvalidGeneration;
}

inline Entity EntityManager::Create() {
  // Reuse a free slot if available
  const int64_t cursor = free_cursor_.load(std::memory_order_relaxed);
  if (cursor > 0) {
    const int64_t new_cursor = cursor - 1;
    // Try to claim the top free slot
    int64_t expected = cursor;
    if (free_cursor_.compare_exchange_strong(expected, new_cursor,
                                             std::memory_order_relaxed)) {
      const Entity::IndexType index =
          free_indices_[static_cast<size_t>(new_cursor)];
      const Entity::GenerationType generation = generations_[index];
      return CreateEntityWithId(index, generation);
    }
  }

  // No free slot available — allocate a new index
  const Entity::IndexType index =
      next_index_.fetch_add(1, std::memory_order_relaxed);
  return CreateEntityWithId(index, 1);
}

inline Entity EntityManager::CreateEntityWithId(
    Entity::IndexType index, Entity::GenerationType generation) {
  if (index >= generations_.size()) {
    generations_.resize(index + 1, Entity::kInvalidGeneration);
  }

  generations_[index] = generation;
  entity_count_.fetch_add(1, std::memory_order_relaxed);

  return {index, generation};
}

}  // namespace helios::ecs
