#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/component/archetype.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/manager.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/details/query_args.hpp>
#include <helios/ecs/query/details/traits.hpp>
#include <helios/ecs/query/iterator.hpp>
#include <helios/utils/common_traits.hpp>
#include <helios/utils/functional_adapters.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <memory_resource>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs {

class World;

template <typename WorldT, typename Allocator, QueryArg... Args>
class BasicQuery;

template <typename WorldT, typename Allocator, QueryArg... Args>
class BasicQueryWithEntity;

namespace details {

template <typename... Cs>
[[nodiscard]] inline bool EntityHasComponentsCheck(
    const ComponentManager& manager, Entity entity,
    std::tuple<Cs...>* /*components*/) {
  return ((manager.template Has<std::remove_cvref_t<Cs>>(entity)) && ...);
}

template <typename... Cs>
[[nodiscard]] inline auto FetchComponentsMutable(
    const Archetype& archetype, Entity entity, ComponentManager& manager,
    std::tuple<Cs...>* /*components*/)
    -> std::tuple<ComponentAccessType_t<Cs>...> {
  return std::tuple<ComponentAccessType_t<Cs>...>(
      FetchComponent<Cs>(archetype, entity, manager)...);
}

template <typename... Cs>
[[nodiscard]] inline auto FetchComponentsConst(
    const Archetype& archetype, Entity entity, const ComponentManager& manager,
    std::tuple<Cs...>* /*components*/)
    -> std::tuple<ComponentAccessType_t<Cs>...> {
  return std::tuple<ComponentAccessType_t<Cs>...>(
      FetchComponentConst<Cs>(archetype, entity, manager)...);
}

}  // namespace details

/**
 * @brief Wrapper that provides entity-aware iteration over query results.
 * @details Each dereferenced value is a tuple starting with the `Entity`,
 * followed by the requested component access values.
 *
 * This wrapper is lightweight and holds a reference to the underlying
 * `BasicQuery`. It must not outlive the query it wraps.
 *
 * @note Not thread-safe.
 * @tparam WorldT World type (World or const World)
 * @tparam Allocator Allocator type for internal storage
 * @tparam Args Component access types and optional With/Without filters
 */
template <typename WorldT, typename Allocator, QueryArg... Args>
class BasicQueryWithEntity {
private:
  using Split = details::QueryArgSplit<Args...>;
  using TypeInfo = details::QueryTypeInfo<WorldT, typename Split::Components>;
  static constexpr bool kIsConst = TypeInfo::kIsConst;

public:
  using iterator = typename TypeInfo::template WithEntityIterator<kIsConst>;
  using const_iterator = typename TypeInfo::template WithEntityIterator<true>;
  using value_type = std::iter_value_t<iterator>;
  using difference_type = std::iter_difference_t<iterator>;
  using pointer = typename iterator::pointer;
  using reference = std::iter_reference_t<iterator>;
  using allocator_type = Allocator;

  /**
   * @brief Constructs entity-aware query wrapper.
   * @param query Reference to the underlying query
   */
  explicit BasicQueryWithEntity(
      BasicQuery<WorldT, Allocator, Args...>& query) noexcept
      : query_(query) {}

  BasicQueryWithEntity(const BasicQueryWithEntity&) = delete;
  BasicQueryWithEntity(BasicQueryWithEntity&&) = delete;
  ~BasicQueryWithEntity() noexcept = default;

  BasicQueryWithEntity& operator=(const BasicQueryWithEntity&) = delete;
  BasicQueryWithEntity& operator=(BasicQueryWithEntity&&) = delete;

  /**
   * @brief Gets all query components for a specific entity (ignores query
   * filters).
   * @param entity The entity to get components for
   * @return Tuple of component access types for the entity
   */
  [[nodiscard]] auto Get(Entity entity) const -> value_type {
    return query_.Get(entity);
  }

  /**
   * @brief Tries to get all query components for a specific entity (ignores
   * query filters).
   * @param entity The entity to get components for
   * @return Optional tuple of component access types for the entity
   */
  [[nodiscard]] auto TryGet(Entity entity) const -> std::optional<value_type> {
    return query_.TryGet(entity);
  }

  /**
   * @brief Tries to get all query components for a specific entity while
   * respecting query filters.
   * @param entity The entity to get components for
   * @return Optional tuple of component access types for the entity
   */
  [[nodiscard]] auto TryGetFiltered(Entity entity) const
      -> std::optional<value_type> {
    return query_.TryGetFiltered(entity);
  }

  /**
   * @brief Collects all results into a vector.
   * @return Vector of tuples containing entity and components
   */
  [[nodiscard]] auto Collect() const -> std::vector<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Collects all results into a vector using a custom allocator.
   * @tparam ResultAlloc STL-compatible allocator type for `value_type`
   * @param alloc Allocator instance to use
   * @return Vector of results using the provided allocator
   */
  template <typename ResultAlloc>
    requires std::same_as<typename ResultAlloc::value_type,
                          typename BasicQueryWithEntity<WorldT, Allocator,
                                                        Args...>::value_type> &&
             (!std::derived_from<std::remove_pointer_t<ResultAlloc>,
                                 std::pmr::memory_resource>)
  [[nodiscard]] auto CollectWith(ResultAlloc alloc) const
      -> std::vector<value_type, ResultAlloc>;

  /**
   * @brief Collects all results using a memory resource.
   * @param resource Memory resource
   * @return Vector of results using provided allocator
   */
  [[nodiscard]] auto CollectWith(std::pmr::memory_resource* resource) const
      -> std::pmr::vector<typename BasicQueryWithEntity<WorldT, Allocator,
                                                        Args...>::value_type>;

  auto CollectWith(std::nullptr_t) const -> std::pmr::vector<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type> =
      delete;

  /**
   * @brief Collects all matching entities into a vector.
   * @return Vector of `Entity` objects
   */
  [[nodiscard]] auto CollectEntities() const -> std::vector<Entity>;

  /**
   * @brief Collects all matching entities using a custom allocator.
   * @tparam ResultAlloc STL-compatible allocator type for `Entity`
   * @param alloc Allocator instance
   * @return Vector of entities using the provided allocator
   */
  template <typename ResultAlloc>
    requires std::same_as<typename ResultAlloc::value_type, Entity> &&
             (!std::derived_from<std::remove_pointer_t<ResultAlloc>,
                                 std::pmr::memory_resource>)
  [[nodiscard]] auto CollectEntitiesWith(ResultAlloc alloc) const
      -> std::vector<Entity, ResultAlloc>;

  /**
   * @brief Collects all matching entities using a memory resource.
   * @param resource Memory resource
   * @return Vector of results using provided allocator
   */
  [[nodiscard]] auto CollectEntitiesWith(
      std::pmr::memory_resource* resource) const -> std::pmr::vector<Entity>;

  auto CollectEntitiesWith(std::nullptr_t) const
      -> std::pmr::vector<Entity> = delete;

  /**
   * @brief Writes all query results into an output iterator.
   * @tparam OutIt Output iterator type
   * @param out Output iterator
   */
  template <typename OutIt>
    requires std::output_iterator<
        OutIt,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  void Into(OutIt out);

  /**
   * @brief Executes an action for each matching entity and components.
   * @tparam Action Function type `(Entity, Components...) -> void`
   * @param action Function to execute for each result
   */
  template <typename Action>
    requires utils::ActionFor<
        Action,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  void ForEach(const Action& action) const;

