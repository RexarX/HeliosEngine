#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/access_policy.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/ecs/system.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace helios::app::details {

/**
 * @brief Metadata about a system.
 * @details Contains static information about a system including its name, type identifier, access policy,
 * execution statistics, ordering constraints, system set membership, and run conditions.
 */
struct SystemInfo {
  std::string name;                               ///< System name (for debugging/profiling)
  ecs::SystemTypeId type_id = 0;                  ///< Unique type identifier
  app::AccessPolicy access_policy;                ///< Access policy for validation
  size_t execution_count = 0;                     ///< Number of times system has executed
  std::vector<ecs::SystemTypeId> before_systems;  ///< Systems that must run after this system
  std::vector<ecs::SystemTypeId> after_systems;   ///< Systems that must run before this system
  std::vector<SystemSetId> system_sets;           ///< System sets this system belongs to
  std::vector<size_t> run_condition_indices;      ///< Indices of run conditions in condition storage
};

}  // namespace helios::app::details
