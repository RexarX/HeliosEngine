#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
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

/// @brief Checks if the iterator yields a genuine reference (lvalue ref), not a
/// proxy/value.
template <typename Iter>
concept IterYieldsReference =
    std::is_lvalue_reference_v<std::iter_reference_t<Iter>>;

/// @brief Selects pointer vs optional depending on whether the iterator yields
/// a real ref.
template <typename Iter>
using find_result_t = std::conditional_t<
    IterYieldsReference<Iter>,
    std::remove_reference_t<std::iter_reference_t<Iter>>*,  // T* / const T*
    std::optional<std::iter_value_t<Iter>>                  // owned copy
    >;

/// @brief Traits for `Iter`, selecting iterator concept from `Iter` if it has
/// one, else forward.
template <typename Iter>
struct adapter_iterator_traits {
  // C++20 concept: propagate from Iter if it has one, else forward
  using iterator_concept =
      std::conditional_t<IterYieldsReference<Iter>, std::forward_iterator_tag,
                         std::input_iterator_tag>;  // proxy — demote for C++17
                                                    // compat

  // C++17 category: always input if proxy, forward if real ref
  using iterator_category =
      std::conditional_t<IterYieldsReference<Iter>, std::forward_iterator_tag,
                         std::input_iterator_tag>;
};

/// @brief Gets the iterator concept from `Iter`.
template <typename Iter>
using adapter_iterator_concept_t =
    typename adapter_iterator_traits<Iter>::iterator_concept;

/// @brief Gets the iterator category from `Iter`.
template <typename Iter>
using adapter_iterator_category_t =
    typename adapter_iterator_traits<Iter>::iterator_category;

/**
 * @brief Concept to check if a type is tuple-like (has tuple_size and get).
 * @details This is used to determine if std::tuple_cat can be applied to a
 * type.
 * @tparam T Type to check
 */
template <typename T>
concept TupleLike = requires {
  typename std::tuple_size<std::remove_cvref_t<T>>::type;
  requires std::tuple_size_v<std::remove_cvref_t<T>> >= 0;
};

/// @brief Helper to extract tuple element types and check if a folder is
/// invocable with accumulator + tuple elements
template <typename Folder, typename Accumulator, typename Tuple>
struct is_folder_applicable_impl : std::false_type {};

template <typename Folder, typename Accumulator, typename... TupleArgs>
struct is_folder_applicable_impl<Folder, Accumulator, std::tuple<TupleArgs...>>
    : std::bool_constant<std::invocable<Folder, Accumulator, TupleArgs...>> {};

template <typename Folder, typename Accumulator, typename Tuple>
concept FolderApplicable =
    is_folder_applicable_impl<Folder, Accumulator, Tuple>::value;

/// @brief Helper to get the result type of folder applied with accumulator +
/// tuple elements
template <typename Folder, typename Accumulator, typename Tuple>
struct folder_apply_result;

template <typename Folder, typename Accumulator, typename... TupleArgs>
struct folder_apply_result<Folder, Accumulator, std::tuple<TupleArgs...>> {
  using type = std::invoke_result_t<Folder, Accumulator, TupleArgs...>;
};

template <typename Folder, typename Accumulator, typename Tuple>
using folder_apply_result_t =
    typename folder_apply_result<Folder, Accumulator, Tuple>::type;

/// @brief Helper to get the result type of either invoke or apply
template <typename Func, typename... Args>
consteval auto GetCallOrApplyResultType() noexcept {
  if constexpr (std::invocable<Func, Args...>) {
    return std::type_identity<std::invoke_result_t<Func, Args...>>{};
  } else if (sizeof...(Args) == 1) {
    return std::type_identity<decltype(std::apply(std::declval<Func>(),
                                                  std::declval<Args>()...))>{};
  } else {
    static_assert(sizeof...(Args) == 1,
                  "Callable must be invocable with Args or via std::apply with "
                  "a single tuple argument");
  }
}

template <typename Func, typename... Args>
using call_or_apply_result_t =
    typename decltype(GetCallOrApplyResultType<Func, Args...>())::type;

template <typename Func, typename... Args>
concept CallableOrApplicable =
    std::invocable<Func, Args...> ||
    (sizeof...(Args) == 1 && requires(Func func, Args&&... args) {
      std::apply(func, std::forward<Args>(args)...);
    });

template <typename Func, typename ReturnType, typename... Args>
concept CallableOrApplicableWithReturn =
    CallableOrApplicable<Func, Args...> &&
    std::convertible_to<call_or_apply_result_t<Func, Args...>, ReturnType>;

}  // namespace details

template <typename Derived>
class FunctionalAdapterBase;

/// @brief Concept for types that are adapter traits (derived from
/// `FunctionalAdapterBase`).
template <typename T>
concept AdapterTrait = std::derived_from<T, FunctionalAdapterBase<T>>;

/// @brief Concept for types that are external ranges (not adapters).
template <typename R>
concept ExternalRange =
    !AdapterTrait<std::remove_cvref_t<R>> && std::ranges::input_range<R>;

/**
 * @brief Concept for types that can be used as base iterators in adapters.
 * @tparam T Type to check
 */
template <typename T>
concept IteratorLike = requires(T iter, const T const_iter) {
  typename std::iter_value_t<T>;
  {
    ++iter
  } -> std::same_as<std::add_lvalue_reference_t<std::remove_cvref_t<T>>>;
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
  {
    --iter
  } -> std::same_as<std::add_lvalue_reference_t<std::remove_cvref_t<T>>>;
};

/**
 * @brief Concept for predicate functions that can be applied to iterator
 * values.
 * @details The predicate must be invocable with the value type and return a
 * boolean-testable result. Supports both direct invocation and `std::apply` for
 * tuple unpacking. The result must be usable in boolean contexts (if
 * statements, etc.). Allows const ref consumption even if the original type is
 * just ref or value.
 * @tparam Pred Predicate type
 * @tparam ValueType Type of values to test
 */
template <typename Pred, typename ValueType>
concept PredicateFor =
    details::CallableOrApplicableWithReturn<Pred, bool, ValueType> ||
    details::CallableOrApplicableWithReturn<
        Pred, bool,
        std::add_lvalue_reference_t<
            std::add_const_t<std::remove_cvref_t<ValueType>>>> ||
    details::CallableOrApplicableWithReturn<Pred, bool,
                                            std::remove_cvref_t<ValueType>>;

/**
 * @brief Concept for transformation functions that can be applied to iterator
 * values.
 * @details The function must be invocable with the value type and return a
 * non-void result. Supports both direct invocation and `std::apply` for tuple
 * unpacking. Allows const ref consumption even if the original type is just ref
 * or value.
 * @tparam Func Function type
 * @tparam ValueType Type of values to transform
 */
template <typename Func, typename ValueType>
concept TransformFor =
    (details::CallableOrApplicable<Func, ValueType> &&
     !std::is_void_v<details::call_or_apply_result_t<Func, ValueType>>) ||
    (details::CallableOrApplicable<
         Func, std::add_lvalue_reference_t<
                   std::add_const_t<std::remove_cvref_t<ValueType>>>> &&
     !std::is_void_v<details::call_or_apply_result_t<
         Func, std::add_lvalue_reference_t<
                   std::add_const_t<std::remove_cvref_t<ValueType>>>>>) ||
    (details::CallableOrApplicable<Func, std::remove_cvref_t<ValueType>> &&
     !std::is_void_v<details::call_or_apply_result_t<
         Func, std::remove_cvref_t<ValueType>>>);

/**
 * @brief Concept for inspection functions that observe but don't modify values.
 * @details The function must be invocable with the value type and return void.
 * Supports both direct invocation and `std::apply` for tuple unpacking.
 * Allows const ref consumption even if the original type is just ref or value.
 * @tparam Func Function type
 * @tparam ValueType Type of values to inspect
 */
template <typename Func, typename ValueType>
concept InspectorFor =
    details::CallableOrApplicableWithReturn<Func, void, ValueType> ||
    details::CallableOrApplicableWithReturn<
        Func, void,
        std::add_lvalue_reference_t<
            std::add_const_t<std::remove_cvref_t<ValueType>>>> ||
    details::CallableOrApplicableWithReturn<Func, void,
                                            std::remove_cvref_t<ValueType>>;

/**
 * @brief Concept for action functions that process values.
 * @details The function must be invocable with the value type.
 * Supports both direct invocation and `std::apply` for tuple unpacking.
 * Allows const ref consumption even if the original type is just ref or value.
 * @tparam Action Action function type
 * @tparam ValueType Type of values to process
 */
template <typename Action, typename ValueType>
concept ActionFor =
    details::CallableOrApplicableWithReturn<Action, void, ValueType> ||
    details::CallableOrApplicableWithReturn<
        Action, void,
        std::add_lvalue_reference_t<
            std::add_const_t<std::remove_cvref_t<ValueType>>>> ||
    details::CallableOrApplicableWithReturn<Action, void,
                                            std::remove_cvref_t<ValueType>>;

/**
 * @brief Concept for folder functions that accumulate values.
 * @details The function must be invocable with an accumulator and a value type.
 * Supports both direct invocation and `std::apply` for tuple unpacking.
 * @tparam Folder Folder function type
 * @tparam Accumulator Accumulator type
 * @tparam ValueType Type of values to fold
 */
template <typename Folder, typename Accumulator, typename ValueType>
concept FolderFor =
    (std::invocable<Folder, Accumulator, ValueType> &&
     std::convertible_to<std::invoke_result_t<Folder, Accumulator, ValueType>,
                         Accumulator>) ||
    (details::FolderApplicable<Folder, Accumulator,
                               std::remove_cvref_t<ValueType>> &&
     std::convertible_to<
         details::folder_apply_result_t<Folder, Accumulator,
                                        std::remove_cvref_t<ValueType>>,
         Accumulator>);

/**
 * @brief Concept to validate `FilterAdapter` requirements.
 * @tparam Iter Iterator type
 * @tparam Pred Predicate type
 */
template <typename Iter, typename Pred>
concept FilterAdapterRequirements =
    IteratorLike<Iter> && PredicateFor<Pred, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate `MapAdapter` requirements.
 * @tparam Iter Iterator type
 * @tparam Func Transform function type
 */
template <typename Iter, typename Func>
concept MapAdapterRequirements =
    IteratorLike<Iter> && TransformFor<Func, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate `TakeAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept TakeAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate `SkipAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept SkipAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate `TakeWhileAdapter` requirements.
 * @tparam Iter Iterator type
 * @tparam Pred Predicate type
 */
template <typename Iter, typename Pred>
concept TakeWhileAdapterRequirements =
    IteratorLike<Iter> && PredicateFor<Pred, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate `SkipWhileAdapter` requirements.
 * @tparam Iter Iterator type
 * @tparam Pred Predicate type
 */
template <typename Iter, typename Pred>
concept SkipWhileAdapterRequirements =
    IteratorLike<Iter> && PredicateFor<Pred, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate `EnumerateAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept EnumerateAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate `InspectAdapter` requirements.
 * @tparam Iter Iterator type
 * @tparam Func Inspector function type
 */
template <typename Iter, typename Func>
concept InspectAdapterRequirements =
    IteratorLike<Iter> && InspectorFor<Func, std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate `StepByAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept StepByAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate `ChainAdapter` requirements.
 * @tparam Iter1 First iterator type
 * @tparam Iter2 Second iterator type
 */
template <typename Iter1, typename Iter2>
concept ChainAdapterRequirements =
    IteratorLike<Iter1> && IteratorLike<Iter2> &&
    std::same_as<std::iter_value_t<Iter1>, std::iter_value_t<Iter2>>;

/**
 * @brief Concept to validate `ReverseAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept ReverseAdapterRequirements = BidirectionalIteratorLike<Iter>;

/**
 * @brief Concept to validate `JoinAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept JoinAdapterRequirements =
    IteratorLike<Iter> && std::ranges::range<std::iter_value_t<Iter>>;

/**
 * @brief Concept to validate `SlideAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept SlideAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate `StrideAdapter` requirements.
 * @tparam Iter Iterator type
 */
template <typename Iter>
concept StrideAdapterRequirements = IteratorLike<Iter>;

/**
 * @brief Concept to validate `ZipAdapter` requirements.
 * @tparam Iter1 First iterator type
 * @tparam Iter2 Second iterator type
 */
template <typename Iter1, typename Iter2>
concept ZipAdapterRequirements = IteratorLike<Iter1> && IteratorLike<Iter2>;

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

/**
 * @brief Iterator adapter that filters elements based on a predicate function.
 * @details This adapter provides lazy filtering of iterator values.
 * The filtering happens during iteration, not upfront, making it memory
 * efficient for large sequences.
 *
 * Supports chaining with other adapters for complex data transformations.
 *
 * @note This adapter maintains forward iterator semantics.
 * @note The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 * @tparam Pred Predicate function type
 *
 * @code
 * auto enemies = query.Filter([](const Health& h) {
 *   return h.points > 0;
 * });
 * @endcode
 */
