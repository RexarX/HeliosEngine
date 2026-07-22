#pragma once

#include <helios/ecs/component/component.hpp>
#include <helios/utils/common_traits.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace helios::ecs {

namespace details {

template <typename... Ts>
struct ComponentBundleTypeList {};

template <typename... Lists>
struct ConcatComponentBundleTypeLists;

template <>
struct ConcatComponentBundleTypeLists<> {
  using Type = ComponentBundleTypeList<>;
};

template <typename... Ts>
struct ConcatComponentBundleTypeLists<ComponentBundleTypeList<Ts...>> {
  using Type = ComponentBundleTypeList<Ts...>;
};

template <typename... Ts, typename... Us, typename... Rest>
struct ConcatComponentBundleTypeLists<ComponentBundleTypeList<Ts...>,
                                      ComponentBundleTypeList<Us...>, Rest...>
    : public ConcatComponentBundleTypeLists<
          ComponentBundleTypeList<Ts..., Us...>, Rest...> {};

template <typename T>
struct ComponentBundleElementInfo {
  using Type = std::remove_cvref_t<T>;
  using ComponentTypes = ComponentBundleTypeList<Type>;

  static constexpr bool kValid =
      std::same_as<T, Type> && !std::is_array_v<Type> && ComponentTrait<Type>;
};

template <typename T>
struct ComponentBundleInfo {
  using ComponentTypes = ComponentBundleTypeList<>;

  static constexpr bool kValid = false;
  static constexpr size_t kSize = 0;
};

template <typename... Ts>
struct ComponentBundleElementInfo<ComponentBundle<Ts...>>;

template <typename... Ts>
struct ComponentBundleInfo<ComponentBundle<Ts...>> {
private:
  template <typename... Us>
  [[nodiscard]] static consteval bool Unique(
      ComponentBundleTypeList<Us...> /*types*/) noexcept {
    return utils::UniqueTypes<Us...>;
  }

public:
  using ComponentTypes = typename ConcatComponentBundleTypeLists<
      typename ComponentBundleElementInfo<Ts>::ComponentTypes...>::Type;

  static constexpr size_t kSize = []<typename... Us>(
                                      ComponentBundleTypeList<Us...>
                                      /*types*/) consteval {
    return sizeof...(Us);
  }(ComponentTypes{});

  static constexpr bool kValid =
      (sizeof...(Ts) > 0) && (... && ComponentBundleElementInfo<Ts>::kValid) &&
      Unique(ComponentTypes{});
};

template <typename... Ts>
struct ComponentBundleElementInfo<ComponentBundle<Ts...>> {
  using Info = ComponentBundleInfo<ComponentBundle<Ts...>>;
  using ComponentTypes = typename Info::ComponentTypes;

  static constexpr bool kValid = Info::kValid;
};

struct ComponentBundleAccess;

template <typename Bundle>
inline constexpr size_t kComponentBundleSize =
    ComponentBundleInfo<std::remove_cvref_t<Bundle>>::kSize;

template <typename Bundle>
using ComponentBundleResult =
    std::conditional_t<kComponentBundleSize<Bundle> == 1, bool,
                       std::array<bool, kComponentBundleSize<Bundle>>>;

}  // namespace details

/**
 * @brief Concept to check if a type is a valid component bundle.
 * @details A component bundle is a non-empty `ComponentBundle` whose elements
 * are unqualified component values or valid component bundles. Flattened
 * component types must be unique.
 */
template <typename T>
concept ComponentBundleTrait =
    details::IsComponentBundle<std::remove_cvref_t<T>>::value &&
    details::ComponentBundleInfo<std::remove_cvref_t<T>>::kValid;

/**
 * @brief Owning collection of components that can be added or removed as one
 * ECS operation.
 * @details Bundles may contain components and other bundles. Nested elements
 * are flattened depth-first in declaration order when consumed by the ECS.
 * @tparam Ts Component or component bundle types
 */
template <typename... Ts>
class ComponentBundle {
public:
  /// @brief Constructs a bundle by default-constructing its elements.
  constexpr ComponentBundle() noexcept(
      (std::is_nothrow_default_constructible_v<Ts> && ...)) = default;

  /**
   * @brief Constructs a bundle from component and nested bundle values.
   * @tparam Us Argument types
   * @param elements Component and nested bundle values
   */
  template <typename... Us>
    requires ComponentBundleTrait<ComponentBundle> &&
             (sizeof...(Us) == sizeof...(Ts)) &&
             (std::constructible_from<Ts, Us &&> && ...)
  explicit(sizeof...(Ts) == 1 || !(std::convertible_to<Us&&, Ts> && ...)) constexpr ComponentBundle(
      Us&&... elements) noexcept((std::is_nothrow_constructible_v<Ts, Us&&> &&
                                  ...))
      : elements_(std::forward<Us>(elements)...) {}

  constexpr ComponentBundle(const ComponentBundle&) noexcept(
      (std::is_nothrow_copy_constructible_v<Ts> && ...)) = default;
  constexpr ComponentBundle(ComponentBundle&&) noexcept(
      (std::is_nothrow_move_constructible_v<Ts> && ...)) = default;
  constexpr ~ComponentBundle() noexcept((std::is_nothrow_destructible_v<Ts> &&
                                         ...)) = default;

  constexpr ComponentBundle& operator=(const ComponentBundle&) noexcept(
      (std::is_nothrow_copy_assignable_v<Ts> && ...)) = default;
  constexpr ComponentBundle& operator=(ComponentBundle&&) noexcept(
      (std::is_nothrow_move_assignable_v<Ts> && ...)) = default;

private:
  friend struct details::ComponentBundleAccess;

  std::tuple<Ts...> elements_;
};

template <typename... Ts>
ComponentBundle(Ts&&...) -> ComponentBundle<std::remove_cvref_t<Ts>...>;

namespace details {

struct ComponentBundleAccess {
public:
  template <ComponentBundleTrait Bundle>
  [[nodiscard]] static constexpr auto Flatten(Bundle&& bundle) {
    return std::apply(
        []<typename... Ts>(Ts&&... elements) {
          return std::tuple_cat(FlattenElement(std::forward<Ts>(elements))...);
        },
        std::forward<Bundle>(bundle).elements_);
  }

private:
  template <typename T>
  [[nodiscard]] static constexpr auto FlattenElement(T&& element) {
    if constexpr (ComponentBundleTrait<T>) {
      return Flatten(std::forward<T>(element));
    } else {
      return std::forward_as_tuple(std::forward<T>(element));
    }
  }
};

template <ComponentBundleTrait Bundle, typename F>
constexpr auto ApplyComponentBundle(Bundle&& bundle, F&& func)
    -> decltype(auto) {
  return std::apply(std::forward<F>(func), ComponentBundleAccess::Flatten(
                                               std::forward<Bundle>(bundle)));
}

template <typename... Ts, typename F>
constexpr auto ApplyComponentBundleTypes(
    ComponentBundleTypeList<Ts...> /*types*/, F&& func) -> decltype(auto) {
  return std::forward<F>(func).template operator()<Ts...>();
}

template <ComponentBundleTrait Bundle, typename F>
constexpr auto ApplyComponentBundleTypes(F&& func) -> decltype(auto) {
  using Types =
      typename ComponentBundleInfo<std::remove_cvref_t<Bundle>>::ComponentTypes;
  return ApplyComponentBundleTypes(Types{}, std::forward<F>(func));
}

}  // namespace details

}  // namespace helios::ecs
