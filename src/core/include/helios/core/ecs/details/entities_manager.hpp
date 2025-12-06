#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Entity manager responsible for entity creation, destruction, and validation.
 * @details Manages entity lifecycle with generation counters to handle entity recycling safely.
 * @note Thread-safe only for validation operations.
 */
class Entities {
public:
  Entities() = default;
  Entities(const Entities&) = delete;
  Entities(Entities&& /*other*/) noexcept;
  ~Entities() = default;

  Entities& operator=(const Entities&) = delete;
  Entities& operator=(Entities&& /*other*/) noexcept;

  /**
   * @brief Creates reserved entities in the metadata.
   * @details Processes all reserved entity IDs and creates their metadata.
   * @note Not thread-safe, must be called from single thread only (typically during World::Update()).
   */
  void FlushReservedEntities();

  /**
   * @brief Clears all entities.
   * @details Destroys all entities and resets the manager state.
   * @note Not thread-safe, should be called from main thread only.
   */
  void Clear() noexcept;

  /**
   * @brief Reserves space for entities to minimize allocations.
   * @details Pre-allocates storage for the specified number of entities.
   * @note Not thread-safe, should be called from main thread only.
   * @param count Number of entities to reserve space for
   */
  void Reserve(size_t count);

  /**
   * @brief Reserves an entity ID that can be used immediately.
   * @details The actual entity creation is deferred until FlushReservedEntities() is called.
   * @note Thread-safe.
   * @return Reserved entity with valid index and generation
   */
  [[nodiscard]] Entity ReserveEntity();

  /**
   * @brief Creates a new entity.
   * @details Reuses dead entity slots when available, otherwise creates new ones.
   * @note Not thread-safe, should be called from main thread only during World::Update().
   * @return Newly created entity with valid index and generation
   */
  [[nodiscard]] Entity CreateEntity();

  /**
   * @brief Creates multiple entities at once and outputs them via an output iterator.
   * @details Batch creation is more efficient than individual calls. Entities are written
   * to the provided output iterator, avoiding internal allocations.
   * @note Not thread-safe, should be called from main thread only during World::Update().
   * @tparam OutputIt Output iterator type that accepts Entity values
   * @param count Number of entities to create
   * @param out Output iterator to write created entities to
   * @return Output iterator pointing past the last written entity
   *
   * @example
   * @code
   * std::vector<Entity> entities;
   * entities.reserve(100);
   * manager.CreateEntities(100, std::back_inserter(entities));
   *
   * // Or with pre-allocated array:
   * std::array<Entity, 10> arr;
   * manager.CreateEntities(10, arr.begin());
   *
   * // Or with span output:
   * Entity buffer[50];
   * manager.CreateEntities(50, std::begin(buffer));
   * @endcode
   */
  template <std::output_iterator<Entity> OutputIt>
  OutputIt CreateEntities(size_t count, OutputIt out);

  /**
   * @brief Destroys an entity by incrementing its generation.
   * @details Marks entity as dead and adds its index to the free list for reuse.
   * @note Not thread-safe, should be called from main thread only during World::Update().
   * Triggers assertion if entity is invalid.
   * Ignored if entity do not exist.
   * @param entity Entity to destroy
   */
  void Destroy(Entity entity);

  /**
   * @brief Destroys an entity by incrementing its generation.
   * @details Marks entity as dead and adds its index to the free list for reuse.
   * @note Not thread-safe, should be called from main thread only during World::Update().
   * Triggers assertion if any entity is invalid.
   * Entities that do not exist are ignored.
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
  [[nodiscard]] bool IsValid(Entity entity) const noexcept;

  /**
   * @brief Gets the current number of living entities.
   * @details Returns count of entities that are currently alive.
   * @note Thread-safe.
   * @return Number of living entities
   */
  [[nodiscard]] size_t Count() const noexcept { return entity_count_.load(std::memory_order_relaxed); }

  /**
   * @brief Returns current generation value for a given index (or kInvalidGeneration if out of range).
   * @warning Triggers assertion if index is invalid.
   */
  [[nodiscard]] Entity::GenerationType GetGeneration(Entity::IndexType index) const noexcept;

private:
  [[nodiscard]] Entity CreateEntityWithId(Entity::IndexType index, Entity::GenerationType generation);

  std::vector<Entity::GenerationType> generations_;  ///< Generation counter for each entity index
  std::vector<Entity::IndexType> free_indices_;      ///< Recycled entity indices
  std::atomic<size_t> entity_count_{0};              ///< Number of living entities
  std::atomic<Entity::IndexType> next_index_{0};     ///< Next available index (thread-safe)
  std::atomic<int64_t> free_cursor_{0};              ///< Cursor for free list (negative means reserved entities)
};

inline Entities::Entities(Entities&& other) noexcept
    : generations_(std::move(other.generations_)),
      free_indices_(std::move(other.free_indices_)),
      entity_count_(other.entity_count_.load(std::memory_order_relaxed)),
      next_index_(other.next_index_.load(std::memory_order_relaxed)),
      free_cursor_(other.free_cursor_.load(std::memory_order_relaxed)) {
  other.entity_count_.store(0, std::memory_order_relaxed);
  other.next_index_.store(0, std::memory_order_relaxed);
  other.free_cursor_.store(0, std::memory_order_relaxed);
}

