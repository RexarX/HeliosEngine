#pragma once

#include <helios/ecs/system/access_policy.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

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

namespace details {

template <typename T>
using SystemParamMakeResult =
    decltype(SystemParamTraits<std::remove_cvref_t<T>>::Make(
        std::declval<World&>(), std::declval<SystemLocalData&>(),
        std::declval<const AccessPolicy&>()));

template <typename T>
concept SystemParamMakeResultMatches =
    std::same_as<SystemParamMakeResult<T>, std::remove_cvref_t<T>> ||
    std::same_as<SystemParamMakeResult<T>, std::remove_cvref_t<T>&>;

}  // namespace details

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
      details::SystemParamMakeResultMatches<std::remove_cvref_t<T>>;
    };

}  // namespace helios::ecs
