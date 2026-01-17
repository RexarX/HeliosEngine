#pragma once

#include <helios/core_pch.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::utils {

namespace details {

/**
 * @brief Concept to check if a type is tuple-like (has tuple_size and get).
 * @details This is used to determine if std::tuple_cat can be applied to a type.
 * @tparam T Type to check
 */
template <typename T>
concept TupleLike = requires {
  typename std::tuple_size<std::remove_cvref_t<T>>::type;
  requires std::tuple_size_v<std::remove_cvref_t<T>> >= 0;
};

/**
 * @brief Helper to extract tuple element types and check if a folder is invocable with accumulator + tuple elements
 */
template <typename Folder, typename Accumulator, typename Tuple>
struct is_folder_applicable_impl : std::false_type {};

template <typename Folder, typename Accumulator, typename... TupleArgs>
struct is_folder_applicable_impl<Folder, Accumulator, std::tuple<TupleArgs...>>
    : std::bool_constant<std::invocable<Folder, Accumulator, TupleArgs...>> {};

template <typename Folder, typename Accumulator, typename Tuple>
inline constexpr bool is_folder_applicable_v = is_folder_applicable_impl<Folder, Accumulator, Tuple>::value;

/**
 * @brief Helper to get the result type of folder applied with accumulator + tuple elements
 */
template <typename Folder, typename Accumulator, typename Tuple>
struct folder_apply_result;

template <typename Folder, typename Accumulator, typename... TupleArgs>
struct folder_apply_result<Folder, Accumulator, std::tuple<TupleArgs...>> {
  using type = std::invoke_result_t<Folder, Accumulator, TupleArgs...>;
};

template <typename Folder, typename Accumulator, typename Tuple>
using folder_apply_result_t = typename folder_apply_result<Folder, Accumulator, Tuple>::type;

/**
 * @brief Helper to get the result type of either invoke or apply
 */
template <typename Func, typename... Args>
consteval auto get_call_or_apply_result_type() noexcept {
  if constexpr (std::invocable<Func, Args...>) {
    return std::type_identity<std::invoke_result_t<Func, Args...>>{};
  } else if (sizeof...(Args) == 1) {
    return std::type_identity<decltype(std::apply(std::declval<Func>(), std::declval<Args>()...))>{};
  } else {
    static_assert(sizeof...(Args) == 1,
                  "Callable must be invocable with Args or via std::apply with a single tuple argument");
  }
}

template <typename Func, typename... Args>
using call_or_apply_result_t = typename decltype(get_call_or_apply_result_type<Func, Args...>())::type;

template <typename Func, typename... Args>
concept CallableOrApplicable =
    std::invocable<Func, Args...> ||
    (sizeof...(Args) == 1 && requires(Func func, Args&&... args) { std::apply(func, std::forward<Args>(args)...); });

template <typename Func, typename ReturnType, typename... Args>
concept CallableOrApplicableWithReturn =
    CallableOrApplicable<Func, Args...> && std::convertible_to<call_or_apply_result_t<Func, Args...>, ReturnType>;

}  // namespace details

/**
 * @brief Concept for types that can be used as base iterators in adapters.
 * @tparam T Type to check
 */
template <typename T>
concept IteratorLike = requires(T iter, const T const_iter) {
  typename std::iter_value_t<T>;
  { ++iter } -> std::same_as<std::add_lvalue_reference_t<std::remove_cvref_t<T>>>;
  { iter++ } -> std::same_as<std::remove_cvref_t<T>>;
  { *const_iter } -> std::convertible_to<std::iter_value_t<T>>;
  { const_iter == const_iter } -> std::convertible_to<bool>;
  { const_iter != const_iter } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for bidirectional iterators that support decrement operations.
 * @tparam T Type to check
 */
template <typename T>
concept BidirectionalIteratorLike = IteratorLike<T> && requires(T iter) {
  { --iter } -> std::same_as<std::add_lvalue_reference_t<std::remove_cvref_t<T>>>;
};

/**
 * @brief Concept for predicate functions that can be applied to iterator values.
 * @details The predicate must be invocable with the value type and return a boolean-testable result.
 * Supports both direct invocation and std::apply for tuple unpacking.
 * The result must be usable in boolean contexts (if statements, etc.).
 * Allows const ref consumption even if the original type is just ref or value.
 * @tparam Pred Predicate type
 * @tparam ValueType Type of values to test
 */
template <typename Pred, typename ValueType>
concept PredicateFor = details::CallableOrApplicableWithReturn<Pred, bool, ValueType> ||
                       details::CallableOrApplicableWithReturn<
                           Pred, bool, std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<ValueType>>>> ||
                       details::CallableOrApplicableWithReturn<Pred, bool, std::remove_cvref_t<ValueType>>;

/**
 * @brief Concept for transformation functions that can be applied to iterator values.
 * @details The function must be invocable with the value type and return a non-void result.
 * Supports both direct invocation and std::apply for tuple unpacking.
 * Allows const ref consumption even if the original type is just ref or value.
 * @tparam Func Function type
 * @tparam ValueType Type of values to transform
 */
template <typename Func, typename ValueType>
concept TransformFor =
    (details::CallableOrApplicable<Func, ValueType> &&
     !std::is_void_v<details::call_or_apply_result_t<Func, ValueType>>) ||
    (details::CallableOrApplicable<Func,
                                   std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<ValueType>>>> &&
     !std::is_void_v<details::call_or_apply_result_t<
         Func, std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<ValueType>>>>>) ||
    (details::CallableOrApplicable<Func, std::remove_cvref_t<ValueType>> &&
     !std::is_void_v<details::call_or_apply_result_t<Func, std::remove_cvref_t<ValueType>>>);

/**
 * @brief Concept for inspection functions that observe but don't modify values.
 * @details The function must be invocable with the value type and return void.
 * Supports both direct invocation and std::apply for tuple unpacking.
 * Allows const ref consumption even if the original type is just ref or value.
 * @tparam Func Function type
 * @tparam ValueType Type of values to inspect
 */
template <typename Func, typename ValueType>
concept InspectorFor = details::CallableOrApplicableWithReturn<Func, void, ValueType> ||
                       details::CallableOrApplicableWithReturn<
                           Func, void, std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<ValueType>>>> ||
                       details::CallableOrApplicableWithReturn<Func, void, std::remove_cvref_t<ValueType>>;

/**
 * @brief Concept for action functions that process values.
 * @details The function must be invocable with the value type.
 * Supports both direct invocation and std::apply for tuple unpacking.
 * Allows const ref consumption even if the original type is just ref or value.
 * @tparam Action Action function type
 * @tparam ValueType Type of values to process
 */
template <typename Action, typename ValueType>
concept ActionFor = details::CallableOrApplicableWithReturn<Action, void, ValueType> ||
                    details::CallableOrApplicableWithReturn<
                        Action, void, std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<ValueType>>>> ||
                    details::CallableOrApplicableWithReturn<Action, void, std::remove_cvref_t<ValueType>>;

/**
 * @brief Concept for folder functions that accumulate values.
 * @details The function must be invocable with an accumulator and a value type.
 * Supports both direct invocation and std::apply for tuple unpacking.
 * @tparam Folder Folder function type
 * @tparam Accumulator Accumulator type
 * @tparam ValueType Type of values to fold
 */
template <typename Folder, typename Accumulator, typename ValueType>
concept FolderFor =
    (std::invocable<Folder, Accumulator, ValueType> &&
     std::convertible_to<std::invoke_result_t<Folder, Accumulator, ValueType>, Accumulator>) ||
    (details::is_folder_applicable_v<Folder, Accumulator, std::remove_cvref_t<ValueType>> &&
     std::convertible_to<details::folder_apply_result_t<Folder, Accumulator, std::remove_cvref_t<ValueType>>,
                         Accumulator>);

/**
 * @brief Concept to validate FilterAdapter requirements.
 * @tparam Iter Iterator type
 * @tparam Pred Predicate type
 */
template <typename Iter, typename Pred>
concept FilterAdapterRequirements = IteratorLike<Iter> && PredicateFor<Pred, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate MapAdapter requirements.
 * @tparam Iter Iterator type
 * @tparam Func Transform function type
 */
template <typename Iter, typename Func>
concept MapAdapterRequirements = IteratorLike<Iter> && TransformFor<Func, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate TakeAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept TakeAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate SkipAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept SkipAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate TakeWhileAdapter requirements.
 * @tparam Iter Iterator type
 * @tparam Pred Predicate type
 */
template <typename Iter, typename Pred>
concept TakeWhileAdapterRequirements = IteratorLike<Iter> && PredicateFor<Pred, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate SkipWhileAdapter requirements.
 * @tparam Iter Iterator type
 * @tparam Pred Predicate type
 */
template <typename Iter, typename Pred>
concept SkipWhileAdapterRequirements = IteratorLike<Iter> && PredicateFor<Pred, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate EnumerateAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept EnumerateAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate InspectAdapter requirements.
 * @tparam Iter Iterator type
 * @tparam Func Inspector function type
 */
template <typename Iter, typename Func>
concept InspectAdapterRequirements = IteratorLike<Iter> && InspectorFor<Func, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate StepByAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept StepByAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate ChainAdapter requirements.
 * @tparam Iter1 First iterator type
 * @tparam Iter2 Second iterator type
 */
template <typename Iter1, typename Iter2>
concept ChainAdapterRequirements =
    IteratorLike<Iter1> && IteratorLike<Iter2> && std::same_as<std::iter_value_t<Iter1>, std::iter_value_t<Iter2>>;

/**
 * @brief Concept to validate ReverseAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept ReverseAdapterRequirements = BidirectionalIteratorLike<Iter>;

/**
 * @brief Concept to validate JoinAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept JoinAdapterRequirements = IteratorLike<Iter> && std::ranges::range<std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate SlideAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept SlideAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate ConcatAdapter requirements.
 * @details ConcatAdapter concatenates two ranges of the same value type.
 * @tparam Iter Iterator type (same type for both ranges)
 */
template <typename Iter>
concept ConcatAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate StrideAdapter requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept StrideAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate ZipAdapter requirements.
 * @tparam Iter1 First iterator type
 * @tparam Iter2 Second iterator type
 */
template <typename Iter1, typename Iter2>
concept ZipAdapterRequirements = IteratorLike<Iter1> && IteratorLike<Iter2>;

template <typename Derived>
class FunctionalAdapterBase;

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
class FilterAdapter;

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
class MapAdapter;

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
class TakeAdapter;

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
class SkipAdapter;

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
class TakeWhileAdapter;

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
class SkipWhileAdapter;

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
class EnumerateAdapter;

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
class InspectAdapter;

template <typename Iter>
  requires StepByAdapterRequirements<Iter>
class StepByAdapter;

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
class ChainAdapter;

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
class ReverseAdapter;

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
class JoinAdapter;

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
class SlideAdapter;

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
class StrideAdapter;

template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
class ZipAdapter;

template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
class ConcatAdapter;

/**
 * @brief Iterator adapter that filters elements based on a predicate function.
 * @details This adapter provides lazy filtering of iterator values.
 * The filtering happens during iteration, not upfront, making it memory efficient for large sequences.
 *
 * Supports chaining with other adapters for complex data transformations.
 *
 * @note This adapter maintains forward iterator semantics.
 * @note The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 * @tparam Pred Predicate function type
 *
 * @example
 * @code
 * auto enemies = query.Filter([](const Health& h) {
 *   return h.points > 0;
 * });
 * @endcode
 */
template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
class FilterAdapter : public FunctionalAdapterBase<FilterAdapter<Iter, Pred>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a filter adapter with the given iterator range and predicate.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param predicate Function to filter elements
   */
  constexpr FilterAdapter(Iter begin, Iter end, Pred predicate) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                                         std::is_nothrow_move_constructible_v<Iter> &&
                                                                         std::is_nothrow_move_constructible_v<Pred> &&
                                                                         noexcept(AdvanceToValid()))
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), predicate_(std::move(predicate)) {
    AdvanceToValid();
  }

  constexpr FilterAdapter(const FilterAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                         std::is_nothrow_copy_constructible_v<Pred>) = default;
  constexpr FilterAdapter(FilterAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                    std::is_nothrow_move_constructible_v<Pred>) = default;
  constexpr ~FilterAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                      std::is_nothrow_destructible_v<Pred>) = default;

  constexpr FilterAdapter& operator=(const FilterAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter> &&
                                                                    std::is_nothrow_copy_assignable_v<Pred>) = default;
  constexpr FilterAdapter& operator=(FilterAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter> &&
                                                               std::is_nothrow_move_assignable_v<Pred>) = default;

  constexpr FilterAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()) && noexcept(AdvanceToValid()));
  constexpr FilterAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<FilterAdapter> &&
                                                   noexcept(++std::declval<FilterAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  [[nodiscard]] constexpr bool operator==(const FilterAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const FilterAdapter& other) const
      noexcept(noexcept(std::declval<const FilterAdapter&>() == std::declval<const FilterAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the iterator has reached the end.
   * @return True if at end, false otherwise
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == end_;
  }

  [[nodiscard]] constexpr FilterAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<FilterAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr FilterAdapter end() const
      noexcept(std::is_nothrow_constructible_v<FilterAdapter, const Iter&, const Iter&, const Pred&>) {
    return {end_, end_, predicate_};
  }

private:
  constexpr void AdvanceToValid() noexcept(std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
                                           noexcept(*std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()) &&
                                           noexcept(std::declval<Iter&>() != std::declval<Iter&>()));

  Iter begin_;      ///< Start of the iterator range
  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Pred predicate_;  ///< Predicate function for filtering
};

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
constexpr auto FilterAdapter<Iter, Pred>::operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                                noexcept(AdvanceToValid())) -> FilterAdapter& {
  ++current_;
  AdvanceToValid();
  return *this;
}

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
constexpr auto FilterAdapter<Iter, Pred>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<FilterAdapter> && noexcept(++std::declval<FilterAdapter&>()))
    -> FilterAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
constexpr void FilterAdapter<Iter, Pred>::AdvanceToValid() noexcept(
    std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> && noexcept(*std::declval<Iter&>()) &&
    noexcept(++std::declval<Iter&>()) && noexcept(std::declval<Iter&>() != std::declval<Iter&>())) {
  while (current_ != end_) {
    auto value = *current_;
    bool matches = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      matches = predicate_(value);
    } else {
      matches = std::apply(predicate_, value);
    }
    if (matches) {
      break;
    }
    ++current_;
  }
}

/**
 * @brief Iterator adapter that transforms each element using a function.
 * @details This adapter applies a transformation function to each element during iteration.
 * The transformation is lazy - it happens when the element is accessed, not when the adapter is created.
 *
 * The transformation function can accept either the full value or individual tuple
 * components if the value is a tuple type.
 *
 * @note This adapter maintains forward iterator semantics.
 * The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 * @tparam Func Transformation function type
 *
 * @example
 * @code
 * auto positions = query.Map([](const Transform& t) {
 *   return t.position;
 * });
 * @endcode
 */
