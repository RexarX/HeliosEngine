#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/access_policy.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/archetype.hpp>
#include <helios/core/ecs/details/components_manager.hpp>
#include <helios/core/ecs/details/query_cache.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>
#include <helios/core/utils/functional_adapters.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs {

namespace details {

/**
 * @brief Type trait to extract the underlying component type from const/reference wrappers.
 * @details Handles cases like:
 * - const Position& -> Position
 * - Position& -> Position
 * - Position -> Position
 * @tparam T Type to extract from (may be const-qualified or reference type)
 */
template <typename T>
struct ComponentTypeExtractor {
  using type = std::remove_cvref_t<T>;
};

template <typename T>
using ComponentTypeExtractor_t = typename ComponentTypeExtractor<T>::type;

/**
 * @brief Helper to check if a type represents a const-qualified component access.
 * @details Determines if the requested access is const-qualified for read-only operations.
 * @tparam T Type to check for const qualification
 */
template <typename T>
constexpr bool IsConstComponent = std::is_const_v<std::remove_reference_t<T>>;

/**
 * @brief Helper to check if all component types are const-qualified.
 * @details Used to determine if an iterator can work with const Components.
 * @tparam Components Component types to check
 */
template <typename... Components>
constexpr bool AllComponentsConst = (IsConstComponent<Components> && ...);

/**
 * @brief Helper to detect if World type is const.
 * @details Used for compile-time validation of const World access.
 * @tparam T World type
 */
template <typename T>
constexpr bool IsConstWorld = WorldType<T> && std::is_const_v<std::remove_reference_t<T>>;

/**
 * @brief Concept for valid World/Component access combinations.
 * @details Ensures that:
 * - Non-const World allows any component access (const or mutable)
 * - Const World only allows const component access
 * @tparam WorldT World type (World or const World)
 * @tparam Components Component access types to validate
 */
template <typename WorldT, typename... Components>
concept ValidWorldComponentAccess =
    WorldType<WorldT> && (!IsConstWorld<WorldT> ||                 // Non-const World allows anything
                          (IsConstComponent<Components> && ...));  // Const World requires all const Components

/**
 * @brief Helper to determine the proper return type for component access.
 * @details Returns appropriate reference type based on const qualification:
 * - const T& for const-qualified access
 * - T& for mutable access
 * - T for value types (copy)
 * @tparam T Requested component access type
 */
template <typename T>
using ComponentAccessType =
    std::conditional_t<std::is_reference_v<T>,
                       T,  // Keep reference as-is (const T& or T&)
                       std::conditional_t<std::is_const_v<T>,
                                          const ComponentTypeExtractor_t<T>&,  // const T -> const T&
                                          ComponentTypeExtractor_t<T>&         // T -> T&
                                          >>;

/**
 * @brief Concept for STL-compatible allocators.
 * @tparam Alloc Allocator type to check
 * @tparam T Value type for the allocator
 */
template <typename Alloc, typename T>
concept AllocatorFor = requires(Alloc alloc, size_t n) {
  { alloc.allocate(n) } -> std::same_as<T*>;
  { alloc.deallocate(std::declval<T*>(), n) };
  typename Alloc::value_type;
  requires std::same_as<typename Alloc::value_type, T>;
};

/**
 * @brief Iterator for query results without entity information.
 * @details Provides forward iteration over entities matching the query criteria,
 * returning tuples of requested component references. Supports const-qualified
 * component access for read-only operations.
 * @note Not thread-safe.
 * @tparam Components Requested component access types (may include const qualifiers)
 */
template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
class QueryIterator : public utils::FunctionalAdapterBase<QueryIterator<Components...>> {
public:
  /// Components manager type based on whether all components are const-qualified
  using ComponentsType =
      std::conditional_t<AllComponentsConst<Components...>, const details::Components, details::Components>;

  // Standard iterator type aliases required by C++20 concepts
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::tuple<ComponentAccessType<Components>...>;
  using difference_type = ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs iterator for query results.
   * @details Initializes iterator to point to the first matching entity.
   * @param archetypes Span of archetypes matching the query
   * @param components Component manager for accessing component data
   * @param archetype_index Starting archetype index (default: 0)
   * @param entity_index Starting entity index within archetype (default: 0)
   */
  QueryIterator(std::span<const std::reference_wrapper<const Archetype>> archetypes, ComponentsType& components,
                size_t archetype_index = 0, size_t entity_index = 0);
  QueryIterator(const QueryIterator&) noexcept = default;
  QueryIterator(QueryIterator&&) noexcept = default;
  ~QueryIterator() noexcept = default;

  QueryIterator& operator=(const QueryIterator&) noexcept = default;
  QueryIterator& operator=(QueryIterator&&) noexcept = default;

  /**
   * @brief Advances iterator to next matching entity.
   * @return Reference to this iterator after advancement
   */
  QueryIterator& operator++();

  /**
   * @brief Advances iterator to next matching entity.
   * @return Copy of iterator before advancement
   */
  QueryIterator operator++(int);

  /**
   * @brief Moves iterator to previous matching entity.
   * @return Reference to this iterator after moving backward
   */
  QueryIterator& operator--();

  /**
   * @brief Moves iterator to previous matching entity.
   * @return Copy of iterator before moving backward
   */
  QueryIterator operator--(int);

  /**
   * @brief Dereferences iterator to get component tuple.
   * @details Returns tuple of component references for the current entity.
   * Handles const-qualified access appropriately.
   * @warning Triggers assertion if iterator is at end or in invalid state.
   * @return Tuple of component references with appropriate const qualifiers
   */
  [[nodiscard]] value_type operator*() const;

  /**
   * @brief Compares iterators for equality.
   * @details Two iterators are equal if they point to the same archetype and entity indices.
   * @param other Iterator to compare with
   * @return True if iterators are equal, false otherwise
   */
  [[nodiscard]] bool operator==(const QueryIterator& other) const noexcept {
    return archetype_index_ == other.archetype_index_ && entity_index_ == other.entity_index_;
  }

  /**
   * @brief Compares iterators for inequality.
   * @param other Iterator to compare with
   * @return True if iterators are not equal, false otherwise
   */
  [[nodiscard]] bool operator!=(const QueryIterator& other) const noexcept { return !(*this == other); }

  /**
   * @brief Returns iterator to the beginning.
   * @return Copy of this iterator
   */
  [[nodiscard]] QueryIterator begin() const noexcept { return *this; }

  /**
   * @brief Returns iterator to the end.
   * @return End iterator
   */
  [[nodiscard]] QueryIterator end() const noexcept { return {archetypes_, components_.get(), archetypes_.size(), 0}; }

private:
  /**
   * @brief Advances iterator position to next valid entity.
   * @details Skips empty archetypes and moves to valid entity positions.
   */
  void AdvanceToValidEntity();

  /**
   * @brief Checks if iterator has reached the end.
   * @return True if iterator is past the last valid entity, false otherwise
   */
  [[nodiscard]] bool IsAtEnd() const noexcept { return archetype_index_ >= archetypes_.size(); }

  std::span<const std::reference_wrapper<const Archetype>> archetypes_;  ///< Archetypes matching the query
  std::reference_wrapper<ComponentsType> components_;                    ///< Component manager for data access
  size_t archetype_index_ = 0;                                           ///< Current archetype index
  size_t entity_index_ = 0;                                              ///< Current entity index within archetype
};

/**
 * @brief Iterator for query results with entity information.
 * @details Provides forward iteration over entities matching the query criteria,
 * returning tuples that include the entity followed by requested component references.
 * @note Not thread-safe.
 * @tparam Components Requested component access types (may include const qualifiers)
 */
template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
class QueryWithEntityIterator : public utils::FunctionalAdapterBase<QueryWithEntityIterator<Components...>> {
public:
  /// Components manager type based on whether all components are const-qualified
  using ComponentsType =
      std::conditional_t<AllComponentsConst<Components...>, const details::Components, details::Components>;

  // Standard iterator type aliases required by C++20 concepts
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::tuple<Entity, ComponentAccessType<Components>...>;
  using difference_type = ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs iterator for query results with entity.
   * @details Initializes iterator to point to the first matching entity.
   * @param archetypes Span of archetypes matching the query
   * @param components Component manager for accessing component data
   * @param archetype_index Starting archetype index (default: 0)
   * @param entity_index Starting entity index within archetype (default: 0)
   */
  QueryWithEntityIterator(std::span<const std::reference_wrapper<const Archetype>> archetypes,
                          ComponentsType& components, size_t archetype_index = 0, size_t entity_index = 0);
  QueryWithEntityIterator(const QueryWithEntityIterator&) noexcept = default;
  QueryWithEntityIterator(QueryWithEntityIterator&&) noexcept = default;
  ~QueryWithEntityIterator() noexcept = default;

  QueryWithEntityIterator& operator=(const QueryWithEntityIterator&) noexcept = default;
  QueryWithEntityIterator& operator=(QueryWithEntityIterator&&) noexcept = default;

  /**
   * @brief Advances iterator to next matching entity.
   * @return Reference to this iterator after advancement
   */
  QueryWithEntityIterator& operator++();

  /**
   * @brief Advances iterator to next matching entity.
   * @return Copy of iterator before advancement
   */
  QueryWithEntityIterator operator++(int);

  /**
   * @brief Moves iterator to previous matching entity.
   * @return Reference to this iterator after moving backward
   */
  QueryWithEntityIterator& operator--();

  /**
   * @brief Moves iterator to previous matching entity.
   * @return Copy of iterator before moving backward
   */
  QueryWithEntityIterator operator--(int);

