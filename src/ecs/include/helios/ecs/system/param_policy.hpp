#pragma once

#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/param_traits.hpp>
#include <helios/ecs/system/system.hpp>

#include <tuple>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief Builds an AccessPolicy by calling `RegisterAccess` on each system
 * parameter type.
 * @tparam Params System parameter types
 * @return The accumulated access policy
 */
template <typename... Params>
[[nodiscard]] constexpr AccessPolicy BuildPolicyFromParams() {
  AccessPolicyBuilder builder;
  RegisterParamAccess<Params...>(builder);
  return builder.Build();
}

/**
 * @brief Deduces parameter types from the system's `operator()` signature and
 * builds the access policy.
 * @tparam T System type satisfying `SystemTrait`
 * @return The accumulated access policy
 */
template <SystemTrait T>
[[nodiscard]] constexpr AccessPolicy BuildPolicyFromSystem() {
  return []<typename... Args>(std::tuple<Args...>*) {
    return BuildPolicyFromParams<Args...>();
  }(static_cast<typename details::MemberFnArgs<
             decltype(&std::remove_cvref_t<T>::operator())>::ArgsTuple*>(
             nullptr));
}

}  // namespace helios::ecs