template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
class MapAdapter : public FunctionalAdapterBase<MapAdapter<Iter, Func>> {
private:
  template <typename T>
  struct DeduceValueType;

  template <typename... Args>
  struct DeduceValueType<std::tuple<Args...>> {
    using Type = decltype(std::apply(std::declval<Func>(), std::declval<std::tuple<Args...>>()));
  };

  template <typename T>
    requires(!requires { typename std::tuple_size<T>::type; })
  struct DeduceValueType<T> {
    using Type = std::invoke_result_t<Func, T>;
  };

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = typename DeduceValueType<std::iter_value_t<Iter>>::Type;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a map adapter with the given iterator range and transform function.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param transform Function to transform elements
   */
  constexpr MapAdapter(Iter begin, Iter end, Func transform) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                      std::is_nothrow_move_constructible_v<Func>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), transform_(std::move(transform)) {}

  constexpr MapAdapter(const MapAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                   std::is_nothrow_copy_constructible_v<Func>) = default;
  constexpr MapAdapter(MapAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                              std::is_nothrow_move_constructible_v<Func>) = default;
  constexpr ~MapAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                   std::is_nothrow_destructible_v<Func>) = default;

  constexpr MapAdapter& operator=(const MapAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter> &&
                                                              std::is_nothrow_copy_assignable_v<Func>) = default;
  constexpr MapAdapter& operator=(MapAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter> &&
                                                         std::is_nothrow_move_assignable_v<Func>) = default;

  constexpr MapAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()));
  constexpr MapAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<MapAdapter> &&
                                                noexcept(++std::declval<MapAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const
      noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> && noexcept(*std::declval<const Iter&>()));

  [[nodiscard]] constexpr bool operator==(const MapAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }
  [[nodiscard]] constexpr bool operator!=(const MapAdapter& other) const
      noexcept(noexcept(std::declval<const MapAdapter&>() == std::declval<const MapAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr MapAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<MapAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr MapAdapter end() const
      noexcept(std::is_nothrow_constructible_v<MapAdapter, const Iter&, const Iter&, const Func&>) {
    return {end_, end_, transform_};
  }

private:
  Iter begin_;      ///< Start of the iterator range
  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Func transform_;  ///< Transformation function
};

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
constexpr auto MapAdapter<Iter, Func>::operator++() noexcept(noexcept(++std::declval<Iter&>())) -> MapAdapter& {
  ++current_;
  return *this;
}

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
constexpr auto MapAdapter<Iter, Func>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<MapAdapter> &&
                                                                noexcept(++std::declval<MapAdapter&>())) -> MapAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
constexpr auto MapAdapter<Iter, Func>::operator*() const
    noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> && noexcept(*std::declval<const Iter&>()))
        -> value_type {
  auto value = *current_;
  if constexpr (std::invocable<Func, decltype(value)>) {
    return transform_(value);
  } else {
    return std::apply(transform_, value);
  }
}

/**
 * @brief Iterator adapter that yields only the first N elements.
 * @details This adapter limits the number of elements yielded by the underlying iterator to at most the specified
 * count. Once the count is reached or the underlying iterator reaches its end, iteration stops.
 * @note This adapter maintains forward iterator semantics.
 * @note The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 *
 * @example
 * @code
 * // Get only the first 5 entities
 * auto first_five = query.Take(5);
 * @endcode
 */
template <typename Iter>
  requires TakeAdapterRequirements<Iter>
class TakeAdapter : public FunctionalAdapterBase<TakeAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a take adapter with the given iterator range and count.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param count Maximum number of elements to yield
   */
  constexpr TakeAdapter(Iter begin, Iter end, size_t count) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                     std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), initial_count_(count), remaining_(count) {}

  constexpr TakeAdapter(const TakeAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr TakeAdapter(TakeAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~TakeAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) = default;

  constexpr TakeAdapter& operator=(const TakeAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr TakeAdapter& operator=(TakeAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr TakeAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()));
  constexpr TakeAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter> &&
                                                 noexcept(++std::declval<TakeAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  [[nodiscard]] constexpr bool operator==(const TakeAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>()));

  [[nodiscard]] constexpr bool operator!=(const TakeAdapter& other) const
      noexcept(noexcept(std::declval<const TakeAdapter&>() == std::declval<const TakeAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the iterator has reached the end.
   * @return True if at end, false otherwise
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return remaining_ == 0 || current_ == end_;
  }

  [[nodiscard]] constexpr TakeAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr TakeAdapter end() const noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter>);

private:
  Iter begin_;                ///< Start of the iterator range
  Iter current_;              ///< Current position in the iteration
  Iter end_;                  ///< End of the iterator range
  size_t initial_count_ = 0;  ///< Initial count limit
  size_t remaining_ = 0;      ///< Remaining elements to yield
};

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr auto TakeAdapter<Iter>::operator++() noexcept(noexcept(++std::declval<Iter&>())) -> TakeAdapter& {
  if (remaining_ > 0 && current_ != end_) {
    ++current_;
    --remaining_;
  }
  return *this;
}

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr auto TakeAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter> &&
                                                           noexcept(++std::declval<TakeAdapter&>())) -> TakeAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr bool TakeAdapter<Iter>::operator==(const TakeAdapter& other) const
    noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
  // Both are at end if either has no remaining elements or both iterators are at end
  const bool this_at_end = (remaining_ == 0) || (current_ == end_);
  const bool other_at_end = (other.remaining_ == 0) || (other.current_ == other.end_);

  if (this_at_end && other_at_end) {
    return true;
  }

  return (current_ == other.current_) && (remaining_ == other.remaining_);
}

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr auto TakeAdapter<Iter>::end() const noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter>)
    -> TakeAdapter {
  auto end_iter = *this;
  end_iter.remaining_ = 0;
  return end_iter;
}

/**
 * @brief Iterator adapter that skips the first N elements.
 * @details This adapter skips over the first N elements of the underlying iterator and yields all remaining elements.
 * If the iterator has fewer than N elements, the result will be empty.
 * @note This adapter maintains forward iterator semantics.
 * @note The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 *
 * @example
 * @code
 * // Skip the first 10 entities
 * auto remaining = query.Skip(10);
 * @endcode
 */
template <typename Iter>
  requires SkipAdapterRequirements<Iter>
class SkipAdapter : public FunctionalAdapterBase<SkipAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a skip adapter with the given iterator range and count.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param count Number of elements to skip
   */
  constexpr SkipAdapter(Iter begin, Iter end, size_t count) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                     noexcept(++std::declval<Iter&>()));

  constexpr SkipAdapter(const SkipAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr SkipAdapter(SkipAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~SkipAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) = default;

  constexpr SkipAdapter& operator=(const SkipAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr SkipAdapter& operator=(SkipAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr SkipAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()));
  constexpr SkipAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<SkipAdapter> &&
                                                 noexcept(++std::declval<SkipAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  [[nodiscard]] constexpr bool operator==(const SkipAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const SkipAdapter& other) const
      noexcept(noexcept(std::declval<const SkipAdapter&>() == std::declval<const SkipAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr SkipAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<SkipAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr SkipAdapter end() const
      noexcept(std::is_nothrow_constructible_v<SkipAdapter, const Iter&, const Iter&, size_t>) {
    return {end_, end_, 0};
  }

private:
  Iter current_;  ///< Current position in the iteration
  Iter end_;      ///< End of the iterator range
};

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
constexpr SkipAdapter<Iter>::SkipAdapter(Iter begin, Iter end,
                                         size_t count) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                noexcept(++std::declval<Iter&>()))
    : current_(std::move(begin)), end_(std::move(end)) {
  for (size_t idx = 0; idx < count && current_ != end_; ++idx) {
    ++current_;
  }
}

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
constexpr auto SkipAdapter<Iter>::operator++() noexcept(noexcept(++std::declval<Iter&>())) -> SkipAdapter& {
  ++current_;
  return *this;
}

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
constexpr auto SkipAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<SkipAdapter> &&
                                                           noexcept(++std::declval<SkipAdapter&>())) -> SkipAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

/**
 * @brief Iterator adapter that takes elements while a predicate returns true.
 * @details This adapter yields elements as long as the predicate returns true.
 * Once the predicate returns false, no more elements are yielded.
 * @tparam Iter Underlying iterator type
 * @tparam Pred Predicate function type
 *
 * @example
 * @code
 * // Take entities while their health is above zero
 * auto alive_entities = query.TakeWhile([](const Health& h) { return h.points > 0; });
 * @endcode
 */
template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
class TakeWhileAdapter : public FunctionalAdapterBase<TakeWhileAdapter<Iter, Pred>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a take-while adapter with the given iterator range and predicate.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param predicate Function to test elements
   */
  constexpr TakeWhileAdapter(Iter begin, Iter end,
                             Pred predicate) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                      std::is_nothrow_copy_constructible_v<Iter> &&
                                                      std::is_nothrow_move_constructible_v<Pred> &&
                                                      noexcept(CheckPredicate()))
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), predicate_(std::move(predicate)) {
    CheckPredicate();
  }

  constexpr TakeWhileAdapter(const TakeWhileAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                               std::is_nothrow_copy_constructible_v<Pred>) = default;
  constexpr TakeWhileAdapter(TakeWhileAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                          std::is_nothrow_move_constructible_v<Pred>) = default;
  constexpr ~TakeWhileAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                         std::is_nothrow_destructible_v<Pred>) = default;

  constexpr TakeWhileAdapter& operator=(const TakeWhileAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> && std::is_nothrow_copy_assignable_v<Pred>) = default;
  constexpr TakeWhileAdapter& operator=(TakeWhileAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter> &&
                                                                     std::is_nothrow_move_assignable_v<Pred>) = default;

  constexpr TakeWhileAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                    noexcept(CheckPredicate()));
  constexpr TakeWhileAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter> &&
                                                      noexcept(++std::declval<TakeWhileAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  [[nodiscard]] constexpr bool operator==(const TakeWhileAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return (stopped_ && other.stopped_) || (current_ == other.current_ && stopped_ == other.stopped_);
  }

  [[nodiscard]] constexpr bool operator!=(const TakeWhileAdapter& other) const
      noexcept(noexcept(std::declval<const TakeWhileAdapter&>() == std::declval<const TakeWhileAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the iterator has reached the end.
   * @return True if at end, false otherwise
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return stopped_ || current_ == end_;
  }

  [[nodiscard]] constexpr TakeWhileAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr TakeWhileAdapter end() const noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter>);

private:
  constexpr void CheckPredicate() noexcept(std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
                                           noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                           noexcept(*std::declval<Iter&>()));

  Iter begin_;            ///< Start of the iterator range
  Iter current_;          ///< Current position in the iteration
  Iter end_;              ///< End of the iterator range
  Pred predicate_;        ///< Predicate function
  bool stopped_ = false;  ///< Whether predicate has failed
};

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr auto TakeWhileAdapter<Iter, Pred>::operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                                   noexcept(std::declval<Iter&>() !=
                                                                            std::declval<Iter&>()) &&
                                                                   noexcept(CheckPredicate())) -> TakeWhileAdapter& {
  if (!stopped_ && current_ != end_) {
    ++current_;
    CheckPredicate();
  }
  return *this;
}

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr auto TakeWhileAdapter<Iter, Pred>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<TakeWhileAdapter> && noexcept(++std::declval<TakeWhileAdapter&>()))
    -> TakeWhileAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr auto TakeWhileAdapter<Iter, Pred>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter>) -> TakeWhileAdapter {
  auto end_iter = *this;
  end_iter.stopped_ = true;
  return end_iter;
}

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr void TakeWhileAdapter<Iter, Pred>::CheckPredicate() noexcept(
    std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
    noexcept(std::declval<Iter&>() == std::declval<Iter&>()) && noexcept(*std::declval<Iter&>())) {
  if (stopped_ || current_ == end_) {
    stopped_ = true;
    return;
  }

  auto value = *current_;
  bool matches = false;
  if constexpr (std::invocable<Pred, decltype(value)>) {
    matches = predicate_(value);
  } else {
    matches = std::apply(predicate_, value);
  }
  if (!matches) {
    stopped_ = true;
  }
}

/**
 * @brief Iterator adapter that skips elements while a predicate returns true.
 * @tparam Iter Underlying iterator type
 * @tparam Pred Predicate function type
 *
 * @example
 * @code
 * // Skip entities while their health is zero
 * auto non_zero_health = query.SkipWhile([](const Health& h) { return h.points == 0; });
 * @endcode
 */
template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
class SkipWhileAdapter : public FunctionalAdapterBase<SkipWhileAdapter<Iter, Pred>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a skip-while adapter with the given iterator range and predicate.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param predicate Function to test elements
   */
  constexpr SkipWhileAdapter(Iter begin, Iter end,
                             Pred predicate) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                      std::is_nothrow_move_constructible_v<Pred> &&
                                                      noexcept(AdvancePastSkipped()))
      : current_(std::move(begin)), end_(std::move(end)), predicate_(std::move(predicate)) {
    AdvancePastSkipped();
  }

  constexpr SkipWhileAdapter(const SkipWhileAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                               std::is_nothrow_copy_constructible_v<Pred>) = default;
  constexpr SkipWhileAdapter(SkipWhileAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                          std::is_nothrow_move_constructible_v<Pred>) = default;
  constexpr ~SkipWhileAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                         std::is_nothrow_destructible_v<Pred>) = default;

  constexpr SkipWhileAdapter& operator=(const SkipWhileAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> && std::is_nothrow_copy_assignable_v<Pred>) = default;
  constexpr SkipWhileAdapter& operator=(SkipWhileAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter> &&
                                                                     std::is_nothrow_move_assignable_v<Pred>) = default;

  constexpr SkipWhileAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()));
  constexpr SkipWhileAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<SkipWhileAdapter> &&
                                                      noexcept(++std::declval<SkipWhileAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  [[nodiscard]] constexpr bool operator==(const SkipWhileAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const SkipWhileAdapter& other) const
      noexcept(noexcept(std::declval<const SkipWhileAdapter&>() == std::declval<const SkipWhileAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr SkipWhileAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<SkipWhileAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr SkipWhileAdapter end() const
      noexcept(std::is_nothrow_constructible_v<SkipWhileAdapter, const Iter&, const Iter&, const Pred&>) {
    return {end_, end_, predicate_};
  }

private:
  constexpr void AdvancePastSkipped() noexcept(std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
                                               noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                               noexcept(*std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()));

  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Pred predicate_;  ///< Predicate function
};

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
constexpr auto SkipWhileAdapter<Iter, Pred>::operator++() noexcept(noexcept(++std::declval<Iter&>()))
    -> SkipWhileAdapter& {
  ++current_;
  return *this;
}

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
constexpr auto SkipWhileAdapter<Iter, Pred>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<SkipWhileAdapter> && noexcept(++std::declval<SkipWhileAdapter&>()))
    -> SkipWhileAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
constexpr void SkipWhileAdapter<Iter, Pred>::AdvancePastSkipped() noexcept(
    std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) && noexcept(*std::declval<Iter&>()) &&
    noexcept(++std::declval<Iter&>())) {
  while (current_ != end_) {
    auto value = *current_;
    bool matches = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      matches = predicate_(value);
    } else {
      matches = std::apply(predicate_, value);
    }
    if (!matches) {
      break;
    }
    ++current_;
  }
}

