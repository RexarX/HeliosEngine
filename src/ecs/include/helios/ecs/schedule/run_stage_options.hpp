#pragma once

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
