#include <pch.hpp>

#include <helios/ecs/schedule/executor/multi_threaded.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/schedule.hpp>

#include <cstddef>
#include <cstdint>

namespace helios::ecs {

void MultiThreadedExecutor::BuildGraph(Schedule& schedule, World& world) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MultiThreadedExecutor::Execute");

  HELIOS_ASSERT(schedule.plan_.has_value(),
                "MultiThreadedExecutor: schedule has no compiled plan!");

  const auto& plan = *schedule.plan_;
  const auto& execution_order = plan.execution_order;
  if (execution_order.empty()) {
    return;
  }

  const size_t size = execution_order.size();

  task_graph_.Clear();
  tasks_.clear();
  tasks_.reserve(size);

  for (size_t i = 0; i < size; ++i) {
    tasks_.push_back(task_graph_.CreatePlaceholder());
  }

  for (size_t exec_idx = 0; exec_idx < size; ++exec_idx) {
    const SystemId id = execution_order[exec_idx];

    const auto it = plan.system_id_to_entry_idx.find(id.id);
    if (it == plan.system_id_to_entry_idx.end()) {
      tasks_[exec_idx].Work([]() {});
      tasks_[exec_idx].Name("SyncPoint");
      continue;
    }

    auto& entries = schedule.system_entries_;
    const size_t entry_idx = it->second;
    HELIOS_ASSERT(entry_idx < entries.size(),
                  "System index out of bounds in MultiThreadedExecutor!");

    tasks_[exec_idx].Work([&world, &schedule, entry_idx, exec_idx]() {
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
          return;
        }
      }

      auto& storage = schedule.system_entries_[entry_idx].storage;
      HELIOS_ECS_PROFILE_SCOPE();
      HELIOS_ECS_PROFILE_ZONE_NAME(storage.name);
      storage.system(world, storage.local_data);
    });

    auto& storage = entries[entry_idx].storage;
    tasks_[exec_idx].Name(storage.name);
  }

  if (size <= 64) {
    for (size_t i = 0; i < size; ++i) {
      uint64_t mask = plan.conflicts.bitmask[i];
      while (mask != 0) {
        const int j = std::countr_zero(mask);
        mask &= mask - 1;
        if (static_cast<size_t>(j) > i) {
          tasks_[i].Precede(tasks_[static_cast<size_t>(j)]);
        }
      }
    }
  } else {
    for (size_t i = 0; i < size; ++i) {
      for (size_t j = i + 1; j < size; ++j) {
        if (plan.conflicts.matrix[i][j]) {
          tasks_[i].Precede(tasks_[j]);
        }
      }
    }
  }
}

void MultiThreadedExecutor::Execute(Schedule& schedule, World& world) {
  BuildGraph(schedule, world);
  future_ = executor_.get().Run(task_graph_);
}

void MultiThreadedExecutor::ExecuteAndWait(Schedule& schedule, World& world) {
  BuildGraph(schedule, world);

  if (executor_.get().IsWorkerThread()) {
    executor_.get().CoRun(task_graph_);
    return;
  }

  executor_.get().Run(task_graph_).Wait();
}

}  // namespace helios::ecs