/**
 * @brief Iterator adapter that adds index information to each element.
 * @details This adapter yields tuples of (index, value) where index starts at 0 and increments for each element.
 * @tparam Iter Underlying iterator type
 *
 * @example
 * @code
 * // Enumerate entities with their indices
 * auto enumerated = query.Enumerate();
 * for (const auto& [index, _] : enumerated) {
 *   // Use index and components
 * }
 * @endcode
 */
template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
class EnumerateAdapter : public FunctionalAdapterBase<EnumerateAdapter<Iter>> {
private:
  template <typename T>
  struct MakeEnumeratedValue {
    using type = std::tuple<size_t, T>;
  };

  template <typename... Args>
  struct MakeEnumeratedValue<std::tuple<Args...>> {
    using type = std::tuple<size_t, Args...>;
  };

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = typename MakeEnumeratedValue<std::iter_value_t<Iter>>::type;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs an enumerate adapter with the given iterator range.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   */
  constexpr EnumerateAdapter(Iter begin, Iter end) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                            std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)) {}

  constexpr EnumerateAdapter(const EnumerateAdapter&) = default;
  constexpr EnumerateAdapter(EnumerateAdapter&&) = default;
  constexpr ~EnumerateAdapter() = default;

  constexpr EnumerateAdapter& operator=(const EnumerateAdapter&) = default;
  constexpr EnumerateAdapter& operator=(EnumerateAdapter&&) = default;

  constexpr EnumerateAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()));
  constexpr EnumerateAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<EnumerateAdapter> &&
                                                      noexcept(++std::declval<EnumerateAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const;

  [[nodiscard]] constexpr bool operator==(const EnumerateAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const EnumerateAdapter& other) const
      noexcept(noexcept(std::declval<const EnumerateAdapter&>() == std::declval<const EnumerateAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr EnumerateAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<EnumerateAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr EnumerateAdapter end() const
      noexcept(std::is_nothrow_constructible_v<EnumerateAdapter, const Iter&, const Iter&>) {
    return {end_, end_};
  }

private:
  Iter begin_;        ///< Start of the iterator range
  Iter current_;      ///< Current position in the iteration
  Iter end_;          ///< End of the iterator range
  size_t index_ = 0;  ///< Current index in the enumeration
};

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
constexpr auto EnumerateAdapter<Iter>::operator++() noexcept(noexcept(++std::declval<Iter&>())) -> EnumerateAdapter& {
  ++current_;
  ++index_;
  return *this;
}

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
constexpr auto EnumerateAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<EnumerateAdapter> && noexcept(++std::declval<EnumerateAdapter&>()))
    -> EnumerateAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
constexpr auto EnumerateAdapter<Iter>::operator*() const -> value_type {
  auto value = *current_;
  if constexpr (details::TupleLike<decltype(value)>) {
    return std::tuple_cat(std::tuple{index_}, value);
  } else {
    return std::tuple{index_, value};
  }
}

/**
 * @brief Iterator adapter that applies a function to each element for observation.
 * @details This adapter allows observing elements without modifying them.
 * The inspector function is called for side effects only.
 * @tparam Iter Underlying iterator type
 * @tparam Func Inspector function type
 *
 * @example
 * @code
 * // Inspect entities to log their IDs
 * auto inspected = query.Inspect([](const Entity& e) { HELIOS_INFO("{}", e.id); });
 * @endcode
 */
template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
class InspectAdapter : public FunctionalAdapterBase<InspectAdapter<Iter, Func>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs an inspect adapter with the given iterator range and inspector function.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param inspector Function to call for each element
   */
  constexpr InspectAdapter(Iter begin, Iter end, Func inspector) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                          std::is_nothrow_copy_constructible_v<Iter> &&
                                                                          std::is_nothrow_move_constructible_v<Func>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), inspector_(std::move(inspector)) {}

  constexpr InspectAdapter(const InspectAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                           std::is_nothrow_copy_constructible_v<Func>) = default;
  constexpr InspectAdapter(InspectAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                      std::is_nothrow_move_constructible_v<Func>) = default;
  constexpr ~InspectAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                       std::is_nothrow_destructible_v<Func>) = default;

  constexpr InspectAdapter& operator=(const InspectAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> && std::is_nothrow_copy_assignable_v<Func>) = default;
  constexpr InspectAdapter& operator=(InspectAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter> &&
                                                                 std::is_nothrow_move_assignable_v<Func>) = default;

  constexpr InspectAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()));
  constexpr InspectAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<InspectAdapter> &&
                                                    noexcept(++std::declval<InspectAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const
      noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> && noexcept(*std::declval<const Iter&>()));

  [[nodiscard]] constexpr bool operator==(const InspectAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const InspectAdapter& other) const
      noexcept(noexcept(std::declval<const InspectAdapter&>() == std::declval<const InspectAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr InspectAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<InspectAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr InspectAdapter end() const
      noexcept(std::is_nothrow_constructible_v<InspectAdapter, const Iter&, const Iter&, const Func&>) {
    return {end_, end_, inspector_};
  }

private:
  Iter begin_;      ///< Start of the iterator range
  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Func inspector_;  ///< Inspector function
};

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
constexpr auto InspectAdapter<Iter, Func>::operator++() noexcept(noexcept(++std::declval<Iter&>())) -> InspectAdapter& {
  ++current_;
  return *this;
}

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
constexpr auto InspectAdapter<Iter, Func>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<InspectAdapter> && noexcept(++std::declval<InspectAdapter&>()))
    -> InspectAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
constexpr auto InspectAdapter<Iter, Func>::operator*() const
    noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> && noexcept(*std::declval<const Iter&>()))
        -> std::iter_value_t<Iter> {
  auto value = *current_;
  if constexpr (std::invocable<Func, decltype(value)>) {
    inspector_(value);
  } else {
    std::apply(inspector_, value);
  }
  return value;
}

/**
 * @brief Iterator adapter that steps through elements by a specified stride.
 * @details This adapter yields every step-th element from the underlying iterator.
 * @tparam Iter Underlying iterator type
 *
 * @example
 * @code
 * // Step through entities by 3
 * auto stepped = query.StepBy(3);
 * @endcode
 */
template <typename Iter>
  requires StepByAdapterRequirements<Iter>
class StepByAdapter : public FunctionalAdapterBase<StepByAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a step-by adapter with the given iterator range and step size.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param step Number of elements to step by
   */
  constexpr StepByAdapter(Iter begin, Iter end, size_t step) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                      std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), step_(step > 0 ? step : 1) {}

  constexpr StepByAdapter(const StepByAdapter&) = default;
  constexpr StepByAdapter(StepByAdapter&&) = default;
  constexpr ~StepByAdapter() = default;

  constexpr StepByAdapter& operator=(const StepByAdapter&) = default;
  constexpr StepByAdapter& operator=(StepByAdapter&&) = default;

  constexpr StepByAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                 noexcept(std::declval<Iter&>() != std::declval<Iter&>()));
  constexpr StepByAdapter operator++(int) noexcept(std::is_copy_constructible_v<StepByAdapter> &&
                                                   noexcept(++std::declval<StepByAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  [[nodiscard]] constexpr bool operator==(const StepByAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const StepByAdapter& other) const
      noexcept(noexcept(std::declval<const StepByAdapter&>() == std::declval<const StepByAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr StepByAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<StepByAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr StepByAdapter end() const
      noexcept(std::is_nothrow_constructible_v<StepByAdapter, const Iter&, const Iter&, size_t>) {
    return {end_, end_, step_};
  }

private:
  Iter begin_;       ///< Start of the iterator range
  Iter current_;     ///< Current position in the iteration
  Iter end_;         ///< End of the iterator range
  size_t step_ = 0;  ///< Step size between elements
};

template <typename Iter>
  requires StepByAdapterRequirements<Iter>
constexpr auto StepByAdapter<Iter>::operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                          noexcept(std::declval<Iter&>() != std::declval<Iter&>()))
    -> StepByAdapter& {
  for (size_t i = 0; i < step_ && current_ != end_; ++i) {
    ++current_;
  }
  return *this;
}

template <typename Iter>
  requires StepByAdapterRequirements<Iter>
constexpr auto StepByAdapter<Iter>::operator++(int) noexcept(std::is_copy_constructible_v<StepByAdapter> &&
                                                             noexcept(++std::declval<StepByAdapter&>()))
    -> StepByAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

/**
 * @brief Iterator adapter that chains two sequences together.
 * @details This adapter yields all elements from the first iterator, then all elements from the second iterator.
 * @tparam Iter1 First iterator type
 * @tparam Iter2 Second iterator type
 *
 * @example
 * @code
 * // Chain two `Health` queries together
 * auto chained = query1.Chain(query2);
 * chained.ForEach([](const Health& h) {
 *   // Process health components from both queries
 * });
 * @endcode
 */
template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
class ChainAdapter : public FunctionalAdapterBase<ChainAdapter<Iter1, Iter2>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter1>;
  using difference_type = std::common_type_t<std::iter_difference_t<Iter1>, std::iter_difference_t<Iter2>>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a chain adapter with two iterator ranges.
   * @param first_begin Start of the first iterator range
   * @param first_end End of the first iterator range
   * @param second_begin Start of the second iterator range
   * @param second_end End of the second iterator range
   */
  constexpr ChainAdapter(Iter1 first_begin, Iter1 first_end, Iter2 second_begin,
                         Iter2 second_end) noexcept(std::is_nothrow_move_constructible_v<Iter1> &&
                                                    std::is_nothrow_move_constructible_v<Iter2> &&
                                                    noexcept(std::declval<Iter1&>() != std::declval<Iter1&>()))
      : first_current_(std::move(first_begin)),
        first_end_(std::move(first_end)),
        second_current_(std::move(second_begin)),
        second_end_(std::move(second_end)),
        in_first_(first_current_ != first_end_) {}

  constexpr ChainAdapter(const ChainAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter1> &&
                                                       std::is_nothrow_copy_constructible_v<Iter2>) = default;
  constexpr ChainAdapter(ChainAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter1> &&
                                                  std::is_nothrow_move_constructible_v<Iter2>) = default;
  constexpr ~ChainAdapter() noexcept(std::is_nothrow_destructible_v<Iter1> &&
                                     std::is_nothrow_destructible_v<Iter2>) = default;

  constexpr ChainAdapter& operator=(const ChainAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter1> &&
                                                                  std::is_nothrow_copy_assignable_v<Iter2>) = default;
  constexpr ChainAdapter& operator=(ChainAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter1> &&
                                                             std::is_nothrow_move_assignable_v<Iter2>) = default;

  constexpr ChainAdapter& operator++() noexcept(noexcept(++std::declval<Iter1&>()) &&
                                                noexcept(++std::declval<Iter2&>()) &&
                                                noexcept(std::declval<Iter1&>() == std::declval<Iter1&>()) &&
                                                noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()));
  constexpr ChainAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter> &&
                                                  noexcept(++std::declval<ChainAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const
      noexcept(noexcept(*std::declval<const Iter1&>()) && noexcept(*std::declval<const Iter2&>())) {
    return in_first_ ? *first_current_ : *second_current_;
  }

  [[nodiscard]] constexpr bool operator==(const ChainAdapter& other) const
      noexcept(noexcept(std::declval<const Iter1&>() == std::declval<const Iter1&>()) &&
               noexcept(std::declval<const Iter2&>() == std::declval<const Iter2&>()));

  [[nodiscard]] constexpr bool operator!=(const ChainAdapter& other) const
      noexcept(noexcept(std::declval<const ChainAdapter&>() == std::declval<const ChainAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr ChainAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr ChainAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter> && std::is_nothrow_copy_constructible_v<Iter1> &&
               std::is_nothrow_copy_constructible_v<Iter2>);

private:
  Iter1 first_current_;   ///< Current position in first iterator
  Iter1 first_end_;       ///< End of first iterator range
  Iter2 second_current_;  ///< Current position in second iterator
  Iter2 second_end_;      ///< End of second iterator range
  bool in_first_ = true;  ///< Whether iterating through first range
};

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr auto ChainAdapter<Iter1, Iter2>::operator++() noexcept(
    noexcept(++std::declval<Iter1&>()) && noexcept(++std::declval<Iter2&>()) &&
    noexcept(std::declval<Iter1&>() == std::declval<Iter1&>()) &&
    noexcept(std::declval<Iter2&>() != std::declval<Iter2&>())) -> ChainAdapter& {
  if (in_first_) {
    ++first_current_;
    if (first_current_ == first_end_) {
      in_first_ = false;
    }
  } else {
    if (second_current_ != second_end_) {
      ++second_current_;
    }
  }
  return *this;
}

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr auto ChainAdapter<Iter1, Iter2>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<ChainAdapter> && noexcept(++std::declval<ChainAdapter&>())) -> ChainAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr bool ChainAdapter<Iter1, Iter2>::operator==(const ChainAdapter& other) const
    noexcept(noexcept(std::declval<const Iter1&>() == std::declval<const Iter1&>()) &&
             noexcept(std::declval<const Iter2&>() == std::declval<const Iter2&>())) {
  // Check if both are at end (in second range and at second_end_)
  const bool this_at_end = !in_first_ && (second_current_ == second_end_);
  const bool other_at_end = !other.in_first_ && (other.second_current_ == other.second_end_);

  if (this_at_end && other_at_end) {
    return true;
  }

  // Otherwise, must be in same range at same position
  if (in_first_ != other.in_first_) {
    return false;
  }

  return in_first_ ? (first_current_ == other.first_current_) : (second_current_ == other.second_current_);
}

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr auto ChainAdapter<Iter1, Iter2>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter> && std::is_nothrow_copy_constructible_v<Iter1> &&
             std::is_nothrow_copy_constructible_v<Iter2>) -> ChainAdapter {
  auto end_iter = *this;
  end_iter.first_current_ = first_end_;
  end_iter.second_current_ = second_end_;
  end_iter.in_first_ = false;
  return end_iter;
}

/**
 * @brief Adapter that iterates through elements in reverse order.
 * @details Requires a bidirectional iterator. Uses operator-- to traverse backwards.
 * @tparam Iter Type of the underlying iterator
 *
 * @example
 * @code
 * // Reverse iterate through entities
 * auto reversed = query.Reverse();
 * @endcode
 */
