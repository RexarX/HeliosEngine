#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/utils/common_traits.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <span>
#include <vector>

namespace helios::ecs {

/**
 * @brief Unique identifier for an archetype, represented as a sorted set of
 * component type indices.
 * @details An archetype is defined by the exact set of component types it
 * contains. The type indices are kept sorted to ensure consistent identity
 * regardless of insertion order. Only archetype-storage components are tracked
 * here; sparse-set components are managed separately.
 */
class ArchetypeId {
public:
  using size_type = size_t;

  /// @brief Constructs an empty archetype id (no components).
  constexpr ArchetypeId() = default;

  /**
   * @brief Constructs an archetype id from a span of component type indices.
   * @details Indices are copied and sorted internally. Duplicates are removed.
   * @param types Span of component type indices
   */
  explicit constexpr ArchetypeId(std::vector<ComponentTypeIndex> types);

  /**
   * @brief Constructs an archetype id from an initializer list of component
   * type indices.
   * @details Indices are copied and sorted internally. Duplicates are removed.
   * @param types Initializer list of component type indices
   */
  constexpr ArchetypeId(std::initializer_list<ComponentTypeIndex> types);
  constexpr ArchetypeId(const ArchetypeId&) = default;
  constexpr ArchetypeId(ArchetypeId&&) noexcept = default;
  constexpr ~ArchetypeId() = default;

  constexpr ArchetypeId& operator=(const ArchetypeId&) = default;
  constexpr ArchetypeId& operator=(ArchetypeId&&) noexcept = default;

  /**
   * @brief Creates an archetype id from a set of component types.
   * @tparam Ts Component types
   * @return Archetype id containing the type indices of all provided types
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  [[nodiscard]] static constexpr ArchetypeId From();

  /**
   * @brief Returns a new archetype id with the given component type added.
   * @details If the type is already present, returns a copy of this id.
   * @param type Component type index to add
   * @return New archetype id with the type included
   */
  [[nodiscard]] constexpr ArchetypeId With(ComponentTypeIndex type) const;

  /**
   * @brief Returns a new archetype id with the given component type removed.
   * @details If the type is not present, returns a copy of this id.
   * @param type Component type index to remove
   * @return New archetype id without the type
   */
  [[nodiscard]] constexpr ArchetypeId Without(ComponentTypeIndex type) const;

  /**
   * @brief Returns a new archetype id with the given component type added.
   * @tparam T Component type to add
   * @return New archetype id with the type included
   */
  template <ComponentTrait T>
  [[nodiscard]] constexpr ArchetypeId With() const {
    return With(ComponentTypeIndex::From<T>());
  }

  /**
   * @brief Returns a new archetype id with the given component type removed.
   * @tparam T Component type to remove
   * @return New archetype id without the type
   */
  template <ComponentTrait T>
  [[nodiscard]] constexpr ArchetypeId Without() const {
    return Without(ComponentTypeIndex::From<T>());
  }

  constexpr bool operator==(const ArchetypeId& other) const noexcept {
    return types_ == other.types_;
  }
  constexpr bool operator!=(const ArchetypeId& other) const noexcept {
    return !(*this == other);
  }
  constexpr bool operator<(const ArchetypeId& other) const noexcept {
    return std::ranges::lexicographical_compare(types_, other.types_);
  }

  /**
   * @brief Checks if this archetype id contains the given component type.
   * @param type Component type index to check
   * @return True if the type is present
   */
  [[nodiscard]] constexpr bool Contains(
      ComponentTypeIndex type) const noexcept {
    return std::ranges::binary_search(types_, type);
  }

  /**
   * @brief Checks if this archetype id contains the given component type.
   * @tparam T Component type to check
   * @return True if the type is present
   */
  template <ComponentTrait T>
  [[nodiscard]] constexpr bool Contains() const noexcept {
    return Contains(ComponentTypeIndex::From<T>());
  }

