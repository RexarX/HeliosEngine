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
#include <helios/ecs/query/args.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/world.hpp>

namespace helios::ecs::details {

/// @brief Helper to register query component access from a component tuple.
template <typename Tuple>
struct RegisterQueryAccess;

template <typename... Cs>
struct RegisterQueryAccess<std::tuple<Cs...>> {
  static constexpr void Apply(AccessPolicyBuilder& builder) {
    builder.Query<Cs...>();
  }
};

}  // namespace helios::ecs::details

HELIOS_MODULE_EXPORT
namespace helios::ecs {

template <QueryArg... Args>
struct SystemParamTraits<Query<Args...>> {
  using ParamType = Query<Args...>;
  using Split = details::QueryArgSplit<Args...>;

  static ParamType Make(World& world, SystemLocalData& data,
                        const AccessPolicy& /*policy*/) {
    return ParamType(world.Components(), &data.allocator);
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    details::RegisterQueryAccess<typename Split::Components>::Apply(builder);
  }
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
