#pragma once

#include <helios/core_pch.hpp>

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
struct UniqueTypesHelper<T, Rest...> : std::bool_constant<
                                           // T must differ from every type in Rest (after remove_cvref)
                                           (!(std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Rest>> || ...))
                                           // and Rest themselves must be unique
                                           && UniqueTypesHelper<Rest...>::value> {};

}  // namespace details

template <typename T>
concept ArithmeticTrait = std::integral<T> || std::floating_point<T>;

template <typename... Ts>
concept UniqueTypes = details::UniqueTypesHelper<Ts...>::value;

}  // namespace helios::utils