template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
class ReverseAdapter final : public FunctionalAdapterBase<ReverseAdapter<Iter>> {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a reverse adapter.
   * @param begin Iterator to the beginning of the range
   * @param end Iterator to the end of the range
   */
  constexpr ReverseAdapter(Iter begin, Iter end) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                          std::is_nothrow_copy_constructible_v<Iter> &&
                                                          noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                                          noexcept(--std::declval<Iter&>()));
  constexpr ReverseAdapter(const ReverseAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr ReverseAdapter(ReverseAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~ReverseAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) = default;

  constexpr ReverseAdapter& operator=(const ReverseAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) =
      default;
  constexpr ReverseAdapter& operator=(ReverseAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr ReverseAdapter& operator++() noexcept(noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                                  noexcept(--std::declval<Iter&>()));
  constexpr ReverseAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
                                                    noexcept(++std::declval<ReverseAdapter&>()));

  constexpr ReverseAdapter& operator--() noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                  noexcept(++std::declval<Iter&>()) &&
                                                  noexcept(std::declval<Iter&>() != std::declval<Iter&>()));
  constexpr ReverseAdapter operator--(int) noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
                                                    noexcept(--std::declval<ReverseAdapter&>()));

  constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) { return *current_; }

  constexpr bool operator==(const ReverseAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>()));
  constexpr bool operator!=(const ReverseAdapter& other) const
      noexcept(noexcept(std::declval<ReverseAdapter>() == std::declval<ReverseAdapter>())) {
    return !(*this == other);
  }

  constexpr ReverseAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter>) {
    return *this;
  }

  constexpr ReverseAdapter end() const noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter>);

private:
  Iter begin_;
  Iter current_;
  Iter end_;
  bool done_ = false;
};

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr ReverseAdapter<Iter>::ReverseAdapter(Iter begin, Iter end) noexcept(
    std::is_nothrow_move_constructible_v<Iter> && std::is_nothrow_copy_constructible_v<Iter> &&
    noexcept(std::declval<Iter&>() == std::declval<Iter&>()) && noexcept(--std::declval<Iter&>()))
    : begin_(std::move(begin)), current_(std::move(end)), end_(current_), done_(begin_ == end_) {
  if (current_ != begin_) {
    --current_;
  }
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::operator++() noexcept(noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                                           noexcept(--std::declval<Iter&>())) -> ReverseAdapter& {
  if (!done_) {
    if (current_ == begin_) {
      done_ = true;
    } else {
      --current_;
    }
  }
  return *this;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
                                                              noexcept(++std::declval<ReverseAdapter&>()))
    -> ReverseAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::operator--() noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                           noexcept(++std::declval<Iter&>()) &&
                                                           noexcept(std::declval<Iter&>() != std::declval<Iter&>()))
    -> ReverseAdapter& {
  if (done_) {
    done_ = false;
    current_ = begin_;
  } else {
    if (current_ != end_) {
      ++current_;
    }
  }
  return *this;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::operator--(int) noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
                                                              noexcept(--std::declval<ReverseAdapter&>()))
    -> ReverseAdapter {
  auto temp = *this;
  --(*this);
  return temp;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr bool ReverseAdapter<Iter>::operator==(const ReverseAdapter& other) const
    noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
  if (done_ && other.done_) {
    return true;
  }
  return done_ == other.done_ && current_ == other.current_;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::end() const noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter>)
    -> ReverseAdapter {
  auto result = *this;
  result.done_ = true;
  return result;
}

/**
 * @brief Adapter that flattens nested ranges into a single sequence.
 * @details Iterates through an outer range of ranges, yielding elements from inner ranges.
 * @tparam Iter Type of the outer iterator
 *
 * @example
 * @code
 * // Join queries of `Position` and `Velocity` components
 * auto joined = query1.Join(query2);
 * joined.ForEach([](const Position& pos, const Velocity& vel) {
 */
template <typename Iter>
  requires JoinAdapterRequirements<Iter>
class JoinAdapter final : public FunctionalAdapterBase<JoinAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using outer_value_type = std::iter_value_t<Iter>;
  using inner_iterator_type = std::ranges::iterator_t<outer_value_type>;
  using value_type = std::iter_value_t<inner_iterator_type>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a join adapter.
   * @param begin Iterator to the beginning of the outer range
   * @param end Iterator to the end of the outer range
   */
  constexpr JoinAdapter(Iter begin, Iter end) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                       std::is_nothrow_copy_constructible_v<Iter> &&
                                                       std::is_nothrow_move_constructible_v<inner_iterator_type> &&
                                                       noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                       noexcept(*std::declval<inner_iterator_type&>()) &&
                                                       noexcept(AdvanceToValid()));
  constexpr JoinAdapter(const JoinAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
                                                     std::is_nothrow_copy_constructible_v<inner_iterator_type>) =
      default;

  constexpr JoinAdapter(JoinAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                std::is_nothrow_move_constructible_v<inner_iterator_type>) = default;
  constexpr ~JoinAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                    std::is_nothrow_destructible_v<inner_iterator_type>) = default;

  constexpr JoinAdapter& operator=(const JoinAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> && std::is_nothrow_copy_assignable_v<inner_iterator_type>) = default;
  constexpr JoinAdapter& operator=(JoinAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> && std::is_nothrow_move_assignable_v<inner_iterator_type>) = default;

  constexpr JoinAdapter& operator++() noexcept(noexcept(std::declval<inner_iterator_type&>() !=
                                                        std::declval<inner_iterator_type&>()) &&
                                               noexcept(++std::declval<inner_iterator_type&>()) &&
                                               noexcept(AdvanceToValid()));
  constexpr JoinAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<JoinAdapter> &&
                                                 noexcept(++std::declval<JoinAdapter&>()));

  constexpr value_type operator*() const noexcept(noexcept(*std::declval<const inner_iterator_type&>())) {
    return *inner_current_;
  }

  constexpr bool operator==(const JoinAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>()) &&
               noexcept(std::declval<const inner_iterator_type&>() == std::declval<const inner_iterator_type&>()));
  constexpr bool operator!=(const JoinAdapter& other) const
      noexcept(noexcept(std::declval<JoinAdapter>() == std::declval<JoinAdapter>())) {
    return !(*this == other);
  }

  constexpr JoinAdapter begin() const noexcept(std::is_nothrow_constructible_v<JoinAdapter, const Iter&, const Iter&>) {
    return {outer_begin_, outer_end_};
  }

  constexpr JoinAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<JoinAdapter> && std::is_nothrow_copy_assignable_v<Iter>);

private:
  constexpr void AdvanceToValid() noexcept(noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                           noexcept(std::declval<inner_iterator_type&>() ==
                                                    std::declval<inner_iterator_type&>()) &&
                                           noexcept(++std::declval<Iter&>()) && noexcept(*std::declval<Iter&>()) &&
                                           std::is_nothrow_move_assignable_v<inner_iterator_type>);

  Iter outer_begin_;
  Iter outer_current_;
  Iter outer_end_;
  inner_iterator_type inner_current_;
  inner_iterator_type inner_end_;
};

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr JoinAdapter<Iter>::JoinAdapter(Iter begin,
                                         Iter end) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                            std::is_nothrow_copy_constructible_v<Iter> &&
                                                            std::is_nothrow_move_constructible_v<inner_iterator_type> &&
                                                            noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                            noexcept(*std::declval<inner_iterator_type&>()) &&
                                                            noexcept(AdvanceToValid()))
    : outer_begin_(std::move(begin)), outer_current_(outer_begin_), outer_end_(std::move(end)) {
  if (outer_current_ != outer_end_) {
    auto& inner_range = *outer_current_;
    inner_current_ = std::ranges::begin(inner_range);
    inner_end_ = std::ranges::end(inner_range);
  }
  AdvanceToValid();
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr auto JoinAdapter<Iter>::operator++() noexcept(noexcept(std::declval<inner_iterator_type&>() !=
                                                                 std::declval<inner_iterator_type&>()) &&
                                                        noexcept(++std::declval<inner_iterator_type&>()) &&
                                                        noexcept(AdvanceToValid())) -> JoinAdapter& {
  if (inner_current_ != inner_end_) {
    ++inner_current_;
  }
  AdvanceToValid();
  return *this;
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr auto JoinAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<JoinAdapter> &&
                                                           noexcept(++std::declval<JoinAdapter&>())) -> JoinAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr bool JoinAdapter<Iter>::operator==(const JoinAdapter& other) const
    noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>()) &&
             noexcept(std::declval<const inner_iterator_type&>() == std::declval<const inner_iterator_type&>())) {
  if (outer_current_ != other.outer_current_) {
    return false;
  }
  if (outer_current_ == outer_end_) {
    return true;
  }
  return inner_current_ == other.inner_current_;
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr auto JoinAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<JoinAdapter> && std::is_nothrow_copy_assignable_v<Iter>)
        -> JoinAdapter {
  auto result = *this;
  result.outer_current_ = result.outer_end_;
  return result;
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr void JoinAdapter<Iter>::AdvanceToValid() noexcept(noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                                            noexcept(std::declval<inner_iterator_type&>() ==
                                                                     std::declval<inner_iterator_type&>()) &&
                                                            noexcept(++std::declval<Iter&>()) &&
                                                            noexcept(*std::declval<Iter&>()) &&
                                                            std::is_nothrow_move_assignable_v<inner_iterator_type>) {
  while (outer_current_ != outer_end_) {
    if (inner_current_ == inner_end_) {
      ++outer_current_;
      if (outer_current_ == outer_end_) {
        return;
      }
      auto& inner_range = *outer_current_;
      inner_current_ = std::ranges::begin(inner_range);
      inner_end_ = std::ranges::end(inner_range);
    }

    if (inner_current_ != inner_end_) {
      return;
    }
  }
}

/**
 * @brief A non-owning view over a sliding window of elements.
 * @details Provides a lightweight, non-allocating view over a contiguous window of elements.
 * @tparam Iter Type of the underlying iterator
 *
 * @example
 * @code
 * std::vector<int> data = {1, 2, 3, 4, 5};
 * auto slide = SlideAdapterFromRange(data, 3);
 * for (const auto& window : slide) {
 *   // window is a SlideView, iterate over it
 *   for (int val : window) {
 *     std::cout << val << " ";
 *   }
 *   std::cout << "\n";
 * }
 * // Output:
 * // 1 2 3
 * // 2 3 4
 * // 3 4 5
 * @endcode
 */
template <IteratorLike Iter>
class SlideView {
public:
  using iterator = Iter;
  using const_iterator = Iter;
  using value_type = std::iter_value_t<Iter>;
  using reference = decltype(*std::declval<Iter>());
  using size_type = size_t;

  /**
   * @brief Constructs a SlideView.
   * @param begin Iterator to the beginning of the window
   * @param size Size of the window
   */
  constexpr SlideView(Iter begin, size_t size) noexcept(std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(begin), size_(size) {}

  constexpr SlideView(const SlideView&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr SlideView(SlideView&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~SlideView() noexcept = default;

  constexpr SlideView& operator=(const SlideView&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr SlideView& operator=(SlideView&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  /**
   * @brief Collects the window elements into a vector.
   * @details Use when you need ownership of the elements.
   * @return Vector containing copies of the window elements
   */
  [[nodiscard]] constexpr auto Collect() const -> std::vector<value_type>
    requires std::copy_constructible<value_type>;

  /**
   * @brief Accesses an element by index.
   * @warning Index must be less than size
   * @param index Index of the element
   * @return Reference to the element
   */
  [[nodiscard]] constexpr reference operator[](size_t index) const
      noexcept(noexcept(*std::declval<Iter>()) && noexcept(++std::declval<Iter&>()));

  /**
   * @brief Compares two SlideViews for equality.
   * @param other SlideView to compare with
   * @return True if both views have the same elements
   */
  [[nodiscard]] constexpr bool operator==(const SlideView& other) const;

  /**
   * @brief Compares SlideView with a vector for equality.
   * @tparam R
   * @param vec Vector to compare with
   * @return True if the view has the same elements as the vector
   */
  template <std::ranges::sized_range R>
  [[nodiscard]] constexpr bool operator==(const R& range) const;

  /**
   * @brief Checks if the window is empty.
   * @return True if the window has no elements
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return size_ == 0; }

  /**
   * @brief Returns the size of the window.
   * @return Number of elements in the window
   */
  [[nodiscard]] constexpr size_t Size() const noexcept { return size_; }

  /**
   * @brief Returns an iterator to the beginning.
   * @return Iterator to the first element
   */
  [[nodiscard]] constexpr Iter begin() const noexcept(std::is_nothrow_copy_constructible_v<Iter>) { return begin_; }

  /**
   * @brief Returns an iterator to the end.
   * @return Iterator past the last element
   */
  [[nodiscard]] constexpr Iter end() const
      noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(++std::declval<Iter&>()));

private:
  Iter begin_;
  size_t size_ = 0;
};

template <IteratorLike Iter>
constexpr auto SlideView<Iter>::Collect() const -> std::vector<value_type>
  requires std::copy_constructible<value_type>
{
  std::vector<value_type> result;
  result.reserve(size_);
  auto it = begin_;
  for (size_t i = 0; i < size_; ++i) {
    result.push_back(*it++);
  }
  return result;
}

template <IteratorLike Iter>
constexpr auto SlideView<Iter>::operator[](size_t index) const
    noexcept(noexcept(*std::declval<Iter>()) && noexcept(++std::declval<Iter&>())) -> reference {
  auto it = begin_;
  for (size_t i = 0; i < index; ++i) {
    ++it;
  }
  return *it;
}

template <IteratorLike Iter>
constexpr bool SlideView<Iter>::operator==(const SlideView& other) const {
  if (size_ != other.size_) {
    return false;
  }

  auto it1 = begin_;
  auto it2 = other.begin_;
  for (size_t i = 0; i < size_; ++i) {
    if (*it1++ != *it2++) {
      return false;
    }
  }
  return true;
}

template <IteratorLike Iter>
template <std::ranges::sized_range R>
constexpr bool SlideView<Iter>::operator==(const R& range) const {
  if (size_ != std::ranges::size(range)) {
    return false;
  }

  return std::equal(std::ranges::cbegin(range), std::ranges::cend(range), begin_);
}

template <IteratorLike Iter>
constexpr Iter SlideView<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(++std::declval<Iter&>())) {
  auto it = begin_;
  for (size_t i = 0; i < size_; ++i) {
    ++it;
  }
  return it;
}

/**
 * @brief Adapter that yields sliding windows of elements.
 * @details Creates overlapping windows of a fixed size, moving one element at a time.
 * Each window is returned as a SlideView, which is a non-allocating view over the elements.
 * @tparam Iter Type of the underlying iterator
 *
 * @example
 * @code
 * std::vector<int> data = {1, 2, 3, 4, 5};
 * auto slide = SlideAdapterFromRange(data, 3);
 *
 * // Iterate over windows
 * for (const auto& window : slide) {
 *   // Each window is a SlideView
 *   for (int val : window) {
 *     std::cout << val << " ";
 *   }
 *   std::cout << "\n";
 * }
 *
 * // Collect windows if needed
 * auto windows = slide.Collect();  // Vector of SlideViews
 *
 * // Convert a window to vector if ownership needed
 * auto first_window = (*slide.begin()).Collect();  // std::vector<int>
 * @endcode
 */
template <typename Iter>
  requires SlideAdapterRequirements<Iter>
class SlideAdapter final : public FunctionalAdapterBase<SlideAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = SlideView<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a slide adapter.
   * @warning window_size must be greater than 0
   * @param begin Iterator to the beginning of the range
   * @param end Iterator to the end of the range
   * @param window_size Size of the sliding window
   */
  constexpr SlideAdapter(Iter begin, Iter end,
                         size_t window_size) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                      std::is_nothrow_copy_constructible_v<Iter> &&
                                                      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                      noexcept(++std::declval<Iter&>()));
  constexpr SlideAdapter(const SlideAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr SlideAdapter(SlideAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~SlideAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) = default;

  constexpr SlideAdapter& operator=(const SlideAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr SlideAdapter& operator=(SlideAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr SlideAdapter& operator++() noexcept(noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                noexcept(++std::declval<Iter&>()));
  constexpr SlideAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<SlideAdapter> &&
                                                  noexcept(++std::declval<SlideAdapter&>()));

  /**
   * @brief Dereferences the iterator to get the current window.
   * @return SlideView representing the current window
   */
  [[nodiscard]] constexpr value_type operator*() const
      noexcept(std::is_nothrow_constructible_v<SlideView<Iter>, Iter, size_t>) {
    return {current_, window_size_};
  }

  [[nodiscard]] constexpr bool operator==(const SlideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const SlideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() != std::declval<const Iter&>())) {
    return current_ != other.current_;
  }

  [[nodiscard]] constexpr SlideAdapter begin() const
      noexcept(std::is_nothrow_constructible_v<SlideAdapter, const Iter&, const Iter&, size_t>) {
    return {begin_, end_, window_size_};
  }

  [[nodiscard]] constexpr SlideAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<SlideAdapter> && std::is_nothrow_copy_assignable_v<Iter> &&
               noexcept(std::declval<Iter&>() != std::declval<const Iter&>()) && noexcept(++std::declval<Iter&>()));

  /**
   * @brief Returns the window size.
   * @return Size of the sliding window
   */
  [[nodiscard]] constexpr size_t WindowSize() const noexcept { return window_size_; }

private:
  Iter begin_;
  Iter current_;
  Iter end_;
  size_t window_size_ = 0;
};

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr SlideAdapter<Iter>::SlideAdapter(Iter begin, Iter end, size_t window_size) noexcept(
    std::is_nothrow_move_constructible_v<Iter> && std::is_nothrow_copy_constructible_v<Iter> &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()))
    : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), window_size_(window_size) {
  // If we can't form a complete window, start at end
  Iter iter = begin_;
  size_t count = 0;
  while (iter != end_) {
    ++iter;
    ++count;
  }
  if (count < window_size_) {
    current_ = end_;
  }
}

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr auto SlideAdapter<Iter>::operator++() noexcept(noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                         noexcept(++std::declval<Iter&>())) -> SlideAdapter& {
  if (current_ != end_) {
    ++current_;
  }
  return *this;
}

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr auto SlideAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<SlideAdapter> &&
                                                            noexcept(++std::declval<SlideAdapter&>())) -> SlideAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr auto SlideAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<SlideAdapter> && std::is_nothrow_copy_assignable_v<Iter> &&
             noexcept(std::declval<Iter&>() != std::declval<const Iter&>()) && noexcept(++std::declval<Iter&>()))
        -> SlideAdapter {
  auto result = *this;

  // Calculate end position: when we can't form a complete window
  Iter iter = begin_;
  size_t count = 0;
  while (iter != end_) {
    ++iter;
    ++count;
  }

  // Position where we can't form more windows
  if (count >= window_size_) {
    result.current_ = begin_;
    for (size_t i = 0; i < count - window_size_ + 1; ++i) {
      ++result.current_;
    }
  } else {
    result.current_ = end_;
  }

  return result;
}

