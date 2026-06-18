#pragma once

#include <helios/ecs/system/access_policy.hpp>

#include <concepts>
#include <type_traits>

namespace helios::ecs {

class World;
struct SystemLocalData;

/**
 * @brief Primary template — deliberately incomplete.
 * @details Every system parameter type must provide a specialisation with:
 * - `static auto Make(World&, SystemLocalData&, const AccessPolicy&) -> T`
 * - `static constexpr void RegisterAccess(AccessPolicyBuilder&)`
 */
template <typename T>
struct SystemParamTraits;

/**
 * @brief Concept for types usable as system function parameters.
 * @details Satisfied when `SystemParamTraits<T>` provides `RegisterAccess` and
 * `Make`.
 */
template <typename T>
concept SystemParam =
    requires(AccessPolicyBuilder& builder, World& world, SystemLocalData& local,
             const AccessPolicy& policy) {
      { SystemParamTraits<std::remove_cvref_t<T>>::RegisterAccess(builder) };
      {
        SystemParamTraits<std::remove_cvref_t<T>>::Make(world, local, policy)
      } -> std::same_as<std::remove_cvref_t<T>>;
    };

}  // namespace helios::ecs
