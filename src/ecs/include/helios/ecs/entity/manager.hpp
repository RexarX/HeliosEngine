#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory_resource>
#include <ranges>
#include <vector>
#endif
#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/entity/entity.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/**
 * @brief Entity manager responsible for entity creation, destruction, and
 * validation.
 * @details Manages entity lifecycle with bit-encoded generations: the top bit
 * marks liveness and the low 31 bits count reuse cycles. Recycled slots store a
 * free (non-alive) generation after `Destroy`; `ReserveEntity` returns the
 * corresponding future alive generation so `Validate` rejects the handle until
 * `Flush` publishes that alive value.
 * @note Partially thread-safe under a phase contract:
 * - During a stable reservation/read phase (no structural mutation),
 *   `ReserveEntity`, `Validate`, `GetGeneration`, `NeedsFlush`, and `Count` may
 *   run concurrently. Generation elements are accessed via `std::atomic_ref`.
 * - Structural operations (`Flush`, `Destroy`, `Create`, `Reserve`, `Clear`,
 *   copy/move, and any vector growth) require all concurrent readers/reservers
 *   to be quiescent first.
 */
class EntityManager {
public:
  EntityManager() = default;

  /**
   * @brief Constructs an entity manager using `resource` for internal storage.
   * @param resource Memory resource for generation and free-list vectors.
   */
  explicit EntityManager(std::pmr::memory_resource* resource);
  EntityManager(std::nullptr_t) = delete;
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
   * @brief Reserves an entity ID that can be used immediately as a handle.
   * @details Prefers recycled indices from the free list, returning the future
   * alive generation (which deliberately does **not** match storage until
   * `Flush`). Otherwise allocates a new index with
   * `Entity::kInitialAliveGeneration`. Metadata materialization is deferred
   * until `Flush()`.
   * @note Thread-safe during a stable reservation phase (no concurrent
   * `Flush`/`Destroy`/`Create`/`Reserve`/copy/move/growth).
   * @return Reserved entity with valid index and (future) alive generation
   */
  [[nodiscard]] Entity ReserveEntity();

  /**
   * @brief Creates a new entity.
   * @details Reuses dead entity slots when available (performing the
   * free->alive transition synchronously), otherwise creates new ones.
   * @note Not thread-safe.
   * @warning Triggers assertion if reserved entities have not been flushed
   * (`NeedsFlush()`).
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
   * @warning Triggers assertion if reserved entities have not been flushed
   * (`NeedsFlush()`).
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
   * std::array<Entity, 10> arr = {};
   * manager.Create(10, arr.begin());
   * @endcode
   */
  template <typename OutputIt>
    requires std::output_iterator<OutputIt, Entity>
  OutputIt Create(size_t count, OutputIt&& out);

  /**
   * @brief Destroys an entity by advancing its generation to a free encoding.
   * @details Marks entity as dead (stores the next free generation) and adds
   * its index to the free list for reuse. Entities that do not exist or are
   * already destroyed are ignored.
   * @note Not thread-safe.
   * @warning Triggers assertion if entity is invalid, or if reserved entities
   * have not been flushed (`NeedsFlush()`).
   * @param entity Entity to destroy
   */
  void Destroy(Entity entity);

  /**
   * @brief Destroys entities by advancing each generation to a free encoding.
   * @details Marks entities as dead and adds their indices to the free list for
   * reuse. Entities that do not exist or are already destroyed are ignored.
   * @note Not thread-safe.
   * @warning Triggers assertion if any entity is invalid, or if reserved
   * entities have not been flushed (`NeedsFlush()`).
   * @tparam R Range type containing Entity elements
   * @param entities Entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void Destroy(const R& entities);

  /**
   * @brief Checks if an entity exists and is currently alive.
   * @details Requires structural validity, an in-range index, exact generation
   * match, and an alive-encoded stored generation.
   * @note Thread-safe during a stable reservation/read phase. Must not overlap
   * structural mutation that may reallocate `generations_`.
   * @param entity Entity to validate
   * @return True if entity exists and is alive, false otherwise
   */
  [[nodiscard]] bool Validate(Entity entity) const noexcept;

  /**
   * @brief Checks whether reserved entities are awaiting `Flush()`.
   * @details Returns true when `ReserveEntity()` has claimed freelist slots or
   * new indices that have not yet been materialized by `Flush()`.
   * @note Thread-safe during a stable reservation phase (free-list structure
   * must not be mutated concurrently).
   * @return True if there are reserved entities to flush
   */
  [[nodiscard]] bool NeedsFlush() const noexcept {
    return free_cursor_.load(std::memory_order_relaxed) !=
           static_cast<int64_t>(free_indices_.size());
  }