template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
class FilterAdapter final
    : public FunctionalAdapterBase<FilterAdapter<Iter, Pred>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a filter adapter with the given iterator range and
   * predicate.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param predicate Function to filter elements
   */
  constexpr FilterAdapter(Iter begin, Iter end, Pred predicate) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Pred> && noexcept(AdvanceToValid()))
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        predicate_(std::move(predicate)) {
    AdvanceToValid();
  }

  /**
   * @brief Constructs a filter adapter from a range.
   * @tparam R Range type
   * @param range Range to filter
   * @param predicate Function to filter elements
   */
  template <ExternalRange R>
    requires FilterAdapterRequirements<std::ranges::iterator_t<R>, Pred>
  constexpr FilterAdapter(R& range, Pred predicate) noexcept(
      noexcept(FilterAdapter(std::ranges::begin(range), std::ranges::end(range),
                             std::move(predicate))))
      : FilterAdapter(std::ranges::begin(range), std::ranges::end(range),
                      std::move(predicate)) {}

  /**
   * @brief Constructs a filter adapter from a const range.
   * @tparam R Range type
   * @param range Range to filter
   * @param predicate Function to filter elements
   */
  template <ExternalRange R>
    requires FilterAdapterRequirements<std::ranges::iterator_t<const R>, Pred>
  constexpr FilterAdapter(const R& range, Pred predicate) noexcept(
      noexcept(FilterAdapter(std::ranges::cbegin(range),
                             std::ranges::cend(range), std::move(predicate))))
      : FilterAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                      std::move(predicate)) {}

  constexpr FilterAdapter(const FilterAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Pred>) = default;
  constexpr FilterAdapter(FilterAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Pred>) = default;
  constexpr ~FilterAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                      std::is_nothrow_destructible_v<Pred>) =
      default;

  constexpr FilterAdapter& operator=(const FilterAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> &&
      std::is_nothrow_copy_assignable_v<Pred>) = default;
  constexpr FilterAdapter& operator=(FilterAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> &&
      std::is_nothrow_move_assignable_v<Pred>) = default;

  constexpr FilterAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()) && noexcept(AdvanceToValid()));
  [[nodiscard]] constexpr FilterAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<FilterAdapter> &&
      noexcept(++std::declval<FilterAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const FilterAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const FilterAdapter& other) const
      noexcept(noexcept(std::declval<const FilterAdapter&>() ==
                        std::declval<const FilterAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the iterator has reached the end.
   * @return True if at end, false otherwise
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == end_;
  }

  [[nodiscard]] constexpr FilterAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<FilterAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr FilterAdapter end() const
      noexcept(std::is_nothrow_constructible_v<FilterAdapter, const Iter&,
                                               const Iter&, const Pred&>) {
    return {end_, end_, predicate_};
  }

private:
  constexpr void AdvanceToValid() noexcept(
      std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
      noexcept(*std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()) &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()));

  Iter begin_;      ///< Start of the iterator range
  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Pred predicate_;  ///< Predicate function for filtering
};

template <ExternalRange R, typename Pred>
FilterAdapter(R&, Pred) -> FilterAdapter<std::ranges::iterator_t<R>, Pred>;

template <ExternalRange R, typename Pred>
FilterAdapter(const R&, Pred)
    -> FilterAdapter<std::ranges::iterator_t<const R>, Pred>;

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
constexpr auto FilterAdapter<Iter, Pred>::operator++() noexcept(
    noexcept(++std::declval<Iter&>()) && noexcept(AdvanceToValid()))
    -> FilterAdapter& {
  ++current_;
  AdvanceToValid();
  return *this;
}

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
constexpr auto FilterAdapter<Iter, Pred>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<FilterAdapter> &&
    noexcept(++std::declval<FilterAdapter&>())) -> FilterAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Pred>
  requires FilterAdapterRequirements<Iter, Pred>
constexpr void FilterAdapter<Iter, Pred>::AdvanceToValid() noexcept(
    std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
    noexcept(*std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()) &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>())) {
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
 * @details This adapter applies a transformation function to each element
 * during iteration. The transformation is lazy - it happens when the element is
 * accessed, not when the adapter is created.
 *
 * The transformation function can accept either the full value or individual
 * tuple components if the value is a tuple type.
 *
 * @note This adapter maintains forward iterator semantics.
 * The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 * @tparam Func Transformation function type
 *
 * @code
 * auto positions = query.Map([](const Transform& t) {
 *   return t.position;
 * });
 * @endcode
 */
template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
class MapAdapter final : public FunctionalAdapterBase<MapAdapter<Iter, Func>> {
private:
  template <typename T>
  struct DeduceValueType;

  template <typename... Args>
  struct DeduceValueType<std::tuple<Args...>> {
    using Type = decltype(std::apply(std::declval<Func>(),
                                     std::declval<std::tuple<Args...>>()));
  };

  template <typename T>
    requires(!requires { typename std::tuple_size<T>::type; })
  struct DeduceValueType<T> {
    using Type = std::invoke_result_t<Func, T>;
  };

public:
  using iterator_concept = std::input_iterator_tag;
  using iterator_category = std::input_iterator_tag;
  using value_type = typename DeduceValueType<std::iter_value_t<Iter>>::Type;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a map adapter with the given iterator range and transform
   * function.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param transform Function to transform elements
   */
  constexpr MapAdapter(Iter begin, Iter end, Func transform) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Func>)
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        transform_(std::move(transform)) {}

  /**
   * @brief Constructs a map adapter from a range and transform function.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param transform Function to transform elements
   */
  template <ExternalRange R>
    requires MapAdapterRequirements<std::ranges::iterator_t<R>, Func>
  constexpr MapAdapter(R& range, Func transform) noexcept(
      noexcept(MapAdapter(std::ranges::begin(range), std::ranges::end(range),
                          std::move(transform))))
      : MapAdapter(std::ranges::begin(range), std::ranges::end(range),
                   std::move(transform)) {}

  /**
   * @brief Constructs a map adapter from a const range and transform function.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param transform Function to transform elements
   */
  template <ExternalRange R>
    requires MapAdapterRequirements<std::ranges::iterator_t<const R>, Func>
  constexpr MapAdapter(const R& range, Func transform) noexcept(
      noexcept(MapAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                          std::move(transform))))
      : MapAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                   std::move(transform)) {}

  constexpr MapAdapter(const MapAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Func>) = default;
  constexpr MapAdapter(MapAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Func>) = default;
  constexpr ~MapAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                   std::is_nothrow_destructible_v<Func>) =
      default;

  constexpr MapAdapter& operator=(const MapAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> &&
      std::is_nothrow_copy_assignable_v<Func>) = default;
  constexpr MapAdapter& operator=(MapAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> &&
      std::is_nothrow_move_assignable_v<Func>) = default;

  constexpr MapAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr MapAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<MapAdapter> &&
      noexcept(++std::declval<MapAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> &&
               noexcept(*std::declval<const Iter&>()));

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const MapAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }
  [[nodiscard]] constexpr bool operator!=(const MapAdapter& other) const
      noexcept(noexcept(std::declval<const MapAdapter&>() ==
                        std::declval<const MapAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr MapAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<MapAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr MapAdapter end() const
      noexcept(std::is_nothrow_constructible_v<MapAdapter, const Iter&,
                                               const Iter&, const Func&>) {
    return {end_, end_, transform_};
  }

private:
  Iter begin_;      ///< Start of the iterator range
  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Func transform_;  ///< Transformation function
};

template <ExternalRange R, typename Func>
MapAdapter(R&, Func) -> MapAdapter<std::ranges::iterator_t<R>, Func>;

template <ExternalRange R, typename Func>
MapAdapter(const R&, Func)
    -> MapAdapter<std::ranges::iterator_t<const R>, Func>;

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
constexpr auto MapAdapter<Iter, Func>::operator++() noexcept(
    noexcept(++std::declval<Iter&>())) -> MapAdapter& {
  ++current_;
  return *this;
}

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
constexpr auto MapAdapter<Iter, Func>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<MapAdapter> &&
    noexcept(++std::declval<MapAdapter&>())) -> MapAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Func>
  requires MapAdapterRequirements<Iter, Func>
constexpr auto MapAdapter<Iter, Func>::operator*() const
    noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> &&
             noexcept(*std::declval<const Iter&>())) -> reference {
  auto value = *current_;
  if constexpr (std::invocable<Func, decltype(value)>) {
    return transform_(value);
  } else {
    return std::apply(transform_, value);
  }
}

/**
 * @brief Iterator adapter that yields only the first N elements.
 * @details This adapter limits the number of elements yielded by the underlying
 * iterator to at most the specified count. Once the count is reached or the
 * underlying iterator reaches its end, iteration stops.
 * @note This adapter maintains forward iterator semantics.
 * @note The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 *
 * @code
 * // Get only the first 5 entities
 * auto first_five = query.Take(5);
 * @endcode
 */
template <typename Iter>
  requires TakeAdapterRequirements<Iter>
class TakeAdapter final : public FunctionalAdapterBase<TakeAdapter<Iter>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a take adapter with the given iterator range and count.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param count Maximum number of elements to yield
   */
  constexpr TakeAdapter(Iter begin, Iter end, size_t count) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        initial_count_(count),
        remaining_(count) {}

  /**
   * @brief Constructs a take adapter from a range and count.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param count The number of elements to take
   */
  template <ExternalRange R>
    requires TakeAdapterRequirements<std::ranges::iterator_t<R>>
  constexpr TakeAdapter(R& range, size_t count) noexcept(noexcept(
      TakeAdapter(std::ranges::begin(range), std::ranges::end(range), count)))
      : TakeAdapter(std::ranges::begin(range), std::ranges::end(range), count) {
  }

  /**
   * @brief Constructs a map adapter from a const range and count.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param count The number of elements to take
   */
  template <ExternalRange R>
    requires TakeAdapterRequirements<std::ranges::iterator_t<const R>>
  constexpr TakeAdapter(const R& range, size_t count) noexcept(noexcept(
      TakeAdapter(std::ranges::cbegin(range), std::ranges::cend(range), count)))
      : TakeAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                    count) {}

  constexpr TakeAdapter(const TakeAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr TakeAdapter(TakeAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~TakeAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) =
      default;

  constexpr TakeAdapter& operator=(const TakeAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr TakeAdapter& operator=(TakeAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr TakeAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr TakeAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<TakeAdapter> &&
      noexcept(++std::declval<TakeAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const TakeAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>()));

  [[nodiscard]] constexpr bool operator!=(const TakeAdapter& other) const
      noexcept(noexcept(std::declval<const TakeAdapter&>() ==
                        std::declval<const TakeAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the iterator has reached the end.
   * @return True if at end, false otherwise
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return remaining_ == 0 || current_ == end_;
  }

  [[nodiscard]] constexpr TakeAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr TakeAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter>);

private:
  Iter begin_;                ///< Start of the iterator range
  Iter current_;              ///< Current position in the iteration
  Iter end_;                  ///< End of the iterator range
  size_t initial_count_ = 0;  ///< Initial count limit
  size_t remaining_ = 0;      ///< Remaining elements to yield
};

template <ExternalRange R>
TakeAdapter(R&, size_t) -> TakeAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
TakeAdapter(const R&, size_t) -> TakeAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr auto TakeAdapter<Iter>::operator++() noexcept(
    noexcept(++std::declval<Iter&>())) -> TakeAdapter& {
  if (remaining_ > 0 && current_ != end_) {
    ++current_;
    --remaining_;
  }
  return *this;
}

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr auto TakeAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<TakeAdapter> &&
    noexcept(++std::declval<TakeAdapter&>())) -> TakeAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr bool TakeAdapter<Iter>::operator==(const TakeAdapter& other) const
    noexcept(noexcept(std::declval<const Iter&>() ==
                      std::declval<const Iter&>())) {
  // Both are at end if either has no remaining elements or both iterators are
  // at end
  const bool this_at_end = (remaining_ == 0) || (current_ == end_);
  const bool other_at_end =
      (other.remaining_ == 0) || (other.current_ == other.end_);

  if (this_at_end && other_at_end) {
    return true;
  }

  return (current_ == other.current_) && (remaining_ == other.remaining_);
}

template <typename Iter>
  requires TakeAdapterRequirements<Iter>
constexpr auto TakeAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<TakeAdapter>) -> TakeAdapter {
  auto end_iter = *this;
  end_iter.remaining_ = 0;
  return end_iter;
}

/**
 * @brief Iterator adapter that skips the first N elements.
 * @details This adapter skips over the first N elements of the underlying
 * iterator and yields all remaining elements. If the iterator has fewer than N
 * elements, the result will be empty.
 * @note This adapter maintains forward iterator semantics.
 * @note The adapter is constexpr-compatible for compile-time evaluation.
 * @tparam Iter Underlying iterator type that satisfies IteratorLike concept
 *
 * @code
 * // Skip the first 10 entities
 * auto remaining = query.Skip(10);
 * @endcode
 */
template <typename Iter>
  requires SkipAdapterRequirements<Iter>
