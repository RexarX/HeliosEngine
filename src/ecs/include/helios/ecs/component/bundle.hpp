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

/**
 * @brief Component and nested-bundle values for a bundle operation.
 * @details Lists component types and/or nested bundle types at compile time.
 * Nested types are flattened depth-first, left-to-right, for add/remove. The
 * same type also stores leaf and nested bundle values in declaration order.
 * @tparam Ts Component types and/or nested bundle types
 */
template <typename... Ts>
struct ComponentBundleTypes {
  using ElementTuple = std::tuple<std::remove_cvref_t<Ts>...>;

  ElementTuple values;

  /// @brief Default-constructs all elements.
  constexpr ComponentBundleTypes() noexcept(
      (std::is_nothrow_default_constructible_v<Ts> && ...)) = default;

  /**
   * @brief Constructs a bundle from element values.
   * @tparam Us Argument types
   * @param elements Element values in declaration order
   */
  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) &&
            (std::constructible_from<Ts, Us &&> && ...)
  constexpr ComponentBundleTypes(Us&&... elements) noexcept(
      (std::is_nothrow_constructible_v<Ts, Us&&> && ...))
      : values(std::forward<Us>(elements)...) {}

  constexpr ComponentBundleTypes(const ComponentBundleTypes&) noexcept(
      (std::is_nothrow_copy_constructible_v<Ts> && ...)) = default;
  constexpr ComponentBundleTypes(ComponentBundleTypes&&) noexcept(
      (std::is_nothrow_move_constructible_v<Ts> && ...)) = default;
  constexpr ~ComponentBundleTypes() noexcept(
      (std::is_nothrow_destructible_v<Ts> && ...)) = default;

  constexpr ComponentBundleTypes&
  operator=(const ComponentBundleTypes&) noexcept(
      (std::is_nothrow_copy_assignable_v<Ts> && ...)) = default;
  constexpr ComponentBundleTypes& operator=(ComponentBundleTypes&&) noexcept(
      (std::is_nothrow_move_assignable_v<Ts> && ...)) = default;
};

namespace details {

template <typename T>
struct IsComponentBundleTypes : std::false_type {};

template <typename... Ts>
struct IsComponentBundleTypes<ComponentBundleTypes<Ts...>> : std::true_type {};

template <typename... Lists>
struct ConcatComponentBundleTypes;

template <>
struct ConcatComponentBundleTypes<> {
  using Type = ComponentBundleTypes<>;
};

template <typename... Ts>
struct ConcatComponentBundleTypes<ComponentBundleTypes<Ts...>> {
  using Type = ComponentBundleTypes<Ts...>;
};

template <typename... Ts, typename... Us, typename... Rest>
struct ConcatComponentBundleTypes<ComponentBundleTypes<Ts...>,
                                  ComponentBundleTypes<Us...>, Rest...>
    : ConcatComponentBundleTypes<ComponentBundleTypes<Ts..., Us...>, Rest...> {
};

template <typename T>
struct ComponentBundleTypeInfo {
  using LeafTypes = ComponentBundleTypes<>;

  static constexpr bool kValid = false;
  static constexpr size_t kSize = 0;
};

template <typename T>
inline constexpr bool kIsNestedBundleType = [] {
  using Decayed = std::remove_cvref_t<T>;
  if constexpr (IsComponentBundleTypes<Decayed>::value) {
    return true;
  } else {
    return requires { typename Decayed::ComponentTypes; }&&
      requires(Decayed bundle)
    {
      {std::move(bundle).Build()}
          ->std::same_as<typename Decayed::ComponentTypes>;
    }
    &&!ComponentTrait<Decayed>;
  }
}();

template <typename T, bool IsNested = kIsNestedBundleType<T>>
struct ComponentBundleElementTypes;

template <typename T>
struct ComponentBundleElementTypes<T, false> {
  using Decayed = std::remove_cvref_t<T>;

