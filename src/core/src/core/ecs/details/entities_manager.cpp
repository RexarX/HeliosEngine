#include <helios/core/ecs/details/entities_manager.hpp>

#include <helios/core/ecs/entity.hpp>
#include <helios/core_pch.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace helios::ecs::details {

void Entities::FlushReservedEntities() {
  const Entity::IndexType current_next_index = next_index_.load(std::memory_order_relaxed);

  // Ensure generations array is large enough
  if (current_next_index > generations_.size()) {
    generations_.resize(current_next_index, Entity::kInvalidGeneration);
  }

  // Initialize any indices that were reserved but not yet created
  size_t new_entities_count = 0;
  for (size_t i = 0; i < current_next_index; ++i) {
    if (generations_[i] == Entity::kInvalidGeneration) {
      generations_[i] = 1;  // Start with generation 1 for new entities
      ++new_entities_count;
    }
  }

  // Update entity count to reflect newly created entities
  if (new_entities_count > 0) {
    entity_count_.fetch_add(new_entities_count, std::memory_order_relaxed);
  }
}

Entity Entities::CreateEntity() {
  Entity::IndexType index = Entity::kInvalidIndex;
  Entity::GenerationType generation = Entity::kInvalidGeneration;

  // Try to reuse a recycled index first
  int64_t cursor = free_cursor_.load(std::memory_order_relaxed);
  if (cursor > 0) {
    // Attempt to pop from free list
    const int64_t new_cursor = cursor - 1;
    if (free_cursor_.compare_exchange_strong(cursor, new_cursor, std::memory_order_relaxed)) {
      // Successfully claimed an index from the free list
      const auto free_index = static_cast<size_t>(new_cursor);
      if (free_index < free_indices_.size()) {
        index = free_indices_[free_index];
        if (index < generations_.size()) {
          generation = generations_[index];
          return CreateEntityWithId(index, generation);
        }
      }
    }
  }

  // No free indices available, allocate a new one
  index = next_index_.fetch_add(1, std::memory_order_relaxed);

  // Ensure generations array is large enough
  if (index >= generations_.size()) {
    generations_.resize(index + 1, Entity::kInvalidGeneration);
  }

  // Start with generation 1 for new entities
  generation = 1;
  generations_[index] = generation;

  return CreateEntityWithId(index, generation);
}

}  // namespace helios::ecs::details