class SkipAdapter final : public FunctionalAdapterBase<SkipAdapter<Iter>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a skip adapter with the given iterator range and count.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param count Number of elements to skip
   */
  constexpr SkipAdapter(Iter begin, Iter end, size_t count) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      noexcept(++std::declval<Iter&>()));

  /**
   * @brief Constructs a skip adapter from a range and count.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param count Number of elements to skip
   */
  template <ExternalRange R>
    requires SkipAdapterRequirements<std::ranges::iterator_t<R>>
  constexpr SkipAdapter(R& range, size_t count) noexcept(noexcept(
      SkipAdapter(std::ranges::begin(range), std::ranges::end(range), count)))
      : SkipAdapter(std::ranges::begin(range), std::ranges::end(range), count) {
  }

  /**
   * @brief Constructs a skip adapter from a const range and count.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param count Number of elements to skip
   */
  template <ExternalRange R>
    requires SkipAdapterRequirements<std::ranges::iterator_t<const R>>
  constexpr SkipAdapter(const R& range, size_t count) noexcept(noexcept(
      SkipAdapter(std::ranges::cbegin(range), std::ranges::cend(range), count)))
      : SkipAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                    count) {}

  constexpr SkipAdapter(const SkipAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr SkipAdapter(SkipAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~SkipAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) =
      default;

  constexpr SkipAdapter& operator=(const SkipAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr SkipAdapter& operator=(SkipAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr SkipAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr SkipAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<SkipAdapter> &&
      noexcept(++std::declval<SkipAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const SkipAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const SkipAdapter& other) const
      noexcept(noexcept(std::declval<const SkipAdapter&>() ==
                        std::declval<const SkipAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr SkipAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<SkipAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr SkipAdapter end() const
      noexcept(std::is_nothrow_constructible_v<SkipAdapter, const Iter&,
                                               const Iter&, size_t>) {
    return {end_, end_, 0};
  }

private:
  Iter current_;  ///< Current position in the iteration
  Iter end_;      ///< End of the iterator range
};

template <ExternalRange R>
SkipAdapter(R&, size_t) -> SkipAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
SkipAdapter(const R&, size_t) -> SkipAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
constexpr SkipAdapter<Iter>::SkipAdapter(
    Iter begin, Iter end,
    size_t count) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                           noexcept(++std::declval<Iter&>()))
    : current_(std::move(begin)), end_(std::move(end)) {
  for (size_t idx = 0; idx < count && current_ != end_; ++idx) {
    ++current_;
  }
}

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
constexpr auto SkipAdapter<Iter>::operator++() noexcept(
    noexcept(++std::declval<Iter&>())) -> SkipAdapter& {
  ++current_;
  return *this;
}

template <typename Iter>
  requires SkipAdapterRequirements<Iter>
constexpr auto SkipAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<SkipAdapter> &&
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
 * @code
 * // Take entities while their health is above zero
 * auto alive_entities = query.TakeWhile([](const Health& h) { return h.points >
 * 0; });
 * @endcode
 */
template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
class TakeWhileAdapter final
    : public FunctionalAdapterBase<TakeWhileAdapter<Iter, Pred>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a take-while adapter with the given iterator range and
   * predicate.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param predicate Function to test elements
   */
  constexpr TakeWhileAdapter(Iter begin, Iter end, Pred predicate) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Pred> && noexcept(CheckPredicate()))
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        predicate_(std::move(predicate)) {
    CheckPredicate();
  }

  /**
   * @brief Constructs a take-while adapter from a range and predicate.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param predicate Function to test elements
   */
  template <ExternalRange R>
    requires TakeWhileAdapterRequirements<std::ranges::iterator_t<R>, Pred>
  constexpr TakeWhileAdapter(R& range, Pred predicate) noexcept(
      noexcept(TakeWhileAdapter(std::ranges::begin(range),
                                std::ranges::end(range), std::move(predicate))))
      : TakeWhileAdapter(std::ranges::begin(range), std::ranges::end(range),
                         std::move(predicate)) {}

  /**
   * @brief Constructs a take-while adapter from a const range and predicate.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param predicate Function to test elements
   */
  template <ExternalRange R>
    requires TakeWhileAdapterRequirements<std::ranges::iterator_t<const R>,
                                          Pred>
  constexpr TakeWhileAdapter(const R& range, Pred predicate) noexcept(noexcept(
      TakeWhileAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                       std::move(predicate))))
      : TakeWhileAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                         std::move(predicate)) {}

  constexpr TakeWhileAdapter(const TakeWhileAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Pred>) = default;
  constexpr TakeWhileAdapter(TakeWhileAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Pred>) = default;
  constexpr ~TakeWhileAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                         std::is_nothrow_destructible_v<Pred>) =
      default;

  constexpr TakeWhileAdapter& operator=(const TakeWhileAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> &&
      std::is_nothrow_copy_assignable_v<Pred>) = default;
  constexpr TakeWhileAdapter& operator=(TakeWhileAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> &&
      std::is_nothrow_move_assignable_v<Pred>) = default;

  constexpr TakeWhileAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()) &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
      noexcept(CheckPredicate()));
  [[nodiscard]] constexpr TakeWhileAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<TakeWhileAdapter> &&
      noexcept(++std::declval<TakeWhileAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const TakeWhileAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return (stopped_ && other.stopped_) ||
           (current_ == other.current_ && stopped_ == other.stopped_);
  }

  [[nodiscard]] constexpr bool operator!=(const TakeWhileAdapter& other) const
      noexcept(noexcept(std::declval<const TakeWhileAdapter&>() ==
                        std::declval<const TakeWhileAdapter&>())) {
    return !(*this == other);
  }

  /**
   * @brief Checks if the iterator has reached the end.
   * @return True if at end, false otherwise
   */
  [[nodiscard]] constexpr bool IsAtEnd() const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return stopped_ || current_ == end_;
  }

  [[nodiscard]] constexpr TakeWhileAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr TakeWhileAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter>);

private:
  constexpr void CheckPredicate() noexcept(
      std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
      noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
      noexcept(*std::declval<Iter&>()));

  Iter begin_;            ///< Start of the iterator range
  Iter current_;          ///< Current position in the iteration
  Iter end_;              ///< End of the iterator range
  Pred predicate_;        ///< Predicate function
  bool stopped_ = false;  ///< Whether predicate has failed
};

template <ExternalRange R, typename Pred>
TakeWhileAdapter(R&, Pred)
    -> TakeWhileAdapter<std::ranges::iterator_t<R>, Pred>;

template <ExternalRange R, typename Pred>
TakeWhileAdapter(const R&, Pred)
    -> TakeWhileAdapter<std::ranges::iterator_t<const R>, Pred>;

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr auto TakeWhileAdapter<Iter, Pred>::operator++() noexcept(
    noexcept(++std::declval<Iter&>()) &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
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
    std::is_nothrow_copy_constructible_v<TakeWhileAdapter> &&
    noexcept(++std::declval<TakeWhileAdapter&>())) -> TakeWhileAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr auto TakeWhileAdapter<Iter, Pred>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<TakeWhileAdapter>)
        -> TakeWhileAdapter {
  auto end_iter = *this;
  end_iter.stopped_ = true;
  return end_iter;
}

template <typename Iter, typename Pred>
  requires TakeWhileAdapterRequirements<Iter, Pred>
constexpr void TakeWhileAdapter<Iter, Pred>::CheckPredicate() noexcept(
    std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
    noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
    noexcept(*std::declval<Iter&>())) {
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
 * @code
 * // Skip entities while their health is zero
 * auto non_zero_health = query.SkipWhile([](const Health& h) { return h.points
 * == 0; });
 * @endcode
 */
template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
class SkipWhileAdapter final
    : public FunctionalAdapterBase<SkipWhileAdapter<Iter, Pred>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a skip-while adapter with the given iterator range and
   * predicate.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param predicate Function to test elements
   */
  constexpr SkipWhileAdapter(Iter begin, Iter end, Pred predicate) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Pred> &&
      noexcept(AdvancePastSkipped()))
      : current_(std::move(begin)),
        end_(std::move(end)),
        predicate_(std::move(predicate)) {
    AdvancePastSkipped();
  }

  /**
   * @brief Constructs a skip-while adapter from a range and predicate.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param predicate Function to test elements
   */
  template <ExternalRange R>
    requires SkipWhileAdapterRequirements<std::ranges::iterator_t<R>, Pred>
  constexpr SkipWhileAdapter(R& range, Pred predicate) noexcept(
      noexcept(SkipWhileAdapter(std::ranges::begin(range),
                                std::ranges::end(range), std::move(predicate))))
      : SkipWhileAdapter(std::ranges::begin(range), std::ranges::end(range),
                         std::move(predicate)) {}

  /**
   * @brief Constructs a skip-while adapter from a const range and predicate.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param predicate Function to test elements
   */
  template <ExternalRange R>
    requires SkipWhileAdapterRequirements<std::ranges::iterator_t<const R>,
                                          Pred>
  constexpr SkipWhileAdapter(const R& range, Pred predicate) noexcept(noexcept(
      SkipWhileAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                       std::move(predicate))))
      : SkipWhileAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                         std::move(predicate)) {}

  constexpr SkipWhileAdapter(const SkipWhileAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Pred>) = default;
  constexpr SkipWhileAdapter(SkipWhileAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Pred>) = default;
  constexpr ~SkipWhileAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                         std::is_nothrow_destructible_v<Pred>) =
      default;

  constexpr SkipWhileAdapter& operator=(const SkipWhileAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> &&
      std::is_nothrow_copy_assignable_v<Pred>) = default;
  constexpr SkipWhileAdapter& operator=(SkipWhileAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> &&
      std::is_nothrow_move_assignable_v<Pred>) = default;

  constexpr SkipWhileAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr SkipWhileAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<SkipWhileAdapter> &&
      noexcept(++std::declval<SkipWhileAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const SkipWhileAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const SkipWhileAdapter& other) const
      noexcept(noexcept(std::declval<const SkipWhileAdapter&>() ==
                        std::declval<const SkipWhileAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr SkipWhileAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<SkipWhileAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr SkipWhileAdapter end() const
      noexcept(std::is_nothrow_constructible_v<SkipWhileAdapter, const Iter&,
                                               const Iter&, const Pred&>) {
    return {end_, end_, predicate_};
  }

private:
  constexpr void AdvancePastSkipped() noexcept(
      std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
      noexcept(*std::declval<Iter&>()) && noexcept(++std::declval<Iter&>()));

  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Pred predicate_;  ///< Predicate function
};

template <ExternalRange R, typename Pred>
SkipWhileAdapter(R&, Pred)
    -> SkipWhileAdapter<std::ranges::iterator_t<R>, Pred>;

template <ExternalRange R, typename Pred>
SkipWhileAdapter(const R&, Pred)
    -> SkipWhileAdapter<std::ranges::iterator_t<const R>, Pred>;

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
constexpr auto SkipWhileAdapter<Iter, Pred>::operator++() noexcept(
    noexcept(++std::declval<Iter&>())) -> SkipWhileAdapter& {
  ++current_;
  return *this;
}

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
constexpr auto SkipWhileAdapter<Iter, Pred>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<SkipWhileAdapter> &&
    noexcept(++std::declval<SkipWhileAdapter&>())) -> SkipWhileAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Pred>
  requires SkipWhileAdapterRequirements<Iter, Pred>
constexpr void SkipWhileAdapter<Iter, Pred>::AdvancePastSkipped() noexcept(
    std::is_nothrow_invocable_v<Pred, std::iter_value_t<Iter>> &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
    noexcept(*std::declval<Iter&>()) && noexcept(++std::declval<Iter&>())) {
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
 * @details This adapter yields tuples of (index, value) where index starts at 0
 * and increments for each element.
 * @tparam Iter Underlying iterator type
 *
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
class EnumerateAdapter final
    : public FunctionalAdapterBase<EnumerateAdapter<Iter>> {
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
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type =
      typename MakeEnumeratedValue<std::iter_value_t<Iter>>::type;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs an enumerate adapter with the given iterator range.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   */
  constexpr EnumerateAdapter(Iter begin, Iter end) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)), current_(begin_), end_(std::move(end)) {}

  /**
   * @brief Constructs a enumerate adapter from a range.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   */
  template <ExternalRange R>
    requires EnumerateAdapterRequirements<std::ranges::iterator_t<R>>
  explicit constexpr EnumerateAdapter(R& range) noexcept(noexcept(
      EnumerateAdapter(std::ranges::begin(range), std::ranges::end(range))))
      : EnumerateAdapter(std::ranges::begin(range), std::ranges::end(range)) {}

  /**
   * @brief Constructs a enumerate adapter from a const range.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   */
  template <ExternalRange R>
    requires EnumerateAdapterRequirements<std::ranges::iterator_t<const R>>
  explicit constexpr EnumerateAdapter(const R& range) noexcept(noexcept(
      EnumerateAdapter(std::ranges::cbegin(range), std::ranges::cend(range))))
      : EnumerateAdapter(std::ranges::cbegin(range), std::ranges::cend(range)) {
  }

  constexpr EnumerateAdapter(const EnumerateAdapter&) = default;
  constexpr EnumerateAdapter(EnumerateAdapter&&) = default;
  constexpr ~EnumerateAdapter() = default;

  constexpr EnumerateAdapter& operator=(const EnumerateAdapter&) = default;
  constexpr EnumerateAdapter& operator=(EnumerateAdapter&&) = default;

  constexpr EnumerateAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr EnumerateAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<EnumerateAdapter> &&
      noexcept(++std::declval<EnumerateAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const;

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const EnumerateAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const EnumerateAdapter& other) const
      noexcept(noexcept(std::declval<const EnumerateAdapter&>() ==
                        std::declval<const EnumerateAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr EnumerateAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<EnumerateAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr EnumerateAdapter end() const
      noexcept(std::is_nothrow_constructible_v<EnumerateAdapter, const Iter&,
                                               const Iter&>) {
    return {end_, end_};
  }

private:
  Iter begin_;        ///< Start of the iterator range
  Iter current_;      ///< Current position in the iteration
  Iter end_;          ///< End of the iterator range
  size_t index_ = 0;  ///< Current index in the enumeration
};

template <ExternalRange R>
EnumerateAdapter(R&) -> EnumerateAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
EnumerateAdapter(const R&)
    -> EnumerateAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
constexpr auto EnumerateAdapter<Iter>::operator++() noexcept(
    noexcept(++std::declval<Iter&>())) -> EnumerateAdapter& {
  ++current_;
  ++index_;
  return *this;
}

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
constexpr auto EnumerateAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<EnumerateAdapter> &&
    noexcept(++std::declval<EnumerateAdapter&>())) -> EnumerateAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires EnumerateAdapterRequirements<Iter>
