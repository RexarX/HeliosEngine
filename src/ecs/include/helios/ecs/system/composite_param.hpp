#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/world.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class AccessPolicy;
class AccessPolicyBuilder;

/**
 * @brief Helper for implementing `SystemParamTraits` on aggregate parameters.
 * @details Inherit from this type in a `SystemParamTraits` specialization when
 * every field is itself a system parameter. Field types in `Fields...` must
 * match the aggregate's member types exactly. Include the headers that define
 * `SystemParamTraits` for each field (for example `query/params.hpp` and
 * `message/params.hpp`) before this specialization is instantiated.
 * @tparam Param Aggregate parameter type
 * @tparam Fields System parameter types for each aggregate field, in order
 */
template <typename Param, typename... Fields>
struct CompositeSystemParam {
  /**
   * @brief Constructs the aggregate from each field's `Make`.
   * @param world World instance
   * @param data Per-system local data
   * @param policy Compiled access policy for the system
   * @return Constructed aggregate parameter
   */
  static Param Make(World& world, SystemLocalData& data,
                    const AccessPolicy& policy) {
    return Param{SystemParamTraits<Fields>::Make(world, data, policy)...};
  }

  /**
   * @brief Registers access for all fields.
   * @param builder Access policy builder
   */
  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    RegisterParamAccess<Fields...>(builder);
  }
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