  static constexpr bool kValid = std::same_as<T, Decayed> &&
                                 !std::is_array_v<Decayed> &&
                                 ComponentTrait<Decayed>;

  using Type = std::conditional_t<kValid, ComponentBundleTypes<Decayed>,
                                  ComponentBundleTypes<>>;
};

template <typename... Ts>
struct ComponentBundleElementTypes<ComponentBundleTypes<Ts...>, true> {
  using NestedTypes = ComponentBundleTypes<Ts...>;

  static constexpr bool kValid = ComponentBundleTypeInfo<NestedTypes>::kValid;
  using Type = typename ComponentBundleTypeInfo<NestedTypes>::LeafTypes;
};

template <typename T>
  requires(!IsComponentBundleTypes<T>::value)
struct ComponentBundleElementTypes<T, true> {
  using NestedTypes = typename T::ComponentTypes;

  static constexpr bool kValid = ComponentBundleTypeInfo<NestedTypes>::kValid;
  using Type = typename ComponentBundleTypeInfo<NestedTypes>::LeafTypes;
};

template <typename... Ts>
struct ComponentBundleTypeInfo<ComponentBundleTypes<Ts...>> {
private:
  template <typename... Us>
  [[nodiscard]] static consteval bool Unique(
      ComponentBundleTypes<Us...> /*types*/) noexcept {
    return utils::UniqueTypes<Us...>;
  }

public:
  using LeafTypes = typename ConcatComponentBundleTypes<
      typename ComponentBundleElementTypes<Ts>::Type...>::Type;

  static constexpr size_t kSize = []<typename... Us>(ComponentBundleTypes<Us...>
                                                     /*types*/) consteval {
    return sizeof...(Us);
  }(LeafTypes{});

  static constexpr bool kValid =
      (sizeof...(Ts) > 0) && (... && ComponentBundleElementTypes<Ts>::kValid) &&
      Unique(LeafTypes{});
};

}  // namespace details

/**
 * @brief Concept for a valid component bundle.
 * @details A bundle is either a `ComponentBundleTypes<...>` specialization or a
 * type that declares `ComponentTypes` and returns it from `Build()`.
 */
template <typename T>
concept ComponentBundleTrait =
    (details::IsComponentBundleTypes<std::remove_cvref_t<T>>::value &&
     details::ComponentBundleTypeInfo<std::remove_cvref_t<T>>::kValid) ||
    (requires(std::remove_cvref_t<T> bundle) {
      typename std::remove_cvref_t<T>::ComponentTypes;
      {
        std::move(bundle).Build()
      } -> std::same_as<typename std::remove_cvref_t<T>::ComponentTypes>;
    } &&
     details::ComponentBundleTypeInfo<
         typename std::remove_cvref_t<T>::ComponentTypes>::kValid);

