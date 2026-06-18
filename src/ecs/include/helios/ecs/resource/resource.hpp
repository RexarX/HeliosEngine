#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

class World;

/// @brief Type index for resources.
using ResourceTypeIndex = utils::TypeIndex;

/// @brief Type id for resources.
using ResourceTypeId = utils::TypeId;

/**
 * @brief Concept for valid resource types.
 * @details A resource must be destructible, be an object, move or copy
 * constructible, and not polymorphic.
 */
template <typename T>
concept ResourceTrait =
    std::destructible<T> &&
    (std::move_constructible<T> || std::copy_constructible<T>) &&
    std::is_object_v<std::remove_cvref_t<T>> &&
    !std::is_polymorphic_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for resources that provide custom names.
 * @details A resource with name trait must satisfy `ResourceTrait` and provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept ResourceWithNameTrait = ResourceTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Concept for resources that provide thread-safety trait.
 * @details A resource with thread-safety trait must satisfy `ResourceTrait` and
 * provide:
 * - `static constexpr bool kThreadSafe` variable
 */
template <typename T>
concept ResourceWithThreadSafetyTrait = ResourceTrait<T> && requires {
  { std::remove_cvref_t<T>::kThreadSafe } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for async resources.
 * @details An async resource must satisfy `ResourceWithThreadSafetyTrait` and
 * provide:
 * - `static constexpr bool kThreadSafe = true` variable
 */
template <typename T>
concept AsyncResourceTrait = ResourceWithThreadSafetyTrait<T> && requires {
  requires std::remove_cvref_t<T>::kThreadSafe;
};

/**
 * @brief Concept for resources that provide insertion callback.
 * @details A resource with insertion callback must satisfy `ResourceTrait` and
 * provide:
 * - `static void OnInsert(World& world)` function
 */
template <typename T>
concept ResourceWithInsertionCallbackTrait =
    ResourceTrait<T> && requires(T& resource, World& world) {
      { resource.OnInsert(world) } -> std::same_as<void>;
    };

/**
 * @brief Concept for resources that provide removal callback.
 * @details A resource with removal callback must satisfy `ResourceTrait` and
 * provide:
 * - `static void OnRemove(World& world)` function
 */
template <typename T>
concept ResourceWithRemovalCallbackTrait =
    ResourceTrait<T> && requires(T& resource, World& world) {
      { resource.OnRemove(world) } -> std::same_as<void>;
    };

/**
 * @brief Gets name for a resource type.
 * @tparam T Resource type
 * @return Name of the resource
 */
template <ResourceTrait T>
[[nodiscard]] constexpr std::string_view ResourceNameOf() noexcept {
  if constexpr (ResourceWithNameTrait<T>) {
    return T::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

/**
 * @brief Gets name for a resource type.
 * @tparam T Resource type
 * @param resource Resource instance
 * @return Name of the resource
 */
template <ResourceTrait T>
[[nodiscard]] constexpr std::string_view ResourceNameOf(
    const T& /*resource*/) noexcept {
  return ResourceNameOf<std::remove_cvref_t<T>>();
}

/**
 * @brief Checks if a resource type is thread-safe.
 * @tparam T Resource type
 * @return True if resource is thread-safe, false otherwise
 */
template <ResourceTrait T>
[[nodiscard]] consteval bool IsResourceThreadSafe() noexcept {
  return AsyncResourceTrait<T>;
}

/**
 * @brief Checks if a resource type is thread-safe.
 * @tparam T Resource type
 * @param resource Resource instance
 * @return True if resource is thread-safe, false otherwise
 */
template <ResourceTrait T>
[[nodiscard]] consteval bool IsResourceThreadSafe(
    const T& /*resource*/) noexcept {
  return IsResourceThreadSafe<std::remove_cvref_t<T>>();
}

/**
 * @brief Calls insertion callback for a resource type if it exists.
 * @tparam T Resource type
 * @param resource Resource instance
 * @param world Reference to the world
 */
template <ResourceTrait T>
constexpr void ResourceCallOnInsert(T& resource, World& world) noexcept {
  if constexpr (ResourceWithInsertionCallbackTrait<T>) {
    resource.OnInsert(world);
  }
}

/**
 * @brief Calls removal callback for a resource type if it exists.
 * @tparam T Resource type
 * @param resource Resource instance
 * @param world Reference to the world
 */
template <ResourceTrait T>
constexpr void ResourceCallOnRemove(T& resource, World& world) noexcept {
  if constexpr (ResourceWithRemovalCallbackTrait<T>) {
    resource.OnRemove(world);
  }
}

}  // namespace helios::ecs
