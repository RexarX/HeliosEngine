#include <pch.hpp>

#include <helios/ecs/entity/manager.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/entity/entity.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <utility>

namespace helios::ecs {

EntityManager::EntityManager(std::pmr::memory_resource* resource)
    : generations_(resource), free_indices_(resource) {}

EntityManager::EntityManager(const EntityManager& other)
    : generations_(other.generations_, other.generations_.get_allocator()),
      free_indices_(other.free_indices_, other.free_indices_.get_allocator()),
      entity_count_(other.entity_count_.load(std::memory_order_relaxed)),
      next_index_(other.next_index_.load(std::memory_order_relaxed)),
      free_cursor_(other.free_cursor_.load(std::memory_order_relaxed)) {}

EntityManager::EntityManager(EntityManager&& other) noexcept
    : generations_(std::move(other.generations_)),
      free_indices_(std::move(other.free_indices_)),
      entity_count_(other.entity_count_.load(std::memory_order_relaxed)),
      next_index_(other.next_index_.load(std::memory_order_relaxed)),
      free_cursor_(other.free_cursor_.load(std::memory_order_relaxed)) {
  other.entity_count_.store(0, std::memory_order_relaxed);
  other.next_index_.store(0, std::memory_order_relaxed);
  other.free_cursor_.store(0, std::memory_order_relaxed);
}

EntityManager& EntityManager::operator=(const EntityManager& other) {
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

EntityManager& EntityManager::operator=(EntityManager&& other) noexcept {
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

void EntityManager::Clear() noexcept {
  std::ranges::fill(generations_, Entity::kInvalidGeneration);
  free_indices_.clear();
  next_index_.store(0, std::memory_order_relaxed);
  free_cursor_.store(0, std::memory_order_relaxed);
  entity_count_.store(0, std::memory_order_relaxed);
}

Entity EntityManager::Create() {
  HELIOS_ASSERT(!NeedsFlush(), "Flush reserved entities before creation!");

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
      free_indices_.pop_back();
      free_cursor_.store(static_cast<int64_t>(free_indices_.size()),
                         std::memory_order_relaxed);
      const Entity::GenerationType free_gen =
          GenRef(index).load(std::memory_order_relaxed);
      return CreateEntityWithId(index,
                                NextGeneration(free_gen, /*alive=*/true));
    }
  }

  // No free slot available — allocate a new index
  const Entity::IndexType index =
      next_index_.fetch_add(1, std::memory_order_relaxed);
  return CreateEntityWithId(index, Entity::kInitialAliveGeneration);
}

void EntityManager::Destroy(Entity entity) {
  HELIOS_ASSERT(!NeedsFlush(), "Flush reserved entities before destruction!");
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if (!Validate(entity)) [[unlikely]] {
    return;
  }

  const Entity::IndexType index = entity.Index();
  GenRef(index).store(NextGeneration(entity.Generation(), /*alive=*/false),
                      std::memory_order_relaxed);
  free_indices_.push_back(index);

  free_cursor_.store(static_cast<int64_t>(free_indices_.size()),
                     std::memory_order_relaxed);
  entity_count_.fetch_sub(1, std::memory_order_relaxed);
}

Entity EntityManager::CreateEntityWithId(Entity::IndexType index,
                                         Entity::GenerationType generation) {
  if (index >= generations_.size()) {
    generations_.resize(index + 1, Entity::kInvalidGeneration);
  }

  GenRef(index).store(generation, std::memory_order_relaxed);
  entity_count_.fetch_add(1, std::memory_order_relaxed);

  return {index, generation};
}

auto EntityManager::GenRef(Entity::IndexType index) noexcept
    -> std::atomic_ref<Entity::GenerationType> {
  static_assert(std::atomic_ref<Entity::GenerationType>::required_alignment <=
                alignof(Entity::GenerationType));
  return std::atomic_ref<Entity::GenerationType>(generations_[index]);
}

auto EntityManager::GenRef(Entity::IndexType index) const noexcept
    -> std::atomic_ref<Entity::GenerationType> {
  static_assert(std::atomic_ref<Entity::GenerationType>::required_alignment <=
                alignof(Entity::GenerationType));
  // libc++ does not support atomic_ref<const T>::load(), const_cast is a
  // workaround
  return std::atomic_ref<Entity::GenerationType>(
      const_cast<Entity::GenerationType&>(generations_[index]));
}

}  // namespace helios::ecs
