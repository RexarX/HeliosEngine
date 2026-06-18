#include <pch.hpp>

#include <helios/ecs/schedule/executor/main_thread.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/schedule.hpp>

#include <cstddef>

namespace helios::ecs {

void MainThreadExecutor::Execute(Schedule& schedule, World& world) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MainThreadExecutor::Execute");
  HELIOS_ASSERT(schedule.plan_.has_value(),
                "MainThreadExecutor: schedule has no compiled plan!");

  const auto& plan = *schedule.plan_;
  const auto& execution_order = plan.execution_order;
  for (size_t exec_idx = 0; exec_idx < execution_order.size(); ++exec_idx) {
    const SystemId id = execution_order[exec_idx];

    const auto it = plan.system_id_to_entry_idx.find(id.id);
    if (it == plan.system_id_to_entry_idx.end()) {
      continue;
    }

    auto& entries = schedule.system_entries_;
    const size_t entry_idx = it->second;
    HELIOS_ASSERT(entry_idx < entries.size(),
                  "System index out of bounds in MainThreadExecutor!");

    auto& entry = entries[entry_idx];
    auto& storage = entry.storage;

    auto& conditions = schedule.conditions_cache_;
    if (exec_idx < conditions.size()) {
      bool should_run = true;
      for (RunConditionStorage* cond_storage : conditions[exec_idx]) {
        HELIOS_ECS_PROFILE_SCOPE();
        HELIOS_ECS_PROFILE_ZONE_NAME(cond_storage->name);
        if (!cond_storage->run_condition(world, cond_storage->local_data)) {
          should_run = false;
          break;
        }
      }
      if (!should_run) {
        continue;
      }
    }

    HELIOS_ECS_PROFILE_ZONE_NAME(storage.name);
    storage.system(world, storage.local_data);
  }
}

}  // namespace helios::ecs