/**
 * @brief Adapter that yields every Nth element from the range.
 * @details Similar to StepBy but with different semantics - takes stride, not step.
 * @tparam Iter Type of the underlying iterator
 *
 * @example
 * @code
 * std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
 * auto strided = StrideAdapterFromRange(data, 3);
 * // Yields: 1, 4, 7
 * for (int val : strided) {
 *   std::cout << val << " ";
 * }
 * @endcode
 */
template <typename Iter>
  requires StrideAdapterRequirements<Iter>
class StrideAdapter final : public FunctionalAdapterBase<StrideAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a stride adapter.
   * @param begin Iterator to the beginning of the range
   * @param end Iterator to the end of the range
   * @param stride Number of elements to skip between yields (1 = every element, 2 = every other, etc.)
   * @warning stride must be greater than 0
   */
  constexpr StrideAdapter(Iter begin, Iter end, size_t stride) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                                        std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)), stride_(stride) {}

  constexpr StrideAdapter(const StrideAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr StrideAdapter(StrideAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~StrideAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) = default;

  constexpr StrideAdapter& operator=(const StrideAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr StrideAdapter& operator=(StrideAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr StrideAdapter& operator++() noexcept(noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                 noexcept(++std::declval<Iter&>()));
  constexpr StrideAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<StrideAdapter> &&
                                                   noexcept(++std::declval<StrideAdapter&>()));

  constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) { return *current_; }

  constexpr bool operator==(const StrideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  constexpr bool operator!=(const StrideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() != std::declval<const Iter&>())) {
    return current_ != other.current_;
  }

  constexpr StrideAdapter begin() const
      noexcept(std::is_nothrow_constructible_v<StrideAdapter, const Iter&, const Iter&, size_t>) {
    return {begin_, end_, stride_};
  }

  constexpr StrideAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<StrideAdapter> && std::is_nothrow_copy_assignable_v<Iter>);

private:
  Iter begin_;
  Iter current_;
  Iter end_;
  size_t stride_;
};

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
constexpr auto StrideAdapter<Iter>::operator++() noexcept(noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
                                                          noexcept(++std::declval<Iter&>())) -> StrideAdapter& {
  for (size_t i = 0; i < stride_ && current_ != end_; ++i) {
    ++current_;
  }
  return *this;
}

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
constexpr auto StrideAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<StrideAdapter> &&
                                                             noexcept(++std::declval<StrideAdapter&>()))
    -> StrideAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
constexpr auto StrideAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<StrideAdapter> && std::is_nothrow_copy_assignable_v<Iter>)
        -> StrideAdapter {
  auto result = *this;
  result.current_ = end_;
  return result;
}

/**
 * @brief Adapter that combines two ranges into pairs.
 * @details Iterates both ranges in parallel, yielding tuples of corresponding elements.
 * Stops when either range is exhausted.
 * @tparam Iter1 Type of the first iterator
 * @tparam Iter2 Type of the second iterator
 *
 * @example
 * @code
 * std::vector<int> ids = {1, 2, 3};
 * std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
 * auto zipped = ZipAdapterFromRange(ids, names);
 * for (const auto& [id, name] : zipped) {
 *   std::cout << id << ": " << name << "\n";
 * }
 * // Output:
 * // 1: Alice
 * // 2: Bob
 * // 3: Charlie
 * @endcode
 */
template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
class ZipAdapter final : public FunctionalAdapterBase<ZipAdapter<Iter1, Iter2>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::tuple<std::iter_value_t<Iter1>, std::iter_value_t<Iter2>>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a zip adapter.
   * @param begin1 Iterator to the beginning of the first range
   * @param end1 Iterator to the end of the first range
   * @param begin2 Iterator to the beginning of the second range
   * @param end2 Iterator to the end of the second range
   */
  constexpr ZipAdapter(Iter1 begin1, Iter1 end1, Iter2 begin2,
                       Iter2 end2) noexcept(std::is_nothrow_move_constructible_v<Iter1> &&
                                            std::is_nothrow_copy_constructible_v<Iter1> &&
                                            std::is_nothrow_move_constructible_v<Iter2> &&
                                            std::is_nothrow_copy_constructible_v<Iter2>)
      : first_begin_(std::move(begin1)),
        first_current_(first_begin_),
        first_end_(std::move(end1)),
        second_begin_(std::move(begin2)),
        second_current_(second_begin_),
        second_end_(std::move(end2)) {}

  constexpr ZipAdapter(const ZipAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter1> &&
                                                   std::is_nothrow_copy_constructible_v<Iter2>) = default;
  constexpr ZipAdapter(ZipAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter1> &&
                                              std::is_nothrow_move_constructible_v<Iter2>) = default;
  constexpr ~ZipAdapter() noexcept(std::is_nothrow_destructible_v<Iter1> &&
                                   std::is_nothrow_destructible_v<Iter2>) = default;

  constexpr ZipAdapter& operator=(const ZipAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter1> &&
                                                              std::is_nothrow_copy_assignable_v<Iter2>) = default;
  constexpr ZipAdapter& operator=(ZipAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter1> &&
                                                         std::is_nothrow_move_assignable_v<Iter2>) = default;

  constexpr ZipAdapter& operator++() noexcept(noexcept(std::declval<Iter1&>() != std::declval<Iter1&>()) &&
                                              noexcept(++std::declval<Iter1&>()) &&
                                              noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()) &&
                                              noexcept(++std::declval<Iter2&>()));

  constexpr ZipAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter> &&
                                                noexcept(++std::declval<ZipAdapter&>()));

  constexpr value_type operator*() const { return std::make_tuple(*first_current_, *second_current_); }

  constexpr bool operator==(const ZipAdapter& other) const
      noexcept(noexcept(std::declval<const Iter1&>() == std::declval<const Iter1&>()) &&
               noexcept(std::declval<const Iter2&>() == std::declval<const Iter2&>())) {
    return first_current_ == other.first_current_ || second_current_ == other.second_current_;
  }

  constexpr bool operator!=(const ZipAdapter& other) const
      noexcept(noexcept(std::declval<const ZipAdapter&>() == std::declval<const ZipAdapter&>())) {
    return !(*this == other);
  }

  constexpr bool IsAtEnd() const noexcept(noexcept(std::declval<const Iter1&>() == std::declval<const Iter1&>()) &&
                                          noexcept(std::declval<const Iter2&>() == std::declval<const Iter2&>())) {
    return first_current_ == first_end_ || second_current_ == second_end_;
  }

  constexpr ZipAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter>) { return *this; }
  constexpr ZipAdapter end() const noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter>);

private:
  Iter1 first_begin_;
  Iter1 first_current_;
  Iter1 first_end_;
  Iter2 second_begin_;
  Iter2 second_current_;
  Iter2 second_end_;
};

template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
constexpr auto ZipAdapter<Iter1, Iter2>::operator++() noexcept(
    noexcept(std::declval<Iter1&>() != std::declval<Iter1&>()) && noexcept(++std::declval<Iter1&>()) &&
    noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()) && noexcept(++std::declval<Iter2&>())) -> ZipAdapter& {
  if (first_current_ != first_end_) {
    ++first_current_;
  }
  if (second_current_ != second_end_) {
    ++second_current_;
  }
  return *this;
}

template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
constexpr auto ZipAdapter<Iter1, Iter2>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter> &&
                                                                  noexcept(++std::declval<ZipAdapter&>()))
    -> ZipAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
constexpr auto ZipAdapter<Iter1, Iter2>::end() const noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter>)
    -> ZipAdapter {
  auto result = *this;
  result.first_current_ = result.first_end_;
  result.second_current_ = result.second_end_;
  return result;
}

/**
 * @brief Adapter that concatenates two ranges of the same type into a single range.
 * @details Iterates through the first range completely, then continues with the second range.
 * This is similar to ChainAdapter but simplified for same-iterator-type ranges.
 * @tparam Iter Type of the underlying iterator (same for both ranges)
 *
 * @example
 * @code
 * std::vector<int> first = {1, 2, 3};
 * std::vector<int> second = {4, 5, 6};
 * auto concat = ConcatAdapterFromRange(first, second);
 *
 * // Iterate over concatenated range
 * for (int val : concat) {
 *   std::cout << val << " ";  // Outputs: 1 2 3 4 5 6
 * }
 *
 * // Collect into vector
 * auto collected = concat.Collect();  // {1, 2, 3, 4, 5, 6}
 *
 * // Use with message/event spans
 * auto [prev, curr] = reader.Spans();
 * auto all_messages = ConcatAdapterFromRange(prev, curr);
 * @endcode
 */
template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
class ConcatAdapter final : public FunctionalAdapterBase<ConcatAdapter<Iter>> {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::iter_value_t<Iter>;
  using difference_type = std::iter_difference_t<Iter>;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a concat adapter with two iterator ranges.
   * @param first_begin Start of the first iterator range
   * @param first_end End of the first iterator range
   * @param second_begin Start of the second iterator range
   * @param second_end End of the second iterator range
   */
  constexpr ConcatAdapter(Iter first_begin, Iter first_end, Iter second_begin,
                          Iter second_end) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                                    noexcept(std::declval<Iter&>() != std::declval<Iter&>()))
      : first_current_(std::move(first_begin)),
        first_end_(std::move(first_end)),
        second_current_(std::move(second_begin)),
        second_end_(std::move(second_end)),
        in_first_(first_current_ != first_end_) {}

  constexpr ConcatAdapter(const ConcatAdapter&) noexcept(std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr ConcatAdapter(ConcatAdapter&&) noexcept(std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~ConcatAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) = default;

  constexpr ConcatAdapter& operator=(const ConcatAdapter&) noexcept(std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr ConcatAdapter& operator=(ConcatAdapter&&) noexcept(std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr ConcatAdapter& operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                 noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                                 noexcept(std::declval<Iter&>() != std::declval<Iter&>()));
  constexpr ConcatAdapter operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ConcatAdapter> &&
                                                   noexcept(++std::declval<ConcatAdapter&>()));

  [[nodiscard]] constexpr value_type operator*() const noexcept(noexcept(*std::declval<const Iter&>())) {
    return in_first_ ? *first_current_ : *second_current_;
  }

  [[nodiscard]] constexpr bool operator==(const ConcatAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>()));

  [[nodiscard]] constexpr bool operator!=(const ConcatAdapter& other) const
      noexcept(noexcept(std::declval<const ConcatAdapter&>() == std::declval<const ConcatAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the adapter is at the end.
   * @return True if all elements have been consumed
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
    return !in_first_ && (second_current_ == second_end_);
  }

  /**
   * @brief Returns the total size of both ranges.
   * @details This requires iterating through both ranges to count elements.
   * @return Total number of elements in both ranges
   */
  [[nodiscard]] constexpr size_t Size() const
      noexcept(noexcept(std::declval<Iter&>() != std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()));

  [[nodiscard]] constexpr ConcatAdapter begin() const noexcept(std::is_nothrow_copy_constructible_v<ConcatAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr ConcatAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<ConcatAdapter> && std::is_nothrow_copy_constructible_v<Iter>);

private:
  Iter first_current_;    ///< Current position in first range
  Iter first_end_;        ///< End of first range
  Iter second_current_;   ///< Current position in second range
  Iter second_end_;       ///< End of second range
  bool in_first_ = true;  ///< Whether currently iterating through first range
};