  /**
   * @brief Filters entities based on a predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Lazy filter view
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Filter(Pred predicate) const -> utils::FilterAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
      Pred>;

  /**
   * @brief Transforms each element using a mapping function.
   * @tparam Func Transformation function type
   * @param transform Function to transform each result
   * @return Lazy map view
   */
  template <typename Func>
    requires utils::TransformFor<
        Func,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Map(Func transform) const -> utils::MapAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
      Func>;

  /**
   * @brief Takes only the first N elements.
   * @param count Maximum number of elements to take
   * @return Lazy take view
   */
  [[nodiscard]] auto Take(size_t count) const -> utils::TakeAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Skips the first N elements.
   * @param count Number of elements to skip
   * @return Lazy skip view
   */
  [[nodiscard]] auto Skip(size_t count) const -> utils::SkipAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Takes elements while a predicate is true.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Lazy take-while view
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto TakeWhile(Pred predicate) const -> utils::TakeWhileAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
      Pred>;

  /**
   * @brief Skips elements while a predicate is true.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Lazy skip-while view
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto SkipWhile(Pred predicate) const -> utils::SkipWhileAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
      Pred>;

  /**
   * @brief Adds an index to each element.
   * @return Lazy enumerate view yielding (index, entity, components...) tuples
   */
  [[nodiscard]] auto Enumerate() const -> utils::EnumerateAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Inspects each element without consuming it.
   * @tparam Func Inspection function type
   * @param inspector Function to call on each result
   * @return Lazy inspect view
   */
  template <typename Func>
    requires utils::InspectorFor<
        Func,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Inspect(Func inspector) const -> utils::InspectAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
      Func>;

  /**
   * @brief Yields every Nth element.
   * @param step Interval between yielded elements (must be > 0)
   * @return Lazy step-by view
   */
  [[nodiscard]] auto StepBy(size_t step) const -> utils::StepByAdapter<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Chains this query with another iterator range.
   * @tparam OtherIter Iterator type of the second range
   * @param other_begin Begin iterator of the second range
   * @param other_end End iterator of the second range
   * @return Lazy chain view
   */
  template <typename OtherIter>
    requires utils::ChainAdapterRequirements<iterator, OtherIter>
  [[nodiscard]] auto Chain(OtherIter other_begin, OtherIter other_end) const
      -> utils::ChainAdapter<iterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Chains this query with another query of the same type.
   * @param other Other query to chain after this one
   * @return Lazy chain view
   */
  [[nodiscard]] auto Chain(const BasicQueryWithEntity& other) const
      -> utils::ChainAdapter<iterator, iterator> {
    return {begin(), end(), other.begin(), other.end()};
  }

  /**
   * @brief Chains this query with another range.
   * @tparam R Range type
   * @param range Range to chain after this query
   * @return Lazy chain view
   */
  template <std::ranges::input_range R>
    requires utils::ChainAdapterRequirements<iterator,
                                             std::ranges::iterator_t<R>>
  [[nodiscard]] auto Chain(R& range) const
      -> utils::ChainAdapter<iterator, std::ranges::iterator_t<R>> {
    return {begin(), end(), std::ranges::begin(range), std::ranges::end(range)};
  }

  /**
   * @brief Chains this query with another const range.
   * @tparam R Range type
   * @param range Range to chain after this query
   * @return Lazy chain view
   */
  template <std::ranges::input_range R>
    requires utils::ChainAdapterRequirements<iterator,
                                             std::ranges::iterator_t<const R>>
  [[nodiscard]] auto Chain(const R& range) const
      -> utils::ChainAdapter<iterator, std::ranges::iterator_t<const R>> {
    return {begin(), end(), std::ranges::cbegin(range),
            std::ranges::cend(range)};
  }

  /**
   * @brief Reverses the order of iteration.
   * @return Lazy reverse view
   */
  [[nodiscard]] auto Reverse() const -> utils::ReverseAdapter<iterator> {
    return {begin(), end()};
  }

  /**
   * @brief Creates sliding windows over query results.
   * @param window_size Size of the sliding window
   * @return Lazy slide view
   * @warning window_size must be greater than 0
   */
  [[nodiscard]] auto Slide(size_t window_size) const
      -> utils::SlideAdapter<iterator> {
    return {begin(), end(), window_size};
  }

  /**
   * @brief Takes every Nth element with stride.
   * @param stride Number of elements to skip between yields
   * @return Lazy stride view
   * @warning stride must be greater than 0
   */
  [[nodiscard]] auto Stride(size_t stride) const
      -> utils::StrideAdapter<iterator> {
    return {begin(), end(), stride};
  }

  /**
   * @brief Zips this query with another iterator range.
   * @tparam OtherIter Iterator type to zip with
   * @param other_begin Begin of other range
   * @param other_end End of other range
   * @return Lazy zip view
   */
  template <typename OtherIter>
    requires utils::ZipAdapterRequirements<iterator, OtherIter>
  [[nodiscard]] auto Zip(OtherIter other_begin, OtherIter other_end) const
      -> utils::ZipAdapter<iterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Zips this query with another query of the same type.
   * @param other Query to zip with
   * @return Lazy zip view
   */
  [[nodiscard]] auto Zip(const BasicQueryWithEntity& other) const
      -> utils::ZipAdapter<iterator, iterator> {
    return {begin(), end(), other.begin(), other.end()};
  }

  /**
   * @brief Zips this query with another range.
   * @tparam R Range type
   * @param range Range to zip with
   * @return Lazy zip view
   */
  template <std::ranges::input_range R>
    requires utils::ZipAdapterRequirements<iterator, std::ranges::iterator_t<R>>
  [[nodiscard]] auto Zip(R& range) const
      -> utils::ZipAdapter<iterator, std::ranges::iterator_t<R>> {
    return {begin(), end(), std::ranges::begin(range), std::ranges::end(range)};
  }

  /**
   * @brief Zips this query with another const range.
   * @tparam R Range type
   * @param range Range to zip with
   * @return Lazy zip view
   */
  template <std::ranges::input_range R>
    requires utils::ZipAdapterRequirements<iterator,
                                           std::ranges::iterator_t<const R>>
  [[nodiscard]] auto Zip(const R& range) const
      -> utils::ZipAdapter<iterator, std::ranges::iterator_t<const R>> {
    return {begin(), end(), std::ranges::cbegin(range),
            std::ranges::cend(range)};
  }

