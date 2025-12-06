#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/system_set.hpp>
#include <helios/core/ecs/system.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace helios::app::details {

/**
 * @brief Metadata about a system set.
 * @details Contains static information about a system set including its name, type identifier,
 * ordering constraints relative to other sets, run conditions, and member systems.
 */
struct SystemSetInfo {
  std::string name;                               ///< System set name (for debugging/profiling)
  SystemSetId id = 0;                             ///< Unique type identifier
  std::vector<SystemSetId> before_sets;           ///< System sets that must run after this set
  std::vector<SystemSetId> after_sets;            ///< System sets that must run before this set
  std::vector<size_t> run_condition_indices;      ///< Indices of run conditions in condition storage
  std::vector<ecs::SystemTypeId> member_systems;  ///< Systems that belong to this set
};

}  // namespace helios::app::details