constexpr auto EnumerateAdapter<Iter>::operator*() const -> reference {
  auto value = *current_;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
  if constexpr (details::TupleLike<decltype(value)>) {
    return std::tuple_cat(std::tuple{index_}, value);
  } else {
    return std::tuple{index_, value};
  }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

/**
 * @brief Iterator adapter that applies a function to each element for
 * observation.
 * @details This adapter allows observing elements without modifying them.
 * The inspector function is called for side effects only.
 * @tparam Iter Underlying iterator type
 * @tparam Func Inspector function type
 *
 * @code
 * // Inspect entities to log their IDs
 * auto inspected = query.Inspect([](const Entity& e) { helios::log::Info("{}",
 * e.id);
 * });
 * @endcode
 */
template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
class InspectAdapter final
    : public FunctionalAdapterBase<InspectAdapter<Iter, Func>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs an inspect adapter with the given iterator range and
   * inspector function.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param inspector Function to call for each element
   */
  constexpr InspectAdapter(Iter begin, Iter end, Func inspector) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Func>)
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        inspector_(std::move(inspector)) {}

  /**
   * @brief Constructs an inspect adapter with the given range and inspector
   * function.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param inspector Function to call for each element
   */
  template <ExternalRange R>
    requires InspectAdapterRequirements<std::ranges::iterator_t<R>, Func>
  constexpr InspectAdapter(R& range, Func inspector) noexcept(
      noexcept(InspectAdapter(std::ranges::begin(range),
                              std::ranges::end(range), std::move(inspector))))
      : InspectAdapter(std::ranges::begin(range), std::ranges::end(range),
                       std::move(inspector)) {}

  /**
   * @brief Constructs an inspect adapter with the given const range and
   * inspector function.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param inspector Function to call for each element
   */
  template <ExternalRange R>
    requires InspectAdapterRequirements<std::ranges::iterator_t<const R>, Func>
  constexpr InspectAdapter(const R& range, Func inspector) noexcept(
      noexcept(InspectAdapter(std::ranges::cbegin(range),
                              std::ranges::cend(range), std::move(inspector))))
      : InspectAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                       std::move(inspector)) {}

  constexpr InspectAdapter(const InspectAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Func>) = default;
  constexpr InspectAdapter(InspectAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<Func>) = default;
  constexpr ~InspectAdapter() noexcept(std::is_nothrow_destructible_v<Iter> &&
                                       std::is_nothrow_destructible_v<Func>) =
      default;

  constexpr InspectAdapter& operator=(const InspectAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> &&
      std::is_nothrow_copy_assignable_v<Func>) = default;
  constexpr InspectAdapter& operator=(InspectAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> &&
      std::is_nothrow_move_assignable_v<Func>) = default;

  constexpr InspectAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr InspectAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<InspectAdapter> &&
      noexcept(++std::declval<InspectAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> &&
               noexcept(*std::declval<const Iter&>()));

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const InspectAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const InspectAdapter& other) const
      noexcept(noexcept(std::declval<const InspectAdapter&>() ==
                        std::declval<const InspectAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr InspectAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<InspectAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr InspectAdapter end() const
      noexcept(std::is_nothrow_constructible_v<InspectAdapter, const Iter&,
                                               const Iter&, const Func&>) {
    return {end_, end_, inspector_};
  }

private:
  Iter begin_;      ///< Start of the iterator range
  Iter current_;    ///< Current position in the iteration
  Iter end_;        ///< End of the iterator range
  Func inspector_;  ///< Inspector function
};

template <ExternalRange R, typename Func>
InspectAdapter(R&, Func) -> InspectAdapter<std::ranges::iterator_t<R>, Func>;

template <ExternalRange R, typename Func>
InspectAdapter(const R&, Func)
    -> InspectAdapter<std::ranges::iterator_t<const R>, Func>;

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
constexpr auto InspectAdapter<Iter, Func>::operator++() noexcept(
    noexcept(++std::declval<Iter&>())) -> InspectAdapter& {
  ++current_;
  return *this;
}

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
constexpr auto InspectAdapter<Iter, Func>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<InspectAdapter> &&
    noexcept(++std::declval<InspectAdapter&>())) -> InspectAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter, typename Func>
  requires InspectAdapterRequirements<Iter, Func>
constexpr auto InspectAdapter<Iter, Func>::operator*() const
    noexcept(std::is_nothrow_invocable_v<Func, std::iter_value_t<Iter>> &&
             noexcept(*std::declval<const Iter&>())) -> reference {
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
 * @details This adapter yields every step-th element from the underlying
 * iterator.
 * @tparam Iter Underlying iterator type
 *
 * @code
 * // Step through entities by 3
 * auto stepped = query.StepBy(3);
 * @endcode
 */
template <typename Iter>
  requires StepByAdapterRequirements<Iter>
class StepByAdapter final : public FunctionalAdapterBase<StepByAdapter<Iter>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a step-by adapter with the given iterator range and step
   * size.
   * @param begin Start of the iterator range
   * @param end End of the iterator range
   * @param step Number of elements to step by
   */
  constexpr StepByAdapter(Iter begin, Iter end, size_t step) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        step_(step > 0 ? step : 1) {}

  /**
   * @brief Constructs a step-by adapter with the given range and step size.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param step Number of elements to step by
   */
  template <ExternalRange R>
    requires StepByAdapterRequirements<std::ranges::iterator_t<R>>
  constexpr StepByAdapter(R& range, size_t step) noexcept(noexcept(
      StepByAdapter(std::ranges::begin(range), std::ranges::end(range), step)))
      : StepByAdapter(std::ranges::begin(range), std::ranges::end(range),
                      step) {}

  /**
   * @brief Constructs a step-by adapter with the given const range and step
   * size.
   * @tparam R The type of the input range
   * @param range The input range to adapt
   * @param step Number of elements to step by
   */
  template <ExternalRange R>
    requires StepByAdapterRequirements<std::ranges::iterator_t<const R>>
  constexpr StepByAdapter(const R& range, size_t step) noexcept(
      noexcept(StepByAdapter(std::ranges::cbegin(range),
                             std::ranges::cend(range), step)))
      : StepByAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                      step) {}

  constexpr StepByAdapter(const StepByAdapter&) = default;
  constexpr StepByAdapter(StepByAdapter&&) = default;
  constexpr ~StepByAdapter() = default;

  constexpr StepByAdapter& operator=(const StepByAdapter&) = default;
  constexpr StepByAdapter& operator=(StepByAdapter&&) = default;

  constexpr StepByAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter&>()) &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()));
  [[nodiscard]] constexpr StepByAdapter operator++(int) noexcept(
      std::is_copy_constructible_v<StepByAdapter> &&
      noexcept(++std::declval<StepByAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const StepByAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const StepByAdapter& other) const
      noexcept(noexcept(std::declval<const StepByAdapter&>() ==
                        std::declval<const StepByAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr StepByAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<StepByAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr StepByAdapter end() const
      noexcept(std::is_nothrow_constructible_v<StepByAdapter, const Iter&,
                                               const Iter&, size_t>) {
    return {end_, end_, step_};
  }

private:
  Iter begin_;       ///< Start of the iterator range
  Iter current_;     ///< Current position in the iteration
  Iter end_;         ///< End of the iterator range
  size_t step_ = 0;  ///< Step size between elements
};

template <ExternalRange R>
StepByAdapter(R&, size_t) -> StepByAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
StepByAdapter(const R&, size_t)
    -> StepByAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires StepByAdapterRequirements<Iter>
constexpr auto StepByAdapter<Iter>::operator++() noexcept(
    noexcept(++std::declval<Iter&>()) &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()))
    -> StepByAdapter& {
  for (size_t i = 0; i < step_ && current_ != end_; ++i) {
    ++current_;
  }
  return *this;
}

template <typename Iter>
  requires StepByAdapterRequirements<Iter>
constexpr auto StepByAdapter<Iter>::operator++(int) noexcept(
    std::is_copy_constructible_v<StepByAdapter> &&
    noexcept(++std::declval<StepByAdapter&>())) -> StepByAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

/**
 * @brief Iterator adapter that chains two sequences together.
 * @details This adapter yields all elements from the first iterator, then all
 * elements from the second iterator.
 * @tparam Iter1 First iterator type
 * @tparam Iter2 Second iterator type
 *
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
class ChainAdapter final
    : public FunctionalAdapterBase<ChainAdapter<Iter1, Iter2>> {
public:
  using iterator_concept =
      std::conditional_t<details::IterYieldsReference<Iter1> &&
                             details::IterYieldsReference<Iter2>,
                         std::forward_iterator_tag, std::input_iterator_tag>;
  using iterator_category =
      std::conditional_t<details::IterYieldsReference<Iter1> &&
                             details::IterYieldsReference<Iter2>,
                         std::forward_iterator_tag, std::input_iterator_tag>;

  using value_type = std::iter_value_t<Iter1>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::common_type_t<std::iter_difference_t<Iter1>,
                                             std::iter_difference_t<Iter2>>;

  /**
   * @brief Constructs a chain adapter with two iterator ranges.
   * @param first_begin Start of the first iterator range
   * @param first_end End of the first iterator range
   * @param second_begin Start of the second iterator range
   * @param second_end End of the second iterator range
   */
  constexpr ChainAdapter(
      Iter1 first_begin, Iter1 first_end, Iter2 second_begin,
      Iter2 second_end) noexcept(std::is_nothrow_move_constructible_v<Iter1> &&
                                 std::is_nothrow_move_constructible_v<Iter2> &&
                                 noexcept(std::declval<Iter1&>() !=
                                          std::declval<Iter1&>()))
      : first_current_(std::move(first_begin)),
        first_end_(std::move(first_end)),
        second_current_(std::move(second_begin)),
        second_end_(std::move(second_end)),
        in_first_(first_current_ != first_end_) {}

  /**
   * @brief Constructs a chain adapter with the given ranges.
   * @tparam R1 The type of the first range
   * @tparam R2 The type of the second range
   * @param first_range The first range to adapt
   * @param second_range The second range to adapt
   */
  template <ExternalRange R1, ExternalRange R2>
    requires ChainAdapterRequirements<std::ranges::iterator_t<R1>,
                                      std::ranges::iterator_t<R2>>
  constexpr ChainAdapter(R1& first_range, R2& second_range) noexcept(
      noexcept(ChainAdapter(std::ranges::begin(first_range),
                            std::ranges::end(first_range),
                            std::ranges::begin(second_range),
                            std::ranges::end(second_range))))
      : ChainAdapter(
            std::ranges::begin(first_range), std::ranges::end(first_range),
            std::ranges::begin(second_range), std::ranges::end(second_range)) {}

  /**
   * @brief Constructs a chain adapter with the given const ranges.
   * @tparam R1 The type of the first range
   * @tparam R2 The type of the second range
   * @param first_range The first range to adapt
   * @param second_range The second range to adapt
   */
  template <ExternalRange R1, ExternalRange R2>
    requires ChainAdapterRequirements<std::ranges::iterator_t<const R1>,
                                      std::ranges::iterator_t<const R2>>
  constexpr ChainAdapter(
      const R1& first_range,
      const R2&
          second_range) noexcept(noexcept(ChainAdapter(std::ranges::
                                                           cbegin(first_range),
                                                       std::ranges::cend(
                                                           first_range),
                                                       std::ranges::cbegin(
                                                           second_range),
                                                       std::ranges::cend(
                                                           second_range))))
      : ChainAdapter(std::ranges::cbegin(first_range),
                     std::ranges::cend(first_range),
                     std::ranges::cbegin(second_range),
                     std::ranges::cend(second_range)) {}

  constexpr ChainAdapter(const ChainAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter1> &&
      std::is_nothrow_copy_constructible_v<Iter2>) = default;
  constexpr ChainAdapter(ChainAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter1> &&
      std::is_nothrow_move_constructible_v<Iter2>) = default;
  constexpr ~ChainAdapter() noexcept(std::is_nothrow_destructible_v<Iter1> &&
                                     std::is_nothrow_destructible_v<Iter2>) =
      default;

  constexpr ChainAdapter& operator=(const ChainAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter1> &&
      std::is_nothrow_copy_assignable_v<Iter2>) = default;
  constexpr ChainAdapter& operator=(ChainAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter1> &&
      std::is_nothrow_move_assignable_v<Iter2>) = default;

  constexpr ChainAdapter& operator++() noexcept(
      noexcept(++std::declval<Iter1&>()) &&
      noexcept(++std::declval<Iter2&>()) &&
      noexcept(std::declval<Iter1&>() == std::declval<Iter1&>()) &&
      noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()));
  [[nodiscard]] constexpr ChainAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<ChainAdapter> &&
      noexcept(++std::declval<ChainAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter1&>()) &&
               noexcept(*std::declval<const Iter2&>())) {
    return in_first_ ? *first_current_ : *second_current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const ChainAdapter& other) const
      noexcept(noexcept(std::declval<const Iter1&>() ==
                        std::declval<const Iter1&>()) &&
               noexcept(std::declval<const Iter2&>() ==
                        std::declval<const Iter2&>()));

  [[nodiscard]] constexpr bool operator!=(const ChainAdapter& other) const
      noexcept(noexcept(std::declval<const ChainAdapter&>() ==
                        std::declval<const ChainAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr ChainAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr ChainAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter> &&
               std::is_nothrow_copy_constructible_v<Iter1> &&
               std::is_nothrow_copy_constructible_v<Iter2>);

private:
  Iter1 first_current_;   ///< Current position in first iterator
  Iter1 first_end_;       ///< End of first iterator range
  Iter2 second_current_;  ///< Current position in second iterator
  Iter2 second_end_;      ///< End of second iterator range
  bool in_first_ = true;  ///< Whether iterating through first range
};

template <ExternalRange R1, ExternalRange R2>
  requires ChainAdapterRequirements<std::ranges::iterator_t<R1>,
                                    std::ranges::iterator_t<R2>>
ChainAdapter(R1&, R2&)
    -> ChainAdapter<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>;

template <ExternalRange R1, ExternalRange R2>
  requires ChainAdapterRequirements<std::ranges::iterator_t<const R1>,
                                    std::ranges::iterator_t<const R2>>