  /**
   * @brief Folds query results into a single value.
   * @tparam T Accumulator type
   * @tparam Func Folder function type
   * @param init Initial accumulator value
   * @param folder Function combining accumulator with each result
   * @return Final accumulated value
   */
  template <typename T, typename Func>
    requires utils::FolderFor<
        Func, T,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] T Fold(T init, const Func& folder) const;

  /**
   * @brief Finds the first result matching a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test each result
   * @return First matching result, or `std::nullopt` if none found
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Find(const Pred& predicate) const -> std::optional<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Counts entities matching a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test each result
   * @return Number of matching entities
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] size_t CountIf(const Pred& predicate) const;

  /**
   * @brief Finds the element with the maximum key value.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key
   * @return Element with maximum key, or `std::nullopt` if empty
   */
  template <typename KeyFunc>
    requires utils::TransformFor<
        KeyFunc,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto MaxBy(const KeyFunc& key_func) const -> std::optional<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Finds the element with the minimum key value.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key
   * @return Element with minimum key, or `std::nullopt` if empty
   */
  template <typename KeyFunc>
    requires utils::TransformFor<
        KeyFunc,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto MinBy(const KeyFunc& key_func) const -> std::optional<
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Partitions results into two groups based on a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test each result
   * @return Pair of vectors: (matching results, non-matching results)
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Partition(const Pred& predicate) const
      -> std::pair<std::vector<typename BasicQueryWithEntity<
                       WorldT, Allocator, Args...>::value_type>,
                   std::vector<typename BasicQueryWithEntity<
                       WorldT, Allocator, Args...>::value_type>>;

  /**
   * @brief Groups results by an extracted key.
   * @tparam KeyExtractor Function type returning a hashable key
   * @param key_extractor Function that extracts the grouping key
   * @return Map from keys to vectors of results
   */
  template <typename KeyExtractor>
    requires utils::TransformFor<
        KeyExtractor,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto
  GroupBy(const KeyExtractor& key_extractor) const -> std::unordered_map<
      std::decay_t<utils::details::call_or_apply_result_t<
          const KeyExtractor&, typename BasicQueryWithEntity<
                                   WorldT, Allocator, Args...>::value_type>>,
      std::vector<typename BasicQueryWithEntity<WorldT, Allocator,
                                                Args...>::value_type>>;

  /**
   * @brief Checks if any entity matches the predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test each result
   * @return True if at least one result matches
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] bool Any(const Pred& predicate) const;

  /**
   * @brief Checks if all entities match the predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test each result
   * @return True if all results match
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] bool All(const Pred& predicate) const;

  /**
   * @brief Checks if no entities match the predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test each result
   * @return True if no results match
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, value_type>
  [[nodiscard]] bool None(const Pred& predicate) const {
    return !Any(predicate);
  }

  /**
   * @brief Checks if query matches no entities.
   * @return True if no entities match
   */
  [[nodiscard]] bool Empty() const noexcept { return query_.Empty(); }

  /**
   * @brief Gets the number of matching entities.
   * @return Total count of entities matching the query
   */
  [[nodiscard]] size_t Count() const noexcept { return query_.Count(); }

  /**
   * @brief Gets iterator to first matching entity and components.
   * @return Iterator to the beginning of the query results
   */
  [[nodiscard]] iterator begin() const;

  /**
   * @brief Gets iterator past the last matching entity and components.
   * @return Iterator to the end of the query results
   */
  [[nodiscard]] iterator end() const noexcept;

private:
  BasicQuery<WorldT, Allocator, Args...>& query_;
};

/**
 * @brief Query result object for iterating over matching entities and
 * components.
 * @details `BasicQuery` provides iteration and functional operations over
 * entities matching specified component criteria. Supports const-qualified
 * component access for read-only operations, optional components via pointer
 * types, value copy, mutable reference, const reference, and rvalue reference
 * (move) access patterns.
 *
 * @note Not thread-safe.
 * @tparam WorldT World type (World or const World)
 * @tparam Allocator Allocator type for internal storage
 * @tparam Args Component access types and optional With/Without filters
 *
 * @code
 * auto query = world.Query<Transform&, const Velocity&, const Gravity*,
 *                          Health, With<Player>, Without<Dead>>();
 *
 * for (auto&& [transform, velocity, gravity, health] : query) {
 *   transform.position += velocity.direction * velocity.speed;
 *   if (gravity) {
 *     transform.position.y += gravity->force;
 *   }
 * }
 *
 * query.ForEach([](Transform& transform, const Velocity& velocity,
 *                  const Gravity* gravity, Health health) {
 *   // ...
 * });
 * @endcode
 */
template <typename WorldT, typename Allocator, QueryArg... Args>
class BasicQuery {
private:
  using Split = details::QueryArgSplit<Args...>;
  using TypeInfo = details::QueryTypeInfo<WorldT, typename Split::Components>;
  static constexpr bool kIsConst = TypeInfo::kIsConst;

  static constexpr auto kWithIndices = Split::kWithIndices;
  static constexpr auto kWithoutIndices = Split::kWithoutIndices;

public:
  using ComponentManagerType = typename TypeInfo::ComponentManagerType;
  using WithEntityIterator =
      typename TypeInfo::template WithEntityIterator<kIsConst>;

  using iterator = typename TypeInfo::template Iterator<kIsConst>;
  using const_iterator = typename TypeInfo::template Iterator<true>;
  using value_type = typename TypeInfo::ValueType;
  using difference_type = std::iter_difference_t<iterator>;
  using pointer = typename iterator::pointer;
  using reference = std::iter_reference_t<iterator>;
  using allocator_type = Allocator;

  /**
   * @brief Constructs query with the given component manager and allocator.
   * @details The With/Without component filters are derived at compile time
   * from the template arguments (via With<> and Without<> filter types).
   * @param components Component manager reference
   * @param alloc Allocator instance
   */
  explicit BasicQuery(ComponentManagerType& components, Allocator alloc = {});

  /**
   * @brief Constructs query from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param components Component manager reference
   * @param resource Memory resource used to construct allocator
   */
  BasicQuery(ComponentManagerType& components,
             std::pmr::memory_resource* resource)
    requires std::constructible_from<Allocator, std::pmr::memory_resource*>
      : BasicQuery(components, Allocator{resource}) {}

  BasicQuery(ComponentManagerType&, std::nullptr_t) = delete;

  BasicQuery(const BasicQuery&) = delete;
  BasicQuery(BasicQuery&&) noexcept = default;
  ~BasicQuery() = default;

  BasicQuery& operator=(const BasicQuery&) = delete;
  BasicQuery& operator=(BasicQuery&&) noexcept = default;

  /**
   * @brief Creates wrapper for entity-aware iteration.
   * @note Only callable on lvalue queries to prevent dangling references.
   * @return BasicQueryWithEntity wrapper for this query
   */
  [[nodiscard]] auto WithEntity() & noexcept
      -> BasicQueryWithEntity<WorldT, Allocator, Args...> {
    return BasicQueryWithEntity<WorldT, Allocator, Args...>(*this);
  }

  /**
   * @brief Gets all query components for a specific entity (ignores query
   * filters).
   * @param entity The entity to get components for
   * @return Tuple of component access types for the entity
   */
  [[nodiscard]] auto Get(Entity entity) const ->
      typename BasicQuery<WorldT, Allocator, Args...>::value_type;

  /**
   * @brief Tries to get all query components for a specific entity (ignores
   * query filters).
   * @param entity The entity to get components for
   * @return Optional tuple of component access types for the entity
   */
  [[nodiscard]] auto TryGet(Entity entity) const -> std::optional<
      typename BasicQuery<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Tries to get all query components for a specific entity while
   * respecting query filters.
   * @param entity The entity to get components for
   * @return Optional tuple of component access types for the entity
   */
  [[nodiscard]] auto TryGetFiltered(Entity entity) const -> std::optional<
      typename BasicQuery<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Collects all results into a vector.
   * @return Vector of tuples containing components
   */
  [[nodiscard]] auto Collect() const -> std::vector<
      typename BasicQuery<WorldT, Allocator, Args...>::value_type>;

  /**
   * @brief Collects all results using a custom allocator.
   * @tparam ResultAlloc STL-compatible allocator type for `value_type`
   * @param alloc Allocator instance
   * @return Vector of results using provided allocator
   */
  template <typename ResultAlloc>
    requires std::same_as<
                 typename ResultAlloc::value_type,
                 typename BasicQuery<WorldT, Allocator, Args...>::value_type> &&
             (!std::derived_from<std::remove_pointer_t<ResultAlloc>,
                                 std::pmr::memory_resource>)
  [[nodiscard]] auto CollectWith(ResultAlloc alloc) const -> std::vector<
      typename BasicQuery<WorldT, Allocator, Args...>::value_type, ResultAlloc>;

