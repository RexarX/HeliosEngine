#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief Helper to detect const components.
 * @details Used to determine if a component type is const-qualified.
 */
template <typename T>
struct IsConstComponent : std::false_type {};

template <typename T>
struct IsConstComponent<const T> : std::true_type {};

template <typename T>
inline constexpr bool IsConstComponent_v = IsConstComponent<T>::value;

/**
 * @brief Concept for const-qualified component types.
 * @details Used to constrain template parameters to const components.
 */
template <typename T>
concept ConstComponentTrait = IsConstComponent_v<T>;

/**
 * @brief Concept to check if a type can be used as a component.
 */
template <typename T>
concept ComponentTrait = std::is_object_v<std::remove_cvref_t<T>> && std::is_destructible_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for components that provide a static name method.
 * @details Named components can provide a human-readable identifier.
 */
template <typename T>
concept ComponentWithNameTrait = ComponentTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Concept for tag components (empty).
 */
template <typename T>
concept TagComponentTrait = ComponentTrait<T> && std::is_empty_v<T>;

/**
 * @brief Concept for trivially copyable components (POD-like).
 */
template <typename T>
concept TrivialComponentTrait =
    ComponentTrait<T> && std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

/**
 * @brief Concept for tiny components (single cache line portion), where sizeof(T) <= 16.
 */
template <typename T>
concept TinyComponentTrait = ComponentTrait<T> && (sizeof(T) <= 16);

/**
 * @brief Concept for small components that fit in cache line, where sizeof(T) <= 64.
 */
template <typename T>
concept SmallComponentTrait = ComponentTrait<T> && (sizeof(T) <= 64);

/**
 * @brief Concept for medium components (multiple cache line portions), where 64 < sizeof(T) <= 256.
 */
template <typename T>
concept MediumComponentTrait = ComponentTrait<T> && (sizeof(T) > 64 && sizeof(T) <= 256);

/**
 * @brief Concept for large components that don't fit in cache line, where sizeof(T) > 256.
 */
template <typename T>
concept LargeComponentTrait = ComponentTrait<T> && (sizeof(T) > 256);

/**
 * @brief Component traits for optimization decisions.
 */
template <ComponentTrait T>
struct ComponentTraits {
  static constexpr bool kIsTrivial = TrivialComponentTrait<T>;
  static constexpr bool kIsTiny = TinyComponentTrait<T>;
  static constexpr bool kIsSmall = SmallComponentTrait<T>;
  static constexpr bool kIsMedium = MediumComponentTrait<T>;
  static constexpr bool kIsLarge = LargeComponentTrait<T>;
  static constexpr size_t kSize = sizeof(T);
  static constexpr size_t kAlignment = alignof(T);
};

/**
 * @brief Type ID for components.
 */
using ComponentTypeId = size_t;

/**
 * @brief Type ID for components using CTTI.
 * @tparam T Component type
 * @return Unique type ID for the component
 */
template <ComponentTrait T>
constexpr ComponentTypeId ComponentTypeIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

template <ComponentTrait T>
[[nodiscard]] constexpr std::string_view ComponentNameOf() noexcept {
  if constexpr (ComponentWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

/**
 * @brief Component type info for runtime operations.
 */
class ComponentTypeInfo {
public:
  constexpr ComponentTypeInfo(const ComponentTypeInfo&) noexcept = default;
  constexpr ComponentTypeInfo(ComponentTypeInfo&&) noexcept = default;
  constexpr ~ComponentTypeInfo() noexcept = default;

  constexpr ComponentTypeInfo& operator=(const ComponentTypeInfo&) noexcept = default;
  constexpr ComponentTypeInfo& operator=(ComponentTypeInfo&&) noexcept = default;

  template <ComponentTrait T>
  [[nodiscard]] static constexpr ComponentTypeInfo Create() noexcept {
    return {ComponentTypeIdOf<T>(), ComponentTraits<T>::kSize, ComponentTraits<T>::kAlignment,
            ComponentTraits<T>::kIsTrivial};
  }

  constexpr bool operator==(const ComponentTypeInfo& other) const noexcept { return type_id_ == other.type_id_; }
  constexpr bool operator!=(const ComponentTypeInfo& other) const noexcept { return !(*this == other); }

  constexpr bool operator<(const ComponentTypeInfo& other) const noexcept { return type_id_ < other.type_id_; }

  [[nodiscard]] constexpr size_t TypeId() const noexcept { return type_id_; }
  [[nodiscard]] constexpr size_t Size() const noexcept { return size_; }
  [[nodiscard]] constexpr size_t Alignment() const noexcept { return alignment_; }
  [[nodiscard]] constexpr bool IsTrivial() const noexcept { return is_trivial_; }

private:
  constexpr ComponentTypeInfo(ComponentTypeId type_id, size_t size, size_t alignment, bool is_trivial) noexcept
      : type_id_(type_id), size_(size), alignment_(alignment), is_trivial_(is_trivial) {}

  ComponentTypeId type_id_ = 0;
  size_t size_ = 0;
  size_t alignment_ = 0;
  bool is_trivial_ = false;
};

}  // namespace helios::ecs

template <>
struct std::hash<helios::ecs::ComponentTypeInfo> {
  constexpr size_t operator()(const helios::ecs::ComponentTypeInfo& info) const noexcept { return info.TypeId(); }
};