ChainAdapter(const R1&, const R2&)
    -> ChainAdapter<std::ranges::iterator_t<const R1>,
                    std::ranges::iterator_t<const R2>>;

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr auto ChainAdapter<Iter1, Iter2>::operator++() noexcept(
    noexcept(++std::declval<Iter1&>()) && noexcept(++std::declval<Iter2&>()) &&
    noexcept(std::declval<Iter1&>() == std::declval<Iter1&>()) &&
    noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()))
    -> ChainAdapter& {
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
    std::is_nothrow_copy_constructible_v<ChainAdapter> &&
    noexcept(++std::declval<ChainAdapter&>())) -> ChainAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr bool ChainAdapter<Iter1, Iter2>::operator==(const ChainAdapter& other)
    const noexcept(noexcept(std::declval<const Iter1&>() ==
                            std::declval<const Iter1&>()) &&
                   noexcept(std::declval<const Iter2&>() ==
                            std::declval<const Iter2&>())) {
  // Check if both are at end (in second range and at second_end_)
  const bool this_at_end = !in_first_ && (second_current_ == second_end_);
  const bool other_at_end =
      !other.in_first_ && (other.second_current_ == other.second_end_);

  if (this_at_end && other_at_end) {
    return true;
  }

  // Otherwise, must be in same range at same position
  if (in_first_ != other.in_first_) {
    return false;
  }

  return in_first_ ? (first_current_ == other.first_current_)
                   : (second_current_ == other.second_current_);
}

template <typename Iter1, typename Iter2>
  requires ChainAdapterRequirements<Iter1, Iter2>
constexpr auto ChainAdapter<Iter1, Iter2>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<ChainAdapter> &&
             std::is_nothrow_copy_constructible_v<Iter1> &&
             std::is_nothrow_copy_constructible_v<Iter2>) -> ChainAdapter {
  auto end_iter = *this;
  end_iter.first_current_ = first_end_;
  end_iter.second_current_ = second_end_;
  end_iter.in_first_ = false;
  return end_iter;
}

/**
 * @brief Adapter that iterates through elements in reverse order.
 * @details Requires a bidirectional iterator. Uses operator-- to traverse
 * backwards.
 * @tparam Iter Type of the underlying iterator
 *
 * @code
 * // Reverse iterate through entities
 * auto reversed = query.Reverse();
 * @endcode
 */
template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
class ReverseAdapter final
    : public FunctionalAdapterBase<ReverseAdapter<Iter>> {
public:
  using iterator_concept =
      std::conditional_t<details::IterYieldsReference<Iter>,
                         std::bidirectional_iterator_tag,
                         std::input_iterator_tag>;
  using iterator_category =
      std::conditional_t<details::IterYieldsReference<Iter>,
                         std::bidirectional_iterator_tag,
                         std::input_iterator_tag>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a reverse adapter.
   * @param begin Iterator to the beginning of the range
   * @param end Iterator to the end of the range
   */
  constexpr ReverseAdapter(Iter begin, Iter end) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter> &&
      noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
      noexcept(--std::declval<Iter&>()));

  /**
   * @brief Constructs a reverse adapter with the given range.
   * @tparam R The type of the range
   * @param range The range to adapt
   */
  template <ExternalRange R>
    requires ReverseAdapterRequirements<std::ranges::iterator_t<R>> &&
             std::ranges::bidirectional_range<R>
  explicit constexpr ReverseAdapter(R& range) noexcept(noexcept(
      ReverseAdapter(std::ranges::begin(range), std::ranges::end(range))))
      : ReverseAdapter(std::ranges::begin(range), std::ranges::end(range)) {}

  /**
   * @brief Constructs a reverse adapter with the given const range.
   * @tparam R The type of the range
   * @param range The range to adapt
   */
  template <ExternalRange R>
    requires ReverseAdapterRequirements<std::ranges::iterator_t<const R>> &&
             std::ranges::bidirectional_range<R>
  explicit constexpr ReverseAdapter(const R& range) noexcept(noexcept(
      ReverseAdapter(std::ranges::cbegin(range), std::ranges::cend(range))))
      : ReverseAdapter(std::ranges::cbegin(range), std::ranges::cend(range)) {}

  constexpr ReverseAdapter(const ReverseAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr ReverseAdapter(ReverseAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~ReverseAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) =
      default;

  constexpr ReverseAdapter& operator=(const ReverseAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr ReverseAdapter& operator=(ReverseAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr ReverseAdapter& operator++() noexcept(
      noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
      noexcept(--std::declval<Iter&>()));
  [[nodiscard]] constexpr ReverseAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
      noexcept(++std::declval<ReverseAdapter&>()));

  constexpr ReverseAdapter& operator--() noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      noexcept(++std::declval<Iter&>()) &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()));
  [[nodiscard]] constexpr ReverseAdapter operator--(int) noexcept(
      std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
      noexcept(--std::declval<ReverseAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  constexpr bool operator==(const ReverseAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>()));
  constexpr bool operator!=(const ReverseAdapter& other) const
      noexcept(noexcept(std::declval<ReverseAdapter>() ==
                        std::declval<ReverseAdapter>())) {
    return !(*this == other);
  }

  constexpr ReverseAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter>) {
    return *this;
  }

  constexpr ReverseAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter>);

private:
  Iter begin_;
  Iter current_;
  Iter end_;
  bool done_ = false;
};