  /**
   * @brief Collects all results using a memory resource.
   * @param resource Memory resource
   * @return Vector of results using provided allocator
   */
  [[nodiscard]] auto CollectWith(std::pmr::memory_resource* resource) const
      -> std::pmr::vector<
          typename BasicQuery<WorldT, Allocator, Args...>::value_type>;

  auto CollectWith(std::nullptr_t) const -> std::pmr::vector<
      typename BasicQuery<WorldT, Allocator, Args...>::value_type> = delete;

  /**
   * @brief Writes all query results into an output iterator.
   * @tparam OutIt Output iterator type
   * @param out Output iterator
   */
  template <typename OutIt>
    requires std::output_iterator<
        OutIt, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  void Into(OutIt out) const;

  /**
   * @brief Executes an action for each matching entity's components.
   * @tparam Action Function type `(Components...) -> void`
   * @param action Function to execute for each result
   */
  template <typename Action>
    requires utils::ActionFor<
        Action, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  void ForEach(const Action& action) const;

  /**
   * @brief Executes an action for each entity and its components.
   * @tparam Action Function type `(Entity, Components...) -> void`
   * @param action Function to execute for each result
   */
  template <typename Action>
    requires utils::ActionFor<
        Action,
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
  void ForEachWithEntity(const Action& action) const;

  /**
   * @brief Filters query results based on a predicate.
   * @tparam Pred Predicate function type `(Components...) -> bool`
   * @param predicate Function to test each result
   * @return Lazy filter view
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Filter(Pred predicate) const& -> utils::FilterAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator, Pred>;

  /**
   * @brief Transforms each element using a mapping function.
   * @tparam Func Transformation function type `(Components...) -> U`
   * @param transform Function to transform each result
   * @return Lazy map view
   */
  template <typename Func>
    requires utils::TransformFor<
        Func, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Map(Func transform) const& -> utils::MapAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator, Func>;

  /**
   * @brief Takes only the first N elements.
   * @param count Maximum number of elements to take
   * @return Lazy take view
   */
  [[nodiscard]] auto Take(size_t count) const& -> utils::TakeAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Skips the first N elements.
   * @param count Number of elements to skip
   * @return Lazy skip view
   */
  [[nodiscard]] auto Skip(size_t count) const& -> utils::SkipAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Takes elements while a predicate is true.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Lazy take-while view
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto TakeWhile(Pred predicate)
      const& -> utils::TakeWhileAdapter<
          typename BasicQuery<WorldT, Allocator, Args...>::iterator, Pred>;

  /**
   * @brief Skips elements while a predicate is true.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Lazy skip-while view
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto SkipWhile(Pred predicate)
      const& -> utils::SkipWhileAdapter<
          typename BasicQuery<WorldT, Allocator, Args...>::iterator, Pred>;

  /**
   * @brief Adds an index to each element.
   * @return Lazy enumerate view
   */
  [[nodiscard]] auto Enumerate() const& -> utils::EnumerateAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Inspects each element without consuming it.
   * @tparam Func Inspection function type
   * @param inspector Function to call on each result
   * @return Lazy inspect view
   */
  template <typename Func>
    requires utils::InspectorFor<
        Func, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto Inspect(Func inspector) const& -> utils::InspectAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator, Func>;

  /**
   * @brief Yields every Nth element.
   * @param step Interval between yielded elements (must be > 0)
   * @return Lazy step-by view
   */
  [[nodiscard]] auto StepBy(size_t step) const& -> utils::StepByAdapter<
      typename BasicQuery<WorldT, Allocator, Args...>::iterator>;

  /**
   * @brief Chains this query with another iterator range.
   * @tparam OtherIter Iterator type of the second range
   * @param other_begin Begin iterator of the second range
   * @param other_end End iterator of the second range
   * @return Lazy chain view
   */
  template <typename OtherIter>
    requires utils::ChainAdapterRequirements<iterator, OtherIter>
  [[nodiscard]] auto Chain(OtherIter other_begin, OtherIter other_end)
      const& -> utils::ChainAdapter<iterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Chains this query with another query of the same type.
   * @param other Other query to chain after this one
   * @return Lazy chain view
   */
  [[nodiscard]] auto Chain(const BasicQuery& other)
      const& -> utils::ChainAdapter<iterator, iterator> {
    return {begin(), end(), other.begin(), other.end()};
  }

  /**
   * @brief Chains this query with another range.
   * @tparam R Range type
   * @param range Range to chain after this query
   * @return Lazy chain view
   */
  template <std::ranges::input_range R>
    requires utils::ChainAdapterRequirements<iterator,
                                             std::ranges::iterator_t<R>>
  [[nodiscard]] auto Chain(R& range)
      const& -> utils::ChainAdapter<iterator, std::ranges::iterator_t<R>> {
    return {begin(), end(), std::ranges::begin(range), std::ranges::end(range)};
  }

  /**
   * @brief Chains this query with another const range.
   * @tparam R Range type
   * @param range Range to chain after this query
   * @return Lazy chain view
   */
  template <std::ranges::input_range R>
    requires utils::ChainAdapterRequirements<iterator,
                                             std::ranges::iterator_t<const R>>
  [[nodiscard]] auto Chain(const R& range) const& -> utils::ChainAdapter<
      iterator, std::ranges::iterator_t<const R>> {
    return {begin(), end(), std::ranges::cbegin(range),
            std::ranges::cend(range)};
  }

  /**
   * @brief Reverses the order of iteration.
   * @return Lazy reverse view
   */
  [[nodiscard]] auto Reverse() const& -> utils::ReverseAdapter<iterator> {
    return {begin(), end()};
  }

  /**
   * @brief Creates sliding windows over query results.
   * @param window_size Size of the sliding window
   * @return Lazy slide view
   * @warning window_size must be greater than 0
   */
  [[nodiscard]] auto Slide(
      size_t window_size) const& -> utils::SlideAdapter<iterator> {
    return {begin(), end(), window_size};
  }

  /**
   * @brief Takes every Nth element with stride.
   * @param stride Number of elements to skip between yields
   * @return Lazy stride view
   * @warning stride must be greater than 0
   */
  [[nodiscard]] auto Stride(
      size_t stride) const& -> utils::StrideAdapter<iterator> {
    return {begin(), end(), stride};
  }

  /**
   * @brief Zips this query with another iterator range.
   * @tparam OtherIter Iterator type to zip with
   * @param other_begin Begin of other range
   * @param other_end End of other range
   * @return Lazy zip view
   */
  template <typename OtherIter>
    requires utils::ZipAdapterRequirements<iterator, OtherIter>
  [[nodiscard]] auto Zip(OtherIter other_begin, OtherIter other_end)
      const& -> utils::ZipAdapter<iterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Zips this query with another query of the same type.
   * @param other Query to zip with
   * @return Lazy zip view
   */
  [[nodiscard]] auto Zip(
      const BasicQuery& other) const& -> utils::ZipAdapter<iterator, iterator> {
    return {begin(), end(), other.begin(), other.end()};
  }

  /**
   * @brief Zips this query with another range.
   * @tparam R Range type
   * @param range Range to zip with
   * @return Lazy zip view
   */
  template <std::ranges::input_range R>
    requires utils::ZipAdapterRequirements<iterator, std::ranges::iterator_t<R>>
  [[nodiscard]] auto Zip(R& range)
      const& -> utils::ZipAdapter<iterator, std::ranges::iterator_t<R>> {
    return {begin(), end(), std::ranges::begin(range), std::ranges::end(range)};
  }