  /**
   * @brief Dereferences iterator to get entity and component tuple.
   * @details Returns tuple starting with entity followed by component references.
   * @warning Triggers assertion if iterator is at end or in invalid state.
   * @return Tuple of entity and component references
   */
  [[nodiscard]] value_type operator*() const;

  /**
   * @brief Compares iterators for equality.
   * @param other Iterator to compare with
   * @return True if iterators are equal, false otherwise
   */
  [[nodiscard]] bool operator==(const QueryWithEntityIterator& other) const noexcept {
    return archetype_index_ == other.archetype_index_ && entity_index_ == other.entity_index_;
  }

  /**
   * @brief Compares iterators for inequality.
   * @param other Iterator to compare with
   * @return True if iterators are not equal, false otherwise
   */
  [[nodiscard]] bool operator!=(const QueryWithEntityIterator& other) const noexcept { return !(*this == other); }

  /**
   * @brief Returns iterator to the beginning.
   * @return Copy of this iterator
   */
  [[nodiscard]] QueryWithEntityIterator begin() const noexcept { return *this; }

  /**
   * @brief Returns iterator to the end.
   * @return End iterator
   */
  [[nodiscard]] QueryWithEntityIterator end() const noexcept {
    return {archetypes_, components_.get(), archetypes_.size(), 0};
  }

private:
  /**
   * @brief Advances iterator position to next valid entity.
   * @details Skips empty archetypes and moves to valid entity positions.
   */
  void AdvanceToValidEntity();

  /**
   * @brief Checks if iterator has reached the end.
   * @return True if iterator is past the last valid entity, false otherwise
   */
  [[nodiscard]] bool IsAtEnd() const noexcept { return archetype_index_ >= archetypes_.size(); }