template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
constexpr auto ConcatAdapter<Iter>::operator++() noexcept(noexcept(++std::declval<Iter&>()) &&
                                                          noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
                                                          noexcept(std::declval<Iter&>() != std::declval<Iter&>()))
    -> ConcatAdapter& {
  if (in_first_) {
    ++first_current_;
    if (first_current_ == first_end_) {
      in_first_ = false;
    }
  } else {
    if (second_current_ != second_end_) {
      ++second_current_;
    }
  }
  return *this;
}

template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
constexpr auto ConcatAdapter<Iter>::operator++(int) noexcept(std::is_nothrow_copy_constructible_v<ConcatAdapter> &&
                                                             noexcept(++std::declval<ConcatAdapter&>()))
    -> ConcatAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
constexpr bool ConcatAdapter<Iter>::operator==(const ConcatAdapter& other) const
    noexcept(noexcept(std::declval<const Iter&>() == std::declval<const Iter&>())) {
  // Check if both are at end (in second range and at second_end_)
  const bool this_at_end = !in_first_ && (second_current_ == second_end_);
  const bool other_at_end = !other.in_first_ && (other.second_current_ == other.second_end_);

  if (this_at_end && other_at_end) {
    return true;
  }

  // Otherwise, must be in same range at same position
  if (in_first_ != other.in_first_) {
    return false;
  }

  return in_first_ ? (first_current_ == other.first_current_) : (second_current_ == other.second_current_);
}

template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
constexpr size_t ConcatAdapter<Iter>::Size() const
    noexcept(noexcept(std::declval<Iter&>() != std::declval<Iter&>()) && noexcept(++std::declval<Iter&>())) {
  size_t count = std::accumulate(first_current_, first_end_, 0);
  count += std::accumulate(second_current_, second_end_, 0);
  return count;
}

template <typename Iter>
  requires ConcatAdapterRequirements<Iter>
constexpr auto ConcatAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<ConcatAdapter> && std::is_nothrow_copy_constructible_v<Iter>)
        -> ConcatAdapter {
  auto end_iter = *this;
  end_iter.first_current_ = first_end_;
  end_iter.second_current_ = second_end_;
  end_iter.in_first_ = false;
  return end_iter;
}

/**
 * @brief CRTP base class providing common adapter operations.
 * @details Provides chaining methods (Filter, Map, Take, Skip, etc.) that can be used by any derived adapter class.
 * Uses CRTP pattern to return the correct derived type.
 * @tparam Derived The derived adapter class
 */
template <typename Derived>
class FunctionalAdapterBase {
public:
  /**
   * @brief Chains another filter operation on top of this iterator.
   * @tparam Pred Predicate type
   * @param predicate Function to filter elements
   * @return FilterAdapter that applies this adapter then filters
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto Filter(Pred predicate) const
      noexcept(noexcept(FilterAdapter<Derived, Pred>(GetDerived().begin(), GetDerived().end(), std::move(predicate)))) {
    return FilterAdapter<Derived, Pred>(GetDerived().begin(), GetDerived().end(), std::move(predicate));
  }

  /**
   * @brief Transforms each element using the given function.
   * @tparam Func Transformation function type
   * @param transform Function to apply to each element
   * @return MapAdapter that transforms adapted results
   */
  template <typename Func>
  [[nodiscard]] constexpr auto Map(Func transform) const
      noexcept(noexcept(MapAdapter<Derived, Func>(GetDerived().begin(), GetDerived().end(), std::move(transform)))) {
    return MapAdapter<Derived, Func>(GetDerived().begin(), GetDerived().end(), std::move(transform));
  }

  /**
   * @brief Limits the number of elements to at most count.
   * @param count Maximum number of elements to yield
   * @return TakeAdapter that limits adapted results
   */
  [[nodiscard]] constexpr auto Take(size_t count) const
      noexcept(noexcept(TakeAdapter<Derived>(GetDerived().begin(), GetDerived().end(), count))) {
    return TakeAdapter<Derived>(GetDerived().begin(), GetDerived().end(), count);
  }

  /**
   * @brief Skips the first count elements.
   * @param count Number of elements to skip
   * @return SkipAdapter that skips adapted results
   */
  [[nodiscard]] constexpr auto Skip(size_t count) const
      noexcept(noexcept(SkipAdapter<Derived>(GetDerived().begin(), GetDerived().end(), count))) {
    return SkipAdapter<Derived>(GetDerived().begin(), GetDerived().end(), count);
  }

  /**
   * @brief Takes elements while a predicate is true.
   * @tparam Pred Predicate type for take-while operation
   * @param predicate Predicate to determine when to stop taking
   * @return TakeWhileAdapter that conditionally takes elements
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto TakeWhile(Pred predicate) const
      noexcept(noexcept(TakeWhileAdapter<Derived, Pred>(GetDerived().begin(), GetDerived().end(),
                                                        std::move(predicate)))) {
    return TakeWhileAdapter<Derived, Pred>(GetDerived().begin(), GetDerived().end(), std::move(predicate));
  }

  /**
   * @brief Skips elements while a predicate is true.
   * @tparam Pred Predicate type for skip-while operation
   * @param predicate Predicate to determine when to stop skipping
   * @return SkipWhileAdapter that conditionally skips elements
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto SkipWhile(Pred predicate) const
      noexcept(noexcept(SkipWhileAdapter<Derived, Pred>(GetDerived().begin(), GetDerived().end(),
                                                        std::move(predicate)))) {
    return SkipWhileAdapter<Derived, Pred>(GetDerived().begin(), GetDerived().end(), std::move(predicate));
  }

  /**
   * @brief Adds an index to each element.
   * @return EnumerateAdapter that pairs indices with values
   */
  [[nodiscard]] constexpr auto Enumerate() const
      noexcept(noexcept(EnumerateAdapter<Derived>(GetDerived().begin(), GetDerived().end()))) {
    return EnumerateAdapter<Derived>(GetDerived().begin(), GetDerived().end());
  }

  /**
   * @brief Observes each element without modifying it.
   * @tparam Func Inspector function type
   * @param inspector Function to call on each element
   * @return InspectAdapter for side effects
   */
  template <typename Func>
  [[nodiscard]] constexpr auto Inspect(Func inspector) const
      noexcept(noexcept(InspectAdapter<Derived, Func>(GetDerived().begin(), GetDerived().end(),
                                                      std::move(inspector)))) {
    return InspectAdapter<Derived, Func>(GetDerived().begin(), GetDerived().end(), std::move(inspector));
  }

  /**
   * @brief Takes every Nth element.
   * @param step Step size between elements
   * @return StepByAdapter that skips elements
   */
  [[nodiscard]] constexpr auto StepBy(size_t step) const
      noexcept(noexcept(StepByAdapter<Derived>(GetDerived().begin(), GetDerived().end(), step))) {
    return StepByAdapter<Derived>(GetDerived().begin(), GetDerived().end(), step);
  }

  /**
   * @brief Chains another range after this one.
   * @tparam OtherIter Iterator type of the other adapter
   * @param begin Begin iterator of the other adapter
   * @param end End iterator of the other adapter
   * @return ChainAdapter that yields elements from both ranges
   */
  template <typename OtherIter>
  [[nodiscard]] constexpr auto Chain(OtherIter begin, OtherIter end) const
      noexcept(noexcept(ChainAdapter<Derived, OtherIter>(GetDerived().begin(), GetDerived().end(), std::move(begin),
                                                         std::move(end)))) {
    return ChainAdapter<Derived, OtherIter>(GetDerived().begin(), GetDerived().end(), std::move(begin), std::move(end));
  }

  /**
   * @brief Chains another range after this one.
   * @tparam R Range type of the other adapter
   * @param range The other range to chain
   * @return ChainAdapter that yields elements from both ranges
   */
  template <typename R>
  [[nodiscard]] constexpr auto Chain(R& range) const noexcept(noexcept(ChainAdapterFromRange(GetDerived(), range))) {
    return ChainAdapterFromRange(GetDerived(), range);
  }

  /**
   * @brief Chains another range after this one.
   * @tparam R Range type of the other adapter
   * @param range The other range to chain
   * @return ChainAdapter that yields elements from both ranges
   */
  template <typename R>
  [[nodiscard]] constexpr auto Chain(const R& range) const
      noexcept(noexcept(ChainAdapterFromRange(GetDerived(), range))) {
    return ChainAdapterFromRange(GetDerived(), range);
  }

  /**
   * @brief Reverses the order of elements.
   * @note Requires bidirectional iterator support
   * @return ReverseAdapter that yields elements in reverse order
   */
  [[nodiscard]] constexpr auto Reverse() const
      noexcept(noexcept(ReverseAdapter<Derived>(GetDerived().begin(), GetDerived().end()))) {
    return ReverseAdapter<Derived>(GetDerived().begin(), GetDerived().end());
  }

  /**
   * @brief Creates sliding windows over elements.
   * @param window_size Size of the sliding window
   * @return SlideAdapter that yields windows of elements
   * @warning window_size must be greater than 0
   */
  [[nodiscard]] constexpr auto Slide(size_t window_size) const
      noexcept(noexcept(SlideAdapter<Derived>(GetDerived().begin(), GetDerived().end(), window_size))) {
    return SlideAdapter<Derived>(GetDerived().begin(), GetDerived().end(), window_size);
  }

  /**
   * @brief Takes every Nth element with stride.
   * @param stride Number of elements to skip between yields
   * @return StrideAdapter that yields every Nth element
   * @warning stride must be greater than 0
   */
  [[nodiscard]] constexpr auto Stride(size_t stride) const
      noexcept(noexcept(StrideAdapter<Derived>(GetDerived().begin(), GetDerived().end(), stride))) {
    return StrideAdapter<Derived>(GetDerived().begin(), GetDerived().end(), stride);
  }

  /**
   * @brief Zips another range with this one.
   * @tparam OtherIter Iterator type to zip with
   * @param begin Begin iterator for the other range
   * @param end End iterator for the other range
   * @return ZipAdapter that yields tuples of corresponding elements
   */
  template <typename OtherIter>
  [[nodiscard]] constexpr auto Zip(OtherIter begin, OtherIter end) const
      noexcept(noexcept(ZipAdapter<Derived, OtherIter>(GetDerived().begin(), GetDerived().end(), std::move(begin),
                                                       std::move(end)))) {
    return ZipAdapter<Derived, OtherIter>(GetDerived().begin(), GetDerived().end(), std::move(begin), std::move((end)));
  }

  /**
   * @brief Zips another range with this one.
   * @tparam R Other range
   * @param range The other range to zip with
   * @return ZipAdapter that
   */
  template <typename R>
  [[nodiscard]] constexpr auto Zip(R& range) const noexcept(noexcept(ZipAdapterFromRange(GetDerived(), range))) {
    return ZipAdapterFromRange(GetDerived(), range);
  }

  /**
   * @brief Zips another range with this one.
   * @tparam R Other range
   * @param range The other range to zip with
   * @return ZipAdapter that
   */
  template <typename R>
  [[nodiscard]] constexpr auto Zip(const R& range) const noexcept(noexcept(ZipAdapterFromRange(GetDerived(), range))) {
    return ZipAdapterFromRange(GetDerived(), range);
  }

  /**
   * @brief Terminal operation: applies an action to each element.
   * @tparam Action Function type that processes each element
   * @param action Function to apply to each element
   */
  template <typename Action>
  constexpr void ForEach(const Action& action) const;

  /**
   * @brief Terminal operation: reduces elements to a single value using a folder function.
   * @tparam T Accumulator type
   * @tparam Folder Function type that combines accumulator with each element
   * @param init Initial accumulator value
   * @param folder Function to fold elements
   * @return Final accumulated value
   */
  template <typename T, typename Folder>
  [[nodiscard]] constexpr T Fold(T init, const Folder& folder) const;

  /**
   * @brief Terminal operation: checks if any element satisfies a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return True if any element satisfies the predicate, false otherwise
   */
  template <typename Pred>
  [[nodiscard]] constexpr bool Any(const Pred& predicate) const;

  /**
   * @brief Terminal operation: checks if all elements satisfy a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return True if all elements satisfy the predicate, false otherwise
   */
  template <typename Pred>
  [[nodiscard]] constexpr bool All(const Pred& predicate) const;

  /**
   * @brief Terminal operation: checks if no elements satisfy a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return True if no elements satisfy the predicate, false otherwise
   */
  template <typename Pred>
  [[nodiscard]] constexpr bool None(const Pred& predicate) const {
    return !Any(predicate);
  }

  /**
   * @brief Terminal operation: finds the first element satisfying a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return Optional containing the first matching element, or empty if none found
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto Find(const Pred& predicate) const;

  /**
   * @brief Terminal operation: counts elements satisfying a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return Number of elements that satisfy the predicate
   */
  template <typename Pred>
  [[nodiscard]] constexpr size_t CountIf(const Pred& predicate) const;

  /**
   * @brief Terminal operation: partitions elements into two groups based on a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return Pair of vectors: first contains elements satisfying predicate, second contains the rest
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto Partition(const Pred& predicate) const;

  /**
   * @brief Terminal operation: finds the element with the maximum value according to a key function.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key from each element
   * @return Optional containing the element with maximum key, or empty if no elements
   */
  template <typename KeyFunc>
  [[nodiscard]] constexpr auto MaxBy(const KeyFunc& key_func) const;

  /**
   * @brief Terminal operation: finds the element with the minimum value according to a key function.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key from each element
   * @return Optional containing the element with minimum key, or empty if no elements
   */
  template <typename KeyFunc>
  [[nodiscard]] constexpr auto MinBy(const KeyFunc& key_func) const;

  /**
   * @brief Terminal operation: groups elements by a key function.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract grouping key from each element
   * @return Map from keys to vectors of elements with that key
   */
  template <typename KeyFunc>
  [[nodiscard]] constexpr auto GroupBy(const KeyFunc& key_func) const;

  /**
   * @brief Terminal operation: collects all elements into a vector.
   * @return Vector containing all elements
   */
  [[nodiscard]] constexpr auto Collect() const;

  /**
   * @brief Terminal operation: writes all elements into an output iterator.
   * @details Consumes the adapter and writes each element to the provided output iterator.
   * This is more efficient than Collect() when you already have a destination container.
   * @tparam OutIt Output iterator type
   * @param out Output iterator to write elements into
   *
   * @example
   * @code
   * std::vector<int> results;
   * query.Filter([](int x) { return x > 5; }).Into(std::back_inserter(results));
   * @endcode
   */
  template <typename OutIt>
  constexpr void Into(OutIt out) const;

protected:
  /**
   * @brief Gets reference to derived class instance.
   * @return Reference to derived class
   */
  [[nodiscard]] constexpr Derived& GetDerived() noexcept { return static_cast<Derived&>(*this); }

  /**
   * @brief Gets const reference to derived class instance.
   * @return Const reference to derived class
   */
  [[nodiscard]] constexpr const Derived& GetDerived() const noexcept { return static_cast<const Derived&>(*this); }
};

template <typename Derived>
template <typename Action>
constexpr void FunctionalAdapterBase<Derived>::ForEach(const Action& action) const {
  for (auto&& value : GetDerived()) {
    if constexpr (std::invocable<Action, decltype(value)>) {
      action(std::forward<decltype(value)>(value));
    } else {
      std::apply(action, std::forward<decltype(value)>(value));
    }
  }
}

