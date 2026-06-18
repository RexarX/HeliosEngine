#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/component/archetype.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/manager.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/details/traits.hpp>
#include <helios/utils/common_traits.hpp>
#include <helios/utils/functional_adapters.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

namespace helios::ecs {

namespace details {

/**
 * @brief Fetches a single component for the current entity from the archetype,
 * returning the appropriate access type.
 * @details Handles all access patterns:
 * - `T&`       (mutable ref)   -> returns `T&`
 * - `const T&` (const ref)     -> returns `const T&`
 * - `T&&`      (rvalue ref)    -> returns `T&&` (via std::move)
 * - `T` / `const T` (value)   -> returns `T` by value
 * - `T*`       (nullable mutable pointer) -> returns `T*`, `nullptr` if absent
 * - `const T*` (nullable const pointer)   -> returns `const T*`, `nullptr` if
 * absent
 * @tparam AccessSpec The original access specifier from the query arg list
 * @param archetype The archetype the entity belongs to
 * @param entity The entity to fetch the component for
 * @param components The component manager
 * @return The component with the appropriate access type
 */
template <typename AccessSpec>
inline auto FetchComponent(const Archetype& archetype, Entity entity,
                           ComponentManager& components)
    -> ComponentAccessType_t<AccessSpec> {
  using RawType = ComponentTypeExtractor_t<AccessSpec>;
  using AccessType = ComponentAccessType_t<AccessSpec>;

  if constexpr (kIsPointerAccess<AccessSpec>) {
    // Nullable component: return pointer, nullptr if not present
    constexpr auto storage_type = ComponentStorageTypeOf<RawType>();

    if constexpr (storage_type == ComponentStorageType::kSparseSet) {
      if constexpr (std::is_const_v<std::remove_pointer_t<AccessType>>) {
        return std::as_const(components).TryGet<RawType>(entity);
      } else {
        return components.TryGet<RawType>(entity);
      }
    } else {
      if constexpr (std::is_const_v<std::remove_pointer_t<AccessType>>) {
        return std::as_const(archetype).TryGet<RawType>(entity);
      } else {
        // Safe: ComponentManager controls mutability; we receive const
        // Archetype& from the matching list but mutable access is routed
        // through the manager.
        return components.TryGet<RawType>(entity);
      }
    }
  } else if constexpr (TagComponentTrait<RawType>) {
    // Tag component: no data, but we still need to satisfy the return type.
    // For tag components the user typically does Get<Tag&> or Get<const Tag&>.
    // We return a reference to a static default-constructed instance.
    // This is safe because tag components are empty (size 0).
    static RawType tag_instance{};
    if constexpr (std::same_as<AccessType, RawType&&>) {
      return std::move(tag_instance);
    } else if constexpr (std::is_lvalue_reference_v<AccessType>) {
      return static_cast<AccessType>(tag_instance);
    } else {
      return RawType{};
    }
  } else if constexpr (ComponentStorageTypeOf<RawType>() ==
                       ComponentStorageType::kSparseSet) {
    // Sparse-set stored component
    if constexpr (std::same_as<AccessType, RawType&&>) {
      return std::move(components.Get<RawType>(entity));
    } else if constexpr (std::same_as<AccessType, RawType&>) {
      return components.Get<RawType>(entity);
    } else if constexpr (std::same_as<AccessType, const RawType&>) {
      return std::as_const(components).Get<RawType>(entity);
    } else {
      // Value copy
      return RawType(std::as_const(components).Get<RawType>(entity));
    }
  } else {
    // Archetype-stored component
    if constexpr (std::same_as<AccessType, RawType&&>) {
      return std::move(components.Get<RawType>(entity));
    } else if constexpr (std::same_as<AccessType, RawType&>) {
      return components.Get<RawType>(entity);
    } else if constexpr (std::same_as<AccessType, const RawType&>) {
      return std::as_const(components).Get<RawType>(entity);
    } else {
      // Value copy
      return RawType(std::as_const(components).Get<RawType>(entity));
    }
  }
}

/// @brief Const-world variant: always returns const access.
template <typename AccessSpec>
inline auto FetchComponentConst(const Archetype& archetype, Entity entity,
                                const ComponentManager& components)
    -> ComponentAccessType_t<AccessSpec> {
  using RawType = ComponentTypeExtractor_t<AccessSpec>;
  using AccessType = ComponentAccessType_t<AccessSpec>;

  if constexpr (kIsPointerAccess<AccessSpec>) {
    constexpr auto storage_type = ComponentStorageTypeOf<RawType>();
    if constexpr (storage_type == ComponentStorageType::kSparseSet) {
      return components.TryGet<RawType>(entity);
    } else {
      return archetype.TryGet<RawType>(entity);
    }
  } else if constexpr (TagComponentTrait<RawType>) {
    static constexpr RawType tag_instance{};
    if constexpr (std::is_lvalue_reference_v<AccessType>) {
      return static_cast<AccessType>(tag_instance);
    } else {
      return RawType{};
    }
  } else if constexpr (ComponentStorageTypeOf<RawType>() ==
                       ComponentStorageType::kSparseSet) {
    if constexpr (std::is_lvalue_reference_v<AccessType>) {
      return components.Get<RawType>(entity);
    } else {
      return RawType(components.Get<RawType>(entity));
    }
  } else {
    if constexpr (std::is_lvalue_reference_v<AccessType>) {
      return archetype.Get<RawType>(entity);
    } else {
      return RawType(archetype.Get<RawType>(entity));
    }
  }
}

}  // namespace details

/**
 * @brief Iterator for query results without entity information.
 * @details Provides bidirectional iteration over entities matching the query
 * criteria, returning tuples of requested component references/values. Supports
 * all access patterns including const ref, mutable ref, rvalue ref (move),
 * value copy, and nullable pointer (`T*`, `const T*`).
 * @note Not thread-safe.
 * @tparam IsConst Whether the query operates on a const world (read-only mode)
 * @tparam Components Requested component access types (may include const
 * qualifiers, references, `T*`, `const T*`)
 */
template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
class BasicQueryIter final : public utils::FunctionalAdapterBase<
                                 BasicQueryIter<IsConst, Components...>> {
public:
  using ComponentManagerType =
      std::conditional_t<IsConst, const ComponentManager, ComponentManager>;

  using iterator_category = std::bidirectional_iterator_tag;
  using iterator_concept = std::input_iterator_tag;
  using value_type = std::tuple<details::ComponentAccessType_t<Components>...>;
  using reference = value_type;
  using pointer = void;
  using difference_type = ptrdiff_t;

  /**
   * @brief Constructs iterator for query results.
   * @param archetypes Span of archetypes matching the query
   * @param components Component manager for accessing component data
   * @param archetype_index Starting archetype index
   * @param entity_index Starting entity index within archetype
   * @param without_types Span of component types to exclude (default empty)
   */
  BasicQueryIter(
      std::span<const std::reference_wrapper<const Archetype>> archetypes,
      ComponentManagerType& components, size_t archetype_index,
      size_t entity_index,
      std::span<const ComponentTypeIndex> without_types = {}) noexcept
      : archetypes_(archetypes),
        components_(components),
        archetype_index_(archetype_index),
        entity_index_(entity_index),
        without_types_(without_types) {
    AdvanceToValidEntity();
  }

  BasicQueryIter(const BasicQueryIter&) noexcept = default;
  BasicQueryIter(BasicQueryIter&&) noexcept = default;
  ~BasicQueryIter() noexcept = default;

  BasicQueryIter& operator=(const BasicQueryIter&) noexcept = default;
  BasicQueryIter& operator=(BasicQueryIter&&) noexcept = default;

  /**
   * @brief Advances iterator to next matching entity.
   * @return Reference to this iterator after advancement
   */
  BasicQueryIter& operator++();

  /**
   * @brief Advances iterator to next matching entity (postfix).
   * @return Copy of iterator before advancement
   */
  BasicQueryIter operator++(int);

  /**
   * @brief Moves iterator to previous matching entity.
   * @return Reference to this iterator after moving backward
   */
  BasicQueryIter& operator--();

  /**
   * @brief Moves iterator to previous matching entity (postfix).
   * @return Copy of iterator before moving backward
   */
  BasicQueryIter operator--(int);

  /**
   * @brief Dereferences iterator to get component tuple.
   * @details Returns tuple of component values/references/pointers for the
   * current entity.
   * @warning Triggers assertion if iterator is at end or in invalid state.
   * @return Tuple of component access values
   */
  [[nodiscard]] reference operator*() const;

  pointer operator->() const = delete;

  /**
   * @brief Compares iterators for equality.
   * @param other Iterator to compare with
   * @return True if iterators point to the same position
   */
  [[nodiscard]] bool operator==(const BasicQueryIter& other) const noexcept {
    return archetype_index_ == other.archetype_index_ &&
           entity_index_ == other.entity_index_;
  }

  /**
   * @brief Compares iterators for inequality.
   * @param other Iterator to compare with
   * @return True if iterators point to different positions
   */
  [[nodiscard]] bool operator!=(const BasicQueryIter& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief Returns copy of this iterator as begin iterator (for range-based
   * for).
   * @return Copy of this iterator
   */
  [[nodiscard]] BasicQueryIter begin() const noexcept { return *this; }

  /**
   * @brief Returns end iterator for this query.
   * @return End iterator (points past the last valid entity)
   */
  [[nodiscard]] BasicQueryIter end() const noexcept {
    return {archetypes_, components_.get(), archetypes_.size(), 0,
            without_types_};
  }

private:
  /// @brief Advances past empty archetypes to the next valid entity position.
  void AdvanceToValidEntity();

  /// @brief Checks if iterator has reached the end.
  [[nodiscard]] bool IsAtEnd() const noexcept {
    return archetype_index_ >= archetypes_.size();
  }

  std::span<const std::reference_wrapper<const Archetype>> archetypes_;
  std::reference_wrapper<ComponentManagerType> components_;
  size_t archetype_index_ = 0;
  size_t entity_index_ = 0;
  std::span<const ComponentTypeIndex> without_types_;
};

/**
 * @brief Iterator for query results without entity information.
 * @details Provides bidirectional iteration over entities matching the query
 * criteria, returning tuples of requested component references/values. Supports
 * all access patterns including const ref, mutable ref, rvalue ref (move),
 * value copy, and nullable pointer (`T*`, `const T*`).
 * @note Not thread-safe.
 * @tparam Components Requested component access types (may include const
 * qualifiers, references, `T*`, `const T*`)
 */
template <typename... Components>
using QueryIter = BasicQueryIter<false, Components...>;

/**
 * @brief Read-only iterator for query results without entity information.
 * @details Provides bidirectional iteration over entities matching the query
 * criteria, returning tuples of requested component references/values. Supports
 * all access patterns including const ref, mutable ref, rvalue ref (move),
 * value copy, and nullable pointer (`T*`, `const T*`).
 * @note Not thread-safe.
 * @tparam Components Requested component access types (may include const
 * qualifiers, references, `T*`, `const T*`)
 */
template <typename... Components>
using ReadOnlyQueryIter = BasicQueryIter<true, Components...>;

/**
 * @brief Iterator for query results including the entity.
 * @details Same as `BasicQueryIter` but each dereferenced value is a tuple
 * starting with the Entity, followed by the requested component access values.
 * @note Not thread-safe.
 * @tparam IsConst Whether the query operates on a const world
 * @tparam Components Requested component access types
 */
template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
class BasicQueryWithEntityIter final
    : public utils::FunctionalAdapterBase<
          BasicQueryWithEntityIter<IsConst, Components...>> {
public:
  using ComponentManagerType =
      std::conditional_t<IsConst, const ComponentManager, ComponentManager>;

  using iterator_category = std::bidirectional_iterator_tag;
  using iterator_concept = std::input_iterator_tag;
  using value_type =
      std::tuple<Entity, details::ComponentAccessType_t<Components>...>;
  using reference = value_type;
  using pointer = void;
  using difference_type = ptrdiff_t;

  /**
   * @brief Constructs iterator for query results with entity.
   * @param archetypes Span of archetypes matching the query
   * @param components Component manager for accessing component data
   * @param archetype_index Starting archetype index
   * @param entity_index Starting entity index within archetype
   * @param without_types Span of component types to exclude (default empty)
   */
  BasicQueryWithEntityIter(
      std::span<const std::reference_wrapper<const Archetype>> archetypes,
      ComponentManagerType& components, size_t archetype_index,
      size_t entity_index,
      std::span<const ComponentTypeIndex> without_types = {}) noexcept
      : archetypes_(archetypes),
        components_(components),
        archetype_index_(archetype_index),
        entity_index_(entity_index),
        without_types_(without_types) {
    AdvanceToValidEntity();
  }

  BasicQueryWithEntityIter(const BasicQueryWithEntityIter&) noexcept = default;
  BasicQueryWithEntityIter(BasicQueryWithEntityIter&&) noexcept = default;
  ~BasicQueryWithEntityIter() noexcept = default;

  BasicQueryWithEntityIter& operator=(
      const BasicQueryWithEntityIter&) noexcept = default;
  BasicQueryWithEntityIter& operator=(BasicQueryWithEntityIter&&) noexcept =
      default;

  /**
   * @brief Advances iterator to next matching entity.
   * @return Reference to this iterator after advancement
   */
  BasicQueryWithEntityIter& operator++();

  /**
   * @brief Advances iterator to next matching entity (postfix).
   * @return Copy of iterator before advancement
   */
  BasicQueryWithEntityIter operator++(int);

  /**
   * @brief Moves iterator to previous matching entity.
   * @return Reference to this iterator after moving backward
   */
  BasicQueryWithEntityIter& operator--();

  /**
   * @brief Moves iterator to previous matching entity (postfix).
   * @return Copy of iterator before moving backward
   */
  BasicQueryWithEntityIter operator--(int);

  /**
   * @brief Dereferences iterator to get entity and component tuple.
   * @warning Triggers assertion if iterator is at end or in invalid state.
   * @return Tuple of (Entity, component access values...)
   */
  [[nodiscard]] reference operator*() const;

  [[nodiscard]] bool operator==(
      const BasicQueryWithEntityIter& other) const noexcept {
    return archetype_index_ == other.archetype_index_ &&
           entity_index_ == other.entity_index_;
  }

  [[nodiscard]] bool operator!=(
      const BasicQueryWithEntityIter& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief Returns copy of this iterator as begin iterator (for range-based
   * for).
   * @return Copy of this iterator
   */
  [[nodiscard]] BasicQueryWithEntityIter begin() const noexcept {
    return *this;
  }

  /**
   * @brief Returns end iterator for this query.
   * @return End iterator (points past the last valid entity)
   */
  [[nodiscard]] BasicQueryWithEntityIter end() const noexcept {
    return {archetypes_, components_.get(), archetypes_.size(), 0,
            without_types_};
  }

private:
  void AdvanceToValidEntity();

  [[nodiscard]] bool IsAtEnd() const noexcept {
    return archetype_index_ >= archetypes_.size();
  }

  std::span<const std::reference_wrapper<const Archetype>> archetypes_;
  std::reference_wrapper<ComponentManagerType> components_;
  size_t archetype_index_ = 0;
  size_t entity_index_ = 0;
  std::span<const ComponentTypeIndex> without_types_;
};

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryIter<IsConst, Components...>::operator++()
    -> BasicQueryIter& {
  ++entity_index_;
  AdvanceToValidEntity();
  return *this;
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryIter<IsConst, Components...>::operator++(int)
    -> BasicQueryIter {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryIter<IsConst, Components...>::operator--()
    -> BasicQueryIter& {
  while (true) {
    if (entity_index_ > 0) {
      --entity_index_;
      return *this;
    }

    if (archetype_index_ == 0) {
      return *this;
    }

    --archetype_index_;
    if (archetype_index_ < archetypes_.size()) {
      entity_index_ = archetypes_[archetype_index_].get().EntityCount();
      if (entity_index_ > 0) {
        --entity_index_;
        return *this;
      }
    }
  }
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryIter<IsConst, Components...>::operator--(int)
    -> BasicQueryIter {
  auto copy = *this;
  --(*this);
  return copy;
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryIter<IsConst, Components...>::operator*() const
    -> reference {
  HELIOS_ASSERT(!IsAtEnd(), "Cannot dereference end iterator!");
  HELIOS_ASSERT(archetype_index_ < archetypes_.size(),
                "Archetype index out of bounds!");
  HELIOS_ASSERT(
      entity_index_ < archetypes_[archetype_index_].get().Entities().size(),
      "Entity index out of bounds!");

  const auto& archetype = archetypes_[archetype_index_].get();
  const Entity entity = archetype.Entities()[entity_index_];

  if constexpr (IsConst) {
    return reference(details::FetchComponentConst<Components>(
        archetype, entity, components_.get())...);
  } else {
    return reference(details::FetchComponent<Components>(archetype, entity,
                                                         components_.get())...);
  }
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline void BasicQueryIter<IsConst, Components...>::AdvanceToValidEntity() {
  while (!IsAtEnd()) {
    if (archetype_index_ < archetypes_.size() &&
        entity_index_ < archetypes_[archetype_index_].get().Entities().size()) {
      // Check that the current entity has all required (non-pointer) sparse
      // components. Archetype-stored required components are already guaranteed
      // by archetype matching.
      const Entity entity =
          archetypes_[archetype_index_].get().Entities()[entity_index_];
      bool has_all_sparse = true;
      (
          [&]() {
            if constexpr (!details::kIsPointerAccess<Components>) {
              using RawType = details::ComponentTypeExtractor_t<Components>;
              if constexpr (ComponentStorageTypeOf<RawType>() ==
                            ComponentStorageType::kSparseSet) {
                if (!std::as_const(components_.get())
                         .template HasComponent<RawType>(entity)) {
                  has_all_sparse = false;
                }
              }
            }
          }(),
          ...);
      if (has_all_sparse) {
        // Check that the entity does NOT have any excluded sparse component.
        bool has_none_excluded = std::ranges::all_of(
            without_types_, [&entity, this](const ComponentTypeIndex type) {
              const auto* meta =
                  std::as_const(components_.get()).MetadataByIndex(type);
              if (meta == nullptr) {
                return true;
              }
              if (meta->storage_type == ComponentStorageType::kSparseSet) {
                const auto* entry =
                    std::as_const(components_.get()).SparseEntry(type);
                if (entry != nullptr) {
                  bool has_comp = entry->Contains(entity);
                  if (has_comp) {
                    return false;
                  }
                }
              }
              return true;
            });
        if (has_none_excluded) {
          return;
        }
      }
      ++entity_index_;
      continue;
    }
    ++archetype_index_;
    entity_index_ = 0;
  }
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryWithEntityIter<IsConst, Components...>::operator++()
    -> BasicQueryWithEntityIter& {
  ++entity_index_;
  AdvanceToValidEntity();
  return *this;
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryWithEntityIter<IsConst, Components...>::operator++(int)
    -> BasicQueryWithEntityIter {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryWithEntityIter<IsConst, Components...>::operator--()
    -> BasicQueryWithEntityIter& {
  while (true) {
    if (entity_index_ > 0) {
      --entity_index_;
      return *this;
    }

    if (archetype_index_ == 0) {
      return *this;
    }

    --archetype_index_;
    if (archetype_index_ < archetypes_.size()) {
      entity_index_ = archetypes_[archetype_index_].get().EntityCount();
      if (entity_index_ > 0) {
        --entity_index_;
        return *this;
      }
    }
  }
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryWithEntityIter<IsConst, Components...>::operator--(int)
    -> BasicQueryWithEntityIter {
  auto copy = *this;
  --(*this);
  return copy;
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline auto BasicQueryWithEntityIter<IsConst, Components...>::operator*() const
    -> reference {
  HELIOS_ASSERT(!IsAtEnd(), "Cannot dereference end iterator!");
  HELIOS_ASSERT(archetype_index_ < archetypes_.size(),
                "Archetype index out of bounds!");
  HELIOS_ASSERT(
      entity_index_ < archetypes_[archetype_index_].get().Entities().size(),
      "Entity index out of bounds!");

  const auto& archetype = archetypes_[archetype_index_].get();
  const Entity entity = archetype.Entities()[entity_index_];

  if constexpr (IsConst) {
    return reference(entity, details::FetchComponentConst<Components>(
                                 archetype, entity, components_.get())...);
  } else {
    return reference(entity, details::FetchComponent<Components>(
                                 archetype, entity, components_.get())...);
  }
}

template <bool IsConst, typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (details::ValidComponentAccess<Components> && ...)
inline void
BasicQueryWithEntityIter<IsConst, Components...>::AdvanceToValidEntity() {
  while (!IsAtEnd()) {
    if (archetype_index_ < archetypes_.size() &&
        entity_index_ < archetypes_[archetype_index_].get().Entities().size()) {
      // Check that the current entity has all required (non-pointer) sparse
      // components.
      const Entity entity =
          archetypes_[archetype_index_].get().Entities()[entity_index_];
      bool has_all_sparse = true;
      (
          [&]() {
            if constexpr (!details::kIsPointerAccess<Components>) {
              using RawType = details::ComponentTypeExtractor_t<Components>;
              if constexpr (ComponentStorageTypeOf<RawType>() ==
                            ComponentStorageType::kSparseSet) {
                if (!std::as_const(components_.get())
                         .template HasComponent<RawType>(entity)) {
                  has_all_sparse = false;
                }
              }
            }
          }(),
          ...);
      if (has_all_sparse) {
        bool has_none_excluded = std::ranges::all_of(
            without_types_, [&entity, this](const ComponentTypeIndex type) {
              const auto* meta =
                  std::as_const(components_.get()).MetadataByIndex(type);
              if (meta == nullptr) {
                return true;
              }
              if (meta->storage_type == ComponentStorageType::kSparseSet) {
                const auto* entry =
                    std::as_const(components_.get()).SparseEntry(type);
                if (entry != nullptr && entry->Contains(entity)) {
                  return false;
                }
              }
              return true;
            });
        if (has_none_excluded) {
          return;
        }
      }
      ++entity_index_;
      continue;
    }
    ++archetype_index_;
    entity_index_ = 0;
  }
}

/**
 * @brief Iterator for query results including the entity.
 * @details Same as `BasicQueryIter` but each dereferenced value is a tuple
 * starting with the Entity, followed by the requested component access values.
 * @note Not thread-safe.
 * @tparam Components Requested component access types
 */
template <typename... Components>
using QueryWithEntityIter = BasicQueryWithEntityIter<false, Components...>;

/**
 * @brief Read-only iterator for query results including the entity.
 * @details Same as `BasicQueryIter` but each dereferenced value is a tuple
 * starting with the Entity, followed by the requested component access values.
 * @note Not thread-safe.
 * @tparam Components Requested component access types
 */
template <typename... Components>
using ReadOnlyQueryWithEntityIter =
    BasicQueryWithEntityIter<true, Components...>;

}  // namespace helios::ecs