template <ExternalRange R>
ReverseAdapter(R&) -> ReverseAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
ReverseAdapter(const R&) -> ReverseAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr ReverseAdapter<Iter>::ReverseAdapter(Iter begin, Iter end) noexcept(
    std::is_nothrow_move_constructible_v<Iter> &&
    std::is_nothrow_copy_constructible_v<Iter> &&
    noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
    noexcept(--std::declval<Iter&>()))
    : begin_(std::move(begin)),
      current_(std::move(end)),
      end_(current_),
      done_(begin_ == end_) {
  if (current_ != begin_) {
    --current_;
  }
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::operator++() noexcept(
    noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
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
constexpr auto ReverseAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
    noexcept(++std::declval<ReverseAdapter&>())) -> ReverseAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::operator--() noexcept(
    std::is_nothrow_copy_constructible_v<Iter> &&
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
constexpr auto ReverseAdapter<Iter>::operator--(int) noexcept(
    std::is_nothrow_copy_constructible_v<ReverseAdapter> &&
    noexcept(--std::declval<ReverseAdapter&>())) -> ReverseAdapter {
  auto temp = *this;
  --(*this);
  return temp;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr bool ReverseAdapter<Iter>::operator==(const ReverseAdapter& other)
    const noexcept(noexcept(std::declval<const Iter&>() ==
                            std::declval<const Iter&>())) {
  if (done_ && other.done_) {
    return true;
  }
  return done_ == other.done_ && current_ == other.current_;
}

template <typename Iter>
  requires ReverseAdapterRequirements<Iter>
constexpr auto ReverseAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<ReverseAdapter>)
        -> ReverseAdapter {
  auto result = *this;
  result.done_ = true;
  return result;
}

/**
 * @brief Adapter that flattens nested ranges into a single sequence.
 * @details Iterates through an outer range of ranges, yielding elements from
 * inner ranges.
 * @tparam Iter Type of the outer iterator
 *
 * @code
 * // Join queries of `Position` and `Velocity` components
 * auto joined = query1.Join(query2);
 * joined.ForEach([](const Position& pos, const Velocity& vel) {
 */
template <typename Iter>
  requires JoinAdapterRequirements<Iter>
class JoinAdapter final : public FunctionalAdapterBase<JoinAdapter<Iter>> {
public:
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using outer_value_type = std::iter_value_t<Iter>;
  using inner_iterator_type = std::ranges::iterator_t<outer_value_type>;
  using value_type = std::iter_value_t<inner_iterator_type>;
  using difference_type = ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type;

  /**
   * @brief Constructs a join adapter.
   * @param begin Iterator to the beginning of the outer range
   * @param end Iterator to the end of the outer range
   */
  constexpr JoinAdapter(Iter begin, Iter end) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<inner_iterator_type> &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
      noexcept(*std::declval<inner_iterator_type&>()) &&
      noexcept(AdvanceToValid()));

  /**
   * @brief Constructs a join adapter from a range.
   * @tparam R The type of the range
   * @param range The range to adapt
   */
  template <ExternalRange R>
    requires JoinAdapterRequirements<std::ranges::iterator_t<R>>
  explicit constexpr JoinAdapter(R& range) noexcept(
      noexcept(JoinAdapter(std::ranges::begin(range), std::ranges::end(range))))
      : JoinAdapter(std::ranges::begin(range), std::ranges::end(range)) {}

  /**
   * @brief Constructs a join adapter from a range.
   * @tparam R The type of the range
   * @param range The range to adapt
   */
  template <ExternalRange R>
    requires JoinAdapterRequirements<std::ranges::iterator_t<const R>>
  explicit constexpr JoinAdapter(const R& range) noexcept(noexcept(
      JoinAdapter(std::ranges::cbegin(range), std::ranges::cend(range))))
      : JoinAdapter(std::ranges::cbegin(range), std::ranges::cend(range)) {}

  constexpr JoinAdapter(const JoinAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<inner_iterator_type>) = default;

  constexpr JoinAdapter(JoinAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_move_constructible_v<inner_iterator_type>) = default;
  constexpr ~JoinAdapter() noexcept(
      std::is_nothrow_destructible_v<Iter> &&
      std::is_nothrow_destructible_v<inner_iterator_type>) = default;

  constexpr JoinAdapter& operator=(const JoinAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter> &&
      std::is_nothrow_copy_assignable_v<inner_iterator_type>) = default;
  constexpr JoinAdapter& operator=(JoinAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter> &&
      std::is_nothrow_move_assignable_v<inner_iterator_type>) = default;

  constexpr JoinAdapter& operator++() noexcept(
      noexcept(std::declval<inner_iterator_type&>() !=
               std::declval<inner_iterator_type&>()) &&
      noexcept(++std::declval<inner_iterator_type&>()) &&
      noexcept(AdvanceToValid()));
  [[nodiscard]] constexpr JoinAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<JoinAdapter> &&
      noexcept(++std::declval<JoinAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const inner_iterator_type&>())) {
    return *inner_current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const JoinAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>()) &&
               noexcept(std::declval<const inner_iterator_type&>() ==
                        std::declval<const inner_iterator_type&>()));
  [[nodiscard]] constexpr bool operator!=(const JoinAdapter& other) const
      noexcept(noexcept(std::declval<JoinAdapter>() ==
                        std::declval<JoinAdapter>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr JoinAdapter begin() const noexcept(
      std::is_nothrow_constructible_v<JoinAdapter, const Iter&, const Iter&>) {
    return {outer_begin_, outer_end_};
  }

  [[nodiscard]] constexpr JoinAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<JoinAdapter> &&
               std::is_nothrow_copy_assignable_v<Iter>);

private:
  constexpr void AdvanceToValid() noexcept(
      noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
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

template <ExternalRange R>
  requires JoinAdapterRequirements<std::ranges::iterator_t<R>>
JoinAdapter(R&) -> JoinAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
  requires JoinAdapterRequirements<std::ranges::iterator_t<const R>>
JoinAdapter(const R&) -> JoinAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr JoinAdapter<Iter>::JoinAdapter(Iter begin, Iter end) noexcept(
    std::is_nothrow_move_constructible_v<Iter> &&
    std::is_nothrow_copy_constructible_v<Iter> &&
    std::is_nothrow_move_constructible_v<inner_iterator_type> &&
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
    noexcept(*std::declval<inner_iterator_type&>()) &&
    noexcept(AdvanceToValid()))
    : outer_begin_(std::move(begin)),
      outer_current_(outer_begin_),
      outer_end_(std::move(end)) {
  if (outer_current_ != outer_end_) {
    auto& inner_range = *outer_current_;
    inner_current_ = std::ranges::begin(inner_range);
    inner_end_ = std::ranges::end(inner_range);
  }
  AdvanceToValid();
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr auto JoinAdapter<Iter>::operator++() noexcept(
    noexcept(std::declval<inner_iterator_type&>() !=
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
constexpr auto JoinAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<JoinAdapter> &&
    noexcept(++std::declval<JoinAdapter&>())) -> JoinAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr bool JoinAdapter<Iter>::operator==(const JoinAdapter& other) const
    noexcept(noexcept(std::declval<const Iter&>() ==
                      std::declval<const Iter&>()) &&
             noexcept(std::declval<const inner_iterator_type&>() ==
                      std::declval<const inner_iterator_type&>())) {
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
    noexcept(std::is_nothrow_copy_constructible_v<JoinAdapter> &&
             std::is_nothrow_copy_assignable_v<Iter>) -> JoinAdapter {
  auto result = *this;
  result.outer_current_ = result.outer_end_;
  return result;
}

template <typename Iter>
  requires JoinAdapterRequirements<Iter>
constexpr void JoinAdapter<Iter>::AdvanceToValid() noexcept(
    noexcept(std::declval<Iter&>() == std::declval<Iter&>()) &&
    noexcept(std::declval<inner_iterator_type&>() ==
             std::declval<inner_iterator_type&>()) &&
    noexcept(++std::declval<Iter&>()) && noexcept(*std::declval<Iter&>()) &&
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
 * @details Provides a lightweight, non-allocating view over a contiguous window
 * of elements.
 * @tparam Iter Type of the underlying iterator
 *
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
  constexpr SlideView(Iter begin, size_t size) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(begin), size_(size) {}

  constexpr SlideView(const SlideView&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr SlideView(SlideView&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~SlideView() noexcept = default;

  constexpr SlideView& operator=(const SlideView&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr SlideView& operator=(SlideView&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter>) = default;

  /**
   * @brief Collects the window elements into a vector.
   * @details Use when you need ownership of the elements.
   * @return Vector containing copies of the window elements
   */
  [[nodiscard]] constexpr auto Collect() const -> std::vector<value_type>
    requires std::copy_constructible<value_type>;

  /**
   * @brief Collects the window elements into a vector with a custom allocator.
   * @details Use when you need ownership of the elements with a specific
   * allocator.
   * @tparam Allocator Allocator type
   * @param allocator Allocator to use for the vector
   * @return Vector containing copies of the window elements
   */
  template <typename Allocator>
    requires(!std::derived_from<std::remove_pointer_t<Allocator>,
                                std::pmr::memory_resource>)
  [[nodiscard]] constexpr auto CollectWith(Allocator allocator = {}) const
      -> std::vector<value_type, Allocator>
    requires std::copy_constructible<value_type>;

  /**
   * @brief Collects the window elements into a vector with a specific memory
   * resource.
   * @details Use when you need ownership of the elements with a specific
   * memory resource.
   * @param resource Memory resource to use for the vector
   * @return Vector containing copies of the window elements
   */
  [[nodiscard]] constexpr auto CollectWith(
      std::pmr::memory_resource* resource) const -> std::pmr::vector<value_type>
    requires std::copy_constructible<value_type>;

  auto CollectWith(std::nullptr_t) const
      -> std::pmr::vector<value_type> = delete;

  /**
   * @brief Accesses an element by index.
   * @warning Index must be less than size
   * @param index Index of the element
   * @return Reference to the element
   */
  [[nodiscard]] constexpr reference operator[](size_t index) const
      noexcept(noexcept(*std::declval<Iter>()) &&
               noexcept(++std::declval<Iter&>()));

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
  [[nodiscard]] constexpr Iter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<Iter>) {
    return begin_;
  }

  /**
   * @brief Returns an iterator to the end.
   * @return Iterator past the last element
   */
  [[nodiscard]] constexpr Iter end() const
      noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
               noexcept(++std::declval<Iter&>()));

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
template <typename Allocator>
  requires(!std::derived_from<std::remove_pointer_t<Allocator>,
                              std::pmr::memory_resource>)
constexpr auto SlideView<Iter>::CollectWith(Allocator allocator) const
    -> std::vector<value_type, Allocator>
  requires std::copy_constructible<value_type>
{
  std::vector<value_type, Allocator> result{std::move(allocator)};
  result.reserve(size_);
  auto it = begin_;
  for (size_t i = 0; i < size_; ++i) {
    result.push_back(*it++);
  }
  return result;
}

template <IteratorLike Iter>
constexpr auto SlideView<Iter>::CollectWith(
    std::pmr::memory_resource* resource) const -> std::pmr::vector<value_type>
  requires std::copy_constructible<value_type>
{
  std::pmr::vector<value_type> result{resource};
  result.reserve(size_);
  auto it = begin_;
  for (size_t i = 0; i < size_; ++i) {
    result.push_back(*it++);
  }
  return result;
}

template <IteratorLike Iter>
constexpr auto SlideView<Iter>::operator[](size_t index) const
    noexcept(noexcept(*std::declval<Iter>()) &&
             noexcept(++std::declval<Iter&>())) -> reference {
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

  return std::equal(std::ranges::cbegin(range), std::ranges::cend(range),
                    begin_);
}

template <IteratorLike Iter>
constexpr Iter SlideView<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<Iter> &&
             noexcept(++std::declval<Iter&>())) {
  auto it = begin_;
  for (size_t i = 0; i < size_; ++i) {
    ++it;
  }
  return it;
}

/**
 * @brief Adapter that yields sliding windows of elements.
 * @details Creates overlapping windows of a fixed size, moving one element at a
 * time. Each window is returned as a SlideView, which is a non-allocating view
 * over the elements.
 * @tparam Iter Type of the underlying iterator
 *
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
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;
  using value_type = SlideView<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a slide adapter.
   * @warning window_size must be greater than 0
   * @param begin Iterator to the beginning of the range
   * @param end Iterator to the end of the range
   * @param window_size Size of the sliding window
   */
  constexpr SlideAdapter(Iter begin, Iter end, size_t window_size) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter> &&
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
      noexcept(++std::declval<Iter&>()));

  /**
   * @brief Constructs a slide adapter from a range.
   * @tparam R The type of the range
   * @param range The range to adapt
   * @param window_size The size of the sliding window
   */
  template <ExternalRange R>
    requires SlideAdapterRequirements<std::ranges::iterator_t<R>>
  explicit constexpr SlideAdapter(R& range, size_t window_size) noexcept(
      noexcept(SlideAdapter(std::ranges::begin(range), std::ranges::end(range),
                            window_size)))
      : SlideAdapter(std::ranges::begin(range), std::ranges::end(range),
                     window_size) {}

  /**
   * @brief Constructs a slide adapter from a const range.
   * @tparam R The type of the range
   * @param range The range to adapt
   * @param window_size The size of the sliding window
   */
  template <ExternalRange R>
    requires SlideAdapterRequirements<std::ranges::iterator_t<const R>>
  explicit constexpr SlideAdapter(const R& range, size_t window_size) noexcept(
      noexcept(SlideAdapter(std::ranges::cbegin(range),
                            std::ranges::cend(range), window_size)))
      : SlideAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                     window_size) {}

  constexpr SlideAdapter(const SlideAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr SlideAdapter(SlideAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~SlideAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) =
      default;

  constexpr SlideAdapter& operator=(const SlideAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr SlideAdapter& operator=(SlideAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr SlideAdapter& operator++() noexcept(
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr SlideAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<SlideAdapter> &&
      noexcept(++std::declval<SlideAdapter&>()));

  /**
   * @brief Dereferences the iterator to get the current window.
   * @return SlideView representing the current window
   */
  [[nodiscard]] constexpr reference operator*() const
      noexcept(std::is_nothrow_constructible_v<SlideView<Iter>, Iter, size_t>) {
    return {current_, window_size_};
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const SlideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const SlideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() !=
                        std::declval<const Iter&>())) {
    return current_ != other.current_;
  }

  [[nodiscard]] constexpr SlideAdapter begin() const
      noexcept(std::is_nothrow_constructible_v<SlideAdapter, const Iter&,
                                               const Iter&, size_t>) {
    return {begin_, end_, window_size_};
  }

  [[nodiscard]] constexpr SlideAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<SlideAdapter> &&
               std::is_nothrow_copy_assignable_v<Iter> &&
               noexcept(std::declval<Iter&>() != std::declval<const Iter&>()) &&
               noexcept(++std::declval<Iter&>()));

  /**
   * @brief Returns the window size.
   * @return Size of the sliding window
   */
  [[nodiscard]] constexpr size_t WindowSize() const noexcept {
    return window_size_;
  }

private:
  Iter begin_;
  Iter current_;
  Iter end_;
  size_t window_size_ = 0;
};

template <ExternalRange R>
  requires SlideAdapterRequirements<std::ranges::iterator_t<R>>
SlideAdapter(R&, size_t) -> SlideAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
  requires SlideAdapterRequirements<std::ranges::iterator_t<const R>>
SlideAdapter(const R&, size_t)
    -> SlideAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr SlideAdapter<Iter>::SlideAdapter(
    Iter begin, Iter end,
    size_t window_size) noexcept(std::is_nothrow_move_constructible_v<Iter> &&
                                 std::is_nothrow_copy_constructible_v<Iter> &&
                                 noexcept(std::declval<Iter&>() !=
                                          std::declval<Iter&>()) &&
                                 noexcept(++std::declval<Iter&>()))
    : begin_(std::move(begin)),
      current_(begin_),
      end_(std::move(end)),
      window_size_(window_size) {
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
constexpr auto SlideAdapter<Iter>::operator++() noexcept(
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
    noexcept(++std::declval<Iter&>())) -> SlideAdapter& {
  if (current_ != end_) {
    ++current_;
  }
  return *this;
}

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr auto SlideAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<SlideAdapter> &&
    noexcept(++std::declval<SlideAdapter&>())) -> SlideAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires SlideAdapterRequirements<Iter>
constexpr auto SlideAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<SlideAdapter> &&
             std::is_nothrow_copy_assignable_v<Iter> &&
             noexcept(std::declval<Iter&>() != std::declval<const Iter&>()) &&
             noexcept(++std::declval<Iter&>())) -> SlideAdapter {
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
 * @details Similar to StepBy but with different semantics - takes stride, not
 * step.
 * @tparam Iter Type of the underlying iterator
 *
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
  using iterator_concept = details::adapter_iterator_concept_t<Iter>;
  using iterator_category = details::adapter_iterator_category_t<Iter>;
  using value_type = std::iter_value_t<Iter>;
  using reference = value_type;
  using pointer = void;
  using difference_type = std::iter_difference_t<Iter>;

  /**
   * @brief Constructs a stride adapter.
   * @param begin Iterator to the beginning of the range
   * @param end Iterator to the end of the range
   * @param stride Number of elements to skip between yields (1 = every element,
   * 2 = every other, etc.)
   * @warning stride must be greater than 0
   */
  constexpr StrideAdapter(Iter begin, Iter end, size_t stride) noexcept(
      std::is_nothrow_move_constructible_v<Iter> &&
      std::is_nothrow_copy_constructible_v<Iter>)
      : begin_(std::move(begin)),
        current_(begin_),
        end_(std::move(end)),
        stride_(stride) {}

  /**
   * @brief Constructs a stride adapter from a range and a stride.
   * @tparam R The type of the range
   * @param range The range to adapt
   * @param stride Number of elements to skip between yields (1 = every element,
   * 2 = every other, etc.)
   * @warning stride must be greater than 0
   */
  template <ExternalRange R>
    requires StrideAdapterRequirements<std::ranges::iterator_t<R>>
  constexpr StrideAdapter(R& range, size_t stride) noexcept(
      noexcept(StrideAdapter(std::ranges::begin(range), std::ranges::end(range),
                             stride)))
      : StrideAdapter(std::ranges::begin(range), std::ranges::end(range),
                      stride) {}

  /**
   * @brief Constructs a stride adapter from a const range and a stride.
   * @tparam R The type of the range
   * @param range The range to adapt
   * @param stride Number of elements to skip between yields (1 = every element,
   * 2 = every other, etc.)
   * @warning stride must be greater than 0
   */
  template <ExternalRange R>
    requires StrideAdapterRequirements<std::ranges::iterator_t<const R>>
  constexpr StrideAdapter(const R& range, size_t stride) noexcept(
      noexcept(StrideAdapter(std::ranges::cbegin(range),
                             std::ranges::cend(range), stride)))
      : StrideAdapter(std::ranges::cbegin(range), std::ranges::cend(range),
                      stride) {}

  constexpr StrideAdapter(const StrideAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter>) = default;
  constexpr StrideAdapter(StrideAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter>) = default;
  constexpr ~StrideAdapter() noexcept(std::is_nothrow_destructible_v<Iter>) =
      default;

  constexpr StrideAdapter& operator=(const StrideAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter>) = default;
  constexpr StrideAdapter& operator=(StrideAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter>) = default;

  constexpr StrideAdapter& operator++() noexcept(
      noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
      noexcept(++std::declval<Iter&>()));
  [[nodiscard]] constexpr StrideAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<StrideAdapter> &&
      noexcept(++std::declval<StrideAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const
      noexcept(noexcept(*std::declval<const Iter&>())) {
    return *current_;
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const StrideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() ==
                        std::declval<const Iter&>())) {
    return current_ == other.current_;
  }

  [[nodiscard]] constexpr bool operator!=(const StrideAdapter& other) const
      noexcept(noexcept(std::declval<const Iter&>() !=
                        std::declval<const Iter&>())) {
    return current_ != other.current_;
  }

  [[nodiscard]] constexpr StrideAdapter begin() const
      noexcept(std::is_nothrow_constructible_v<StrideAdapter, const Iter&,
                                               const Iter&, size_t>) {
    return {begin_, end_, stride_};
  }

  [[nodiscard]] constexpr StrideAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<StrideAdapter> &&
               std::is_nothrow_copy_assignable_v<Iter>);

private:
  Iter begin_;
  Iter current_;
  Iter end_;
  size_t stride_ = 0;
};

template <ExternalRange R>
  requires StrideAdapterRequirements<std::ranges::iterator_t<R>>
StrideAdapter(R&, size_t) -> StrideAdapter<std::ranges::iterator_t<R>>;

template <ExternalRange R>
  requires StrideAdapterRequirements<std::ranges::iterator_t<const R>>
StrideAdapter(const R&, size_t)
    -> StrideAdapter<std::ranges::iterator_t<const R>>;

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
constexpr auto StrideAdapter<Iter>::operator++() noexcept(
    noexcept(std::declval<Iter&>() != std::declval<Iter&>()) &&
    noexcept(++std::declval<Iter&>())) -> StrideAdapter& {
  for (size_t i = 0; i < stride_ && current_ != end_; ++i) {
    ++current_;
  }
  return *this;
}

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
constexpr auto StrideAdapter<Iter>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<StrideAdapter> &&
    noexcept(++std::declval<StrideAdapter&>())) -> StrideAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter>
  requires StrideAdapterRequirements<Iter>
constexpr auto StrideAdapter<Iter>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<StrideAdapter> &&
             std::is_nothrow_copy_assignable_v<Iter>) -> StrideAdapter {
  auto result = *this;
  result.current_ = end_;
  return result;
}

