#pragma once

#include <helios/core_pch.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

namespace helios::ecs {

/**
 * @brief Unique identifier for entities with generation counter to handle recycling.
 * @details Entity uses a combination of index and generation to provide stable references
 * even when entities are destroyed and their indices are recycled. The generation counter
 * ensures that old entity references become invalid when the index is reused.
 *
 * Memory layout: 32-bit index + 32-bit generation = 64-bit total
 *
 * @note This class is thread-safe for all operations.
 */
class Entity {
public:
  using IndexType = uint32_t;
  using GenerationType = uint32_t;

  static constexpr IndexType kInvalidIndex = std::numeric_limits<IndexType>::max();
  static constexpr GenerationType kInvalidGeneration = 0;

  /**
   * @brief Constructs an invalid entity.
   * @details Creates an entity with invalid index and generation values.
   */
  constexpr Entity() noexcept = default;

  /**
   * @brief Constructs entity with specific index and generation.
   * @details Private constructor used by entity manager to create valid entities.
   * @param index The entity index
   * @param generation The entity generation
   */
  constexpr Entity(IndexType index, GenerationType generation) noexcept : index_(index), generation_(generation) {}
  constexpr Entity(const Entity&) noexcept = default;
  constexpr Entity(Entity&&) noexcept = default;
  constexpr ~Entity() noexcept = default;

  constexpr Entity& operator=(const Entity&) noexcept = default;
  constexpr Entity& operator=(Entity&&) noexcept = default;

  constexpr bool operator==(const Entity&) const noexcept = default;
  constexpr bool operator!=(const Entity&) const noexcept = default;
  constexpr bool operator<(const Entity& other) const noexcept;

  /**
   * @brief Checks if the entity is valid.
   * @details An entity is valid if both its index and generation are not the reserved invalid values.
   * @return True if entity has valid index and generation, false otherwise
   */
  [[nodiscard]] constexpr bool Valid() const noexcept {
    return index_ != kInvalidIndex && generation_ != kInvalidGeneration;
  }

  /**
   * @brief Generates a hash value for this entity.
   * @details Combines index and generation into a 64-bit hash value.
   * Invalid entities always return hash value of 0.
   * @return Hash combining generation (high bits) and index (low bits)
   */
  [[nodiscard]] constexpr size_t Hash() const noexcept;

  /**
   * @brief Gets the index component of the entity.
   * @details The index identifies the entity's storage location in sparse arrays.
   * @return Entity index, or kInvalidIndex if entity is invalid
   */
  [[nodiscard]] constexpr IndexType Index() const noexcept { return index_; }

  /**
   * @brief Gets the generation component of the entity.
   * @details The generation counter prevents use of stale entity references after recycling.
   * @return Entity generation, or kInvalidGeneration if entity is invalid
   */
  [[nodiscard]] constexpr GenerationType Generation() const noexcept { return generation_; }

private:
  IndexType index_ = kInvalidIndex;                 ///< Entity index for storage lookup
  GenerationType generation_ = kInvalidGeneration;  ///< Generation counter for recycling safety
};

constexpr bool Entity::operator<(const Entity& other) const noexcept {
  if (index_ != other.index_) {
    return index_ < other.index_;
  }
  return generation_ < other.generation_;
}

constexpr size_t Entity::Hash() const noexcept {
  if (!Valid()) [[unlikely]] {
    return 0;
  }

  return (static_cast<size_t>(generation_) << (sizeof(size_t) / 2)) | static_cast<size_t>(index_);
}

}  // namespace helios::ecs

/**
 * @brief Standard hash specialization for Entity.
 * @details Enables use of Entity as key in standard hash-based containers.
 */
template <>
struct std::hash<helios::ecs::Entity> {
  constexpr size_t operator()(const helios::ecs::Entity& entity) const noexcept { return entity.Hash(); }
};
