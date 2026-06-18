#pragma once

#include <helios/ecs/component/component.hpp>
#include <helios/utils/common_traits.hpp>

#include <concepts>
#include <type_traits>

namespace helios::ecs {

class World;

namespace details {

/**
 * @brief Extracts the raw component type from any access specifier.
 * @details Handles all forms:
 * - `T`, `T&`, `const T&`, `T&&` -> `T`
 * - `T*`, `const T*`             -> `T`
 */
template <typename T>
struct ComponentTypeExtractor {
  using type = std::remove_cvref_t<T>;
};

template <typename T>
struct ComponentTypeExtractor<T*> {
  using type = std::remove_cv_t<T>;
};

template <typename T>
struct ComponentTypeExtractor<const T*> {
  using type = T;
};

template <typename T>
using ComponentTypeExtractor_t =
    typename ComponentTypeExtractor<std::remove_cvref_t<T>>::type;

/**
 * @brief Checks if an access specifier is a nullable pointer (optional
 * component access).
 * @details Returns true for `T*` and `const T*`. These map to components that
 * may be absent — the iterator yields `nullptr` for entities that don't have
 * them.
 */
template <typename T>
struct IsPointerAccess : std::false_type {};

template <typename T>
struct IsPointerAccess<T*> : std::true_type {};

template <typename T>
struct IsPointerAccess<const T*> : std::true_type {};

template <typename T>
inline constexpr bool kIsPointerAccess =
    IsPointerAccess<std::remove_cvref_t<T>>::value;

/**
 * @brief Checks if a component access specifier is const-qualified.
 * @details Returns true for:
 * - `const T&`, `const T`
 * - `const T*`, `T*` (pointer-to-non-const is still read-only at the query
 *   scheduling level — mutable pointer access is expressed via `T*` from a
 *   non-const world)
 * - Plain `T` (value copy is non-mutating)
 *
 * Returns false for:
 * - `T&`, `T&&`
 * - `T*` (mutable pointer — requires non-const world)
 */
template <typename T>
struct IsConstAccess : std::false_type {};

// const T& -> const
template <typename T>
struct IsConstAccess<const T&> : std::true_type {};

// const T -> const
template <typename T>
  requires(!std::is_reference_v<T> && std::is_const_v<T>)
struct IsConstAccess<T> : std::true_type {};

// T (plain value, no ref, no const, no pointer) -> const (copy = read-only)
template <typename T>
  requires(!std::is_reference_v<T> && !std::is_const_v<T> &&
           !std::is_pointer_v<T>)
struct IsConstAccess<T> : std::true_type {};

// const T* -> const
template <typename T>
struct IsConstAccess<const T*> : std::true_type {};

// T* -> mutable (requires non-const world)
template <typename T>
struct IsConstAccess<T*> : std::false_type {};

template <typename T>
inline constexpr bool kIsConstAccess = IsConstAccess<T>::value;

/**
 * @brief Checks if a component access specifier is mutable (requires write
 * access).
 */
template <typename T>
inline constexpr bool kIsMutableAccess = !kIsConstAccess<T>;

/**
 * @brief Checks if a component access specifier requests rvalue-reference
 * (move) semantics.
 */
template <typename T>
struct IsRvalueAccess : std::false_type {};

template <typename T>
struct IsRvalueAccess<T&&> : std::true_type {};

template <typename T>
inline constexpr bool kIsRvalueAccess = IsRvalueAccess<T>::value;

/// @brief Checks if all component access specifiers are const-qualified.
template <typename... Components>
inline constexpr bool kAllComponentsConst = (kIsConstAccess<Components> && ...);

/**
 * @brief Computes the actual type returned when iterating a query for a given
 * access specifier.
 * @details Mapping:
 * - `T&`       -> `T&`
 * - `const T&` -> `const T&`
 * - `T&&`      -> `T&&`
 * - `T`        -> `T`        (value copy)
 * - `const T`  -> `T`        (value copy, top-level const stripped)
 * - `T*`       -> `T*`       (nullable mutable pointer)
 * - `const T*` -> `const T*` (nullable const pointer)
 */
template <typename T>
struct ComponentAccessType {
  // Default: value copy
  using type = std::remove_cv_t<T>;
};

template <typename T>
  requires(!std::is_const_v<T>)
struct ComponentAccessType<T&> {
  using type = T&;
};

template <typename T>
struct ComponentAccessType<const T&> {
  using type = const T&;
};

template <typename T>
struct ComponentAccessType<T&&> {
  using type = T&&;
};

template <typename T>
struct ComponentAccessType<T*> {
  using type = T*;
};

template <typename T>
struct ComponentAccessType<const T*> {
  using type = const T*;
};

template <typename T>
using ComponentAccessType_t = typename ComponentAccessType<T>::type;

/**
 * @brief Checks if a world type is const-qualified.
 */
template <typename WorldT>
inline constexpr bool kIsConstWorld =
    std::is_const_v<std::remove_reference_t<WorldT>>;

/**
 * @brief Concept ensuring that when the world is const, only const (read-only)
 * access is requested.
 */
template <typename WorldT, typename... Components>
concept ValidWorldComponentAccess =
    !kIsConstWorld<WorldT> || kAllComponentsConst<Components...>;

/**
 * @brief Concept for valid component access specifiers in a query.
 * @details Each specifier must have a raw component type satisfying
 * `ComponentTrait`. Accepted forms: `T`, `T&`, `const T&`, `T&&`, `T*`,
 * `const T*`.
 */
template <typename T>
concept ValidComponentAccess = ComponentTrait<ComponentTypeExtractor_t<T>>;

/// @brief Concept for STL-compatible allocators.
template <typename Alloc, typename T>
concept AllocatorFor = requires(Alloc alloc, size_t n) {
  { alloc.allocate(n) } -> std::same_as<T*>;
  { alloc.deallocate(std::declval<T*>(), n) };
  typename Alloc::value_type;
  requires std::same_as<typename Alloc::value_type, T>;
};

/**
 * @brief Checks if a component access specifier refers to a tag (empty)
 * component.
 */
template <typename T>
inline constexpr bool kIsTagAccess =
    TagComponentTrait<ComponentTypeExtractor_t<T>>;

/**
 * @brief Gets the ComponentTypeIndex for an access specifier.
 * @return The component type index for the access specifier
 */
template <typename T>
[[nodiscard]] consteval auto ComponentTypeIndexFromAccess() noexcept
    -> ComponentTypeIndex {
  return ComponentTypeIndex::From<ComponentTypeExtractor_t<T>>();
}

/**
 * @brief Checks that all access specifiers refer to unique underlying component
 * types.
 */
template <typename... Components>
concept UniqueComponentAccess =
    utils::UniqueTypes<ComponentTypeExtractor_t<Components>...>;

}  // namespace details

}  // namespace helios::ecs