/**
 * @brief Adapter that combines two ranges into pairs.
 * @details Iterates both ranges in parallel, yielding tuples of corresponding
 * elements. Stops when either range is exhausted.
 * @tparam Iter1 Type of the first iterator
 * @tparam Iter2 Type of the second iterator
 *
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
class ZipAdapter final
    : public FunctionalAdapterBase<ZipAdapter<Iter1, Iter2>> {
public:
  using iterator_concept = std::input_iterator_tag;
  using iterator_category = std::input_iterator_tag;
  using value_type =
      std::tuple<std::iter_value_t<Iter1>, std::iter_value_t<Iter2>>;
  using reference = value_type;
  using pointer = void;
  using difference_type = ptrdiff_t;

  /**
   * @brief Constructs a zip adapter.
   * @param begin1 Iterator to the beginning of the first range
   * @param end1 Iterator to the end of the first range
   * @param begin2 Iterator to the beginning of the second range
   * @param end2 Iterator to the end of the second range
   */
  constexpr ZipAdapter(
      Iter1 begin1, Iter1 end1, Iter2 begin2,
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

  template <ExternalRange R1, ExternalRange R2>
    requires ZipAdapterRequirements<std::ranges::iterator_t<R1>,
                                    std::ranges::iterator_t<R2>>
  constexpr ZipAdapter(R1& first_range, R2& second_range) noexcept(noexcept(
      ZipAdapter(std::ranges::begin(first_range), std::ranges::end(first_range),
                 std::ranges::begin(second_range),
                 std::ranges::end(second_range))))
      : ZipAdapter(
            std::ranges::begin(first_range), std::ranges::end(first_range),
            std::ranges::begin(second_range), std::ranges::end(second_range)) {}

  template <ExternalRange R1, ExternalRange R2>
    requires ZipAdapterRequirements<std::ranges::iterator_t<const R1>,
                                    std::ranges::iterator_t<const R2>>
  constexpr ZipAdapter(const R1& first_range, const R2& second_range) noexcept(
      noexcept(ZipAdapter(std::ranges::cbegin(first_range),
                          std::ranges::cend(first_range),
                          std::ranges::cbegin(second_range),
                          std::ranges::cend(second_range))))
      : ZipAdapter(std::ranges::cbegin(first_range),
                   std::ranges::cend(first_range),
                   std::ranges::cbegin(second_range),
                   std::ranges::cend(second_range)) {}

  constexpr ZipAdapter(const ZipAdapter&) noexcept(
      std::is_nothrow_copy_constructible_v<Iter1> &&
      std::is_nothrow_copy_constructible_v<Iter2>) = default;
  constexpr ZipAdapter(ZipAdapter&&) noexcept(
      std::is_nothrow_move_constructible_v<Iter1> &&
      std::is_nothrow_move_constructible_v<Iter2>) = default;
  constexpr ~ZipAdapter() noexcept(std::is_nothrow_destructible_v<Iter1> &&
                                   std::is_nothrow_destructible_v<Iter2>) =
      default;

  constexpr ZipAdapter& operator=(const ZipAdapter&) noexcept(
      std::is_nothrow_copy_assignable_v<Iter1> &&
      std::is_nothrow_copy_assignable_v<Iter2>) = default;
  constexpr ZipAdapter& operator=(ZipAdapter&&) noexcept(
      std::is_nothrow_move_assignable_v<Iter1> &&
      std::is_nothrow_move_assignable_v<Iter2>) = default;

  constexpr ZipAdapter& operator++() noexcept(
      noexcept(std::declval<Iter1&>() != std::declval<Iter1&>()) &&
      noexcept(++std::declval<Iter1&>()) &&
      noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()) &&
      noexcept(++std::declval<Iter2&>()));

  [[nodiscard]] constexpr ZipAdapter operator++(int) noexcept(
      std::is_nothrow_copy_constructible_v<ZipAdapter> &&
      noexcept(++std::declval<ZipAdapter&>()));

  [[nodiscard]] constexpr reference operator*() const {
    return std::make_tuple(*first_current_, *second_current_);
  }

  pointer operator->() const = delete;

  [[nodiscard]] constexpr bool operator==(const ZipAdapter& other) const
      noexcept(noexcept(std::declval<const Iter1&>() ==
                        std::declval<const Iter1&>()) &&
               noexcept(std::declval<const Iter2&>() ==
                        std::declval<const Iter2&>())) {
    return first_current_ == other.first_current_ ||
           second_current_ == other.second_current_;
  }

  [[nodiscard]] constexpr bool operator!=(const ZipAdapter& other) const
      noexcept(noexcept(std::declval<const ZipAdapter&>() ==
                        std::declval<const ZipAdapter&>())) {
    return !(*this == other);
  }

  [[nodiscard]] constexpr bool IsAtEnd() const noexcept(
      noexcept(std::declval<const Iter1&>() == std::declval<const Iter1&>()) &&
      noexcept(std::declval<const Iter2&>() == std::declval<const Iter2&>())) {
    return first_current_ == first_end_ || second_current_ == second_end_;
  }

  [[nodiscard]] constexpr ZipAdapter begin() const
      noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter>) {
    return *this;
  }

  [[nodiscard]] constexpr ZipAdapter end() const
      noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter>);

private:
  Iter1 first_begin_;
  Iter1 first_current_;
  Iter1 first_end_;
  Iter2 second_begin_;
  Iter2 second_current_;
  Iter2 second_end_;
};

template <ExternalRange R1, ExternalRange R2>
  requires ZipAdapterRequirements<std::ranges::iterator_t<R1>,
                                  std::ranges::iterator_t<R2>>
ZipAdapter(R1&, R2&)
    -> ZipAdapter<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>;

template <ExternalRange R1, ExternalRange R2>
  requires ZipAdapterRequirements<std::ranges::iterator_t<const R1>,
                                  std::ranges::iterator_t<const R2>>
ZipAdapter(const R1&, const R2&)
    -> ZipAdapter<std::ranges::iterator_t<const R1>,
                  std::ranges::iterator_t<const R2>>;

