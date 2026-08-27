#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <tuple>
#endif
#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/system.hpp>

HELIOS_MODULE_EXPORT
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
  }(static_cast<details::SystemArgsTuple<T>*>(nullptr));
}

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
