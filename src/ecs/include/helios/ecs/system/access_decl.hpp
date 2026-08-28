#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/utils/common_traits.hpp>

#include <type_traits>
#endif
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/system/param.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class AccessPolicyBuilder;

/**
 * @brief Registers access declarations for each parameter type.
 * @tparam Params System parameter types
 * @param builder Access policy builder to accumulate into
 */
template <typename... Params>
constexpr void RegisterParamAccess(AccessPolicyBuilder& builder) {
  (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(builder),
   ...);
}

/**
 * @brief Declares read-only component access.
 * @tparam Cs Component types
 * @param builder Access policy builder
 */
template <ComponentTrait... Cs>
  requires utils::UniqueTypes<Cs...>
constexpr void DeclareReadComponents(AccessPolicyBuilder& builder) {
  builder.Query<const Cs&...>();
}

/**
 * @brief Declares mutable component access.
 * @tparam Cs Component types
 * @param builder Access policy builder
 */
template <ComponentTrait... Cs>
  requires utils::UniqueTypes<Cs...>
constexpr void DeclareWriteComponents(AccessPolicyBuilder& builder) {
  builder.Query<Cs&...>();
}

/**
 * @brief Declares component access using query access specifiers.
 * @tparam AccessTypes Component access types (`const T&`, `T&`, etc.)
 * @param builder Access policy builder
 */
template <typename... AccessTypes>
constexpr void DeclareQueryAccess(AccessPolicyBuilder& builder) {
  builder.Query<AccessTypes...>();
}

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
