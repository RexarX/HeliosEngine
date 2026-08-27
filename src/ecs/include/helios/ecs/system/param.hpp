#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <concepts>
#include <type_traits>
#include <utility>
#endif
#include <helios/ecs/system/access_policy.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;
struct SystemLocalData;

/**
 * @brief Primary template — deliberately incomplete.
 * @details Every system parameter type must provide a specialisation with:
 * - `static auto Make(World&, SystemLocalData&, const AccessPolicy&) -> T` or
 *   `T&`
 * - `static constexpr void RegisterAccess(AccessPolicyBuilder&)`
 */
template <typename T>
struct SystemParamTraits;

}  // namespace helios::ecs

namespace helios::ecs::details {

template <typename T>
using SystemParamMakeResult =
    decltype(SystemParamTraits<std::remove_cvref_t<T>>::Make(
        std::declval<World&>(), std::declval<SystemLocalData&>(),
        std::declval<const AccessPolicy&>()));

template <typename T>
concept SystemParamMakeResultMatches =
    std::same_as<SystemParamMakeResult<T>, std::remove_cvref_t<T>> ||
    std::same_as<SystemParamMakeResult<T>, std::remove_cvref_t<T>&>;

}  // namespace helios::ecs::details

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/**
 * @brief Concept for types usable as system function parameters.
 * @details Satisfied when `SystemParamTraits<T>` provides `RegisterAccess` and
 * `Make` returning `T` or `T&`.
 */
template <typename T>
concept SystemParam =
    requires(AccessPolicyBuilder& builder, World& world, SystemLocalData& local,
             const AccessPolicy& policy) {
      { SystemParamTraits<std::remove_cvref_t<T>>::RegisterAccess(builder) };
      SystemParamTraits<std::remove_cvref_t<T>>::Make(world, local, policy);
      requires details::SystemParamMakeResultMatches<std::remove_cvref_t<T>>;
    };

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