  /**
   * @brief Returns the encoded generation stored for `index`.
   * @details The returned value includes the alive/free bit tagging — it is
   * not a raw reuse counter. Out-of-range indices yield `kInvalidGeneration`.
   * @note Thread-safe during a stable reservation/read phase.
   * @warning Triggers assertion if index is invalid.
   */
  [[nodiscard]] Entity::GenerationType GetGeneration(
      Entity::IndexType index) const noexcept;

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
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor (or copied from the
   * source manager)
   */
  [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() const noexcept {
    return generations_.get_allocator().resource();
  }

private:
  [[nodiscard]] Entity CreateEntityWithId(Entity::IndexType index,
                                          Entity::GenerationType generation);

  /**
   * @brief Atomic view of a generation slot for concurrent access.
   * @warning `index` must be in range and the vector must not reallocate for
   * the duration of the concurrent access phase.
   */
  [[nodiscard]] auto GenRef(Entity::IndexType index) noexcept
      -> std::atomic_ref<Entity::GenerationType>;

  /**
   * @brief Atomic view of a generation slot for concurrent access.
   * @warning `index` must be in range and the vector must not reallocate for
   * the duration of the concurrent access phase.
   */
  [[nodiscard]] auto GenRef(Entity::IndexType index) const noexcept
      -> std::atomic_ref<Entity::GenerationType>;

  /// Generation per entity index. Concurrent element access MUST go through
  /// `GenRef()` while a reservation/read phase is active. Plain vector
  /// copy/move/resize is only legal while concurrent accessors are quiescent.
  std::pmr::vector<Entity::GenerationType> generations_;
  std::pmr::vector<Entity::IndexType>
      free_indices_;                     ///< Recycled entity indices
  std::atomic<size_t> entity_count_{0};  ///< Number of living entities

  /// Next available index (thread-safe)
  std::atomic<Entity::IndexType> next_index_{0};

  /// Cursor for free list (negative means reserved brand-new entities)
  std::atomic<int64_t> free_cursor_{0};
};

template <typename F>
  requires std::invocable<F&, Entity>
inline void EntityManager::Flush(const F& callback) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::EntityManager::Flush");

  const int64_t current_free_cursor =
      free_cursor_.load(std::memory_order_relaxed);

  size_t new_free_cursor = 0;
  size_t new_entities_count = 0;

  if (current_free_cursor >= 0) {
    new_free_cursor = static_cast<size_t>(current_free_cursor);
  } else {
    // Negative free_cursor: allocate brand-new indices beyond next_index_.
    const Entity::IndexType old_next =
        next_index_.load(std::memory_order_relaxed);
    const auto freshly_reserved =
        static_cast<Entity::IndexType>(-current_free_cursor);
    const Entity::IndexType new_next = old_next + freshly_reserved;

    if (new_next > generations_.size()) {
      generations_.resize(new_next, Entity::kInvalidGeneration);
    }

    for (Entity::IndexType index = old_next; index < new_next; ++index) {
      GenRef(index).store(Entity::kInitialAliveGeneration,
                          std::memory_order_relaxed);
      callback(Entity{index, Entity::kInitialAliveGeneration});
    }

    new_entities_count += freshly_reserved;
    next_index_.store(new_next, std::memory_order_relaxed);
    new_free_cursor = 0;
  }

  // Reserved freelist entries live in free_indices_[new_free_cursor .. end).
  // Storage still holds the free generation from Destroy; publish the alive
  // encoding before invoking the callback.
  HELIOS_ASSERT(new_free_cursor <= free_indices_.size(),
                "free_cursor is out of sync with free list!");
  new_free_cursor = std::min(new_free_cursor, free_indices_.size());
  const size_t recycled_reserved = free_indices_.size() - new_free_cursor;
  for (size_t i = new_free_cursor; i < free_indices_.size(); ++i) {
    const Entity::IndexType index = free_indices_[i];
    const Entity::GenerationType free_gen =
        GenRef(index).load(std::memory_order_relaxed);
    const Entity::GenerationType alive_gen =
        NextGeneration(free_gen, /*alive=*/true);
    GenRef(index).store(alive_gen, std::memory_order_relaxed);
    callback(Entity{index, alive_gen});
  }
  new_entities_count += recycled_reserved;