  /**
   * @brief Checks if this archetype id contains all the given component types.
   * @tparam R Range of `ComponentTypeIndex` to check
   * @param types Range of component type indices to check
   * @return True if all types are present
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, ComponentTypeIndex>
  [[nodiscard]] constexpr bool ContainsAll(const R& types) const noexcept {
    return std::ranges::all_of(
        types, [this](ComponentTypeIndex type) { return Contains(type); });
  }

  /**
   * @brief Checks if this archetype id contains any of the given component
   * types.
   * @tparam R Range of `ComponentTypeIndex` to check
   * @param types Range of component type indices to check
   * @return True if any type is present
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, ComponentTypeIndex>
  [[nodiscard]] constexpr bool ContainsAny(const R& types) const noexcept {
    return std::ranges::any_of(
        types, [this](ComponentTypeIndex type) { return Contains(type); });
  }

  /**
   * @brief Checks if this archetype id contains none of the given component
   * types.
   * @tparam R Range of `ComponentTypeIndex` to check
   * @param types Range of component type indices to check
   * @return True if no type is present
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, ComponentTypeIndex>
  [[nodiscard]] constexpr bool ContainsNone(const R& types) const noexcept {
    return std::ranges::none_of(
        types, [this](ComponentTypeIndex type) { return Contains(type); });
  }

  /**
   * @brief Gets the sorted component type indices.
   * @return Span of component type indices in this archetype
   */
  [[nodiscard]] constexpr auto Types() const noexcept
      -> std::span<const ComponentTypeIndex> {
    return types_;
  }

  /**
   * @brief Gets the precomputed hash of this archetype id.
   * @return Hash value for this archetype id
   */
  [[nodiscard]] constexpr size_t Hash() const noexcept { return hash_; }

  /**
   * @brief Checks if this archetype id has no component types.
   * @return True if this archetype id is empty (no component types)
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return types_.empty(); }

  /**
   * @brief Gets the number of component types in this archetype id.
   * @return Count of component types in this archetype id
   */
  [[nodiscard]] constexpr size_type Size() const noexcept {
    return types_.size();
  }

private:
  constexpr void SortAndDeduplicate();
  constexpr void ComputeHash() noexcept;

  std::vector<ComponentTypeIndex> types_;
  size_t hash_ = 0;
};

constexpr ArchetypeId::ArchetypeId(std::vector<ComponentTypeIndex> types)
    : types_(std::move(types)) {
  SortAndDeduplicate();
  ComputeHash();
}

constexpr ArchetypeId::ArchetypeId(
    std::initializer_list<ComponentTypeIndex> types)
    : types_(types) {
  SortAndDeduplicate();
  ComputeHash();
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
constexpr ArchetypeId ArchetypeId::From() {
  if constexpr (sizeof...(Ts) == 0) {
    return {};
  } else {
    return ArchetypeId({ComponentTypeIndex::From<Ts>()...});
  }
}

constexpr ArchetypeId ArchetypeId::With(ComponentTypeIndex type) const {
  if (Contains(type)) {
    return *this;
  }

  ArchetypeId result;
  result.types_.reserve(types_.size() + 1);
  result.types_ = types_;
  auto it = std::ranges::lower_bound(result.types_, type);
  result.types_.insert(it, type);
  result.ComputeHash();
  return result;
}

constexpr ArchetypeId ArchetypeId::Without(ComponentTypeIndex type) const {
  if (!Contains(type)) {
    return *this;
  }

  ArchetypeId result;
  result.types_.reserve(types_.size() - 1);
  for (const auto& ty : types_) {
    if (ty != type) {
      result.types_.push_back(ty);
    }
  }
  result.ComputeHash();
  return result;
}

constexpr void ArchetypeId::SortAndDeduplicate() {
  std::ranges::sort(types_);
  auto [first, last] = std::ranges::unique(types_);
  types_.erase(first, last);
}

constexpr void ArchetypeId::ComputeHash() noexcept {
  size_t seed = 0;
  for (const auto type_index : types_) {
    // Use a good hash combiner (boost-style).
    seed ^=
        type_index.Hash() + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  }
  hash_ = seed;
}

}  // namespace helios::ecs

namespace std {

template <>
struct hash<helios::ecs::ArchetypeId> {
  constexpr size_t operator()(
      const helios::ecs::ArchetypeId& id) const noexcept {
    return id.Hash();
  }
};

}  // namespace std
