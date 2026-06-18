#pragma once

#include <helios/ecs/system/system_param.hpp>
#include <helios/utils/hash.hpp>
#include <helios/utils/type_info.hpp>

#include <compare>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

/// @brief Type id for systems.
using SystemTypeId = utils::TypeId;

/// @brief Type index for systems.
using SystemTypeIndex = utils::TypeIndex;

namespace details {

/// @brief Extracts the argument types of a member function pointer.
template <typename F>
struct MemberFnArgs;

template <typename R, typename C, typename... Args>
struct MemberFnArgs<R (C::*)(Args...)> {
  using ArgsTuple = std::tuple<Args...>;
};

template <typename R, typename C, typename... Args>
struct MemberFnArgs<R (C::*)(Args...) const> {
  using ArgsTuple = std::tuple<Args...>;
};

/// @brief True if every Arg in the pack satisfies `SystemParam`.
template <typename... Args>
inline constexpr bool kAllAreSystemParams =
    (SystemParam<std::remove_cvref_t<Args>> && ...);

/// @brief Concept: a type's `operator()` arguments all have
/// `SystemParamTraits`.
template <typename R, typename... Args>
struct MemberFnArgs<R (*)(Args...)> {
  using ArgsTuple = std::tuple<Args...>;
};

template <typename R, typename... Args>
struct MemberFnArgs<R(Args...)> {
  using ArgsTuple = std::tuple<Args...>;
};

template <typename F>
concept HasValidSystemParams = requires {
  typename MemberFnArgs<F>::ArgsTuple;
} && []<typename... Args>(std::tuple<Args...>*) {
  return kAllAreSystemParams<Args...>;
}(static_cast<typename MemberFnArgs<F>::ArgsTuple*>(nullptr));

template <typename T>
concept HasSystemParamCallOperator =
    HasValidSystemParams<T> ||
    HasValidSystemParams<decltype(&std::remove_cvref_t<T>::operator())>;

}  // namespace details

/**
 * @brief Concept for systems that declare all data access via parameter types.
 * @details A system must be destructible, move or copy constructible, not
 * polymorphic, an object type, and its `operator()` parameters must all have
 * `SystemParamTraits` specialisations.
 */
template <typename T>
concept SystemTrait =
    std::destructible<T> &&
    (std::move_constructible<T> || std::copy_constructible<T>) &&
    !std::is_polymorphic_v<std::remove_cvref_t<T>> &&
    std::is_object_v<std::remove_cvref_t<T>> &&
    details::HasSystemParamCallOperator<T>;

/**
 * @brief Concept for lambda closure types that satisfy `SystemTrait`.
 * @details Lambdas are detected via `utils::IsLambda<T>()`. They should always
 * be registered with an explicit human-readable name.
 */
template <typename T>
concept LambdaSystemTrait =
    SystemTrait<T> && utils::LambdaTrait<std::remove_cvref_t<T>>;

/// @brief Concept for system types that are not lambda closures.
template <typename T>
concept FunctorSystemTrait = SystemTrait<T> && !LambdaSystemTrait<T>;

/**
 * @brief Concept for systems that declare all data access via parameter types.
 * @details A system must be destructible, move or copy constructible, not
 * polymorphic, an object type, and its `operator()` parameters must all have
 * `SystemParamTraits` specialisations.
 */
template <typename T>
concept SystemWithNameTrait = SystemTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Gets the name of a system.
 * @details Returns provided name or type name as fallback.
 * @tparam T System type
 * @return System name
 */
template <SystemTrait T>
[[nodiscard]] constexpr std::string_view SystemNameOf() noexcept {
  if constexpr (SystemWithNameTrait<T>) {
    return T::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

/**
 * @brief Gets the name of a system.
 * @tparam T System type
 * @param instance System instance
 * @return System name
 */
template <SystemTrait T>
[[nodiscard]] constexpr std::string_view SystemNameOf(
    const T& /*instance*/) noexcept {
  return SystemNameOf<std::remove_cvref_t<T>>();
}

/// @brief Id for systems.
struct SystemId {
  size_t id = 0;

  /**
   * @brief Creates system id from a name.
   * @param name System name
   * @return System id
   */
  [[nodiscard]] static constexpr SystemId From(std::string_view name) noexcept {
    return SystemId{.id = utils::Fnv1aHash(name)};
  }

  /**
   * @brief Creates system id from a system type index.
   * @param index System type index
   * @return System id
   */
  [[nodiscard]] static constexpr SystemId From(SystemTypeIndex index) noexcept {
    return SystemId{.id = index.Hash()};
  }

  /**
   * @brief Creates system id from a system type id.
   * @param id System type id
   * @return System id
   */
  [[nodiscard]] static constexpr SystemId From(SystemTypeId id) noexcept {
    return From(id.Index());
  }

  /**
   * @brief Creates system id from a system type.
   * @tparam T System type
   * @return System id
   */
  template <SystemTrait T>
  [[nodiscard]] static constexpr SystemId From() noexcept {
    return From(SystemTypeIndex::From<T>());
  }

  /**
   * @brief Creates system id from a system type.
   * @tparam T System type
   * @param instance System instance
   * @return System id
   */
  template <SystemTrait T>
  [[nodiscard]] static constexpr SystemId From(const T& /*instance*/) noexcept {
    return From<std::remove_cvref_t<T>>();
  }

  constexpr std::strong_ordering operator<=>(const SystemId&) const noexcept =
      default;
};

}  // namespace helios::ecs

namespace std {

template <>
struct hash<helios::ecs::SystemId> {
  constexpr size_t operator()(helios::ecs::SystemId id) const noexcept {
    return id.id;
  }
};

}  // namespace std
