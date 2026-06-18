#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <type_traits>

namespace helios::utils {

namespace details {

// Helper: recursively check uniqueness after removing cv/ref.
// Primary: empty pack => true
template <typename...>
struct UniqueTypesHelper : std::true_type {};

// Recursive case
template <typename T, typename... Rest>
struct UniqueTypesHelper<T, Rest...>
    : std::bool_constant<
          // T must differ from every type in Rest (after remove_cvref)
          (!(std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Rest>> ||
             ...))
          // and Rest themselves must be unique
          && UniqueTypesHelper<Rest...>::value> {};

}  // namespace details

/**
 * @brief Concept for arithmetic types (integral or floating-point).
 */
template <typename T>
concept ArithmeticTrait = std::integral<T> || std::floating_point<T>;

/**
 * @brief Concept for lambda closure types.
 * @details Detects lambdas via compiler-specific type-name heuristics (or
 * C++26 reflection when available).
 */
template <typename T>
concept LambdaTrait = details::IsLambdaType<T>();

/**
 * @brief Concept for regular functor types (named structs/classes that are not
 * lambda closures).
 */
template <typename T>
concept FunctorTrait = std::is_class_v<T> && !LambdaTrait<T>;

/**
 * @brief Concept that checks if all types in a pack are unique (after removing
 * cv/ref qualifiers).
 */
template <typename... Ts>
concept UniqueTypes = details::UniqueTypesHelper<Ts...>::value;

/**
 * @brief Concept to allow argument conversion with polymorphic relationships.
 * @details Allows regular convertibility or base/derived relationship for
 * polymorphic types. This is useful for type-erased containers and delegates to
 * provide flexible argument handling.
 *
 * The concept is satisfied when:
 * - From is convertible to To (standard conversion), OR
 * - Both From and To are polymorphic types and one derives from the other
 *
 * @tparam From Source type.
 * @tparam To Target type.
 */
template <typename From, typename To>
concept PolymorphicConvertible =
    std::convertible_to<From, To> ||
    (std::is_polymorphic_v<std::remove_reference_t<From>> &&
     std::is_polymorphic_v<std::remove_reference_t<To>> &&
     (std::derived_from<std::remove_cvref_t<From>, std::remove_cvref_t<To>> ||
      std::derived_from<std::remove_cvref_t<To>, std::remove_cvref_t<From>>));

}  // namespace helios::utils
