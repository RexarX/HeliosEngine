#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

template <typename... Ts>
class ComponentBundle;

namespace details {

template <typename T>
struct IsComponentBundle : std::false_type {};

template <typename... Ts>
struct IsComponentBundle<ComponentBundle<Ts...>> : std::true_type {};

}  // namespace details

/// @brief Type index for components.
using ComponentTypeIndex = utils::TypeIndex;

/// @brief Type id for components.
using ComponentTypeId = utils::TypeId;

/// @brief Storage type for components.
enum class ComponentStorageType : uint8_t {
  kArchetype,  ///< Component is stored in an archetype.
  kSparseSet,  ///< Component is stored in a sparse set.
};

/**
 * @brief Concept to check if a type can be used as a component.
 * @details A component must be destructible, move or copy constructible, be an
 * object, and not polymorphic.
 */
template <typename T>
concept ComponentTrait =
    !details::IsComponentBundle<std::remove_cvref_t<T>>::value &&
    std::destructible<T> &&
    (std::move_constructible<T> || std::copy_constructible<T>) &&
    !std::is_polymorphic_v<std::remove_cvref_t<T>> &&
    std::is_object_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept to check if the type is considered a tag component.
 * @details A tag component is a component that is empty and can be used for
 * tagging entities.
 */
template <typename T>
concept TagComponentTrait =
    ComponentTrait<T> && std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for components that provide a name.
 * @details A component with name trait must satisfy `ComponentTrait` and
 * provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept ComponentWithNameTrait = ComponentTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Concept for components that provide a storage type.
 * @details A component with name trait must satisfy `ComponentTrait` and
 * provide:
 * - `static constexpr ComponentStorageType kStorageType` variable
 */
template <typename T>
concept ComponentWithStorageTypeTrait = ComponentTrait<T> && requires {
  {
    std::remove_cvref_t<T>::kStorageType
  } -> std::convertible_to<ComponentStorageType>;
};

/**
 * @brief Component name for debugging and serialization.
 * @tparam T Component type
 * @return Component name
 */
