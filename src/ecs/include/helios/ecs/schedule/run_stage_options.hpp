#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/// @brief Options controlling how `Scheduler::RunStage` executes stage members.
struct RunStageOptions {
  /**
   * @brief When true, schedules already executing on this thread are skipped.
   * @details Enables nested stage pumps (for example during OS modal event
   * loops) without re-entering the schedule that triggered the pump.
   */
  bool skip_active_schedules = false;
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