template <typename Derived>
template <typename T, typename Folder>
constexpr T FunctionalAdapterBase<Derived>::Fold(T init, const Folder& folder) const {
  for (auto&& value : GetDerived()) {
    if constexpr (std::invocable<Folder, T&&, decltype(value)>) {
      init = folder(std::move(init), std::forward<decltype(value)>(value));
    } else {
      init = std::apply(
          [&init, &folder](auto&&... args) { return folder(std::move(init), std::forward<decltype(args)>(args)...); },
          std::forward<decltype(value)>(value));
    }
  }
  return init;
}

template <typename Derived>
template <typename Pred>
constexpr bool FunctionalAdapterBase<Derived>::Any(const Pred& predicate) const {
  for (auto&& value : GetDerived()) {
    bool result = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      result = predicate(std::forward<decltype(value)>(value));
    } else {
      result = std::apply(predicate, std::forward<decltype(value)>(value));
    }
    if (result) {
      return true;
    }
  }
  return false;
}

template <typename Derived>
template <typename Pred>
constexpr bool FunctionalAdapterBase<Derived>::All(const Pred& predicate) const {
  for (auto&& value : GetDerived()) {
    bool result = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      result = predicate(std::forward<decltype(value)>(value));
    } else {
      result = std::apply(predicate, std::forward<decltype(value)>(value));
    }
    if (!result) {
      return false;
    }
  }
  return true;
}

template <typename Derived>
template <typename Pred>
constexpr auto FunctionalAdapterBase<Derived>::Find(const Pred& predicate) const {
  using ValueType = std::iter_value_t<Derived>;
  for (auto&& value : GetDerived()) {
    bool result = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      result = predicate(std::forward<decltype(value)>(value));
    } else {
      result = std::apply(predicate, std::forward<decltype(value)>(value));
    }
    if (result) {
      return std::optional<ValueType>(std::forward<decltype(value)>(value));
    }
  }
  return std::optional<ValueType>{};
}

template <typename Derived>
template <typename Pred>
constexpr size_t FunctionalAdapterBase<Derived>::CountIf(const Pred& predicate) const {
  size_t count = 0;
  for (auto&& value : GetDerived()) {
    bool result = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      result = predicate(std::forward<decltype(value)>(value));
    } else {
      result = std::apply(predicate, std::forward<decltype(value)>(value));
    }
    if (result) {
      ++count;
    }
  }
  return count;
}

template <typename Derived>
constexpr auto FunctionalAdapterBase<Derived>::Collect() const {
  using ValueType = std::iter_value_t<Derived>;
  std::vector<ValueType> result;
  result.reserve(static_cast<size_t>(std::distance(GetDerived().begin(), GetDerived().end())));
  for (auto&& value : GetDerived()) {
    result.push_back(std::forward<decltype(value)>(value));
  }
  return result;
}

template <typename Derived>
template <typename OutIt>
constexpr void FunctionalAdapterBase<Derived>::Into(OutIt out) const {
  for (auto&& value : GetDerived()) {
    *out++ = std::forward<decltype(value)>(value);
  }
}

template <typename Derived>
template <typename Pred>
constexpr auto FunctionalAdapterBase<Derived>::Partition(const Pred& predicate) const {
  using ValueType = std::iter_value_t<Derived>;
  std::vector<ValueType> matched;
  std::vector<ValueType> not_matched;

  for (auto&& value : GetDerived()) {
    bool result = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      result = predicate(std::forward<decltype(value)>(value));
    } else {
      result = std::apply(predicate, std::forward<decltype(value)>(value));
    }

    if (result) {
      matched.push_back(std::forward<decltype(value)>(value));
    } else {
      not_matched.push_back(std::forward<decltype(value)>(value));
    }
  }

  return std::pair{std::move(matched), std::move(not_matched)};
}

template <typename Derived>
template <typename KeyFunc>
constexpr auto FunctionalAdapterBase<Derived>::MaxBy(const KeyFunc& key_func) const {
  using ValueType = std::iter_value_t<Derived>;
  auto iter = GetDerived().begin();
  const auto end_iter = GetDerived().end();

  if (iter == end_iter) {
    return std::nullopt;
  }

  std::optional<ValueType> max_element;
  max_element.emplace(*iter);

  auto get_key = [&key_func](auto&& val) {
    if constexpr (std::invocable<KeyFunc, decltype(val)>) {
      return key_func(std::forward<decltype(val)>(val));
    } else {
      return std::apply(key_func, std::forward<decltype(val)>(val));
    }
  };

  auto max_key = get_key(*max_element);
  ++iter;

  while (iter != end_iter) {
    auto current = *iter;
    auto current_key = get_key(current);
    if (current_key > max_key) {
      max_key = current_key;
      max_element.emplace(current);
    }
    ++iter;
  }

  return max_element;
}

template <typename Derived>
template <typename KeyFunc>
constexpr auto FunctionalAdapterBase<Derived>::MinBy(const KeyFunc& key_func) const {
  using ValueType = std::iter_value_t<Derived>;
  auto iter = GetDerived().begin();
  const auto end_iter = GetDerived().end();

  if (iter == end_iter) {
    return std::nullopt;
  }

  std::optional<ValueType> min_element;
  min_element.emplace(*iter);

  auto get_key = [&key_func](auto&& val) {
    if constexpr (std::invocable<KeyFunc, decltype(val)>) {
      return key_func(std::forward<decltype(val)>(val));
    } else {
      return std::apply(key_func, std::forward<decltype(val)>(val));
    }
  };

  auto min_key = get_key(*min_element);
  ++iter;

  while (iter != end_iter) {
    auto current = *iter;
    auto current_key = get_key(current);
    if (current_key < min_key) {
      min_key = current_key;
      min_element.emplace(current);
    }
    ++iter;
  }

  return min_element;
}

template <typename Derived>
template <typename KeyFunc>
constexpr auto FunctionalAdapterBase<Derived>::GroupBy(const KeyFunc& key_func) const {
  using ValueType = std::iter_value_t<Derived>;
  using KeyType = std::decay_t<std::invoke_result_t<KeyFunc, ValueType>>;
  std::unordered_map<KeyType, std::vector<ValueType>> groups;

  for (auto&& value : GetDerived()) {
    KeyType key;
    if constexpr (std::invocable<KeyFunc, decltype(value)>) {
      key = key_func(std::forward<decltype(value)>(value));
    } else {
      key = std::apply(key_func, std::forward<decltype(value)>(value));
    }
    groups[std::move(key)].push_back(std::forward<decltype(value)>(value));
  }

  return groups;
}

/**
 * @brief Helper function to create a FilterAdapter from a non-const range.
 * @tparam R Range type
 * @tparam Pred Predicate type
 * @param range Range to filter
 * @param predicate Function to filter elements
 * @return FilterAdapter for the range
 */
template <std::ranges::range R, typename Pred>
  requires FilterAdapterRequirements<std::ranges::iterator_t<R>, Pred>
constexpr auto FilterAdapterFromRange(R& range, Pred predicate) noexcept(
    noexcept(FilterAdapter<std::ranges::iterator_t<R>, Pred>(std::ranges::begin(range), std::ranges::end(range),
                                                             std::move(predicate))))
    -> FilterAdapter<std::ranges::iterator_t<R>, Pred> {
  return {std::ranges::begin(range), std::ranges::end(range), std::move(predicate)};
}

/**
 * @brief Helper function to create a FilterAdapter from a const range.
 * @tparam R Range type
 * @tparam Pred Predicate type
 * @param range Range to filter
 * @param predicate Function to filter elements
 * @return FilterAdapter for the range
 */
template <std::ranges::range R, typename Pred>
  requires FilterAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>())), Pred>
constexpr auto FilterAdapterFromRange(const R& range, Pred predicate) noexcept(
    noexcept(FilterAdapter<decltype(std::ranges::cbegin(range)), Pred>(std::ranges::cbegin(range),
                                                                       std::ranges::cend(range), std::move(predicate))))
    -> FilterAdapter<decltype(std::ranges::cbegin(range)), Pred> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), std::move(predicate)};
}

/**
 * @brief Helper function to create a MapAdapter from a range.
 * @tparam R Range type
 * @tparam Func Transformation function type
 * @param range Range to map
 * @param transform Function to apply to each element
 * @return MapAdapter for the range
 */
template <std::ranges::range R, typename Func>
  requires MapAdapterRequirements<std::ranges::iterator_t<R>, Func>
constexpr auto MapAdapterFromRange(R& range, Func transform) noexcept(
    noexcept(MapAdapter<std::ranges::iterator_t<R>, Func>(std::ranges::begin(range), std::ranges::end(range),
                                                          std::move(transform))))
    -> MapAdapter<std::ranges::iterator_t<R>, Func> {
  return {std::ranges::begin(range), std::ranges::end(range), std::move(transform)};
}

/**
 * @brief Helper function to create a MapAdapter from a const range.
 * @tparam R Range type
 * @tparam Func Transformation function type
 * @param range Range to map
 * @param transform Function to apply to each element
 * @return MapAdapter for the range
 */
template <std::ranges::range R, typename Func>
  requires MapAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>())), Func>
constexpr auto MapAdapterFromRange(const R& range, Func transform) noexcept(
    noexcept(MapAdapter<decltype(std::ranges::cbegin(range)), Func>(std::ranges::cbegin(range),
                                                                    std::ranges::cend(range), std::move(transform))))
    -> MapAdapter<decltype(std::ranges::cbegin(range)), Func> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), std::move(transform)};
}

/**
 * @brief Helper function to create a TakeAdapter from a non-const range.
 * @tparam R Range type
 * @param range Range to take from
 * @param count Maximum number of elements to yield
 * @return TakeAdapter for the range
 */
template <std::ranges::range R>
  requires TakeAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto TakeAdapterFromRange(R& range, size_t count) noexcept(
    noexcept(TakeAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range), count)))
    -> TakeAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range), count};
}

/**
 * @brief Helper function to create a TakeAdapter from a const range.
 * @tparam R Range type
 * @param range Range to take from
 * @param count Maximum number of elements to yield
 * @return TakeAdapter for the range
 */
template <std::ranges::range R>
  requires TakeAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto TakeAdapterFromRange(const R& range, size_t count) noexcept(noexcept(
    TakeAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range), count)))
    -> TakeAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), count};
}

/**
 * @brief Helper function to create a SkipAdapter from a non-const range.
 * @tparam R Range type
 * @param range Range to skip from
 * @param count Number of elements to skip
 * @return SkipAdapter for the range
 */
template <std::ranges::range R>
  requires SkipAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto SkipAdapterFromRange(R& range, size_t count) noexcept(
    noexcept(SkipAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range), count)))
    -> SkipAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range), count};
}

/**
 * @brief Helper function to create a SkipAdapter from a const range.
 * @tparam R Range type
 * @param range Range to skip from
 * @param count Number of elements to skip
 * @return SkipAdapter for the range
 */
template <std::ranges::range R>
  requires SkipAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto SkipAdapterFromRange(const R& range, size_t count) noexcept(noexcept(
    SkipAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range), count)))
    -> SkipAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), count};
}

/**
 * @brief Helper function to create a TakeWhileAdapter from a non-const range.
 * @tparam R Range type
 * @tparam Pred Predicate type
 * @param range Range to take from
 * @param predicate Predicate to test elements
 * @return TakeWhileAdapter for the range
 */
template <std::ranges::range R, typename Pred>
  requires TakeWhileAdapterRequirements<std::ranges::iterator_t<R>, Pred>
constexpr auto TakeWhileAdapterFromRange(R& range, Pred predicate) noexcept(
    noexcept(TakeWhileAdapter<std::ranges::iterator_t<R>, Pred>(std::ranges::begin(range), std::ranges::end(range),
                                                                std::move(predicate))))
    -> TakeWhileAdapter<std::ranges::iterator_t<R>, Pred> {
  return {std::ranges::begin(range), std::ranges::end(range), std::move(predicate)};
}

/**
 * @brief Helper function to create a TakeWhileAdapter from a const range.
 * @tparam R Range type
 * @tparam Pred Predicate type
 * @param range Range to take from
 * @param predicate Predicate to test elements
 * @return TakeWhileAdapter for the range
 */
template <std::ranges::range R, typename Pred>
  requires TakeWhileAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>())), Pred>
constexpr auto TakeWhileAdapterFromRange(const R& range, Pred predicate) noexcept(noexcept(
    TakeWhileAdapter<decltype(std::ranges::cbegin(range)), Pred>(std::ranges::cbegin(range), std::ranges::cend(range),
                                                                 std::move(predicate))))
    -> TakeWhileAdapter<decltype(std::ranges::cbegin(range)), Pred> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), std::move(predicate)};
}

/**
 * @brief Helper function to create a SkipWhileAdapter from a non-const range.
 * @tparam R Range type
 * @tparam Pred Predicate type
 * @param range Range to skip from
 * @param predicate Predicate to test elements
 * @return SkipWhileAdapter for the range
 */
template <std::ranges::range R, typename Pred>
  requires SkipWhileAdapterRequirements<std::ranges::iterator_t<R>, Pred>
constexpr auto SkipWhileAdapterFromRange(R& range, Pred predicate) noexcept(
    noexcept(SkipWhileAdapter<std::ranges::iterator_t<R>, Pred>(std::ranges::begin(range), std::ranges::end(range),
                                                                std::move(predicate))))
    -> SkipWhileAdapter<std::ranges::iterator_t<R>, Pred> {
  return {std::ranges::begin(range), std::ranges::end(range), std::move(predicate)};
}

/**
 * @brief Helper function to create a SkipWhileAdapter from a const range.
 * @tparam R Range type
 * @tparam Pred Predicate type
 * @param range Range to skip from
 * @param predicate Predicate to test elements
 * @return SkipWhileAdapter for the range
 */
template <std::ranges::range R, typename Pred>
  requires SkipWhileAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>())), Pred>
constexpr auto SkipWhileAdapterFromRange(const R& range, Pred predicate) noexcept(noexcept(
    SkipWhileAdapter<decltype(std::ranges::cbegin(range)), Pred>(std::ranges::cbegin(range), std::ranges::cend(range),
                                                                 std::move(predicate))))
    -> SkipWhileAdapter<decltype(std::ranges::cbegin(range)), Pred> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), std::move(predicate)};
}

/**
 * @brief Helper function to create an EnumerateAdapter from a non-const range.
 * @tparam R Range type
 * @param range Range to enumerate
 * @return EnumerateAdapter for the range
 */
template <std::ranges::range R>
  requires EnumerateAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto EnumerateAdapterFromRange(R& range) noexcept(
    noexcept(EnumerateAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range))))
    -> EnumerateAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range)};
}

/**
 * @brief Helper function to create an EnumerateAdapter from a const range.
 * @tparam R Range type
 * @param range Range to enumerate
 * @return EnumerateAdapter for the range
 */