  free_indices_.resize(new_free_cursor);
  free_cursor_.store(static_cast<int64_t>(new_free_cursor),
                     std::memory_order_relaxed);

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

template <typename OutputIt>
  requires std::output_iterator<OutputIt, Entity>
inline OutputIt EntityManager::Create(size_t count, OutputIt&& out) {
  HELIOS_ASSERT(!NeedsFlush(), "Flush reserved entities before creation!");

  if (count == 0) [[unlikely]] {
    return out;
  }

  // Try to satisfy as many as possible from the free list first
  size_t remaining = count;
  int64_t cursor = free_cursor_.load(std::memory_order_relaxed);
  // Use std::max with explicit signed type to avoid non-standard integer
  // literal suffix
  const auto available_free = static_cast<size_t>(std::max<int64_t>(0, cursor));
  const size_t from_free_list = std::min(remaining, available_free);

  if (from_free_list > 0) {
    const int64_t new_cursor = cursor - static_cast<int64_t>(from_free_list);
    if (free_cursor_.compare_exchange_strong(cursor, new_cursor,
                                             std::memory_order_relaxed)) {
      // Successfully claimed indices from free list
      for (size_t i = 0; i < from_free_list; ++i) {
        const auto free_index = static_cast<size_t>(new_cursor) + i;
        if (free_index >= free_indices_.size()) {
          continue;
        }

        const Entity::IndexType index = free_indices_[free_index];
        if (index >= generations_.size()) {
          continue;
        }

        const Entity::GenerationType free_gen =
            GenRef(index).load(std::memory_order_relaxed);
        const Entity::GenerationType alive_gen =
            NextGeneration(free_gen, /*alive=*/true);
        *out = CreateEntityWithId(index, alive_gen);
        ++out;
      }
      remaining -= from_free_list;
      free_indices_.resize(
          static_cast<size_t>(std::max(int64_t{0}, new_cursor)));
      free_cursor_.store(static_cast<int64_t>(free_indices_.size()),
                         std::memory_order_relaxed);
    }
  }

  // Create new entities for remaining count
  if (remaining > 0) {
    const auto start_index = next_index_.fetch_add(
        static_cast<Entity::IndexType>(remaining), std::memory_order_relaxed);
    const auto end_index =
        start_index + static_cast<Entity::IndexType>(remaining);

    // Ensure generations array is large enough
    if (end_index > generations_.size()) {
      generations_.resize(end_index, Entity::kInvalidGeneration);
    }

    for (Entity::IndexType index = start_index; index < end_index; ++index) {
      GenRef(index).store(Entity::kInitialAliveGeneration,
                          std::memory_order_relaxed);
      *out = CreateEntityWithId(index, Entity::kInitialAliveGeneration);
      ++out;
    }
  }

  return out;
}

inline Entity EntityManager::ReserveEntity() {
  // Atomically claim one slot: freelist first, then brand-new indices.
  // Do NOT mutate `generations_` or `entity_count_` here — concurrent callers
  // only touch `free_cursor_` (and read stable free_indices_/generations_
  // slots). Metadata is materialized in `Flush()`.
  const int64_t n = free_cursor_.fetch_sub(1, std::memory_order_relaxed);
  if (n > 0) {
    const Entity::IndexType index = free_indices_[static_cast<size_t>(n - 1)];
    // Destroy published the free generation before this index hit the free
    // list; reserve hands out the future alive generation, which deliberately
    // does NOT match storage until Flush writes it in.
    const Entity::GenerationType free_gen =
        GenRef(index).load(std::memory_order_relaxed);
    return {index, NextGeneration(free_gen, /*alive=*/true)};
  }

  // `free_cursor_` is 0 or negative: hand out a fresh index at/after
  // `next_index_`. As the cursor goes more negative, indices extend farther.
  const auto index = static_cast<Entity::IndexType>(
      static_cast<int64_t>(next_index_.load(std::memory_order_relaxed)) - n);
  return {index, Entity::kInitialAliveGeneration};
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void EntityManager::Destroy(const R& entities) {
  HELIOS_ASSERT(!NeedsFlush(), "Flush reserved entities before destruction!");

  if constexpr (std::ranges::sized_range<R>) {
    const size_t incoming = std::ranges::size(entities);
    free_indices_.reserve(free_indices_.size() + incoming);
  }

  // Process each entity: validate, store free generation, collect index.
  // We transition immediately so behaviour matches the single-entity Destroy
  // (i.e. duplicates in the input will fail the second validation).
  for (const auto& entity : entities) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
    if (!Validate(entity)) [[unlikely]] {
      // Skip entities already destroyed / wrong generation
      continue;
    }

    const Entity::IndexType index = entity.Index();
    GenRef(index).store(NextGeneration(entity.Generation(), /*alive=*/false),
                        std::memory_order_relaxed);
    free_indices_.push_back(index);
    entity_count_.fetch_sub(1, std::memory_order_relaxed);
  }

  free_cursor_.store(static_cast<int64_t>(free_indices_.size()),
                     std::memory_order_relaxed);
}

inline Entity::GenerationType EntityManager::GetGeneration(
    Entity::IndexType index) const noexcept {
  HELIOS_ASSERT(index != Entity::kInvalidIndex, "Provided index is invalid!");
  if (index >= generations_.size()) {
    return Entity::kInvalidGeneration;
  }
  return GenRef(index).load(std::memory_order_relaxed);
}

inline bool EntityManager::Validate(Entity entity) const noexcept {
  if (!entity.Valid()) [[unlikely]] {
    return false;
  }

  const Entity::IndexType index = entity.Index();
  if (index >= generations_.size()) {
    return false;
  }

  const Entity::GenerationType stored =
      GenRef(index).load(std::memory_order_relaxed);
  return stored == entity.Generation() && IsAliveGeneration(stored);
}

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