namespace details {

template <typename Bundle>
struct BundleTypeListImpl;

template <typename... Ts>
struct BundleTypeListImpl<ComponentBundleTypes<Ts...>> {
  using Type = ComponentBundleTypes<Ts...>;
};

template <typename T>
  requires(!IsComponentBundleTypes<T>::value)
struct BundleTypeListImpl<T> {
  using Type = typename T::ComponentTypes;
};

template <typename Bundle>
using BundleTypeList =
    typename BundleTypeListImpl<std::remove_cvref_t<Bundle>>::Type;

template <typename Bundle>
using BundleLeafTypes =
    typename ComponentBundleTypeInfo<BundleTypeList<Bundle>>::LeafTypes;

template <typename Bundle>
inline constexpr size_t kComponentBundleSize =
    ComponentBundleTypeInfo<BundleTypeList<Bundle>>::kSize;

template <typename Bundle>
using ComponentBundleResult =
    std::conditional_t<kComponentBundleSize<Bundle> == 1, bool,
                       std::array<bool, kComponentBundleSize<Bundle>>>;

template <typename T>
constexpr auto FlattenValue(T&& value);

template <typename... Ts>
constexpr auto FlattenBundleValues(ComponentBundleTypes<Ts...>& bundle) {
  return std::apply(
      []<typename... Us>(Us&... elements) {
        return std::tuple_cat(FlattenValue(elements)...);
      },
      bundle.values);
}

template <typename... Ts>
constexpr auto FlattenBundleValues(const ComponentBundleTypes<Ts...>& bundle) {
  return std::apply(
      []<typename... Us>(const Us&... elements) {
        return std::tuple_cat(FlattenValue(elements)...);
      },
      bundle.values);
}

template <typename... Ts>
constexpr auto FlattenBundleValues(ComponentBundleTypes<Ts...>&& bundle) {
  return std::apply(
      []<typename... Us>(Us&&... elements) {
        return std::tuple_cat(FlattenValue(std::forward<Us>(elements))...);
      },
      std::move(bundle.values));
}

template <typename T>
constexpr auto FlattenValue(T&& value) {
  using Decayed = std::remove_cvref_t<T>;
  if constexpr (IsComponentBundleTypes<Decayed>::value) {
    return FlattenBundleValues(std::forward<T>(value));
  } else if constexpr (kIsNestedBundleType<Decayed>) {
    if constexpr (std::is_lvalue_reference_v<T&&>) {
      auto copy = value;
      return FlattenBundleValues(std::move(copy).Build());
    } else {
      return FlattenBundleValues(std::move(value).Build());
    }
  } else {
    return std::make_tuple(std::forward<T>(value));
  }
}

template <ComponentBundleTrait Bundle>
constexpr auto ExtractBundleValues(Bundle&& bundle) {
  using BundleType = std::remove_cvref_t<Bundle>;
  if constexpr (IsComponentBundleTypes<BundleType>::value) {
    return FlattenBundleValues(std::forward<Bundle>(bundle));
  } else if constexpr (std::is_lvalue_reference_v<Bundle&&>) {
    auto copy = bundle;
    return FlattenBundleValues(std::move(copy).Build());
  } else {
    return FlattenBundleValues(std::move(bundle).Build());
  }
}

template <ComponentBundleTrait Bundle, typename F>
constexpr auto ApplyComponentBundle(Bundle&& bundle, F&& func)
    -> decltype(auto) {
  return std::apply(std::forward<F>(func),
                    ExtractBundleValues(std::forward<Bundle>(bundle)));
}

template <typename... Ts, typename F>
constexpr auto ApplyComponentBundleTypes(ComponentBundleTypes<Ts...> /*types*/,
                                         F&& func) -> decltype(auto) {
  return std::forward<F>(func).template operator()<Ts...>();
}

template <ComponentBundleTrait Bundle, typename F>
constexpr auto ApplyComponentBundleTypes(F&& func) -> decltype(auto) {
  return ApplyComponentBundleTypes(BundleLeafTypes<Bundle>{},
                                   std::forward<F>(func));
}

template <typename... Ts>
struct IsComponentBundle<ComponentBundleTypes<Ts...>> : std::true_type {};

template <typename T>
struct HasStructBundleBuild : std::false_type {};

template <typename T>
  requires requires(std::remove_cvref_t<T> bundle) {
    typename std::remove_cvref_t<T>::ComponentTypes;
    {
      std::move(bundle).Build()
    } -> std::same_as<typename std::remove_cvref_t<T>::ComponentTypes>;
  }
struct HasStructBundleBuild<T> : std::true_type {};

template <typename T, bool HasBuild = HasStructBundleBuild<T>::value>
struct IsStructComponentBundle : std::false_type {};

template <typename T>
struct IsStructComponentBundle<T, true>
    : std::bool_constant<
          ComponentBundleTypeInfo<typename T::ComponentTypes>::kValid> {};

template <typename T>
  requires(!IsComponentBundleTypes<std::remove_cvref_t<T>>::value)
struct IsComponentBundle<T> : IsStructComponentBundle<std::remove_cvref_t<T>> {
};

}  // namespace details

}  // namespace helios::ecs