template <ComponentTrait T>
[[nodiscard]] constexpr std::string_view ComponentNameOf() noexcept {
  if constexpr (ComponentWithNameTrait<T>) {
    return T::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

/**
 * @brief Component name for debugging and serialization.
 * @tparam T Component type
 * @param instance Component instance
 * @return Component name
 */
template <ComponentTrait T>
[[nodiscard]] constexpr std::string_view ComponentNameOf(
    const T& /*instance*/) noexcept {
  return ComponentNameOf<std::remove_cvref_t<T>>();
}

/**
 * @brief Component storage type.
 * @details If component does not provide a storage type, it defaults to:
 * - `ComponentStorageType::kSparseSet` for tag components
 * - `ComponentStorageType::kArchetype` for non-tag components
 * @tparam T Component type
 * @return Component storage type
 */
template <ComponentTrait T>
[[nodiscard]] consteval ComponentStorageType ComponentStorageTypeOf() noexcept {
  if constexpr (ComponentWithStorageTypeTrait<T>) {
    return T::kStorageType;
  } else {
    if constexpr (TagComponentTrait<T>) {
      return ComponentStorageType::kSparseSet;
    } else {
      return ComponentStorageType::kArchetype;
    }
  }
}

/**
 * @brief Component storage type.
 * @details If component does not provide a storage type, it defaults to
 * `ComponentStorageType::kArchetype`.
 * @tparam T Component type
 * @param instance Component instance
 * @return Component storage type
 */
template <ComponentTrait T>
[[nodiscard]] consteval ComponentStorageType ComponentStorageTypeOf(
    const T& /*instance*/) noexcept {
  return ComponentStorageTypeOf<std::remove_cvref_t<T>>();
}

/**
 * @brief Concept for components that are stored in an archetype.
 * @details An archetype component must satisfy `ComponentTrait`
 * and have a storage type of `ComponentStorageType::kArchetype`.
 */
template <typename T>
concept ArchetypeComponentTrait =
    ComponentTrait<T> && (ComponentStorageTypeOf<std::remove_cvref_t<T>>() ==
                          ComponentStorageType::kArchetype);

/**
 * @brief Concept for components that are stored in a sparse set.
 * @details A sparse set component must satisfy `ComponentTrait`
 * and have a storage type of `ComponentStorageType::kSparseSet`.
 */
template <typename T>
concept SparseComponentTrait =
    ComponentTrait<T> && (ComponentStorageTypeOf<std::remove_cvref_t<T>>() ==
                          ComponentStorageType::kSparseSet);

/**
 * @brief Component traits for optimization decisions.
 * @tparam T Component type
 */
template <ComponentTrait T>
struct ComponentTraits {
  static constexpr size_t kSize = sizeof(T);
  static constexpr size_t kAlignment = alignof(T);
  static constexpr ComponentStorageType kStorageType =
      ComponentStorageTypeOf<T>();
  static constexpr bool kIsTag = TagComponentTrait<T>;
};

/// @brief Component type info for runtime operations.
class ComponentTypeInfo {
public:
  constexpr ComponentTypeInfo(const ComponentTypeInfo&) noexcept = default;
  constexpr ComponentTypeInfo(ComponentTypeInfo&&) noexcept = default;
  constexpr ~ComponentTypeInfo() noexcept = default;

  constexpr ComponentTypeInfo& operator=(const ComponentTypeInfo&) noexcept =
      default;
  constexpr ComponentTypeInfo& operator=(ComponentTypeInfo&&) noexcept =
      default;

  /**
   * @brief Create a `ComponentTypeInfo` from a component type.
   * @tparam T Component type
   * @return `ComponentTypeInfo` for the given component type
   */
  template <ComponentTrait T>
  [[nodiscard]] static constexpr ComponentTypeInfo From() noexcept {
    constexpr ComponentTraits<T> traits;
    return {ComponentTypeIndex::From<T>(), traits.kSize, traits.kAlignment,
            traits.kStorageType, traits.kIsTag};
  }

  constexpr bool operator==(const ComponentTypeInfo& other) const noexcept {
    return type_index_ == other.type_index_;
  }
  constexpr bool operator!=(const ComponentTypeInfo& other) const noexcept {
    return !(*this == other);
  }

  constexpr bool operator<(const ComponentTypeInfo& other) const noexcept {
    return type_index_ < other.type_index_;
  }

  /**
   * @brief Get the component type index.
   * @return Component type index
   */
  [[nodiscard]] constexpr ComponentTypeIndex TypeIndex() const noexcept {
    return type_index_;
  }

  /**
   * @brief Get the size of the component type.
   * @return Size of the component type
   */
  [[nodiscard]] constexpr size_t Size() const noexcept { return size_; }

  /**
   * @brief Get the alignment of the component type.
   * @return Alignment of the component type
   */
  [[nodiscard]] constexpr size_t Alignment() const noexcept {
    return alignment_;
  }

  /**
   * @brief Get the storage type of the component type.
   * @return Storage type of the component type
   */
  [[nodiscard]] constexpr ComponentStorageType StorageType() const noexcept {
    return storage_type_;
  }

  /**
   * @brief Check if the component type is a tag (empty).
   * @return True if the component type is a tag, false otherwise
   */
  [[nodiscard]] constexpr bool IsTag() const noexcept { return is_tag_; }

private:
  constexpr ComponentTypeInfo(ComponentTypeIndex type_index, size_t size,
                              size_t alignment,
                              ComponentStorageType storage_type,
                              bool is_tag) noexcept
      : type_index_(type_index),
        size_(size),
        alignment_(alignment),
        storage_type_(storage_type),
        is_tag_(is_tag) {}

  ComponentTypeIndex type_index_;
  size_t size_ = 0;
  size_t alignment_ = 0;
  ComponentStorageType storage_type_ = ComponentStorageType::kArchetype;
  bool is_tag_ = false;
};

}  // namespace helios::ecs

namespace std {

template <>
struct hash<helios::ecs::ComponentTypeInfo> {
  consteval size_t operator()(
      const helios::ecs::ComponentTypeInfo& info) const noexcept {
    return info.TypeIndex().Hash();
  }
};

}  // namespace std