inline Entities& Entities::operator=(Entities&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  generations_ = std::move(other.generations_);
  free_indices_ = std::move(other.free_indices_);
  entity_count_.store(other.entity_count_.load(std::memory_order_relaxed), std::memory_order_relaxed);
  next_index_.store(other.next_index_.load(std::memory_order_relaxed), std::memory_order_relaxed);
  free_cursor_.store(other.free_cursor_.load(std::memory_order_relaxed), std::memory_order_relaxed);

  return *this;
}

inline void Entities::Clear() noexcept {
  std::ranges::fill(generations_, Entity::kInvalidGeneration);
  free_indices_.clear();
  next_index_.store(0, std::memory_order_relaxed);
  free_cursor_.store(0, std::memory_order_relaxed);
  entity_count_.store(0, std::memory_order_relaxed);
}

inline void Entities::Reserve(size_t count) {
  if (count > generations_.size()) {
    generations_.resize(count, Entity::kInvalidGeneration);
  }
  free_indices_.reserve(count);
}

inline Entity Entities::ReserveEntity() {
  // Atomically reserve an index by incrementing the next available index.
  // NOTE: Do NOT mutate metadata (e.g. `generations_` or `entity_count_`) here because
  // this function is thread-safe and may be called concurrently. The actual metadata
  // initialization for reserved indices is performed in `FlushReservedEntities()` which
  // must be called from the main thread.
  const Entity::IndexType index = next_index_.fetch_add(1, std::memory_order_relaxed);

  // Return a placeholder entity with generation 1. This is only a reservation handle;
  // the real creation/metadata setup happens in FlushReservedEntities().
  return {index, 1};
}

inline void Entities::Destroy(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to destroy entity: Entity is invalid!");
  if (!IsValid(entity)) [[unlikely]] {
    return;
  }

  const Entity::IndexType index = entity.Index();
  ++generations_[index];  // Invalidate entity
  free_indices_.push_back(index);

  free_cursor_.store(static_cast<int64_t>(free_indices_.size()), std::memory_order_relaxed);
  entity_count_.fetch_sub(1, std::memory_order_relaxed);
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void Entities::Destroy(const R& entities) {
  // If the incoming range is sized we can reserve capacity up front to avoid reallocations.
  if constexpr (std::ranges::sized_range<R>) {
    const auto incoming = std::ranges::size(entities);
    free_indices_.reserve(free_indices_.size() + incoming);
  }

  // Small local buffer to collect indices we're going to free.
  // We collect them locally so we can do a single insert into free_indices_,
  // and do a single atomic update for free_cursor_ and entity_count_.
  std::vector<Entity::IndexType> batch;
  if constexpr (std::ranges::sized_range<R>) {
    batch.reserve(std::ranges::size(entities));
  } else {
    batch.reserve(16);  // small default to avoid too many reallocs for small ranges
  }

  // Process each entity: validate, increment generation, collect index.
  // We increment generation immediately so behaviour matches the single-entity Destroy
  // (i.e. duplicates in the input will fail the second validation).
  for (const Entity& entity : entities) {
    HELIOS_ASSERT(entity.Valid(), "Failed to destroy entities: Entity is invalid!");
    if (!IsValid(entity)) {
      // Skip entities already destroyed / wrong generation
      continue;
    }

    const Entity::IndexType index = entity.Index();
    ++generations_[index];  // Invalidate entity
    batch.push_back(index);
  }

  if (!batch.empty()) {
    free_indices_.insert_range(free_indices_.end(), batch);
    free_cursor_.store(static_cast<int64_t>(free_indices_.size()), std::memory_order_relaxed);
    entity_count_.fetch_sub(batch.size(), std::memory_order_relaxed);
  }
}

template <std::output_iterator<Entity> OutputIt>
inline OutputIt Entities::CreateEntities(size_t count, OutputIt out) {
  if (count == 0) [[unlikely]] {
    return out;
  }

  // Try to satisfy as many as possible from the free list first
  size_t remaining = count;
  int64_t cursor = free_cursor_.load(std::memory_order_relaxed);
  // Use std::max with explicit signed type to avoid non-standard integer literal suffix
  const size_t available_free = static_cast<size_t>(std::max<int64_t>(int64_t{0}, cursor));
  const size_t from_free_list = std::min(remaining, available_free);

  if (from_free_list > 0) {
    const int64_t new_cursor = cursor - static_cast<int64_t>(from_free_list);
    if (free_cursor_.compare_exchange_strong(cursor, new_cursor, std::memory_order_relaxed)) {
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
    const Entity::IndexType start_index =
        next_index_.fetch_add(static_cast<Entity::IndexType>(remaining), std::memory_order_relaxed);
    const Entity::IndexType end_index = start_index + static_cast<Entity::IndexType>(remaining);

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

inline bool Entities::IsValid(Entity entity) const noexcept {
  if (!entity.Valid()) [[unlikely]] {
    return false;
  }

  const Entity::IndexType index = entity.Index();
  return index < generations_.size() && generations_[index] == entity.Generation() &&
         generations_[index] != Entity::kInvalidGeneration;
}

inline Entity::GenerationType Entities::GetGeneration(Entity::IndexType index) const noexcept {
  HELIOS_ASSERT(index != Entity::kInvalidIndex, "Failed to get generation: index is invalid!");
  return index < generations_.size() ? generations_[index] : Entity::kInvalidGeneration;
}

inline Entity Entities::CreateEntityWithId(Entity::IndexType index, Entity::GenerationType generation) {
  if (index >= generations_.size()) {
    generations_.resize(index + 1, Entity::kInvalidGeneration);
  }

  generations_[index] = generation;
  entity_count_.fetch_add(1, std::memory_order_relaxed);

  return {index, generation};
}

}  // namespace helios::ecs::details