  /**
   * @brief Zips this query with another const range.
   * @tparam R Range type
   * @param range Range to zip with
   * @return Lazy zip view
   */
  template <std::ranges::input_range R>
    requires utils::ZipAdapterRequirements<iterator,
                                           std::ranges::iterator_t<const R>>
  [[nodiscard]] auto Zip(const R& range)
      const& -> utils::ZipAdapter<iterator, std::ranges::iterator_t<const R>> {
    return {begin(), end(), std::ranges::cbegin(range),
            std::ranges::cend(range)};
  }

  /**
   * @brief Folds query results into a single value.
   * @tparam T Accumulator type
   * @tparam Func Folder function type `(T, Components...) -> T`
   * @param init Initial accumulator value
   * @param folder Function combining accumulator with each result
   * @return Final accumulated value
   */
  template <typename T, typename Func>
    requires utils::FolderFor<
        Func, T, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] T Fold(T init, const Func& folder) const;

  /**
   * @brief Finds the first element matching a predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return First matching element, or `std::nullopt` if none found
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, value_type>
  [[nodiscard]] auto Find(const Pred& predicate) const
      -> std::optional<value_type> {
    return begin().Find(predicate);
  }

  /**
   * @brief Counts elements matching a predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Number of matching elements
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] size_t CountIf(const Pred& predicate) const;

  /**
   * @brief Partitions elements into two groups based on a predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return Pair of vectors: (matching, non-matching)
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, value_type>
  [[nodiscard]] auto Partition(const Pred& predicate) const
      -> std::pair<std::vector<value_type>, std::vector<value_type>> {
    return begin().Partition(predicate);
  }

  /**
   * @brief Finds the element with the maximum key value.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key
   * @return Element with maximum key, or `std::nullopt` if empty
   */
  template <typename KeyFunc>
    requires utils::TransformFor<KeyFunc, value_type>
  [[nodiscard]] auto MaxBy(const KeyFunc& key_func) const
      -> std::optional<value_type> {
    return begin().MaxBy(key_func);
  }

  /**
   * @brief Finds the element with the minimum key value.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key
   * @return Element with minimum key, or `std::nullopt` if empty
   */
  template <typename KeyFunc>
    requires utils::TransformFor<KeyFunc, value_type>
  [[nodiscard]] auto MinBy(const KeyFunc& key_func) const
      -> std::optional<value_type> {
    return begin().MinBy(key_func);
  }

  /**
   * @brief Groups elements by a key extracted from each result.
   * @tparam KeyExtractor Key extraction function type
   * @param key_extractor Function that extracts the grouping key
   * @return Map from keys to vectors of matching elements
   */
  template <typename KeyExtractor>
    requires utils::TransformFor<
        KeyExtractor,
        typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] auto
  GroupBy(const KeyExtractor& key_extractor) const -> std::unordered_map<
      std::decay_t<utils::details::call_or_apply_result_t<
          const KeyExtractor&,
          typename BasicQuery<WorldT, Allocator, Args...>::value_type>>,
      std::vector<typename BasicQuery<WorldT, Allocator, Args...>::value_type>>;

  /**
   * @brief Checks if any element matches the predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return True if at least one result matches
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, value_type>
  [[nodiscard]] bool Any(const Pred& predicate) const {
    return Find(predicate).has_value();
  }

  /**
   * @brief Checks if all elements match the predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return True if all results match
   */
  template <typename Pred>
    requires utils::PredicateFor<
        Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
  [[nodiscard]] bool All(const Pred& predicate) const;

  /**
   * @brief Checks if no elements match the predicate.
   * @tparam Pred Predicate function type
   * @param predicate Function to test each result
   * @return True if no results match
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, value_type>
  [[nodiscard]] bool None(const Pred& predicate) const {
    return !Any(predicate);
  }

  /**
   * @brief Checks if query matches no entities.
   * @return True if no entities match
   */
  [[nodiscard]] bool Empty() const noexcept;

  /**
   * @brief Gets the number of matching entities.
   * @details O(A) where A is number of matching archetypes.
   * @return Total count of entities matching the query
   */
  [[nodiscard]] size_t Count() const noexcept;

  /**
   * @brief Gets iterator to the first matching entity and components.
   * @return Iterator to the beginning of the query results
   */
  [[nodiscard]] iterator begin() const;

  /**
   * @brief Gets iterator past the last matching entity and components.
   * @return Iterator to the end of the query results
   */
  [[nodiscard]] iterator end() const {
    RefreshArchetypes();
    return {matching_archetypes_, GetComponentManager(),
            matching_archetypes_.size(), 0, WithoutTypes()};
  }

  /**
   * @brief Gets excluded component type indices for this query.
   * @return Span of component type indices that are excluded by this query
   */
  [[nodiscard]] auto WithoutTypes() const noexcept
      -> std::span<const ComponentTypeIndex> {
    return {kWithoutIndices.data(), kWithoutIndices.size()};
  }

  /**
   * @brief Gets the allocator used for internal storage.
   * @return Allocator instance
   */
  [[nodiscard]] allocator_type get_allocator() const
      noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) {
    return alloc_;
  }

private:
  [[nodiscard]] bool EntityHasComponents(Entity entity) const;
  [[nodiscard]] bool EntityPassesFilters(Entity entity) const;
  [[nodiscard]] bool EntityPassesExclusions(Entity entity) const;

  /// @brief Refreshes the cached list of matching archetypes.
  void RefreshArchetypes() const;

  /// @brief Gets the component manager with correct const qualification.
  [[nodiscard]] ComponentManagerType& GetComponentManager() const noexcept {
    return components_.get();
  }

  /// @brief Gets the matching archetypes span.
  [[nodiscard]] auto GetMatchingArchetypes() const noexcept
      -> std::span<const std::reference_wrapper<const Archetype>> {
    return matching_archetypes_;
  }

  std::reference_wrapper<ComponentManagerType> components_;

  /// Rebind allocator for archetype references
  using ArchetypeAllocator =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<
          std::reference_wrapper<const Archetype>>;
  mutable std::vector<std::reference_wrapper<const Archetype>,
                      ArchetypeAllocator>
      matching_archetypes_;

  [[no_unique_address]] allocator_type alloc_;

  friend class BasicQueryWithEntity<WorldT, Allocator, Args...>;
};

