#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <atomic>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief Concept for valid resource types.
 * @details Resources must be object types that can be stored and accessed.
 */
template <typename T>
concept ResourceTrait = std::is_object_v<std::remove_cvref_t<T>> && std::is_destructible_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for resources that provide custom names.
 * @details A resource with name trait must satisfy ResourceTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 */
template <typename T>
concept ResourceWithNameTrait = ResourceTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Concept for resources that provide thread-safety trait.
 * @details A resource with thread-safety trait must satisfy ResourceTrait and provide:
 * - `static constexpr bool ThreadSafe() noexcept`
 */
template <typename T>
concept ResourceWithThreadSafetyTrait = ResourceTrait<T> && requires {
  { T::ThreadSafe() } -> std::same_as<bool>;
};

/**
 * @brief Concept for atomic resources that can be accessed concurrently.
 * @details Atomic resources don't affect scheduling and are thread-safe.
 */
template <typename T>
concept AtomicResourceTrait = ResourceTrait<T> && requires { std::atomic<T>{}; };

/**
 * @brief Type ID for resources.
 */
using ResourceTypeId = size_t;

/**
 * @brief Gets type ID for a resource type.
 * @tparam T Resource type
 * @return Unique type ID for the resource
 */
template <ResourceTrait T>
constexpr ResourceTypeId ResourceTypeIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets name for a resource type.
 * @tparam T Resource type
 * @return Name of the resource
 */
template <ResourceTrait T>
[[nodiscard]] constexpr std::string_view ResourceNameOf() noexcept {
  if constexpr (ResourceWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

/**
 * @brief Checks if a resource type is thread-safe.
 * @tparam T Resource type
 * @return True if resource is thread-safe, false otherwise
 */
template <ResourceTrait T>
[[nodiscard]] constexpr bool IsResourceThreadSafe() noexcept {
  if constexpr (ResourceWithThreadSafetyTrait<T>) {
    return T::ThreadSafe();
  } else {
    return false;
  }
}

}  // namespace helios::ecs