  std::span<const std::reference_wrapper<const Archetype>> archetypes_;  ///< Archetypes matching the query
  std::reference_wrapper<ComponentsType> components_;                    ///< Component manager for data access
  size_t archetype_index_ = 0;                                           ///< Current archetype index
  size_t entity_index_ = 0;                                              ///< Current entity index within archetype
};

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
QueryIterator<Components...>::QueryIterator(std::span<const std::reference_wrapper<const Archetype>> archetypes,
                                            ComponentsType& components, size_t archetype_index, size_t entity_index)
    : archetypes_(archetypes), components_(components), archetype_index_(archetype_index), entity_index_(entity_index) {
  AdvanceToValidEntity();
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryIterator<Components...>::operator++() -> QueryIterator& {
  ++entity_index_;
  AdvanceToValidEntity();
  return *this;
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryIterator<Components...>::operator++(int) -> QueryIterator {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryIterator<Components...>::operator--() -> QueryIterator& {
  // If at end or beginning of current archetype, need to go back
  while (true) {
    if (entity_index_ > 0) {
      --entity_index_;
      return *this;
    }

    // Need to move to previous archetype
    if (archetype_index_ == 0) {
      // Already at beginning, can't go back further
      return *this;
    }

    --archetype_index_;
    // Move to last entity in previous archetype
    if (archetype_index_ < archetypes_.size()) {
      entity_index_ = archetypes_[archetype_index_].get().EntityCount();
      if (entity_index_ > 0) {
        --entity_index_;
        return *this;
      }
    }
  }
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryIterator<Components...>::operator--(int) -> QueryIterator {
  auto copy = *this;
  --(*this);
  return copy;
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryIterator<Components...>::operator*() const -> value_type {
  HELIOS_ASSERT(!IsAtEnd(), "Cannot dereference end iterator!");
  HELIOS_ASSERT(archetype_index_ < archetypes_.size(), "Archetype index out of bounds!");
  HELIOS_ASSERT(entity_index_ < archetypes_[archetype_index_].get().Entities().size(), "Entity index out of bounds!");

  const auto& archetype = archetypes_[archetype_index_].get();
  const Entity entity = archetype.Entities()[entity_index_];

  return {[&]<typename ComponentType = ComponentTypeExtractor_t<Components>>() -> ComponentAccessType<Components> {
    return static_cast<ComponentAccessType<Components>>(components_.get().template GetComponent<ComponentType>(entity));
  }()...};
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline void QueryIterator<Components...>::AdvanceToValidEntity() {
  while (!IsAtEnd()) {
    if (archetype_index_ < archetypes_.size() &&
        entity_index_ < archetypes_[archetype_index_].get().Entities().size()) {
      // Found valid entity
      return;
    }

    // Move to next archetype
    ++archetype_index_;
    entity_index_ = 0;
  }
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
QueryWithEntityIterator<Components...>::QueryWithEntityIterator(
    std::span<const std::reference_wrapper<const Archetype>> archetypes, ComponentsType& components,
    size_t archetype_index, size_t entity_index)
    : archetypes_(archetypes), components_(components), archetype_index_(archetype_index), entity_index_(entity_index) {
  AdvanceToValidEntity();
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryWithEntityIterator<Components...>::operator++() -> QueryWithEntityIterator& {
  ++entity_index_;
  AdvanceToValidEntity();
  return *this;
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryWithEntityIterator<Components...>::operator++(int) -> QueryWithEntityIterator {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryWithEntityIterator<Components...>::operator--() -> QueryWithEntityIterator& {
  // If at end or beginning of current archetype, need to go back
  while (true) {
    if (entity_index_ > 0) {
      --entity_index_;
      return *this;
    }

    // Need to move to previous archetype
    if (archetype_index_ == 0) {
      // Already at beginning, can't go back further
      return *this;
    }

    --archetype_index_;
    // Move to last entity in previous archetype
    if (archetype_index_ < archetypes_.size()) {
      entity_index_ = archetypes_[archetype_index_].get().EntityCount();
      if (entity_index_ > 0) {
        --entity_index_;
        return *this;
      }
    }
  }
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryWithEntityIterator<Components...>::operator--(int) -> QueryWithEntityIterator {
  auto copy = *this;
  --(*this);
  return copy;
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto QueryWithEntityIterator<Components...>::operator*() const -> value_type {
  HELIOS_ASSERT(!IsAtEnd(), "Cannot dereference end iterator!");
  HELIOS_ASSERT(archetype_index_ < archetypes_.size(), "Archetype index out of bounds!");
  HELIOS_ASSERT(entity_index_ < archetypes_[archetype_index_].get().Entities().size(), "Entity index out of bounds!");

  const auto& archetype = archetypes_[archetype_index_].get();
  const Entity entity = archetype.Entities()[entity_index_];

  return {entity,
          [&]<typename ComponentType = ComponentTypeExtractor_t<Components>>() -> ComponentAccessType<Components> {
            return static_cast<ComponentAccessType<Components>>(
                components_.get().template GetComponent<ComponentType>(entity));
          }()...};
}

template <ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline void QueryWithEntityIterator<Components...>::AdvanceToValidEntity() {
  while (!IsAtEnd()) {
    if (archetype_index_ < archetypes_.size() &&
        entity_index_ < archetypes_[archetype_index_].get().Entities().size()) {
      // Found valid entity
      return;
    }

    // Move to next archetype
    ++archetype_index_;
    entity_index_ = 0;
  }
}

}  // namespace details

// Forward declarations

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
class BasicQuery;

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
class BasicQueryWithEntity;

/**
 * @brief Wrapper for queries that include entity in iteration.
 * @details Provides iteration interface that returns both entity and components.
 * Supports const-qualified component access for read-only operations.
 * @note Not thread-safe.
 * @tparam WorldT World type (World or const World), must satisfy WorldType concept
 * @tparam Allocator Allocator type for internal storage
 * @tparam Components Requested component access types (may include const qualifiers)
 */
template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
class BasicQueryWithEntity {
public:
  using iterator = details::QueryWithEntityIterator<Components...>;
  using value_type = std::iter_value_t<details::QueryWithEntityIterator<Components...>>;
  using difference_type = std::iter_difference_t<details::QueryWithEntityIterator<Components...>>;
  using pointer = typename iterator::pointer;
  using reference = std::iter_reference_t<details::QueryWithEntityIterator<Components...>>;
  using allocator_type = Allocator;

  /**
   * @brief Constructs entity-aware query wrapper.
   * @param query Reference to the underlying query
   */
  explicit BasicQueryWithEntity(BasicQuery<WorldT, Allocator, Components...>& query) noexcept : query_(query) {}
  BasicQueryWithEntity(const BasicQueryWithEntity&) = delete;
  BasicQueryWithEntity(BasicQueryWithEntity&&) = delete;
  ~BasicQueryWithEntity() noexcept = default;

  BasicQueryWithEntity& operator=(const BasicQueryWithEntity&) = delete;
  BasicQueryWithEntity& operator=(BasicQueryWithEntity&&) = delete;

  /**
   * @brief Collects all results into a vector.
   * @return Vector of tuples containing entity and components
   *
   * @example
   * @code
   * auto all_results = query.WithEntity().Collect();
   * @endcode
   */
  [[nodiscard]] auto Collect() const -> std::vector<value_type>;

  /**
   * @brief Collects all results into a vector using a custom allocator.
   * @details Eagerly evaluates the query and returns all matching results using the provided allocator.
   * Useful for temporary allocations with frame allocators.
   * @warning When using a frame allocator (via SystemContext::MakeFrameAllocator()),
   * the returned data is only valid for the current frame. All pointers and references
   * to frame-allocated data become invalid after the frame ends.
   * Do not store frame-allocated data in components, resources, or any persistent storage.
   * @tparam ResultAlloc STL-compatible allocator type for value_type
   * @param alloc Allocator instance to use for the result vector
   * @return Vector of tuples containing entity and components, using the provided allocator
   *
   * @example
   * @code
   * void MySystem::Update(app::SystemContext& ctx) {
   *   auto query = ctx.Query().Get<Position&>();
   *   auto alloc = ctx.MakeFrameAllocator<decltype(query)::value_type>();
   *   auto results = query.WithEntity().CollectWith(alloc);
   *   // WARNING: Do not store 'results' or pointers to its contents beyond this frame!
   * }
   * @endcode
   */
  template <typename ResultAlloc>
    requires std::same_as<typename ResultAlloc::value_type,
                          std::iter_value_t<details::QueryWithEntityIterator<Components...>>>
  [[nodiscard]] auto CollectWith(ResultAlloc alloc) const -> std::vector<value_type, ResultAlloc>;

  /**
   * @brief Collects all entities into a vector.
   * @return Vector of Entity objects
   *
   * @example
   * @code
   * std::vector<Entity> all_entities = query.WithEntity().CollectEntities();
   * @endcode
   */
  [[nodiscard]] auto CollectEntities() const -> std::vector<Entity>;

  /**
   * @brief Collects all entities into a vector using a custom allocator.
   * @warning When using a frame allocator (via SystemContext::MakeFrameAllocator()),
   * the returned data is only valid for the current frame. All pointers and references
   * to frame-allocated data become invalid after the frame ends.
   * Do not store frame-allocated data in components, resources, or any persistent storage.
   * @tparam ResultAlloc STL-compatible allocator type for Entity
   * @param alloc Allocator instance to use for the result vector
   * @return Vector of Entity objects using the provided allocator
   */
  template <typename ResultAlloc>
    requires std::same_as<typename ResultAlloc::value_type, Entity>
  [[nodiscard]] auto CollectEntitiesWith(ResultAlloc alloc) const -> std::vector<Entity, ResultAlloc>;

  /**
   * @brief Writes all query results into an output iterator.
   * @details Terminal operation that writes each (entity, components...) tuple to the provided output iterator.
   * More efficient than Collect() when you have a destination container.
   * @tparam OutIt Output iterator type
   * @param out Output iterator
   *
   * @example
   * @code
   * std::vector<std::tuple<Entity, const Position&, Health&>> results;
   * query.Into(std::back_inserter(results));
   * @endcode
   */
  template <typename OutIt>
    requires std::output_iterator<OutIt, std::iter_value_t<details::QueryWithEntityIterator<Components...>>>
  void Into(OutIt out);

  /**
   * @brief Executes an action for each matching entity and components.
   * @details Convenience method for side-effect operations.
   * @tparam Action Function type (Entity, Components...) -> void
   * @param action Function to execute for each result
   *
   * @example
   * @code
   * query.WithEntity()
   *   .ForEach([](Entity entity, Health& health) { health.Regenerate(1.0F); });
   * @endcode
   */
  template <typename Action>
    requires utils::ActionFor<Action, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  void ForEach(const Action& action);

  /**
   * @brief Calculates the maximum value using a key function.
   * @details Finds the element that produces the maximum value when passed to the key function.
   * @tparam KeyFunc Function type (Entity, Components...) -> Comparable
   * @param key_func Function to extract the comparison value
   * @return Optional containing the element with maximum key, or empty if query is empty
   *
   * @example
   * @code
   * auto strongest = query.WithEntity()
   *   .MaxBy([](Entity e, const Stats& s) { return s.strength; });
   * @endcode
   */
  template <typename KeyFunc>
    requires utils::TransformFor<KeyFunc, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto MaxBy(const KeyFunc& key_func) const -> std::optional<value_type>;

  /**
   * @brief Calculates the minimum value using a key function.
   * @details Finds the element that produces the minimum value when passed to the key function.
   * @tparam KeyFunc Function type (Entity, Components...) -> Comparable
   * @param key_func Function to extract the comparison value
   * @return Optional containing the element with minimum key, or empty if query is empty
   *
   * @example
   * @code
   * auto weakest = query.WithEntity()
   *   .MinBy([](Entity e, const Stats& s) { return s.strength; });
   * @endcode
   */
  template <typename KeyFunc>
    requires utils::TransformFor<KeyFunc, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto MinBy(const KeyFunc& key_func) const -> std::optional<value_type>;

  /**
   * @brief Filters entities based on a predicate.
   * @details Lazily filters the query results, only yielding elements that satisfy the predicate.
   * @tparam Pred Predicate function type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return View that yields only matching elements
   *
   * @example
   * @code
   * auto low_health = query.WithEntity()
   *   .Filter([](Entity e, const Health& h) { return h.current < 20.0F; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Filter(Pred predicate) const& -> utils::FilterAdapter<iterator, Pred>;

  /**
   * @brief Transforms each element using a mapping function.
   * @details Lazily transforms query results by applying a function to each element.
   * @tparam Func Transformation function type (Entity, Components...) -> U
   * @param transform Function to transform each result
   * @return View that yields transformed elements
   *
   * @example
   * @code
   * auto positions = query.WithEntity()
   *   .Map([](Entity e, const Transform& t) { return t.position; });
   * @endcode
   */
  template <typename Func>
    requires utils::TransformFor<Func, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Map(Func transform) const& -> utils::MapAdapter<iterator, Func>;

  /**
   * @brief Takes only the first N elements.
   * @details Lazily yields at most N elements from the query results.
   * @param count Maximum number of elements to take
   * @return View that yields at most count elements
   *
   * @example
   * @code
   * auto first_five = query.WithEntity().Take(5);
   * @endcode
   */
  [[nodiscard]] auto Take(size_t count) const& -> utils::TakeAdapter<iterator>;

  /**
   * @brief Skips the first N elements.
   * @details Lazily skips the first N elements and yields the rest.
   * @param count Number of elements to skip
   * @return View that yields elements after skipping count elements
   *
   * @example
   * @code
   * auto after_ten = query.WithEntity().Skip(10);
   * @endcode
   */
  [[nodiscard]] auto Skip(size_t count) const& -> utils::SkipAdapter<iterator>;

  /**
   * @brief Takes elements while a predicate is true.
   * @details Lazily yields elements until the predicate returns false.
   * @tparam Pred Predicate function type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return View that yields elements while predicate is true
   *
   * @example
   * @code
   * auto while_healthy = query.WithEntity()
   *   .TakeWhile([](Entity e, const Health& h) { return !h.IsDead(); });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto TakeWhile(Pred predicate) const& -> utils::TakeWhileAdapter<iterator, Pred>;

  /**
   * @brief Skips elements while a predicate is true.
   * @details Lazily skips elements until the predicate returns false, then yields the rest.
   * @tparam Pred Predicate function type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return View that yields elements after predicate becomes false
   *
   * @example
   * @code
   * auto after_full_health = query.WithEntity()
   *   .SkipWhile([](Entity e, const Health& h) { return h.current == h.max; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto SkipWhile(Pred predicate) const& -> utils::SkipWhileAdapter<iterator, Pred>;

  /**
   * @brief Adds an index to each element.
   * @details Lazily yields tuples with an index followed by the original elements.
   * @return View that yields (index, entity, components...) tuples
   *
   * @example
   * @code
   * for (auto [idx, entity, health] : query.WithEntity().Enumerate()) {
   *   // idx is 0, 1, 2, ...
   * }
   * @endcode
   */
  [[nodiscard]] auto Enumerate() const& -> utils::EnumerateAdapter<iterator>;

  /**
   * @brief Inspects each element without consuming it.
   * @details Calls a function on each element for side effects, then passes it through.
   * @tparam Func Inspection function type (Entity, Components...) -> void
   * @param inspector Function to call on each result
   * @return View that yields the same elements after inspection
   *
   * @example
   * @code
   * auto result = query.WithEntity()
   *   .Inspect([](Entity e, const Health& h) {
   *     std::cout << "Entity " << e.Index() << " health: " << h.current << "\n";
   *   })
   *   .Collect();
   * @endcode
   */
  template <typename Func>
    requires utils::InspectorFor<Func, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Inspect(Func inspector) const& -> utils::InspectAdapter<iterator, Func>;

  /**
   * @brief Yields every Nth element.
   * @details Lazily yields elements at regular intervals.
   * @param step Interval between yielded elements (must be > 0)
   * @return View that yields every step-th element
   *
   * @example
   * @code
   * auto every_third = query.WithEntity().StepBy(3);
   * @endcode
   */
  [[nodiscard]] auto StepBy(size_t step) const& -> utils::StepByAdapter<iterator>;

  /**
   * @brief Reverses the order of iteration.
   * @details Iterates through query results in reverse order.
   * @return View that yields elements in reverse order
   * @note Requires bidirectional iterator support
   *
   * @example
   * @code
   * auto reversed = query.WithEntity().Reverse();
   * @endcode
   */
  [[nodiscard]] auto Reverse() const& -> utils::ReverseAdapter<iterator> { return {begin(), end()}; }

  /**
   * @brief Creates sliding windows over query results.
   * @details Yields overlapping windows of a fixed size.
   * @param window_size Size of the sliding window
   * @return View that yields windows of elements
   * @warning window_size must be greater than 0
   *
   * @example
   * @code
   * auto windows = query.WithEntity().Slide(3);
   * @endcode
   */
  [[nodiscard]] auto Slide(size_t window_size) const& -> utils::SlideAdapter<iterator> {
    return {begin(), end(), window_size};
  }

  /**
   * @brief Takes every Nth element from the query results.
   * @details Yields elements at regular stride intervals.
   * @param stride Number of elements to skip between yields
   * @return View that yields every Nth element
   * @warning stride must be greater than 0
   *
   * @example
   * @code
   * auto every_third = query.WithEntity().Stride(3);
   * @endcode
   */
  [[nodiscard]] auto Stride(size_t stride) const& -> utils::StrideAdapter<iterator> { return {begin(), end(), stride}; }

  /**
   * @brief Zips this query with another iterator.
   * @details Combines query results with another range into tuples.
   * @tparam OtherIter Iterator type to zip with
   * @param other_begin Beginning of the other range
   * @param other_end End of the other range
   * @return View that yields tuples of corresponding elements
   *
   * @example
   * @code
   * std::vector<int> scores = {1, 2, 3};
   * auto zipped = query.WithEntity().Zip(scores.begin(), scores.end());
   * @endcode
   */
  template <typename OtherIter>
    requires utils::IteratorLike<OtherIter>
  [[nodiscard]] auto Zip(OtherIter other_begin, OtherIter other_end) const& -> utils::ZipAdapter<iterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Checks if any entity matches the predicate.
   * @tparam Pred Predicate type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return True if at least one result matches, false otherwise
   *
   * @example
   * @code
   * bool has_low_health = query.WithEntity().Any([](Entity entity, const Health& health) {
   *   return health.current < 10.0F;
   * });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] bool Any(const Pred& predicate) {
    return FindFirst(predicate).has_value();
  }

  /**
   * @brief Checks if all entities match the predicate.
   * @tparam Pred Predicate type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return True if all results match, false otherwise
   *
   * @example
   * @code
   * bool all_alive = query.WithEntity().All([](Entity entity, const Health& health) {
   *   return !health.IsDead();
   * });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] bool All(const Pred& predicate);

  /**
   * @brief Checks if no entities match the predicate.
   * @tparam Pred Predicate type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return True if no results match, false otherwise
   *
   * @example
   * @code
   * bool not_dead = query.WithEntity().None([](Entity entity, const Health& health) {
   *   return health.IsDead();
   * });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] bool None(const Pred& predicate) {
    return !Any(predicate);
  }

  /**
   * @brief Finds the first entity matching a predicate.
   * @tparam Pred Predicate type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return Optional containing the first matching tuple, or empty if none found
   *
   * @example
   * @code
   * auto boss = query.WithEntity().FindFirst([](Entity entity, const Enemy& enemy) {
   *   return enemy.type == EnemyType::Boss;
   * });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto FindFirst(const Pred& predicate) -> std::optional<value_type>;

  /**
   * @brief Counts entities matching a predicate.
   * @tparam Pred Predicate type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return Number of matching entities
   *
   * @example
   * @code
   * size_t low_health_count = query.WithEntity().CountIf(
   *   [](Entity entity, const Health& health) { return health.current < 20.0F; }
   * );
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] size_t CountIf(const Pred& predicate);

  /**
   * @brief Partitions entities into two groups based on a predicate.
   * @tparam Pred Predicate type (Entity, Components...) -> bool
   * @param predicate Function to test each result
   * @return Pair of vectors: first contains entities matching predicate, second contains non-matching
   *
   * @example
   * @code
   * auto [alive, dead] = query.WithEntity().Partition(
   *   [](Entity entity, const Health& health) { return !health.IsDead(); }
   * );
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Partition(const Pred& predicate) -> std::pair<std::vector<Entity>, std::vector<Entity>>;

  /**
   * @brief Reduces entities to a single value using an accumulator function.
   * @tparam T Result type
   * @tparam Func Reducer function type (T, Entity, Components...) -> T
   * @param init Initial accumulator value
   * @param reducer Function that combines accumulator with each result
   * @return Final accumulated value
   *
   * @example
   * @code
   * float total_health = query.WithEntity().Reduce(0.0F,
   *   [](float sum, Entity entitiy, const Health& health) { return sum + health.current; }
   * );
   * @endcode
   */
  template <typename T, typename Func>
    requires utils::FolderFor<Func, T, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] T Reduce(T init, const Func& reducer);

  /**
   * @brief Groups entities by a key extracted from components.
   * @tparam KeyExtractor Function type (Entity, Components...) -> Key
   * @param key_extractor Function that extracts the grouping key
   * @return Map from keys to vectors of entities with that key
   *
   * @example
   * @code
   * auto by_team = query.WithEntity().GroupBy(
   *   [](Entity entity, const Team& team) { return team.id; }
   * );
   * @endcode
   */
  template <typename KeyExtractor>
    requires utils::TransformFor<KeyExtractor, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto GroupBy(const KeyExtractor& key_extractor)
      -> std::unordered_map<std::invoke_result_t<KeyExtractor, Entity, details::ComponentAccessType<Components>...>,
                            std::vector<Entity>>;

  /**
   * @brief Gets iterator to first matching entity and components.
   * @return Iterator pointing to first result
   */
  [[nodiscard]] iterator begin() const;

  /**
   * @brief Gets iterator past the last matching entity.
   * @return End iterator
   */
  [[nodiscard]] iterator end() const;

private:
  BasicQuery<WorldT, Allocator, Components...>& query_;  ///< Reference to underlying query
};

/**
 * @brief Query result object for iterating over matching entities and components.
 * @details BasicQuery provides iteration and functional operations over entities
 * matching specified component criteria.
 * Supports const-qualified component access for read-only operations and custom allocators for internal storage.
 * @note Not thread-safe.
 * @tparam WorldT World type (World or const World), must satisfy WorldType concept
 * @tparam Allocator Allocator type for internal storage
 * @tparam Components Requested component access types with optional const qualifiers
 *
 * @example
 * @code
 * // Mutable access to both components
 * auto query = QueryBuilder(world).Get<Position&, Velocity&>();
 *
 * // Const access to Position, mutable access to Velocity
 * auto query = QueryBuilder(world).Get<const Position&, Velocity&>();
 *
 * // Copy Position, mutable access to Velocity
 * auto query = QueryBuilder(world).Get<Position, Velocity&>();
 * @endcode
 */
template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
class BasicQuery {
public:
  /// Components manager type based on whether all components are const-qualified
  using ComponentsType =
      std::conditional_t<details::AllComponentsConst<Components...>, const details::Components, details::Components>;

  using WithEntityIterator = details::QueryWithEntityIterator<Components...>;

  using iterator = details::QueryIterator<Components...>;
  using value_type = std::iter_value_t<details::QueryIterator<Components...>>;
  using difference_type = std::iter_difference_t<details::QueryIterator<Components...>>;
  using pointer = typename iterator::pointer;
  using reference = std::iter_reference_t<details::QueryIterator<Components...>>;
  using allocator_type = Allocator;

  /// Rebind allocator for archetype references
  using ArchetypeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<
      std::reference_wrapper<const details::Archetype>>;

  /**
   * @brief Constructs query with specified filtering criteria.
   * @details Creates query that matches entities with required components and without forbidden ones.
   * @param world Reference to ECS world
   * @param with_components Component types that entities must have
   * @param without_components Component types that entities must not have
   * @param alloc Allocator instance for internal storage
   */
  BasicQuery(WorldT& world, std::vector<ComponentTypeId, Allocator> with_components,
             std::vector<ComponentTypeId, Allocator> without_components = {}, Allocator alloc = {});

  BasicQuery(const BasicQuery&) = delete;
  BasicQuery(BasicQuery&&) noexcept = default;
  ~BasicQuery() = default;

  BasicQuery& operator=(const BasicQuery&) = delete;
  BasicQuery& operator=(BasicQuery&&) noexcept = default;

  /**
   * @brief Creates wrapper for entity-aware iteration.
   * @details Returns a wrapper that provides iterators returning entity + components.
   * @note Only callable on lvalue queries to prevent dangling references.
   * @warning Calling on rvalue query results in undefined behavior. Create a named query variable first.
   * @return BasicQueryWithEntity wrapper for this query
   */
  [[nodiscard]] auto WithEntity() & noexcept -> BasicQueryWithEntity<WorldT, Allocator, Components...> {
    return BasicQueryWithEntity<WorldT, Allocator, Components...>(*this);
  }

  /**
   * @brief Collects all results into a vector.
   * @details Eagerly evaluates the query and returns all matching results.
   * @return Vector of tuples containing components
   *
   * @example
   * @code
   * auto all_results = query.Collect();
   * @endcode
   */
  [[nodiscard]] auto Collect() const -> std::vector<value_type>;

  /**
   * @brief Collects all results into a vector using a custom allocator.
   * @details Eagerly evaluates the query and returns all matching results using the provided allocator.
   * Useful for temporary allocations with frame allocators.
   * @warning When using a frame allocator (via SystemContext::MakeFrameAllocator()),
   * the returned data is only valid for the current frame.
   * All pointers and references to frame-allocated data become invalid after the frame ends.
   * Do not store frame-allocated data in components, resources, or any persistent storage.
   * @tparam ResultAlloc STL-compatible allocator type for value_type
   * @param alloc Allocator instance to use for the result vector
   * @return Vector of tuples containing components, using the provided allocator
   *
   * @example
   * @code
   * void MySystem::Update(app::SystemContext& ctx) {
   *   auto query = ctx.Query().Get<Position&, Velocity&>();
   *   auto alloc = ctx.MakeFrameAllocator<decltype(query)::value_type>();
   *   auto results = query.CollectWith(alloc);
   *   // results uses frame allocator, automatically reclaimed at frame end
   *   // WARNING: Do not store 'results' or pointers to its contents beyond this frame!
   * }
   * @endcode
   */
  template <typename ResultAlloc>
    requires std::same_as<typename ResultAlloc::value_type, std::iter_value_t<details::QueryIterator<Components...>>>
  [[nodiscard]] auto CollectWith(ResultAlloc alloc) const -> std::vector<value_type, ResultAlloc>;

  /**
   * @brief Writes all query results into an output iterator.
   * @details Terminal operation that writes each result tuple to the provided output iterator.
   * More efficient than Collect() when you have a destination container.
   * @tparam OutIt Output iterator type
   * @param out Output iterator
   *
   * @example
   * @code
   * std::vector<std::tuple<const Position&, Health&>> results;
   * query.Into(std::back_inserter(results));
   * @endcode
   */
  template <typename OutIt>
    requires std::output_iterator<OutIt, std::iter_value_t<details::QueryIterator<Components...>>>
  void Into(OutIt out) const;

  /**
   * @brief Executes an action for each matching entity and components.
   * @details Convenience method for side-effect operations.
   * @tparam Action Function type (Components...) -> void
   * @param action Function to execute for each result
   *
   * @example
   * @code
   * query.ForEach([](Position& pos, const Velocity& vel) {
   *   pos.x += vel.dx;
   *   pos.y += vel.dy;
   * });
   * @endcode
   */
  template <typename Action>
    requires utils::ActionFor<Action, std::tuple<details::ComponentAccessType<Components>...>>
  void ForEach(const Action& action) const;

  /**
   * @brief Executes an action for each entity and its components.
   * @details Variant that also provides the entity as the first argument.
   * @tparam Action Function type (Entity, Components...) -> void
   * @param action Function to execute for each result
   *
   * @example
   * @code
   * query.ForEachWithEntity([](Entity entity, Position& pos) {
   *   HELIOS_INFO("Entity {} at ({}, {})", entity.Index(), pos.x, pos.y);
   * });
   * @endcode
   */
  template <typename Action>
    requires utils::ActionFor<Action, std::tuple<Entity, details::ComponentAccessType<Components>...>>
  void ForEachWithEntity(const Action& action) const;

  /**
   * @brief Filters entities based on a predicate.
   * @details Lazily filters the query results, only yielding elements that satisfy the predicate.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return View that yields only matching elements
   *
   * @example
   * @code
   * auto moving = query.Filter([](const Position&, const Velocity& vel) {
   *   return vel.dx != 0.0F || vel.dy != 0.0F;
   * });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Filter(Pred predicate) const& -> utils::FilterAdapter<iterator, Pred>;

  /**
   * @brief Transforms each element using a mapping function.
   * @details Lazily transforms query results by applying a function to each element.
   * @tparam Func Transformation function type (Components...) -> U
   * @param transform Function to transform each result
   * @return View that yields transformed elements
   *
   * @example
   * @code
   * auto x_positions = query.Map([](const Position& pos, const Velocity&) {
   *   return pos.x;
   * });
   * @endcode
   */
  template <typename Func>
    requires utils::TransformFor<Func, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Map(Func transform) const& -> utils::MapAdapter<iterator, Func>;

  /**
   * @brief Takes only the first N elements.
   * @details Lazily yields at most N elements from the query results.
   * @param count Maximum number of elements to take
   * @return View that yields at most count elements
   *
   * @example
   * @code
   * auto first_ten = query.Take(10);
   * @endcode
   */
  [[nodiscard]] auto Take(size_t count) const& -> utils::TakeAdapter<iterator>;

  /**
   * @brief Skips the first N elements.
   * @details Lazily skips the first N elements and yields the rest.
   * @param count Number of elements to skip
   * @return View that yields elements after skipping count elements
   *
   * @example
   * @code
   * auto after_five = query.Skip(5);
   * @endcode
   */
  [[nodiscard]] auto Skip(size_t count) const& -> utils::SkipAdapter<iterator>;

  /**
   * @brief Reverses the order of iteration.
   * @details Iterates through query results in reverse order.
   * @return View that yields elements in reverse order
   * @note Requires bidirectional iterator support
   *
   * @example
   * @code
   * auto reversed = query.Reverse();
   * @endcode
   */
  [[nodiscard]] auto Reverse() const& -> utils::ReverseAdapter<iterator> { return {begin(), end()}; }

  /**
   * @brief Creates sliding windows over query results.
   * @details Yields overlapping windows of a fixed size.
   * @param window_size Size of the sliding window
   * @return View that yields windows of elements
   * @warning window_size must be greater than 0
   *
   * @example
   * @code
   * auto windows = query.Slide(3);
   * @endcode
   */
  [[nodiscard]] auto Slide(size_t window_size) const& -> utils::SlideAdapter<iterator> {
    return {begin(), end(), window_size};
  }

  /**
   * @brief Takes every Nth element from the query results.
   * @details Yields elements at regular stride intervals.
   * @param stride Number of elements to skip between yields
   * @return View that yields every Nth element
   * @warning stride must be greater than 0
   *
   * @example
   * @code
   * auto every_third = query.Stride(3);
   * @endcode
   */
  [[nodiscard]] auto Stride(size_t stride) const& -> utils::StrideAdapter<iterator> { return {begin(), end(), stride}; }

  /**
   * @brief Zips this query with another iterator.
   * @details Combines query results with another range into tuples.
   * @tparam OtherIter Iterator type to zip with
   * @param other_begin Beginning of the other range
   * @param other_end End of the other range
   * @return View that yields tuples of corresponding elements
   *
   * @example
   * @code
   * std::vector<int> indices = {0, 1, 2};
   * auto zipped = query.Zip(indices.begin(), indices.end());
   * @endcode
   */
  template <typename OtherIter>
    requires utils::IteratorLike<OtherIter>
  [[nodiscard]] auto Zip(OtherIter other_begin, OtherIter other_end) const& -> utils::ZipAdapter<iterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Takes elements while a predicate is true.
   * @details Lazily yields elements until the predicate returns false.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return View that yields elements while predicate is true
   *
   * @example
   * @code
   * auto while_positive = query.TakeWhile([](const Health& h) { return h.points > 0; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto TakeWhile(Pred predicate) const& -> utils::TakeWhileAdapter<iterator, Pred>;

  /**
   * @brief Skips elements while a predicate is true.
   * @details Lazily skips elements until the predicate returns false, then yields the rest.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return View that yields elements after predicate becomes false
   *
   * @example
   * @code
   * auto after_full = query.SkipWhile([](const Health& h) { return h.points == h.max; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto SkipWhile(Pred predicate) const& -> utils::SkipWhileAdapter<iterator, Pred>;

  /**
   * @brief Adds an index to each element.
   * @details Lazily yields tuples with an index followed by the original elements.
   * @return View that yields (index, components...) tuples
   *
   * @example
   * @code
   * for (auto [idx, pos, vel] : query.Enumerate()) {
   *   // idx is 0, 1, 2, ...
   * }
   * @endcode
   */
  [[nodiscard]] auto Enumerate() const& -> utils::EnumerateAdapter<iterator>;

  /**
   * @brief Inspects each element without consuming it.
   * @details Calls a function on each element for side effects, then passes it through.
   * @tparam Func Inspection function type (Components...) -> void
   * @param inspector Function to call on each result
   * @return View that yields the same elements after inspection
   *
   * @example
   * @code
   * auto result = query.Inspect([](const Position& p) {
   *   std::cout << "Position: " << p.x << ", " << p.y << "\n";
   * }).Collect();
   * @endcode
   */
  template <typename Func>
    requires utils::InspectorFor<Func, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Inspect(Func inspector) const& -> utils::InspectAdapter<iterator, Func>;

  /**
   * @brief Yields every Nth element.
   * @details Lazily yields elements at regular intervals.
   * @param step Interval between yielded elements (must be > 0)
   * @return View that yields every step-th element
   *
   * @example
   * @code
   * auto every_other = query.StepBy(2);
   * @endcode
   */
  [[nodiscard]] auto StepBy(size_t step) const& -> utils::StepByAdapter<iterator>;

  /**
   * @brief Checks if any element matches the predicate.
   * @details Short-circuits on first match for efficiency.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return True if at least one result matches, false otherwise
   *
   * @example
   * @code
   * bool has_low_health = query.Any([](const Health& h) { return h.current < 10.0F; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] bool Any(const Pred& predicate) const {
    return FindFirst(predicate).has_value();
  }

  /**
   * @brief Checks if all elements match the predicate.
   * @details Short-circuits on first non-match for efficiency.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return True if all results match, false otherwise
   *
   * @example
   * @code
   * bool all_alive = query.All([](const Health& h) { return !h.IsDead(); });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] bool All(const Pred& predicate) const;

  /**
   * @brief Checks if no elements match the predicate.
   * @details Short-circuits on first match for efficiency.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return True if no results match, false otherwise
   *
   * @example
   * @code
   * bool none_dead = query.None([](const Health& h) { return h.IsDead(); });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] bool None(const Pred& predicate) const {
    return !Any(predicate);
  }

  /**
   * @brief Counts elements matching a predicate.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return Number of matching elements
   *
   * @example
   * @code
   * size_t low_health_count = query.CountIf([](const Health& h) { return h.current < 20.0F; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] size_t CountIf(const Pred& predicate) const;

  /**
   * @brief Folds the query results into a single value.
   * @details Applies a binary function to an initial value and each result.
   * @tparam T Accumulator type
   * @tparam Func Function type (T, Components...) -> T
   * @param init Initial value
   * @param folder Function that combines accumulator with each result
   * @return Final folded value
   *
   * @example
   * @code
   * float total_mass = query.Fold(0.0F, [](float acc, const Physics& p) { return acc + p.mass; });
   * @endcode
   */
  template <typename T, typename Func>
    requires utils::FolderFor<Func, T, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] T Fold(T init, const Func& folder) const;

  /**
   * @brief Finds the first element matching a predicate.
   * @details Searches through query results until finding one that satisfies the predicate.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return Optional containing the first matching tuple, or empty if none found
   *
   * @example
   * @code
   * auto low_health = query.FindFirst([](const Health& h) { return h.current < 10.0F; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto FindFirst(const Pred& predicate) const -> std::optional<value_type>;

  /**
   * @brief Partitions elements into two groups based on a predicate.
   * @tparam Pred Predicate function type (Components...) -> bool
   * @param predicate Function to test each result
   * @return Pair of vectors: first contains tuples matching predicate, second contains non-matching
   *
   * @example
   * @code
   * auto [alive, dead] = query.Partition([](const Health& h) { return !h.IsDead(); });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto Partition(const Pred& predicate) const
      -> std::pair<std::vector<value_type>, std::vector<value_type>>;

  /**
   * @brief Calculates the maximum value using a key function.
   * @details Finds the element that produces the maximum value when passed to the key function.
   * @tparam KeyFunc Function type (Components...) -> Comparable
   * @param key_func Function to extract the comparison value
   * @return Optional containing the element with maximum key, or empty if query is empty
   *
   * @example
   * @code
   * auto strongest = query.MaxBy([](const Stats& s) { return s.strength; });
   * @endcode
   */
  template <typename KeyFunc>
    requires utils::TransformFor<KeyFunc, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto MaxBy(const KeyFunc& key_func) const -> std::optional<value_type>;

  /**
   * @brief Calculates the minimum value using a key function.
   * @details Finds the element that produces the minimum value when passed to the key function.
   * @tparam KeyFunc Function type (Components...) -> Comparable
   * @param key_func Function to extract the comparison value
   * @return Optional containing the element with minimum key, or empty if query is empty
   *
   * @example
   * @code
   * auto weakest = query.MinBy([](const Stats& s) { return s.strength; });
   * @endcode
   */
  template <typename KeyFunc>
    requires utils::TransformFor<KeyFunc, std::tuple<details::ComponentAccessType<Components>...>>
  [[nodiscard]] auto MinBy(const KeyFunc& key_func) const -> std::optional<value_type>;

  /**
   * @brief Checks if any entities match the query.
   * @details Fast check for query result emptiness without full iteration.
   * @return True if at least one entity matches, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept;

  /**
   * @brief Gets the number of matching entities.
   * @details Counts entities across all matching archetypes.
   * Time complexity: O(A) where A is the number of matching archetypes.
   * @return Total count of entities that match the query
   */
  [[nodiscard]] size_t Count() const noexcept;

  /**
   * @brief Gets iterator to first matching entity.
   * @return Iterator pointing to first result
   */
  [[nodiscard]] iterator begin() const;

  /**
   * @brief Gets iterator past the last matching entity.
   * @return End iterator
   */
  [[nodiscard]] iterator end() const noexcept {
    return {matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0};
  }

  /**
   * @brief Gets the allocator used by this query.
   * @return Copy of the allocator
   */
  [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

private:
  /**
   * @brief Refreshes the list of matching archetypes.
   * @details Updates cached archetype list based on current world state.
   */
  void RefreshArchetypes() const {
    auto result = world_.get().GetArchetypes().FindMatchingArchetypes(with_components_, without_components_);
    matching_archetypes_.clear();
    matching_archetypes_.reserve(result.size());
    for (auto& archetype : result) {
      matching_archetypes_.push_back(archetype);
    }
  }

  [[nodiscard]] WorldT& GetWorld() noexcept { return world_.get(); }
  [[nodiscard]] const WorldT& GetWorld() const noexcept { return world_.get(); }

  [[nodiscard]] ComponentsType& GetComponents() const noexcept;

  [[nodiscard]] auto GetMatchingArchetypes() const noexcept
      -> std::span<const std::reference_wrapper<const details::Archetype>> {
    return {matching_archetypes_.data(), matching_archetypes_.size()};
  }

  std::reference_wrapper<WorldT> world_;                        ///< Reference to ECS world
  std::vector<ComponentTypeId, Allocator> with_components_;     ///< Required component types
  std::vector<ComponentTypeId, Allocator> without_components_;  ///< Forbidden component types
  mutable std::vector<std::reference_wrapper<const details::Archetype>, ArchetypeAllocator>
      matching_archetypes_;                ///< Cached matching archetypes
  [[no_unique_address]] Allocator alloc_;  ///< Allocator instance

  friend class BasicQueryWithEntity<WorldT, Allocator, Components...>;
};

// ================================================================================================
// BasicQueryWithEntity implementation
// ================================================================================================

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Collect() const -> std::vector<value_type> {
  std::vector<value_type> result;
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename ResultAlloc>
  requires std::same_as<typename ResultAlloc::value_type,
                        std::iter_value_t<details::QueryWithEntityIterator<Components...>>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::CollectWith(ResultAlloc alloc) const
    -> std::vector<value_type, ResultAlloc> {
  std::vector<value_type, ResultAlloc> result{std::move(alloc)};
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::CollectEntities() const -> std::vector<Entity> {
  std::vector<Entity> result;
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(std::get<0>(tuple));
  }
  return result;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename ResultAlloc>
  requires std::same_as<typename ResultAlloc::value_type, Entity>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::CollectEntitiesWith(ResultAlloc alloc) const
    -> std::vector<Entity, ResultAlloc> {
  std::vector<Entity, ResultAlloc> result{std::move(alloc)};
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(std::get<0>(tuple));
  }
  return result;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename OutIt>
  requires std::output_iterator<OutIt, std::iter_value_t<details::QueryWithEntityIterator<Components...>>>
inline void BasicQueryWithEntity<WorldT, Allocator, Components...>::Into(OutIt out) {
  for (auto&& result : *this) {
    *out++ = std::forward<decltype(result)>(result);
  }
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Action>
  requires utils::ActionFor<Action, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline void BasicQueryWithEntity<WorldT, Allocator, Components...>::ForEach(const Action& action) {
  for (auto&& result : *this) {
    std::apply(action, result);
  }
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Filter(
    Pred predicate) const& -> utils::FilterAdapter<iterator, Pred> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Func>
  requires utils::TransformFor<Func, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Map(
    Func transform) const& -> utils::MapAdapter<iterator, Func> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(transform)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Take(
    size_t count) const& -> utils::TakeAdapter<iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, count};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Skip(
    size_t count) const& -> utils::SkipAdapter<iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, count};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::TakeWhile(
    Pred predicate) const& -> utils::TakeWhileAdapter<iterator, Pred> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::SkipWhile(
    Pred predicate) const& -> utils::SkipWhileAdapter<iterator, Pred> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Enumerate()
    const& -> utils::EnumerateAdapter<iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Func>
  requires utils::InspectorFor<Func, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Inspect(
    Func inspector) const& -> utils::InspectAdapter<iterator, Func> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(inspector)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::StepBy(
    size_t step) const& -> utils::StepByAdapter<iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, step};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::FindFirst(const Pred& predicate)
    -> std::optional<value_type> {
  for (auto&& result : *this) {
    if (std::apply(predicate, result)) {
      return result;
    }
  }
  return std::nullopt;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename KeyFunc>
  requires utils::TransformFor<KeyFunc, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::MaxBy(const KeyFunc& key_func) const
    -> std::optional<value_type> {
  auto iter = begin();
  const auto end_iter = end();

  if (iter == end_iter) {
    return std::nullopt;
  }

  std::optional<value_type> max_element;
  max_element.emplace(*iter);
  auto max_key = std::apply(key_func, *max_element);
  ++iter;

  while (iter != end_iter) {
    auto current = *iter;
    auto current_key = std::apply(key_func, current);
    if (current_key > max_key) {
      max_key = current_key;
      max_element.emplace(current);
    }
    ++iter;
  }

  return max_element;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename KeyFunc>
  requires utils::TransformFor<KeyFunc, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::MinBy(const KeyFunc& key_func) const
    -> std::optional<value_type> {
  auto iter = begin();
  const auto end_iter = end();

  if (iter == end_iter) {
    return std::nullopt;
  }

  std::optional<value_type> min_element;
  min_element.emplace(*iter);
  auto min_key = std::apply(key_func, *min_element);
  ++iter;

  while (iter != end_iter) {
    auto current = *iter;
    auto current_key = std::apply(key_func, current);
    if (current_key < min_key) {
      min_key = current_key;
      min_element.emplace(current);
    }
    ++iter;
  }

  return min_element;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline size_t BasicQueryWithEntity<WorldT, Allocator, Components...>::CountIf(const Pred& predicate) {
  size_t count = 0;
  for (auto&& tuple : *this) {
    if (std::apply(predicate, tuple)) {
      ++count;
    }
  }
  return count;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline bool BasicQueryWithEntity<WorldT, Allocator, Components...>::All(const Pred& predicate) {
  for (auto&& result : *this) {
    if (!std::apply(predicate, result)) {
      return false;
    }
  }
  return true;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::Partition(const Pred& predicate)
    -> std::pair<std::vector<Entity>, std::vector<Entity>> {
  std::vector<Entity> matched;
  std::vector<Entity> not_matched;

  for (auto&& tuple : *this) {
    if (std::apply(predicate, tuple)) {
      matched.push_back(std::get<0>(tuple));
    } else {
      not_matched.push_back(std::get<0>(tuple));
    }
  }

  return {std::move(matched), std::move(not_matched)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename T, typename Func>
  requires utils::FolderFor<Func, T, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline T BasicQueryWithEntity<WorldT, Allocator, Components...>::Reduce(T init, const Func& reducer) {
  for (auto&& tuple : *this) {
    init = std::apply(
        [&init, &reducer](Entity entity, auto&&... components) {
          return reducer(std::move(init), entity, std::forward<decltype(components)>(components)...);
        },
        tuple);
  }
  return init;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename KeyExtractor>
  requires utils::TransformFor<KeyExtractor, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::GroupBy(const KeyExtractor& key_extractor)
    -> std::unordered_map<std::invoke_result_t<KeyExtractor, Entity, details::ComponentAccessType<Components>...>,
                          std::vector<Entity>> {
  using KeyType = std::invoke_result_t<KeyExtractor, Entity, details::ComponentAccessType<Components>...>;
  std::unordered_map<KeyType, std::vector<Entity>> groups;

  for (auto&& tuple : *this) {
    auto key = std::apply(key_extractor, tuple);
    groups[key].push_back(std::get<0>(tuple));
  }

  return groups;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::begin() const -> iterator {
  query_.RefreshArchetypes();
  return {query_.GetMatchingArchetypes(), query_.GetComponents(), 0, 0};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQueryWithEntity<WorldT, Allocator, Components...>::end() const -> iterator {
  return {query_.GetMatchingArchetypes(), query_.GetComponents(), query_.GetMatchingArchetypes().size(), 0};
}

// ================================================================================================
// BasicQuery implementation
// ================================================================================================

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
BasicQuery<WorldT, Allocator, Components...>::BasicQuery(WorldT& world,
                                                         std::vector<ComponentTypeId, Allocator> with_components,
                                                         std::vector<ComponentTypeId, Allocator> without_components,
                                                         Allocator alloc)
    : world_(world),
      with_components_(std::move(with_components)),
      without_components_(std::move(without_components)),
      matching_archetypes_(ArchetypeAllocator(alloc)),
      alloc_(alloc) {
  static_assert(details::ValidWorldComponentAccess<WorldT, Components...>,
                "Cannot request mutable component access from const World! "
                "Use const-qualified components (e.g., 'const Position&') or pass mutable World.");
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename OutIt>
  requires std::output_iterator<OutIt, std::iter_value_t<details::QueryIterator<Components...>>>
inline void BasicQuery<WorldT, Allocator, Components...>::Into(OutIt out) const {
  for (auto&& result : *this) {
    *out++ = std::forward<decltype(result)>(result);
  }
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Action>
  requires utils::ActionFor<Action, std::tuple<details::ComponentAccessType<Components>...>>
inline void BasicQuery<WorldT, Allocator, Components...>::ForEach(const Action& action) const {
  for (auto&& result : *this) {
    std::apply(action, result);
  }
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Action>
  requires utils::ActionFor<Action, std::tuple<Entity, details::ComponentAccessType<Components>...>>
inline void BasicQuery<WorldT, Allocator, Components...>::ForEachWithEntity(const Action& action) const {
  RefreshArchetypes();
  auto begin_iter = WithEntityIterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = WithEntityIterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  for (auto iter = begin_iter; iter != end_iter; ++iter) {
    std::apply([&action](Entity entity, auto&&... components) { action(entity, components...); }, *iter);
  }
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::Filter(
    Pred predicate) const& -> utils::FilterAdapter<iterator, Pred> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Func>
  requires utils::TransformFor<Func, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::Map(
    Func transform) const& -> utils::MapAdapter<iterator, Func> {
  RefreshArchetypes();
  auto begin_iter = iterator(GetMatchingArchetypes(), GetComponents(), 0, 0);
  auto end_iter = iterator(GetMatchingArchetypes(), GetComponents(), GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(transform)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::Take(size_t count) const& -> utils::TakeAdapter<iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, count};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::Skip(size_t count) const& -> utils::SkipAdapter<iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, count};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::TakeWhile(
    Pred predicate) const& -> utils::TakeWhileAdapter<iterator, Pred> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::SkipWhile(
    Pred predicate) const& -> utils::SkipWhileAdapter<iterator, Pred> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::Enumerate() const& -> utils::EnumerateAdapter<iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(GetMatchingArchetypes(), GetComponents(), 0, 0);
  auto end_iter = iterator(GetMatchingArchetypes(), GetComponents(), GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Func>
  requires utils::InspectorFor<Func, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::Inspect(
    Func inspector) const& -> utils::InspectAdapter<iterator, Func> {
  RefreshArchetypes();
  auto begin_iter = iterator(GetMatchingArchetypes(), GetComponents(), 0, 0);
  auto end_iter = iterator(GetMatchingArchetypes(), GetComponents(), GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(inspector)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::StepBy(size_t step) const& -> utils::StepByAdapter<iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponents(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponents(), matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, step};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::Collect() const -> std::vector<value_type> {
  std::vector<value_type> result;
  result.reserve(Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename ResultAlloc>
  requires std::same_as<typename ResultAlloc::value_type, std::iter_value_t<details::QueryIterator<Components...>>>
inline auto BasicQuery<WorldT, Allocator, Components...>::CollectWith(ResultAlloc alloc) const
    -> std::vector<value_type, ResultAlloc> {
  std::vector<value_type, ResultAlloc> result{std::move(alloc)};
  result.reserve(Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename T, typename Func>
  requires utils::FolderFor<Func, T, std::tuple<details::ComponentAccessType<Components>...>>
inline T BasicQuery<WorldT, Allocator, Components...>::Fold(T init, const Func& folder) const {
  for (auto&& tuple : *this) {
    init = std::apply(
        [&init, &folder](auto&&... components) {
          return folder(std::move(init), std::forward<decltype(components)>(components)...);
        },
        tuple);
  }
  return init;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::FindFirst(const Pred& predicate) const
    -> std::optional<value_type> {
  for (auto&& result : *this) {
    if (std::apply(predicate, result)) {
      return result;
    }
  }
  return std::nullopt;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline bool BasicQuery<WorldT, Allocator, Components...>::All(const Pred& predicate) const {
  for (auto&& result : *this) {
    if (!std::apply(predicate, result)) {
      return false;
    }
  }
  return true;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline size_t BasicQuery<WorldT, Allocator, Components...>::CountIf(const Pred& predicate) const {
  size_t count = 0;
  for (auto&& tuple : *this) {
    if (std::apply(predicate, tuple)) {
      ++count;
    }
  }
  return count;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename Pred>
  requires utils::PredicateFor<Pred, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::Partition(const Pred& predicate) const
    -> std::pair<std::vector<value_type>, std::vector<value_type>> {
  std::vector<value_type> matched;
  std::vector<value_type> not_matched;

  for (auto&& tuple : *this) {
    if (std::apply(predicate, tuple)) {
      matched.push_back(tuple);
    } else {
      not_matched.push_back(tuple);
    }
  }

  return {std::move(matched), std::move(not_matched)};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename KeyFunc>
  requires utils::TransformFor<KeyFunc, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::MaxBy(const KeyFunc& key_func) const
    -> std::optional<value_type> {
  auto iter = begin();
  const auto end_iter = end();

  if (iter == end_iter) {
    return std::nullopt;
  }

  std::optional<value_type> max_element;
  max_element.emplace(*iter);
  auto max_key = std::apply(key_func, *max_element);
  ++iter;

  while (iter != end_iter) {
    auto current = *iter;
    auto current_key = std::apply(key_func, current);
    if (current_key > max_key) {
      max_key = current_key;
      max_element.emplace(current);
    }
    ++iter;
  }

  return max_element;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
template <typename KeyFunc>
  requires utils::TransformFor<KeyFunc, std::tuple<details::ComponentAccessType<Components>...>>
inline auto BasicQuery<WorldT, Allocator, Components...>::MinBy(const KeyFunc& key_func) const
    -> std::optional<value_type> {
  auto iter = begin();
  const auto end_iter = end();

  if (iter == end_iter) {
    return std::nullopt;
  }

  std::optional<value_type> min_element;
  min_element.emplace(*iter);
  auto min_key = std::apply(key_func, *min_element);
  ++iter;

  while (iter != end_iter) {
    auto current = *iter;
    auto current_key = std::apply(key_func, current);
    if (current_key < min_key) {
      min_key = current_key;
      min_element.emplace(current);
    }
    ++iter;
  }

  return min_element;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline bool BasicQuery<WorldT, Allocator, Components...>::Empty() const noexcept {
  RefreshArchetypes();
  return matching_archetypes_.empty() || Count() == 0;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline size_t BasicQuery<WorldT, Allocator, Components...>::Count() const noexcept {
  RefreshArchetypes();
  size_t count = 0;
  for (const auto& archetype_ref : matching_archetypes_) {
    count += archetype_ref.get().EntityCount();
  }
  return count;
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::begin() const -> iterator {
  RefreshArchetypes();
  return {matching_archetypes_, GetComponents(), 0, 0};
}

template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
  requires utils::UniqueTypes<Components...>
inline auto BasicQuery<WorldT, Allocator, Components...>::GetComponents() const noexcept ->
    typename BasicQuery<WorldT, Allocator, Components...>::ComponentsType& {
  if constexpr (details::AllComponentsConst<Components...> || details::IsConstWorld<WorldT>) {
    return std::as_const(world_.get()).GetComponents();
  } else {
    return world_.get().GetComponents();
  }
}

// ================================================================================================
// BasicQueryBuilder
// ================================================================================================

/**
 * @brief Query builder for filtering entities by components with fluent interface.
 * @details Provides a fluent API for constructing queries with required and forbidden component types.
 * Supports const-qualified World access for read-only queries and custom allocators for internal storage.
 * @note Not thread-safe.
 * @tparam WorldT World type (World or const World), must satisfy WorldType concept
 * @tparam Allocator Allocator type for internal storage
 *
 * @example
 * @code
 * // Query with mutable access
 * auto query = QueryBuilder(world)
 *     .Without<Frozen>()
 *     .Get<Position&, Velocity&>();
 *
 * // Query with mixed const/mutable access
 * auto query = QueryBuilder(world)
 *     .With<Player>()
 *     .Get<const Position&, Velocity&>();
 *
 * // Read-only query from const World
 * auto query = ReadOnlyQueryBuilder(const_world)
 *     .Get<const Position&>();  // Only const access allowed
 * @endcode
 */
template <WorldType WorldT = World, typename Allocator = std::allocator<ComponentTypeId>>
class BasicQueryBuilder {
public:
  using allocator_type = Allocator;

  /**
   * @brief Constructs query builder for the specified world.
   * @param world Reference to ECS world to query
   * @param alloc Allocator for internal storage
   */
  explicit BasicQueryBuilder(WorldT& world, Allocator alloc = {}) noexcept
      : world_(world), with_components_(alloc), without_components_(alloc), alloc_(alloc) {}

  /**
   * @brief Constructs query builder for the specified world with access validation.
   * @param world Reference to ECS world to query
   * @param policy Access policy for validation
   * @param alloc Allocator for internal storage
   */
  BasicQueryBuilder(WorldT& world, const app::AccessPolicy& policy, Allocator alloc = {})
      : world_(world), policy_(&policy), with_components_(alloc), without_components_(alloc), alloc_(alloc) {}

  BasicQueryBuilder(const BasicQueryBuilder&) = delete;
  BasicQueryBuilder(BasicQueryBuilder&&) noexcept = default;
  ~BasicQueryBuilder() = default;

  BasicQueryBuilder& operator=(const BasicQueryBuilder&) = delete;
  BasicQueryBuilder& operator=(BasicQueryBuilder&&) noexcept = default;

  /**
   * @brief Adds required component types to the query.
   * @details Entities must have ALL specified component types to match the query.
   * @tparam Comps Component types that entities must have
   * @return Reference to this builder for method chaining
   */
  template <ComponentTrait... Comps>
    requires utils::UniqueTypes<Comps...>
  auto With(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds forbidden component types to the query.
   * @details Entities must NOT have ANY of the specified component types to match the query.
   * @tparam Comps Component types that entities must not have
   * @return Reference to this builder for method chaining
   */
  template <ComponentTrait... Comps>
    requires utils::UniqueTypes<Comps...>
  auto Without(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Builds and executes the query, returning components with specified access.
   * @details Creates query object with the specified component access types.
   * Supports const-qualified access for read-only operations and mixed access patterns.
   *
   * Access type examples:
   * - `T&` - Mutable reference to component T
   * - `const T&` - Const reference to component T
   * - `T` - Copy of component T
   *
   * Const World Restriction:
   * - When BasicQueryBuilder is constructed with `const World&`, only const component access is allowed
   * - Attempting to use mutable component access (e.g., `Get<Position&>()`) will fail at compile time
   *
   * @tparam Comps Component access types (may include const qualifiers and references)
   * @return BasicQuery object for iteration over matching entities
   */
  template <ComponentTrait... Comps>
    requires(sizeof...(Comps) > 0 && utils::UniqueTypes<details::ComponentTypeExtractor_t<Comps>...>)
  [[nodiscard]] auto Get(this auto&& self) -> BasicQuery<WorldT, Allocator, Comps...>;

  /**
   * @brief Builds and executes the query, returning no components.
   * @details Creates query object that only filters entities based on presence/absence
   * of specified components, without returning any component data.
   * Useful for existence checks or counting entities.
   * @return BasicQuery object for iteration over matching entities
   */
  [[nodiscard]] auto Get(this auto&& self) -> BasicQuery<WorldT, Allocator> {
    return {self.world_.get(), std::forward_like<decltype(self)>(self.with_components_),
            std::forward_like<decltype(self)>(self.without_components_), self.alloc_};
  }

  /**
   * @brief Gets the allocator used by this builder.
   * @return Copy of the allocator
   */
  [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

private:
  std::reference_wrapper<WorldT> world_;                        ///< Reference to ECS world
  const app::AccessPolicy* policy_ = nullptr;                   ///< Access policy for Get() check
  std::vector<ComponentTypeId, Allocator> with_components_;     ///< Required component type IDs
  std::vector<ComponentTypeId, Allocator> without_components_;  ///< Forbidden component type IDs
  [[no_unique_address]] Allocator alloc_;                       ///< Allocator instance
};

// Deduction guides for BasicQueryBuilder
template <WorldType WorldT>
BasicQueryBuilder(WorldT&) -> BasicQueryBuilder<WorldT, std::allocator<ComponentTypeId>>;

template <WorldType WorldT>
BasicQueryBuilder(WorldT&, const app::AccessPolicy&) -> BasicQueryBuilder<WorldT, std::allocator<ComponentTypeId>>;

template <WorldType WorldT, typename Allocator>
BasicQueryBuilder(WorldT&, Allocator) -> BasicQueryBuilder<WorldT, Allocator>;

template <WorldType WorldT, typename Allocator>
BasicQueryBuilder(WorldT&, const app::AccessPolicy&, Allocator) -> BasicQueryBuilder<WorldT, Allocator>;

/// Type alias for query builder with mutable world access
template <typename Allocator = std::allocator<ComponentTypeId>>
using QueryBuilder = BasicQueryBuilder<World, Allocator>;

/// Type alias for query builder with read-only world access
template <typename Allocator = std::allocator<ComponentTypeId>>
using ReadOnlyQueryBuilder = BasicQueryBuilder<const World, Allocator>;

/// Type alias for query with mutable world access
template <typename Allocator, ComponentTrait... Components>
using Query = BasicQuery<World, Allocator, Components...>;

/// Type alias for query with read-only world access
template <typename Allocator, ComponentTrait... Components>
using ReadOnlyQuery = BasicQuery<const World, Allocator, Components...>;

/// Type alias for query with entity with mutable world access
template <typename Allocator, ComponentTrait... Components>
using QueryWithEntity = BasicQueryWithEntity<World, Allocator, Components...>;

/// Type alias for query with entity with read-only world access
template <typename Allocator, ComponentTrait... Components>
using ReadOnlyQueryWithEntity = BasicQueryWithEntity<const World, Allocator, Components...>;

template <WorldType WorldT, typename Allocator>
template <ComponentTrait... Comps>
  requires utils::UniqueTypes<Comps...>
inline auto BasicQueryBuilder<WorldT, Allocator>::With(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  (self.with_components_.insert(self.with_components_.end(), {ComponentTypeIdOf<Comps>()...}));
  return std::forward<decltype(self)>(self);
}

template <WorldType WorldT, typename Allocator>
template <ComponentTrait... Comps>
  requires utils::UniqueTypes<Comps...>
inline auto BasicQueryBuilder<WorldT, Allocator>::Without(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  (self.without_components_.insert(self.without_components_.end(), {ComponentTypeIdOf<Comps>()...}));
  return std::forward<decltype(self)>(self);
}

template <WorldType WorldT, typename Allocator>
template <ComponentTrait... Comps>
  requires(sizeof...(Comps) > 0 && utils::UniqueTypes<details::ComponentTypeExtractor_t<Comps>...>)
inline auto BasicQueryBuilder<WorldT, Allocator>::Get(this auto&& self) -> BasicQuery<WorldT, Allocator, Comps...> {
  // Compile-time validation: const World requires const Components
  static_assert(details::ValidWorldComponentAccess<WorldT, Comps...>,
                "Cannot request mutable component access from const World! "
                "Use const-qualified components (e.g., 'const Position&') or pass mutable World.");

#ifdef HELIOS_ENABLE_ASSERTS
  if (self.policy_ != nullptr) [[likely]] {
    // Check that all requested component types are declared in at least one query
    const auto queries = self.policy_->GetQueries();

    // Collect all requested component type IDs
    constexpr std::array requested_types = {ComponentTypeIdOf<details::ComponentTypeExtractor_t<Comps>>()...};

    // Check if each requested type is in any query (read or write)
    for (const auto& requested_id : requested_types) {
      bool found = false;
      for (const auto& query : queries) {
        auto matches_id = [requested_id](const auto& info) { return info.type_id == requested_id; };
        if (std::ranges::any_of(query.read_components, matches_id) ||
            std::ranges::any_of(query.write_components, matches_id)) {
          found = true;
          break;
        }
      }

      HELIOS_ASSERT(found,
                    "Attempted to query component that was not declared in app::AccessPolicy! "
                    "Add .Query<>() with this component type to GetAccessPolicy().");
    }
  }
#endif

  // Add required components for Get<> to with_components_ if not already there
  auto add_if_not_present = [&self](ComponentTypeId type_id) {
    if (std::ranges::find(self.with_components_, type_id) == self.with_components_.end()) {
      self.with_components_.push_back(type_id);
    }
  };

  (add_if_not_present(ComponentTypeIdOf<details::ComponentTypeExtractor_t<Comps>>()), ...);
  return {self.world_.get(), std::forward_like<decltype(self)>(self.with_components_),
          std::forward_like<decltype(self)>(self.without_components_), self.alloc_};
}

}  // namespace helios::ecs
