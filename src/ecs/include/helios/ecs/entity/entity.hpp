#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#endif

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/**
 * @brief Unique identifier for entities with generation counter to handle
 * recycling.
 * @details Entity uses a combination of index and generation to provide stable
 * references even when entities are destroyed and their indices are recycled.
 *
 * Generations encode both a 31-bit reuse counter and a liveness bit:
 * - bit 31 (`kAliveBit`) is set when the slot is living;
 * - bits 0–30 hold the counter.
 *
 * Memory layout: 32-bit index + 32-bit generation = 64-bit total
 *
 * @note Individual Entity value objects are safe to copy freely. Concurrent
 * mutation of the same Entity instance is not synchronized.
 */
class Entity {
public:
  using IndexType = uint32_t;
  using GenerationType = uint32_t;

  static constexpr auto kInvalidIndex = std::numeric_limits<IndexType>::max();
  /// Unreachable sentinel: counter mask with the alive bit set.
  static constexpr auto kInvalidGeneration =
      std::numeric_limits<GenerationType>::max();
  /// Top bit: set when the generation represents a living entity.
  static constexpr GenerationType kAliveBit = 0x8000'0000U;
  /// Low 31 bits: reuse counter. `kCounterMask` itself is reserved so that
  /// `kInvalidGeneration` (`kCounterMask | kAliveBit`) stays unreachable.
  static constexpr GenerationType kCounterMask = 0x7FFF'FFFFU;
  /// Initial alive generation assigned to brand-new entity indices.
  static constexpr GenerationType kInitialAliveGeneration = kAliveBit | 1U;

  /**
   * @brief Constructs an invalid entity.
   * @details Creates an entity with invalid index and generation values.
   */
  constexpr Entity() noexcept = default;

  /**
   * @brief Constructs entity with specific index and generation.
   * @param index The entity index
   * @param generation The entity generation
   */
  constexpr Entity(IndexType index, GenerationType generation) noexcept
      : index_(index), generation_(generation) {}
  constexpr Entity(const Entity&) noexcept = default;
  constexpr Entity(Entity&&) noexcept = default;
  constexpr ~Entity() noexcept = default;

  constexpr Entity& operator=(const Entity&) noexcept = default;
  constexpr Entity& operator=(Entity&&) noexcept = default;

  constexpr bool operator==(const Entity&) const noexcept = default;
  constexpr bool operator!=(const Entity&) const noexcept = default;
  constexpr bool operator<(const Entity& other) const noexcept;

  /**
   * @brief Checks if the entity handle is structurally valid.
   * @details An entity is structurally valid if both its index and generation
   * are not the reserved invalid sentinels. This does **not** mean the entity
   * is currently alive in an `EntityManager` — use
   * `EntityManager::Validate()` for that.
   * @return True if entity has usable index and generation fields
   */
  [[nodiscard]] constexpr bool Valid() const noexcept {
    return index_ != kInvalidIndex && generation_ != kInvalidGeneration;
  }

  /**
   * @brief Checks whether the generation encodes a living entity.
   * @details Uses the alive bit in the generation encoding. This reflects the
   * handle's generation value, not whether the entity is currently alive in an
   * `EntityManager` — use `EntityManager::Validate()` for that.
   * @return True if generation is alive-encoded and not the invalid sentinel
   */
  [[nodiscard]] constexpr bool Alive() const noexcept;

  /**
   * @brief Generates a hash value for this entity.
   * @details Combines index and generation into a 64-bit hash value.
   * Invalid entities always return hash value of 0.
   * @return Hash combining generation (high bits) and index (low bits)
   */
  [[nodiscard]] constexpr size_t Hash() const noexcept;

  /**
   * @brief Gets the reuse counter encoded in the generation.
   * @details Masks off `kAliveBit`. This is the 31-bit counter in bits 0–30,
   * not the packed value returned by `Generation()`. Alive and free encodings
   * of the same slot share this counter until the next destroy bump.
   * @return Reuse counter, or `kCounterMask` when generation is the invalid
   * sentinel
   */
  [[nodiscard]] constexpr GenerationType ReuseCount() const noexcept {
    return generation_ & kCounterMask;
  }

  /**
   * @brief Gets the index component of the entity.
   * @details The index identifies the entity's storage location in sparse
   * arrays.
   * @return Entity index, or `kInvalidIndex` if entity is invalid
   */
  [[nodiscard]] constexpr IndexType Index() const noexcept { return index_; }

  /**
   * @brief Gets the generation component of the entity.
   * @details Returns the bit-encoded generation (alive bit + counter), not a
   * raw reuse count. Stale handles keep their old encoding after recycling.
   * @return Entity generation, or `kInvalidGeneration` if entity is invalid
   */
  [[nodiscard]] constexpr GenerationType Generation() const noexcept {
    return generation_;
  }

private:
  IndexType index_ = kInvalidIndex;  ///< Entity index for storage lookup
  GenerationType generation_ =
      kInvalidGeneration;  ///< Encoded generation (alive bit + counter)
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

  auto hash = static_cast<size_t>(index_);
  hash ^= static_cast<size_t>(generation_) +
          static_cast<size_t>(0x9e3779b97f4a7c15ULL) + (hash << 6U) +
          (hash >> 2U);
  return hash == 0 ? 1 : hash;
}

/**
 * @brief Checks if generation is considered alive.
 * @details Requires the alive bit and rejects the unreachable invalid
 * sentinel.
 * @param gen Generation encoding to check
 * @return True if generation is alive and not invalid
 */
[[nodiscard]] constexpr bool IsAliveGeneration(
    Entity::GenerationType gen) noexcept {
  return (gen & Entity::kAliveBit) != 0U && gen != Entity::kInvalidGeneration;
}

constexpr bool Entity::Alive() const noexcept {
  return IsAliveGeneration(generation_);
}

/**
 * @brief Advances a generation toward the requested liveness state.
 * @details One counter increment occurs per full reuse cycle, on the
 * alive->free transition. Free->alive only sets the alive bit so a reserved
 * handle can carry the future alive value while storage still holds the free
 * encoding (making pre-`Flush` validation fail).
 *
 * The reserved counter value `kCounterMask` is skipped so
 * `kInvalidGeneration` stays unreachable.
 *
 * @param gen Current generation encoding
 * @param alive Target liveness (`true` = free->alive, `false` = alive->free)
 * @return Next generation encoding for the requested state
 */
[[nodiscard]] constexpr Entity::GenerationType NextGeneration(
    Entity::GenerationType gen, bool alive) noexcept {
  if (alive) {
    return (gen & Entity::kCounterMask) | Entity::kAliveBit;
  }

  auto counter = (gen & Entity::kCounterMask) + 1U;
  if (counter == Entity::kCounterMask) {
    counter = 0U;
  }
  return counter & Entity::kCounterMask;
}

}  // namespace helios::ecs

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::ecs::Entity> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(const helios::ecs::Entity& entity,
                               format_context& ctx) {
    return format_to(ctx.out(), "Entity{{index: {}, generation: {}}}",
                     entity.Index(), entity.Generation());
  }
};

template <>
struct hash<helios::ecs::Entity> {
  constexpr size_t operator()(helios::ecs::Entity entity) const noexcept {
    return entity.Hash();
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