template <typename WorldT, typename Allocator, QueryArg... Args>
inline BasicQuery<WorldT, Allocator, Args...>::BasicQuery(
    ComponentManagerType& components, Allocator alloc)
    : components_(components),
      matching_archetypes_(ArchetypeAllocator(alloc)),
      alloc_(alloc) {
  static_assert(
      !std::is_const_v<std::remove_reference_t<WorldT>> || TypeInfo::kAllConst,
      "Cannot request mutable component access from const World! "
      "Use const-qualified components (e.g., 'const Position&') or "
      "pass mutable World!");
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::Get(Entity entity) const ->
    typename BasicQuery<WorldT, Allocator, Args...>::value_type {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const auto& manager = GetComponentManager();
  HELIOS_ASSERT(
      manager.Tracked(entity),
      "Entity '{}' is not tracked, most likely it does not exist in the world!",
      entity);

  HELIOS_ASSERT(EntityHasComponents(entity),
                "Entity '{}' does not have all requested components!", entity);

  const auto& archetype = manager.EntityArchetype(entity);
  if constexpr (kIsConst) {
    return details::FetchComponentsConst(
        archetype, entity, manager,
        static_cast<typename Split::Components*>(nullptr));
  } else {
    return details::FetchComponentsMutable(
        archetype, entity, GetComponentManager(),
        static_cast<typename Split::Components*>(nullptr));
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::TryGet(Entity entity) const
    -> std::optional<
        typename BasicQuery<WorldT, Allocator, Args...>::value_type> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const auto& manager = GetComponentManager();
  HELIOS_ASSERT(
      manager.Tracked(entity),
      "Entity '{}' is not tracked, most likely it does not exist in the world!",
      entity);

  if (!EntityHasComponents(entity)) {
    return std::nullopt;
  }

  const auto& archetype = manager.EntityArchetype(entity);
  if constexpr (kIsConst) {
    return details::FetchComponentsConst(
        archetype, entity, manager,
        static_cast<typename Split::Components*>(nullptr));
  } else {
    return details::FetchComponentsMutable(
        archetype, entity, GetComponentManager(),
        static_cast<typename Split::Components*>(nullptr));
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::TryGetFiltered(
    Entity entity) const
    -> std::optional<
        typename BasicQuery<WorldT, Allocator, Args...>::value_type> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const auto& manager = GetComponentManager();
  HELIOS_ASSERT(
      manager.Tracked(entity),
      "Entity '{}' is not tracked, most likely it does not exist in the world!",
      entity);

  if (!EntityPassesFilters(entity)) {
    return std::nullopt;
  }

  if (!EntityPassesExclusions(entity)) {
    return std::nullopt;
  }

  const auto& archetype = manager.EntityArchetype(entity);
  if constexpr (kIsConst) {
    return details::FetchComponentsConst(
        archetype, entity, manager,
        static_cast<typename Split::Components*>(nullptr));
  } else {
    return details::FetchComponentsMutable(
        archetype, entity, GetComponentManager(),
        static_cast<typename Split::Components*>(nullptr));
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::Collect() const
    -> std::vector<
        typename BasicQuery<WorldT, Allocator, Args...>::value_type> {
  std::vector<value_type> result;
  result.reserve(Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename ResultAlloc>
  requires std::same_as<
               typename ResultAlloc::value_type,
               typename BasicQuery<WorldT, Allocator, Args...>::value_type> &&
           (!std::derived_from<std::remove_pointer_t<ResultAlloc>,
                               std::pmr::memory_resource>)
inline auto BasicQuery<WorldT, Allocator, Args...>::CollectWith(
    ResultAlloc alloc) const
    -> std::vector<typename BasicQuery<WorldT, Allocator, Args...>::value_type,
                   ResultAlloc> {
  std::vector<value_type, ResultAlloc> result{std::move(alloc)};
  result.reserve(Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::CollectWith(
    std::pmr::memory_resource* resource) const
    -> std::pmr::vector<
        typename BasicQuery<WorldT, Allocator, Args...>::value_type> {
  std::pmr::vector<value_type> result{resource};
  result.reserve(Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename OutIt>
  requires std::output_iterator<
      OutIt, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline void BasicQuery<WorldT, Allocator, Args...>::Into(OutIt out) const {
  for (auto&& result : *this) {
    *out++ = std::forward<decltype(result)>(result);
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Action>
  requires utils::ActionFor<
      Action, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline void BasicQuery<WorldT, Allocator, Args...>::ForEach(
    const Action& action) const {
  for (auto&& result : *this) {
    if constexpr (std::invocable<Action, decltype(result)>) {
      action(std::forward<decltype(result)>(result));
    } else {
      std::apply(action, std::forward<decltype(result)>(result));
    }
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Action>
  requires utils::ActionFor<Action, typename BasicQueryWithEntity<
                                        WorldT, Allocator, Args...>::value_type>
inline void BasicQuery<WorldT, Allocator, Args...>::ForEachWithEntity(
    const Action& action) const {
  RefreshArchetypes();
  auto begin_iter =
      WithEntityIterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter =
      WithEntityIterator(matching_archetypes_, GetComponentManager(),
                         matching_archetypes_.size(), 0);
  for (auto iter = begin_iter; iter != end_iter; ++iter) {
    if constexpr (std::invocable<Action, decltype(*iter)>) {
      action(*iter);
    } else {
      std::apply(action, *iter);
    }
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline auto BasicQuery<WorldT, Allocator, Args...>::Filter(Pred predicate)
    const& -> utils::FilterAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator, Pred> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Func>
  requires utils::TransformFor<
      Func, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline auto BasicQuery<WorldT, Allocator, Args...>::Map(Func transform)
    const& -> utils::MapAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator, Func> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(transform)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::Take(size_t count)
    const& -> utils::TakeAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, count};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::Skip(size_t count)
    const& -> utils::SkipAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, count};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline auto BasicQuery<WorldT, Allocator, Args...>::TakeWhile(Pred predicate)
    const& -> utils::TakeWhileAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator, Pred> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline auto BasicQuery<WorldT, Allocator, Args...>::SkipWhile(Pred predicate)
    const& -> utils::SkipWhileAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator, Pred> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::Enumerate()
    const& -> utils::EnumerateAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Func>
  requires utils::InspectorFor<
      Func, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline auto BasicQuery<WorldT, Allocator, Args...>::Inspect(Func inspector)
    const& -> utils::InspectAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator, Func> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, std::move(inspector)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::StepBy(size_t step)
    const& -> utils::StepByAdapter<
        typename BasicQuery<WorldT, Allocator, Args...>::iterator> {
  RefreshArchetypes();
  auto begin_iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0);
  auto end_iter = iterator(matching_archetypes_, GetComponentManager(),
                           matching_archetypes_.size(), 0);
  return {begin_iter, end_iter, step};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline bool BasicQuery<WorldT, Allocator, Args...>::All(
    const Pred& predicate) const {
  for (auto&& result : *this) {
    bool matches = false;
    if constexpr (std::invocable<Pred, decltype(result)>) {
      matches = predicate(std::forward<decltype(result)>(result));
    } else {
      matches = std::apply(predicate, std::forward<decltype(result)>(result));
    }
    if (!matches) {
      return false;
    }
  }
  return true;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline size_t BasicQuery<WorldT, Allocator, Args...>::CountIf(
    const Pred& predicate) const {
  size_t count = 0;
  for (auto&& result : *this) {
    bool matches = false;
    if constexpr (std::invocable<Pred, decltype(result)>) {
      matches = predicate(std::forward<decltype(result)>(result));
    } else {
      matches = std::apply(predicate, std::forward<decltype(result)>(result));
    }
    if (matches) {
      ++count;
    }
  }
  return count;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename T, typename Func>
  requires utils::FolderFor<
      Func, T, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline T BasicQuery<WorldT, Allocator, Args...>::Fold(
    T init, const Func& folder) const {
  for (auto&& tuple : *this) {
    if constexpr (std::invocable<Func, T, decltype(tuple)>) {
      init = folder(std::move(init), std::forward<decltype(tuple)>(tuple));
    } else {
      init = std::apply(
          [&init, &folder](auto&&... components) {
            return folder(std::move(init),
                          std::forward<decltype(components)>(components)...);
          },
          tuple);
    }
  }
  return init;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename KeyExtractor>
  requires utils::TransformFor<
      KeyExtractor, typename BasicQuery<WorldT, Allocator, Args...>::value_type>
inline auto BasicQuery<WorldT, Allocator, Args...>::GroupBy(
    const KeyExtractor& key_extractor) const
    -> std::unordered_map<
        std::decay_t<utils::details::call_or_apply_result_t<
            const KeyExtractor&,
            typename BasicQuery<WorldT, Allocator, Args...>::value_type>>,
        std::vector<
            typename BasicQuery<WorldT, Allocator, Args...>::value_type>> {
  using KeyType = std::decay_t<
      utils::details::call_or_apply_result_t<const KeyExtractor&, value_type>>;
  std::unordered_map<KeyType, std::vector<value_type>> groups;

  for (auto&& result : *this) {
    auto key = [&key_extractor](auto&& value) {
      if constexpr (std::invocable<KeyExtractor, decltype(value)>) {
        return key_extractor(std::forward<decltype(value)>(value));
      } else {
        return std::apply(key_extractor, std::forward<decltype(value)>(value));
      }
    }(std::forward<decltype(result)>(result));
    groups[key].push_back(result);
  }

  return groups;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline bool BasicQuery<WorldT, Allocator, Args...>::Empty() const noexcept {
  RefreshArchetypes();
  return matching_archetypes_.empty() || Count() == 0;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline size_t BasicQuery<WorldT, Allocator, Args...>::Count() const noexcept {
  RefreshArchetypes();
  size_t count = 0;
  auto iter = iterator(matching_archetypes_, GetComponentManager(), 0, 0,
                       WithoutTypes());
  const auto end_iter =
      iterator(matching_archetypes_, GetComponentManager(),
               matching_archetypes_.size(), 0, WithoutTypes());
  while (iter != end_iter) {
    ++count;
    ++iter;
  }
  return count;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQuery<WorldT, Allocator, Args...>::begin() const ->
    typename BasicQuery<WorldT, Allocator, Args...>::iterator {
  RefreshArchetypes();
  return {matching_archetypes_, GetComponentManager(), 0, 0, WithoutTypes()};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline bool BasicQuery<WorldT, Allocator, Args...>::EntityHasComponents(
    Entity entity) const {
  if constexpr (std::tuple_size_v<typename Split::Components> == 0) {
    return true;
  } else {
    return details::EntityHasComponentsCheck(
        GetComponentManager(), entity,
        static_cast<typename Split::Components*>(nullptr));
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline bool BasicQuery<WorldT, Allocator, Args...>::EntityPassesFilters(
    Entity entity) const {
  const auto& manager = GetComponentManager();

  for (const auto type : kWithIndices) {
    const auto* meta = manager.MetadataByIndex(type);
    if (meta == nullptr) [[unlikely]] {
      return false;
    }

    if (meta->storage_type == ComponentStorageType::kSparseSet) {
      const auto* entry = manager.SparseEntry(type);
      if (entry == nullptr || !entry->Contains(entity)) {
        return false;
      }
    } else {
      const auto& archetype = manager.EntityArchetype(entity);
      if (!archetype.Id().Contains(type)) {
        return false;
      }
    }
  }

  return EntityHasComponents(entity);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline bool BasicQuery<WorldT, Allocator, Args...>::EntityPassesExclusions(
    Entity entity) const {
  const auto& manager = GetComponentManager();
  return std::ranges::none_of(kWithoutIndices, [&manager, entity](auto type) {
    const auto* meta = manager.MetadataByIndex(type);
    if (meta == nullptr) {
      return false;
    }
    if (meta->storage_type == ComponentStorageType::kSparseSet) {
      const auto* entry = manager.SparseEntry(type);
      return entry != nullptr && entry->Contains(entity);
    }
    const auto& archetype = manager.EntityArchetype(entity);
    return archetype.Id().Contains(type);
  });
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline void BasicQuery<WorldT, Allocator, Args...>::RefreshArchetypes() const {
  matching_archetypes_.clear();

  auto archetypes = [this]() {
    if constexpr (kIsConst) {
      return std::as_const(components_.get()).Archetypes();
    } else {
      return components_.get().Archetypes();
    }
  }();

  for (const auto& arch_ref : archetypes) {
    const auto& archetype = arch_ref.get();
    if (archetype.Empty()) {
      continue;
    }

    const auto& arch_id = archetype.Id();

    auto arch_contains_type = [this,
                               &arch_id](ComponentTypeIndex type) -> bool {
      const auto* meta = components_.get().MetadataByIndex(type);
      if (meta == nullptr) {
        return false;
      }
      if (meta->storage_type == ComponentStorageType::kSparseSet) {
        return true;
      }
      return arch_id.Contains(type);
    };

    bool has_all_with = std::ranges::all_of(kWithIndices, arch_contains_type);
    if (!has_all_with) {
      continue;
    }

    bool has_all_comp = [this, &arch_id]<typename... CompTypes>(
                            std::tuple<CompTypes...>*) {
      return (
          ([this, &arch_id]() -> bool {
            if constexpr (details::kIsPointerAccess<CompTypes>) {
              return true;
            } else {
              using RawType = details::ComponentTypeExtractor_t<CompTypes>;
              const auto type_idx = ComponentTypeIndex::From<RawType>();
              const auto* meta =
                  std::as_const(components_.get()).MetadataByIndex(type_idx);
              if (meta == nullptr) {
                return false;
              }
              if (meta->storage_type == ComponentStorageType::kSparseSet) {
                return true;
              }
              return arch_id.Contains(type_idx);
            }
          }()) &&
          ...);
    }(static_cast<typename Split::Components*>(nullptr));
    if (!has_all_comp) {
      continue;
    }

    bool has_any_excluded = std::ranges::any_of(
        kWithoutIndices, [this, &arch_id](ComponentTypeIndex type) {
          const auto* meta = components_.get().MetadataByIndex(type);
          if (meta == nullptr) {
            return false;
          }
          if (meta->storage_type == ComponentStorageType::kSparseSet) {
            return false;
          }
          return arch_id.Contains(type);
        });
    if (has_any_excluded) {
      continue;
    }

    matching_archetypes_.push_back(std::cref(archetype));
  }
}

// BasicQueryWithEntity out-of-line definitions

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Collect() const
    -> std::vector<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type> {
  std::vector<value_type> result;
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename ResultAlloc>
  requires std::same_as<typename ResultAlloc::value_type,
                        typename BasicQueryWithEntity<WorldT, Allocator,
                                                      Args...>::value_type> &&
           (!std::derived_from<std::remove_pointer_t<ResultAlloc>,
                               std::pmr::memory_resource>)
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::CollectWith(
    ResultAlloc alloc) const
    -> std::vector<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type,
        ResultAlloc> {
  std::vector<value_type, ResultAlloc> result{std::move(alloc)};
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::CollectWith(
    std::pmr::memory_resource* resource) const
    -> std::pmr::vector<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type> {
  std::pmr::vector<value_type> result{resource};
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(tuple);
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::CollectEntities()
    const -> std::vector<Entity> {
  std::vector<Entity> result;
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(std::get<0>(tuple));
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename ResultAlloc>
  requires std::same_as<typename ResultAlloc::value_type, Entity> &&
           (!std::derived_from<std::remove_pointer_t<ResultAlloc>,
                               std::pmr::memory_resource>)
inline auto
BasicQueryWithEntity<WorldT, Allocator, Args...>::CollectEntitiesWith(
    ResultAlloc alloc) const -> std::vector<Entity, ResultAlloc> {
  std::vector<Entity, ResultAlloc> result{std::move(alloc)};
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(std::get<0>(tuple));
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto
BasicQueryWithEntity<WorldT, Allocator, Args...>::CollectEntitiesWith(
    std::pmr::memory_resource* resource) const -> std::pmr::vector<Entity> {
  std::pmr::vector<Entity> result{resource};
  result.reserve(query_.Count());
  for (auto&& tuple : *this) {
    result.push_back(std::get<0>(tuple));
  }
  return result;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename OutIt>
  requires std::output_iterator<
      OutIt,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline void BasicQueryWithEntity<WorldT, Allocator, Args...>::Into(OutIt out) {
  for (auto&& result : *this) {
    *out++ = std::forward<decltype(result)>(result);
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Action>
  requires utils::ActionFor<Action, typename BasicQueryWithEntity<
                                        WorldT, Allocator, Args...>::value_type>
inline void BasicQueryWithEntity<WorldT, Allocator, Args...>::ForEach(
    const Action& action) const {
  for (auto&& result : *this) {
    if constexpr (std::invocable<Action, decltype(result)>) {
      action(std::forward<decltype(result)>(result));
    } else {
      std::apply(action, std::forward<decltype(result)>(result));
    }
  }
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Filter(
    Pred predicate) const
    -> utils::FilterAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
        Pred> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Func>
  requires utils::TransformFor<
      Func,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Map(
    Func transform) const
    -> utils::MapAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
        Func> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(transform)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Take(size_t count)
    const -> utils::TakeAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, count};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Skip(size_t count)
    const -> utils::SkipAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, count};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::TakeWhile(
    Pred predicate) const
    -> utils::TakeWhileAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
        Pred> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::SkipWhile(
    Pred predicate) const
    -> utils::SkipWhileAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
        Pred> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(predicate)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Enumerate() const
    -> utils::EnumerateAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Func>
  requires utils::InspectorFor<
      Func,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Inspect(
    Func inspector) const
    -> utils::InspectAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator,
        Func> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, std::move(inspector)};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::StepBy(
    size_t step) const
    -> utils::StepByAdapter<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::iterator> {
  query_.RefreshArchetypes();
  auto begin_iter = iterator(query_.GetMatchingArchetypes(),
                             query_.GetComponentManager(), 0, 0);
  auto end_iter =
      iterator(query_.GetMatchingArchetypes(), query_.GetComponentManager(),
               query_.GetMatchingArchetypes().size(), 0);
  return {begin_iter, end_iter, step};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline bool BasicQueryWithEntity<WorldT, Allocator, Args...>::Any(
    const Pred& predicate) const {
  return Find(predicate).has_value();
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline bool BasicQueryWithEntity<WorldT, Allocator, Args...>::All(
    const Pred& predicate) const {
  for (auto&& result : *this) {
    bool matches = false;
    if constexpr (std::invocable<Pred, decltype(result)>) {
      matches = predicate(std::forward<decltype(result)>(result));
    } else {
      matches = std::apply(predicate, std::forward<decltype(result)>(result));
    }
    if (!matches) {
      return false;
    }
  }
  return true;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Find(
    const Pred& predicate) const
    -> std::optional<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type> {
  return begin().Find(predicate);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline size_t BasicQueryWithEntity<WorldT, Allocator, Args...>::CountIf(
    const Pred& predicate) const {
  return begin().CountIf(predicate);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename T, typename Func>
  requires utils::FolderFor<
      Func, T,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline T BasicQueryWithEntity<WorldT, Allocator, Args...>::Fold(
    T init, const Func& folder) const {
  return begin().Fold(std::move(init), folder);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename KeyFunc>
  requires utils::TransformFor<
      KeyFunc,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::MaxBy(
    const KeyFunc& key_func) const
    -> std::optional<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type> {
  return begin().MaxBy(key_func);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename KeyFunc>
  requires utils::TransformFor<
      KeyFunc,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::MinBy(
    const KeyFunc& key_func) const
    -> std::optional<
        typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type> {
  return begin().MinBy(key_func);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename Pred>
  requires utils::PredicateFor<
      Pred,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::Partition(
    const Pred& predicate) const
    -> std::pair<std::vector<typename BasicQueryWithEntity<
                     WorldT, Allocator, Args...>::value_type>,
                 std::vector<typename BasicQueryWithEntity<
                     WorldT, Allocator, Args...>::value_type>> {
  return begin().Partition(predicate);
}

template <typename WorldT, typename Allocator, QueryArg... Args>
template <typename KeyExtractor>
  requires utils::TransformFor<
      KeyExtractor,
      typename BasicQueryWithEntity<WorldT, Allocator, Args...>::value_type>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::GroupBy(
    const KeyExtractor& key_extractor) const
    -> std::unordered_map<
        std::decay_t<utils::details::call_or_apply_result_t<
            const KeyExtractor&, typename BasicQueryWithEntity<
                                     WorldT, Allocator, Args...>::value_type>>,
        std::vector<typename BasicQueryWithEntity<WorldT, Allocator,
                                                  Args...>::value_type>> {
  using KeyType = std::decay_t<
      utils::details::call_or_apply_result_t<const KeyExtractor&, value_type>>;
  std::unordered_map<KeyType, std::vector<value_type>> groups;

  for (auto&& result : *this) {
    auto key = [&key_extractor](auto&& value) {
      if constexpr (std::invocable<KeyExtractor, decltype(value)>) {
        return key_extractor(std::forward<decltype(value)>(value));
      } else {
        return std::apply(key_extractor, std::forward<decltype(value)>(value));
      }
    }(std::forward<decltype(result)>(result));
    groups[key].push_back(result);
  }

  return groups;
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::begin() const
    -> iterator {
  query_.RefreshArchetypes();
  return {query_.GetMatchingArchetypes(), query_.GetComponentManager(), 0, 0,
          query_.WithoutTypes()};
}

template <typename WorldT, typename Allocator, QueryArg... Args>
inline auto BasicQueryWithEntity<WorldT, Allocator, Args...>::end()
    const noexcept -> iterator {
  return {query_.GetMatchingArchetypes(), query_.GetComponentManager(),
          query_.GetMatchingArchetypes().size(), 0};
}

template <typename Alloc, QueryArg... Args>
using MutBasicQuery = BasicQuery<World, Alloc, Args...>;

template <typename Alloc, QueryArg... Args>
using ReadOnlyBasicQuery = BasicQuery<const World, Alloc, Args...>;

template <typename Alloc, QueryArg... Args>
using MutBasicQueryWithEntity = BasicQueryWithEntity<World, Alloc, Args...>;

template <typename Alloc, QueryArg... Args>
using ReadOnlyBasicQueryWithEntity =
    BasicQueryWithEntity<const World, Alloc, Args...>;

template <QueryArg... Args>
using PmrBasicQuery =
    BasicQuery<World, std::pmr::polymorphic_allocator<>, Args...>;

template <QueryArg... Args>
using PmrReadOnlyBasicQuery =
    BasicQuery<const World, std::pmr::polymorphic_allocator<>, Args...>;

template <QueryArg... Args>
using PmrBasicQueryWithEntity =
    BasicQueryWithEntity<World, std::pmr::polymorphic_allocator<>, Args...>;

template <QueryArg... Args>
using PmrReadOnlyBasicQueryWithEntity =
    BasicQueryWithEntity<const World, std::pmr::polymorphic_allocator<>,
                         Args...>;

/**
 * @brief Alias for `BasicQuery` bound to `World` with a PMR allocator.
 * @details The canonical query type for system parameters. Users who need a
 * different world type or allocator can use `BasicQuery` directly.
 * @tparam Args Component access types and optional With/Without filters
 *
 * @code
 * void MySystem(Query<Transform&, const Velocity&, With<Player>> query) {
 *   for (auto&& [transform, velocity] : query) { ... }
 * }
 * @endcode
 */
template <QueryArg... Args>
using Query = BasicQuery<World, std::pmr::polymorphic_allocator<>, Args...>;

}  // namespace helios::ecs