template <std::ranges::range R>
  requires EnumerateAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto EnumerateAdapterFromRange(const R& range) noexcept(noexcept(
    EnumerateAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range))))
    -> EnumerateAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range)};
}

/**
 * @brief Helper function to create an InspectAdapter from a non-const range.
 * @tparam R Range type
 * @tparam Func Inspector function type
 * @param range Range to inspect
 * @param inspector Function to call on each element
 * @return InspectAdapter for the range
 */
template <std::ranges::range R, typename Func>
  requires InspectAdapterRequirements<std::ranges::iterator_t<R>, Func>
constexpr auto InspectAdapterFromRange(R& range, Func inspector) noexcept(
    noexcept(InspectAdapter<std::ranges::iterator_t<R>, Func>(std::ranges::begin(range), std::ranges::end(range),
                                                              std::move(inspector))))
    -> InspectAdapter<std::ranges::iterator_t<R>, Func> {
  return {std::ranges::begin(range), std::ranges::end(range), std::move(inspector)};
}

/**
 * @brief Helper function to create an InspectAdapter from a const range.
 * @tparam R Range type
 * @tparam Func Inspector function type
 * @param range Range to inspect
 * @param inspector Function to call for each element
 * @return InspectAdapter for the range
 */
template <std::ranges::range R, typename Func>
  requires InspectAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>())), Func>
constexpr auto InspectAdapterFromRange(const R& range, Func inspector) noexcept(noexcept(
    InspectAdapter<decltype(std::ranges::cbegin(range)), Func>(std::ranges::cbegin(range), std::ranges::cend(range),
                                                               std::move(inspector))))
    -> InspectAdapter<decltype(std::ranges::cbegin(range)), Func> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), std::move(inspector)};
}

/**
 * @brief Helper function to create a StepByAdapter from a non-const range.
 * @tparam R Range type
 * @param range Range to step through
 * @param step Step size
 * @return StepByAdapter for the range
 */
template <std::ranges::range R>
  requires StepByAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto StepByAdapterFromRange(R& range, size_t step) noexcept(
    noexcept(StepByAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range), step)))
    -> StepByAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range), step};
}

/**
 * @brief Helper function to create a StepByAdapter from a const range.
 * @tparam R Range type
 * @param range Range to step through
 * @param step Number of elements to step by
 * @return StepByAdapter for the range
 */
template <std::ranges::range R>
  requires StepByAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto StepByAdapterFromRange(const R& range, size_t step) noexcept(noexcept(
    StepByAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range), step)))
    -> StepByAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), step};
}

/**
 * @brief Helper function to create a ChainAdapter from two non-const ranges.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ChainAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ChainAdapterRequirements<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>
constexpr auto ChainAdapterFromRange(R1& range1, R2& range2) noexcept(
    noexcept(ChainAdapter<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>(
        std::ranges::begin(range1), std::ranges::end(range1), std::ranges::begin(range2), std::ranges::end(range2))))
    -> ChainAdapter<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>> {
  return {std::ranges::begin(range1), std::ranges::end(range1), std::ranges::begin(range2), std::ranges::end(range2)};
}

/**
 * @brief Helper function to create a ChainAdapter from non-const and const ranges.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ChainAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ChainAdapterRequirements<std::ranges::iterator_t<R1>,
                                    decltype(std::ranges::cbegin(std::declval<const R2&>()))>
constexpr auto ChainAdapterFromRange(R1& range1, const R2& range2) noexcept(
    noexcept(ChainAdapter<std::ranges::iterator_t<R1>, decltype(std::ranges::cbegin(range2))>(
        std::ranges::begin(range1), std::ranges::end(range1), std::ranges::cbegin(range2), std::ranges::cend(range2))))
    -> ChainAdapter<std::ranges::iterator_t<R1>, decltype(std::ranges::cbegin(range2))> {
  return {std::ranges::begin(range1), std::ranges::end(range1), std::ranges::cbegin(range2), std::ranges::cend(range2)};
}

/**
 * @brief Helper function to create a ChainAdapter from const and non-const ranges.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ChainAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ChainAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R1&>())),
                                    std::ranges::iterator_t<R2>>
constexpr auto ChainAdapterFromRange(const R1& range1, R2& range2) noexcept(
    noexcept(ChainAdapter<decltype(std::ranges::cbegin(range1)), std::ranges::iterator_t<R2>>(
        std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::begin(range2), std::ranges::end(range2))))
    -> ChainAdapter<decltype(std::ranges::cbegin(range1)), std::ranges::iterator_t<R2>> {
  return {std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::begin(range2), std::ranges::end(range2)};
}

/**
 * @brief Helper function to create a ChainAdapter from two const ranges.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ChainAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ChainAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R1&>())),
                                    decltype(std::ranges::cbegin(std::declval<const R2&>()))>
constexpr auto ChainAdapterFromRange(const R1& range1, const R2& range2) noexcept(
    noexcept(ChainAdapter<decltype(std::ranges::cbegin(range1)), decltype(std::ranges::cbegin(range2))>(
        std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
        std::ranges::cend(range2))))
    -> ChainAdapter<decltype(std::ranges::cbegin(range1)), decltype(std::ranges::cbegin(range2))> {
  return {std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
          std::ranges::cend(range2)};
}

/**
 * @brief Helper function to create a ReverseAdapter from a range.
 * @tparam R Range type
 * @param range Range to reverse
 * @return ReverseAdapter for the range
 */
template <std::ranges::range R>
  requires ReverseAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto ReverseAdapterFromRange(R& range) noexcept(
    noexcept(ReverseAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range))))
    -> ReverseAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range)};
}

/**
 * @brief Helper function to create a ReverseAdapter from a const range.
 * @tparam R Range type
 * @param range Range to reverse
 * @return ReverseAdapter for the range
 */
template <std::ranges::range R>
  requires ReverseAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto ReverseAdapterFromRange(const R& range) noexcept(noexcept(
    ReverseAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range))))
    -> ReverseAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range)};
}

/**
 * @brief Helper function to create a JoinAdapter from a range.
 * @tparam R Range type
 * @param range Range to join
 * @return JoinAdapter for the range
 */
template <std::ranges::range R>
  requires JoinAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto JoinAdapterFromRange(R& range) noexcept(
    noexcept(JoinAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range))))
    -> JoinAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range)};
}

/**
 * @brief Helper function to create a JoinAdapter from a const range.
 * @tparam R Range type
 * @param range Range to join
 * @return JoinAdapter for the range
 */
template <std::ranges::range R>
  requires JoinAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto JoinAdapterFromRange(const R& range) noexcept(
    noexcept(JoinAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range))))
    -> JoinAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range)};
}

/**
 * @brief Helper function to create a SlideAdapter from a range.
 * @tparam R Range type
 * @param range Range to slide over
 * @param window_size Size of the sliding window
 * @return SlideAdapter for the range
 */
template <std::ranges::range R>
  requires SlideAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto SlideAdapterFromRange(R& range, size_t window_size) noexcept(
    noexcept(SlideAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range), window_size)))
    -> SlideAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range), window_size};
}

/**
 * @brief Helper function to create a SlideAdapter from a const range.
 * @tparam R Range type
 * @param range Range to slide over
 * @param window_size Size of the sliding window
 * @return SlideAdapter for the range
 */
template <std::ranges::range R>
  requires SlideAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto SlideAdapterFromRange(const R& range, size_t window_size) noexcept(
    noexcept(SlideAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range),
                                                                window_size)))
    -> SlideAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), window_size};
}

/**
 * @brief Helper function to create a StrideAdapter from a range.
 * @tparam R Range type
 * @param range Range to stride over
 * @param stride Stride value
 * @return StrideAdapter for the range
 */
template <std::ranges::range R>
  requires StrideAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto StrideAdapterFromRange(R& range, size_t stride) noexcept(
    noexcept(StrideAdapter<std::ranges::iterator_t<R>>(std::ranges::begin(range), std::ranges::end(range), stride)))
    -> StrideAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range), std::ranges::end(range), stride};
}

/**
 * @brief Helper function to create a StrideAdapter from a const range.
 * @tparam R Range type
 * @param range Range to stride over
 * @param stride Stride value
 * @return StrideAdapter for the range
 */
template <std::ranges::range R>
  requires StrideAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto StrideAdapterFromRange(const R& range, size_t stride) noexcept(noexcept(
    StrideAdapter<decltype(std::ranges::cbegin(range))>(std::ranges::cbegin(range), std::ranges::cend(range), stride)))
    -> StrideAdapter<decltype(std::ranges::cbegin(range))> {
  return {std::ranges::cbegin(range), std::ranges::cend(range), stride};
}

/**
 * @brief Helper function to create a ZipAdapter from two ranges.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ZipAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ZipAdapterRequirements<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>
constexpr auto ZipAdapterFromRange(R1& range1, R2& range2) noexcept(
    noexcept(ZipAdapter<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>(
        std::ranges::begin(range1), std::ranges::end(range1), std::ranges::begin(range2), std::ranges::end(range2))))
    -> ZipAdapter<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>> {
  return {std::ranges::begin(range1), std::ranges::end(range1), std::ranges::begin(range2), std::ranges::end(range2)};
}

/**
 * @brief Helper function to create a ZipAdapter from a range and a const range.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ZipAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ZipAdapterRequirements<std::ranges::iterator_t<R1>, decltype(std::ranges::cbegin(std::declval<const R2&>()))>
constexpr auto ZipAdapterFromRange(R1& range1, const R2& range2) noexcept(
    noexcept(ZipAdapter<std::ranges::iterator_t<R1>, decltype(std::ranges::cbegin(range2))>(
        std::ranges::begin(range1), std::ranges::end(range1), std::ranges::cbegin(range2), std::ranges::cend(range2))))
    -> ZipAdapter<std::ranges::iterator_t<R1>, decltype(std::ranges::cbegin(range2))> {
  return {std::ranges::begin(range1), std::ranges::end(range1), std::ranges::cbegin(range2), std::ranges::cend(range2)};
}

/**
 * @brief Helper function to create a ZipAdapter from two const ranges.
 * @tparam R1 First range type
 * @tparam R2 Second range type
 * @param range1 First range
 * @param range2 Second range
 * @return ZipAdapter for the two ranges
 */
template <std::ranges::range R1, std::ranges::range R2>
  requires ZipAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R1&>())),
                                  decltype(std::ranges::cbegin(std::declval<const R2&>()))>
constexpr auto ZipAdapterFromRange(const R1& range1, const R2& range2) noexcept(
    noexcept(ZipAdapter<decltype(std::ranges::cbegin(range1)), decltype(std::ranges::cbegin(range2))>(
        std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
        std::ranges::cend(range2))))
    -> ZipAdapter<decltype(std::ranges::cbegin(range1)), decltype(std::ranges::cbegin(range2))> {
  return {std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
          std::ranges::cend(range2)};
}

/**
 * @brief Helper function to create a ConcatAdapter from two ranges of the same type.
 * @details Concatenates two ranges into a single iterable range.
 * @tparam R Range type (must be the same for both)
 * @param range1 First range
 * @param range2 Second range
 * @return ConcatAdapter for the two ranges
 *
 * @example
 * @code
 * std::vector<int> first = {1, 2, 3};
 * std::vector<int> second = {4, 5, 6};
 * auto concat = ConcatAdapterFromRange(first, second);
 * for (int val : concat) {
 *   std::cout << val << " ";  // Outputs: 1 2 3 4 5 6
 * }
 * @endcode
 */
template <std::ranges::range R>
  requires ConcatAdapterRequirements<std::ranges::iterator_t<R>>
constexpr auto ConcatAdapterFromRange(R& range1, R& range2) noexcept(noexcept(ConcatAdapter<std::ranges::iterator_t<R>>(
    std::ranges::begin(range1), std::ranges::end(range1), std::ranges::begin(range2), std::ranges::end(range2))))
    -> ConcatAdapter<std::ranges::iterator_t<R>> {
  return {std::ranges::begin(range1), std::ranges::end(range1), std::ranges::begin(range2), std::ranges::end(range2)};
}

/**
 * @brief Helper function to create a ConcatAdapter from two const ranges of the same type.
 * @details Concatenates two const ranges into a single iterable range.
 * @tparam R Range type (must be the same for both)
 * @param range1 First range
 * @param range2 Second range
 * @return ConcatAdapter for the two ranges
 *
 * @example
 * @code
 * const std::vector<int> first = {1, 2, 3};
 * const std::vector<int> second = {4, 5, 6};
 * auto concat = ConcatAdapterFromRange(first, second);
 * for (int val : concat) {
 *   std::cout << val << " ";  // Outputs: 1 2 3 4 5 6
 * }
 * @endcode
 */
template <std::ranges::range R>
  requires ConcatAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto ConcatAdapterFromRange(const R& range1, const R& range2) noexcept(noexcept(
    ConcatAdapter<decltype(std::ranges::cbegin(range1))>(std::ranges::cbegin(range1), std::ranges::cend(range1),
                                                         std::ranges::cbegin(range2), std::ranges::cend(range2))))
    -> ConcatAdapter<decltype(std::ranges::cbegin(range1))> {
  return {std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
          std::ranges::cend(range2)};
}

/**
 * @brief Helper function to create a ConcatAdapter from a non-const and const range of the same type.
 * @tparam R Range type (must be the same for both)
 * @param range1 First range (non-const)
 * @param range2 Second range (const)
 * @return ConcatAdapter for the two ranges using const iterators
 */
template <std::ranges::range R>
  requires ConcatAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto ConcatAdapterFromRange(R& range1, const R& range2) noexcept(
    noexcept(ConcatAdapter<decltype(std::ranges::cbegin(std::declval<const R&>()))>(std::ranges::cbegin(range1),
                                                                                    std::ranges::cend(range1),
                                                                                    std::ranges::cbegin(range2),
                                                                                    std::ranges::cend(range2))))
    -> ConcatAdapter<decltype(std::ranges::cbegin(std::declval<const R&>()))> {
  return {std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
          std::ranges::cend(range2)};
}

/**
 * @brief Helper function to create a ConcatAdapter from a const and non-const range of the same type.
 * @tparam R Range type (must be the same for both)
 * @param range1 First range (const)
 * @param range2 Second range (non-const)
 * @return ConcatAdapter for the two ranges using const iterators
 */
template <std::ranges::range R>
  requires ConcatAdapterRequirements<decltype(std::ranges::cbegin(std::declval<const R&>()))>
constexpr auto ConcatAdapterFromRange(const R& range1, R& range2) noexcept(
    noexcept(ConcatAdapter<decltype(std::ranges::cbegin(std::declval<const R&>()))>(std::ranges::cbegin(range1),
                                                                                    std::ranges::cend(range1),
                                                                                    std::ranges::cbegin(range2),
                                                                                    std::ranges::cend(range2))))
    -> ConcatAdapter<decltype(std::ranges::cbegin(std::declval<const R&>()))> {
  return {std::ranges::cbegin(range1), std::ranges::cend(range1), std::ranges::cbegin(range2),
          std::ranges::cend(range2)};
}

}  // namespace helios::utils