template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
constexpr auto ZipAdapter<Iter1, Iter2>::operator++() noexcept(
    noexcept(std::declval<Iter1&>() != std::declval<Iter1&>()) &&
    noexcept(++std::declval<Iter1&>()) &&
    noexcept(std::declval<Iter2&>() != std::declval<Iter2&>()) &&
    noexcept(++std::declval<Iter2&>())) -> ZipAdapter& {
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
constexpr auto ZipAdapter<Iter1, Iter2>::operator++(int) noexcept(
    std::is_nothrow_copy_constructible_v<ZipAdapter> &&
    noexcept(++std::declval<ZipAdapter&>())) -> ZipAdapter {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename Iter1, typename Iter2>
  requires ZipAdapterRequirements<Iter1, Iter2>
constexpr auto ZipAdapter<Iter1, Iter2>::end() const
    noexcept(std::is_nothrow_copy_constructible_v<ZipAdapter>) -> ZipAdapter {
  auto result = *this;
  result.first_current_ = result.first_end_;
  result.second_current_ = result.second_end_;
  return result;
}

/**
 * @brief CRTP base class providing common adapter operations.
 * @details Provides chaining methods (Filter, Map, Take, Skip, etc.) that can
 * be used by any derived adapter class. Uses CRTP pattern to return the correct
 * derived type.
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
      noexcept(noexcept(FilterAdapter<Derived, Pred>(GetDerived().begin(),
                                                     GetDerived().end(),
                                                     std::move(predicate)))) {
    return FilterAdapter<Derived, Pred>(
        GetDerived().begin(), GetDerived().end(), std::move(predicate));
  }

  /**
   * @brief Transforms each element using the given function.
   * @tparam Func Transformation function type
   * @param transform Function to apply to each element
   * @return MapAdapter that transforms adapted results
   */
  template <typename Func>
  [[nodiscard]] constexpr auto Map(Func transform) const
      noexcept(noexcept(MapAdapter<Derived, Func>(GetDerived().begin(),
                                                  GetDerived().end(),
                                                  std::move(transform)))) {
    return MapAdapter<Derived, Func>(GetDerived().begin(), GetDerived().end(),
                                     std::move(transform));
  }

  /**
   * @brief Limits the number of elements to at most count.
   * @param count Maximum number of elements to yield
   * @return TakeAdapter that limits adapted results
   */
  [[nodiscard]] constexpr auto Take(size_t count) const
      noexcept(noexcept(TakeAdapter<Derived>(GetDerived().begin(),
                                             GetDerived().end(), count))) {
    return TakeAdapter<Derived>(GetDerived().begin(), GetDerived().end(),
                                count);
  }

  /**
   * @brief Skips the first count elements.
   * @param count Number of elements to skip
   * @return SkipAdapter that skips adapted results
   */
  [[nodiscard]] constexpr auto Skip(size_t count) const
      noexcept(noexcept(SkipAdapter<Derived>(GetDerived().begin(),
                                             GetDerived().end(), count))) {
    return SkipAdapter<Derived>(GetDerived().begin(), GetDerived().end(),
                                count);
  }

  /**
   * @brief Takes elements while a predicate is true.
   * @tparam Pred Predicate type for take-while operation
   * @param predicate Predicate to determine when to stop taking
   * @return TakeWhileAdapter that conditionally takes elements
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto TakeWhile(Pred predicate) const
      noexcept(noexcept(TakeWhileAdapter<Derived, Pred>(
          GetDerived().begin(), GetDerived().end(), std::move(predicate)))) {
    return TakeWhileAdapter<Derived, Pred>(
        GetDerived().begin(), GetDerived().end(), std::move(predicate));
  }

  /**
   * @brief Skips elements while a predicate is true.
   * @tparam Pred Predicate type for skip-while operation
   * @param predicate Predicate to determine when to stop skipping
   * @return SkipWhileAdapter that conditionally skips elements
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto SkipWhile(Pred predicate) const
      noexcept(noexcept(SkipWhileAdapter<Derived, Pred>(
          GetDerived().begin(), GetDerived().end(), std::move(predicate)))) {
    return SkipWhileAdapter<Derived, Pred>(
        GetDerived().begin(), GetDerived().end(), std::move(predicate));
  }

  /**
   * @brief Adds an index to each element.
   * @return EnumerateAdapter that pairs indices with values
   */
  [[nodiscard]] constexpr auto Enumerate() const
      noexcept(noexcept(EnumerateAdapter<Derived>(GetDerived().begin(),
                                                  GetDerived().end()))) {
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
      noexcept(noexcept(InspectAdapter<Derived, Func>(GetDerived().begin(),
                                                      GetDerived().end(),
                                                      std::move(inspector)))) {
    return InspectAdapter<Derived, Func>(
        GetDerived().begin(), GetDerived().end(), std::move(inspector));
  }

  /**
   * @brief Takes every Nth element.
   * @param step Step size between elements
   * @return StepByAdapter that skips elements
   */
  [[nodiscard]] constexpr auto StepBy(size_t step) const
      noexcept(noexcept(StepByAdapter<Derived>(GetDerived().begin(),
                                               GetDerived().end(), step))) {
    return StepByAdapter<Derived>(GetDerived().begin(), GetDerived().end(),
                                  step);
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
      noexcept(noexcept(ChainAdapter<Derived, OtherIter>(GetDerived().begin(),
                                                         GetDerived().end(),
                                                         std::move(begin),
                                                         std::move(end)))) {
    return ChainAdapter<Derived, OtherIter>(GetDerived().begin(),
                                            GetDerived().end(),
                                            std::move(begin), std::move(end));
  }

  /**
   * @brief Chains another range after this one.
   * @tparam R Range type of the other adapter
   * @param range The other range to chain
   * @return ChainAdapter that yields elements from both ranges
   */
  template <typename R>
  [[nodiscard]] constexpr auto Chain(R& range) const
      noexcept(noexcept(ChainAdapter<Derived, std::ranges::iterator_t<R>>(
          GetDerived().begin(), GetDerived().end(), range))) {
    return ChainAdapter<Derived, std::ranges::iterator_t<R>>(
        GetDerived().begin(), GetDerived().end(), range);
  }

  /**
   * @brief Chains another range after this one.
   * @tparam R Range type of the other adapter
   * @param range The other range to chain
   * @return ChainAdapter that yields elements from both ranges
   */
  template <typename R>
  [[nodiscard]] constexpr auto Chain(const R& range) const
      noexcept(noexcept(ChainAdapter<Derived, std::ranges::iterator_t<const R>>(
          GetDerived().begin(), GetDerived().end(), range))) {
    return ChainAdapter<Derived, std::ranges::iterator_t<const R>>(
        GetDerived().begin(), GetDerived().end(), range);
  }

  /**
   * @brief Reverses the order of elements.
   * @note Requires bidirectional iterator support
   * @return ReverseAdapter that yields elements in reverse order
   */
  [[nodiscard]] constexpr auto Reverse() const
      noexcept(noexcept(ReverseAdapter<Derived>(GetDerived().begin(),
                                                GetDerived().end()))) {
    return ReverseAdapter<Derived>(GetDerived().begin(), GetDerived().end());
  }

  /**
   * @brief Creates sliding windows over elements.
   * @param window_size Size of the sliding window
   * @return SlideAdapter that yields windows of elements
   * @warning window_size must be greater than 0
   */
  [[nodiscard]] constexpr auto Slide(size_t window_size) const
      noexcept(noexcept(SlideAdapter<Derived>(GetDerived().begin(),
                                              GetDerived().end(),
                                              window_size))) {
    return SlideAdapter<Derived>(GetDerived().begin(), GetDerived().end(),
                                 window_size);
  }

  /**
   * @brief Takes every Nth element with stride.
   * @param stride Number of elements to skip between yields
   * @return StrideAdapter that yields every Nth element
   * @warning stride must be greater than 0
   */
  [[nodiscard]] constexpr auto Stride(size_t stride) const
      noexcept(noexcept(StrideAdapter<Derived>(GetDerived().begin(),
                                               GetDerived().end(), stride))) {
    return StrideAdapter<Derived>(GetDerived().begin(), GetDerived().end(),
                                  stride);
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
      noexcept(noexcept(ZipAdapter<Derived, OtherIter>(GetDerived().begin(),
                                                       GetDerived().end(),
                                                       std::move(begin),
                                                       std::move(end)))) {
    return ZipAdapter<Derived, OtherIter>(GetDerived().begin(),
                                          GetDerived().end(), std::move(begin),
                                          std::move((end)));
  }

  /**
   * @brief Zips another range with this one.
   * @tparam R Other range
   * @param range The other range to zip with
   * @return ZipAdapter that
   */
  template <typename R>
  [[nodiscard]] constexpr auto Zip(R& range) const
      noexcept(noexcept(ZipAdapter<Derived, std::ranges::iterator_t<R>>(
          GetDerived().begin(), GetDerived().end(), range))) {
    return ZipAdapter<Derived, std::ranges::iterator_t<R>>(
        GetDerived().begin(), GetDerived().end(), range);
  }

  /**
   * @brief Zips another range with this one.
   * @tparam R Other range
   * @param range The other range to zip with
   * @return ZipAdapter that
   */
  template <typename R>
  [[nodiscard]] constexpr auto Zip(const R& range) const
      noexcept(noexcept(ZipAdapter<Derived, std::ranges::iterator_t<const R>>(
          GetDerived().begin(), GetDerived().end(), range))) {
    return ZipAdapter<Derived, std::ranges::iterator_t<const R>>(
        GetDerived().begin(), GetDerived().end(), range);
  }

  /**
   * @brief Terminal operation: applies an action to each element.
   * @tparam Action Function type that processes each element
   * @param action Function to apply to each element
   */
  template <typename Action>
  constexpr void ForEach(const Action& action) const;

  /**
   * @brief Terminal operation: reduces elements to a single value using a
   * folder function.
   * @tparam T Accumulator type
   * @tparam Folder Function type that combines accumulator with each element
   * @param init Initial accumulator value
   * @param folder Function to fold elements
   * @return Final accumulated value
   */
  template <typename T, typename Folder>
  [[nodiscard]] constexpr T Fold(T init, const Folder& folder) const;

  /**
   * @brief Terminal operation: finds the first element satisfying a predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return Optional or pointer to the element with maximum key, based on the
   * iterator type
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
   * @brief Terminal operation: partitions elements into two groups based on a
   * predicate.
   * @tparam Pred Predicate type
   * @param predicate Function to test elements
   * @return Pair of vectors: first contains elements satisfying predicate,
   * second contains the rest
   */
  template <typename Pred>
  [[nodiscard]] constexpr auto Partition(const Pred& predicate) const;

  /**
   * @brief Terminal operation: partitions elements with a custom allocator.
   * @tparam Pred Predicate type
   * @tparam Allocator Allocator for result vectors
   * @param predicate Function to test elements
   * @param allocator Allocator to use for both result vectors
   * @return Pair of vectors using the provided allocator
   */
  template <typename Pred, typename Allocator>
    requires std::same_as<typename Allocator::value_type,
                          std::iter_value_t<Derived>>
  [[nodiscard]] constexpr auto PartitionWith(const Pred& predicate,
                                             Allocator allocator) const;

  /**
   * @brief Terminal operation: finds the element with the maximum value
   * according to a key function.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key from each element
   * @return Optional or pointer to the element with maximum key, based on the
   * iterator type
   */
  template <typename KeyFunc>
  [[nodiscard]] constexpr auto MaxBy(const KeyFunc& key_func) const;

  /**
   * @brief Terminal operation: finds the element with the minimum value
   * according to a key function.
   * @tparam KeyFunc Key extraction function type
   * @param key_func Function to extract comparison key from each element
   * @return Optional or pointer to the element with maximum key, based on the
   * iterator type
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
   * @brief Terminal operation: groups elements by key with custom allocators.
   * @tparam KeyFunc Key extraction function type
   * @tparam MapAllocator Allocator for unordered_map nodes
   * @tparam ValueAllocator Allocator for grouped vectors
   * @param key_func Function to extract grouping key from each element
   * @param map_allocator Allocator for map storage
   * @param value_allocator Allocator for each grouped vector
   * @return Map from keys to vectors using provided allocators
   */
  template <typename KeyFunc, typename MapAllocator, typename ValueAllocator>
  [[nodiscard]] constexpr auto GroupByWith(
      const KeyFunc& key_func, MapAllocator map_allocator,
      ValueAllocator value_allocator) const;

  /**
   * @brief Terminal operation: collects all elements into a vector.
   * @return Vector containing all elements
   */
  [[nodiscard]] constexpr auto Collect() const;

  /**
   * @brief Terminal operation: collects all elements into a vector with a
   * custom allocator.
   * @tparam Allocator Allocator type
   * @param allocator Allocator to use for the vector
   * @return Vector containing all elements
   */
  template <typename Allocator>
  [[nodiscard]] constexpr auto CollectWith(Allocator allocator = {}) const;

  /**
   * @brief Terminal operation: collects all elements into a vector with a
   * specific memory resource.
   * @param resource Memory resource to use for the vector
   * @return Vector containing all elements
   */
  [[nodiscard]] constexpr auto CollectWith(
      std::pmr::memory_resource* resource) const;

  auto CollectWith(std::nullptr_t) const = delete;

  /**
   * @brief Terminal operation: writes all elements into an output iterator.
   * @details Consumes the adapter and writes each element to the provided
   * output iterator. This is more efficient than `Collect()` when you already
   * have a destination container.
   * @tparam OutIt Output iterator type
   * @param out Output iterator to write elements into
   *
   * @code
   * std::vector<int> results;
   * query.Filter([](int x) { return x > 5;
   * }).Into(std::back_inserter(results));
   * @endcode
   */
  template <typename OutIt>
  constexpr void Into(OutIt out) const;

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

protected:
  /**
   * @brief Gets reference to derived class instance.
   * @return Reference to derived class
   */
  [[nodiscard]] constexpr Derived& GetDerived() noexcept {
    return static_cast<Derived&>(*this);
  }

  /**
   * @brief Gets const reference to derived class instance.
   * @return Const reference to derived class
   */
  [[nodiscard]] constexpr const Derived& GetDerived() const noexcept {
    return static_cast<const Derived&>(*this);
  }
};

template <typename Derived>
template <typename Action>
constexpr void FunctionalAdapterBase<Derived>::ForEach(
    const Action& action) const {
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
constexpr T FunctionalAdapterBase<Derived>::Fold(T init,
                                                 const Folder& folder) const {
  for (auto&& value : GetDerived()) {
    if constexpr (std::invocable<Folder, T&&, decltype(value)>) {
      init = folder(std::move(init), std::forward<decltype(value)>(value));
    } else {
      init = std::apply(
          [&init, &folder](auto&&... args) {
            return folder(std::move(init),
                          std::forward<decltype(args)>(args)...);
          },
          std::forward<decltype(value)>(value));
    }
  }
  return init;
}

template <typename Derived>
template <typename Pred>
constexpr auto FunctionalAdapterBase<Derived>::Find(
    const Pred& predicate) const {
  using IterType = decltype(GetDerived().begin());
  using Result = details::find_result_t<IterType>;

  for (auto&& value : GetDerived()) {
    bool result = false;
    if constexpr (std::invocable<Pred, decltype(value)>) {
      result = predicate(value);
    } else {
      result = std::apply(predicate, value);
    }
    if (result) {
      if constexpr (details::IterYieldsReference<IterType>) {
        return Result{&value};
      } else {
        return Result{value};
      }
    }
  }

  if constexpr (details::IterYieldsReference<IterType>) {
    return Result{nullptr};
  } else {
    return Result{std::nullopt};
  }
}

template <typename Derived>
template <typename Pred>
constexpr size_t FunctionalAdapterBase<Derived>::CountIf(
    const Pred& predicate) const {
  size_t count = 0;
  for (const auto& value : GetDerived()) {
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
template <typename Pred>
constexpr auto FunctionalAdapterBase<Derived>::Partition(
    const Pred& predicate) const {
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
template <typename Pred, typename Allocator>
  requires std::same_as<typename Allocator::value_type,
                        std::iter_value_t<Derived>>
constexpr auto FunctionalAdapterBase<Derived>::PartitionWith(
    const Pred& predicate, Allocator allocator) const {
  using ValueType = std::iter_value_t<Derived>;
  std::vector<ValueType, Allocator> matched(allocator);
  std::vector<ValueType, Allocator> not_matched(std::move(allocator));

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
constexpr auto FunctionalAdapterBase<Derived>::MaxBy(
    const KeyFunc& key_func) const {
  using IterType = decltype(GetDerived().begin());
  using Result = details::find_result_t<IterType>;

  auto iter = GetDerived().begin();
  const auto end_iter = GetDerived().end();

  if (iter == end_iter) {
    if constexpr (details::IterYieldsReference<IterType>) {
      return Result{nullptr};
    } else {
      return Result{std::nullopt};
    }
  }

  auto get_key = [&key_func](auto&& val) {
    if constexpr (std::invocable<KeyFunc, decltype(val)>) {
      return key_func(std::forward<decltype(val)>(val));
    } else {
      return std::apply(key_func, std::forward<decltype(val)>(val));
    }
  };

  Result max_element;
  if constexpr (details::IterYieldsReference<IterType>) {
    max_element = &(*iter);
  } else {
    max_element.emplace(*iter);
  }

  auto max_key = get_key(*max_element);
  ++iter;

  while (iter != end_iter) {
    auto current_key = get_key(*iter);
    if (current_key > max_key) {
      max_key = std::move(current_key);
      if constexpr (details::IterYieldsReference<IterType>) {
        max_element = &(*iter);
      } else {
        max_element.emplace(*iter);
      }
    }
    ++iter;
  }
  return max_element;
}

template <typename Derived>
template <typename KeyFunc>
constexpr auto FunctionalAdapterBase<Derived>::MinBy(
    const KeyFunc& key_func) const {
  using IterType = decltype(GetDerived().begin());
  using Result = details::find_result_t<IterType>;

  auto iter = GetDerived().begin();
  const auto end_iter = GetDerived().end();

  if (iter == end_iter) {
    if constexpr (details::IterYieldsReference<IterType>) {
      return Result{nullptr};
    } else {
      return Result{std::nullopt};
    }
  }

  auto get_key = [&key_func](auto&& val) {
    if constexpr (std::invocable<KeyFunc, decltype(val)>) {
      return key_func(std::forward<decltype(val)>(val));
    } else {
      return std::apply(key_func, std::forward<decltype(val)>(val));
    }
  };

  Result min_element;
  if constexpr (details::IterYieldsReference<IterType>) {
    min_element = &(*iter);
  } else {
    min_element.emplace(*iter);
  }

  auto min_key = get_key(*min_element);
  ++iter;

  while (iter != end_iter) {
    auto current_key = get_key(*iter);
    if (current_key < min_key) {
      min_key = std::move(current_key);
      if constexpr (details::IterYieldsReference<IterType>) {
        min_element = &(*iter);
      } else {
        min_element.emplace(*iter);
      }
    }
    ++iter;
  }

  return min_element;
}

template <typename Derived>
template <typename KeyFunc>
constexpr auto FunctionalAdapterBase<Derived>::GroupBy(
    const KeyFunc& key_func) const {
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

template <typename Derived>
template <typename KeyFunc, typename MapAllocator, typename ValueAllocator>
constexpr auto FunctionalAdapterBase<Derived>::GroupByWith(
    const KeyFunc& key_func, MapAllocator map_allocator,
    ValueAllocator value_allocator) const {
  using ValueType = std::iter_value_t<Derived>;
  using KeyType =
      std::decay_t<details::call_or_apply_result_t<const KeyFunc&, ValueType>>;
  using GroupVector = std::vector<ValueType, ValueAllocator>;
  using MapValueType = std::pair<const KeyType, GroupVector>;
  static_assert(std::same_as<typename MapAllocator::value_type, MapValueType>,
                "Map allocator value_type must match map value type");
  static_assert(std::same_as<typename ValueAllocator::value_type, ValueType>,
                "Value allocator value_type must match grouped element type");

  std::unordered_map<KeyType, GroupVector, std::hash<KeyType>, std::equal_to<>,
                     MapAllocator>
      groups(0, std::hash<KeyType>{}, std::equal_to<>{},
             std::move(map_allocator));

  for (auto&& value : GetDerived()) {
    KeyType key;
    if constexpr (std::invocable<KeyFunc, decltype(value)>) {
      key = key_func(std::forward<decltype(value)>(value));
    } else {
      key = std::apply(key_func, std::forward<decltype(value)>(value));
    }

    auto [iter, inserted] = groups.try_emplace(std::move(key));
    if (inserted) {
      iter->second = GroupVector(value_allocator);
    }
    iter->second.push_back(std::forward<decltype(value)>(value));
  }

  return groups;
}

template <typename Derived>
constexpr auto FunctionalAdapterBase<Derived>::Collect() const {
  using ValueType = std::iter_value_t<Derived>;
  std::vector<ValueType> result;
  result.reserve(static_cast<size_t>(
      std::distance(GetDerived().begin(), GetDerived().end())));
  for (auto&& value : GetDerived()) {
    result.push_back(std::forward<decltype(value)>(value));
  }
  return result;
}

template <typename Derived>
template <typename Allocator>
constexpr auto FunctionalAdapterBase<Derived>::CollectWith(
    Allocator allocator) const {
  using ValueType = std::iter_value_t<Derived>;
  std::vector<ValueType, Allocator> result{std::move(allocator)};
  result.reserve(static_cast<size_t>(
      std::distance(GetDerived().begin(), GetDerived().end())));
  for (auto&& value : GetDerived()) {
    result.push_back(std::forward<decltype(value)>(value));
  }
  return result;
}

template <typename Derived>
constexpr auto FunctionalAdapterBase<Derived>::CollectWith(
    std::pmr::memory_resource* resource) const {
  using ValueType = std::iter_value_t<Derived>;
  return CollectWith(std::pmr::polymorphic_allocator<ValueType>{resource});
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
constexpr bool FunctionalAdapterBase<Derived>::Any(
    const Pred& predicate) const {
  for (const auto& value : GetDerived()) {
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
constexpr bool FunctionalAdapterBase<Derived>::All(
    const Pred& predicate) const {
  for (const auto& value : GetDerived()) {
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

}  // namespace helios::utils
