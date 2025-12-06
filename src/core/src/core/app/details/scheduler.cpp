#include <helios/core/app/details/scheduler.hpp>

#include <helios/core/app/details/system_diagnostics.hpp>
#include <helios/core/app/details/system_info.hpp>
#include <helios/core/app/schedule.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/async/sub_task_graph.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::app::details {

void ScheduleExecutor::Execute(ecs::World& world, async::Executor& executor, std::span<SystemStorage> system_storage) {
  HELIOS_ASSERT(graph_built_, "Failed to execute: Execution graph must be built before executing schedule!");

  if (IsMainStage()) {
    // Main stage: Execute systems sequentially on the main thread
    // Systems run in dependency order for immediate visibility
    for (const size_t index : system_indices_) {
      auto& storage = system_storage[index];
      app::SystemContext ctx(world, storage.info, executor, storage.local_storage);
      storage.system->Update(ctx);
      ++storage.info.execution_count;

      // Merge events after each system in main schedule for immediate visibility
      if (!storage.local_storage.GetEventQueue().Empty()) {
        world.MergeEventQueue(storage.local_storage.GetEventQueue());
        storage.local_storage.GetEventQueue().Clear();
      }
    }
  } else {
    // All other stages: Execute the graph asynchronously via executor
    executor.Run(execution_graph_).Wait();

    // After async execution, merge all local event queues into world's main queue
    for (const size_t index : system_indices_) {
      auto& storage = system_storage[index];
      if (!storage.local_storage.GetEventQueue().Empty()) {
        world.MergeEventQueue(storage.local_storage.GetEventQueue());
        storage.local_storage.GetEventQueue().Clear();
      }
    }
  }
}

void Scheduler::BuildAllGraphs(ecs::World& world) {
  schedule_order_.clear();

  const auto all_schedules = CollectAllScheduleIds();
  if (all_schedules.empty()) {
    return;
  }

  auto [adjacency, in_degree] = BuildScheduleDependencyGraph(all_schedules);
  schedule_order_ = TopologicalSort(all_schedules, adjacency, in_degree);

  // Build execution graphs for all schedules
  for (auto& [_, schedule] : schedules_) {
    schedule.BuildExecutionGraph(world, system_storage_, system_sets_);
  }
}

auto Scheduler::CollectAllScheduleIds() const -> std::vector<ScheduleId> {
  std::vector<ScheduleId> all_schedules;
  all_schedules.reserve(schedules_.size());
  for (const auto& [id, schedule] : schedules_) {
    all_schedules.push_back(id);
  }
  return all_schedules;
}

auto Scheduler::BuildScheduleDependencyGraph(const std::vector<ScheduleId>& all_schedules) const
    -> std::pair<std::unordered_map<ScheduleId, std::vector<ScheduleId>>, std::unordered_map<ScheduleId, size_t>> {
  std::unordered_map<ScheduleId, std::vector<ScheduleId>> adjacency;
  std::unordered_map<ScheduleId, size_t> in_degree;

  // Initialize
  for (const ScheduleId id : all_schedules) {
    adjacency[id] = {};
    in_degree[id] = 0;
  }

  // Build the dependency graph from Before/After constraints
  for (const auto& [schedule_id, ordering] : schedule_constraints_) {
    // "Before" constraints: this schedule must run before others
    for (const ScheduleId before_id : ordering.before) {
      if (schedules_.contains(before_id)) {
        adjacency[schedule_id].push_back(before_id);
        ++in_degree[before_id];
      }
    }

    // "After" constraints: this schedule must run after others
    for (const ScheduleId after_id : ordering.after) {
      if (schedules_.contains(after_id)) {
        adjacency[after_id].push_back(schedule_id);
        ++in_degree[schedule_id];
      }
    }
  }

  return {adjacency, in_degree};
}

auto Scheduler::TopologicalSort(const std::vector<ScheduleId>& all_schedules,
                                const std::unordered_map<ScheduleId, std::vector<ScheduleId>>& adjacency,
                                std::unordered_map<ScheduleId, size_t>& in_degree) -> std::vector<ScheduleId> {
  std::vector<ScheduleId> result;
  result.reserve(all_schedules.size());

  // Kahn's algorithm: collect nodes with no incoming edges
  std::vector<ScheduleId> queue;
  for (const ScheduleId id : all_schedules) {
    if (in_degree[id] == 0) {
      queue.push_back(id);
    }
  }

  // Process nodes in topological order
  while (!queue.empty()) {
    const ScheduleId current = queue.back();
    queue.pop_back();
    result.push_back(current);

    // Reduce in-degree for all dependent schedules
    for (const ScheduleId dependent : adjacency.at(current)) {
      --in_degree[dependent];
      if (in_degree[dependent] == 0) {
        queue.push_back(dependent);
      }
    }
  }

  // Check for cycles
  if (result.size() != all_schedules.size()) {
    HELIOS_ERROR("Schedule dependency cycle detected! Some schedules will not execute.");
    // Add remaining schedules anyway to avoid crashes
    for (const ScheduleId id : all_schedules) {
      if (std::ranges::find(result, id) == result.end()) {
        result.push_back(id);
      }
    }
  }

  return result;
}

void ScheduleExecutor::BuildExecutionGraph(ecs::World& world, std::span<SystemStorage> system_storage,
                                           const std::unordered_map<SystemSetId, SystemSetInfo>& system_sets) {
  if (system_indices_.empty()) {
    graph_built_ = true;
    return;
  }

  execution_graph_.Clear();

  auto [tasks, system_id_to_task_index] = CreateSystemTasks(system_storage, world);
  ApplyExplicitOrdering(tasks, system_id_to_task_index, system_storage);
  ApplySetOrdering(tasks, system_id_to_task_index, system_sets);
  ApplyAccessPolicyOrdering(tasks, system_storage);

  graph_built_ = true;
}

auto ScheduleExecutor::CreateSystemTasks(std::span<SystemStorage> system_storage, ecs::World& world)
    -> std::pair<std::vector<async::Task>, std::unordered_map<ecs::SystemTypeId, size_t>> {
  std::vector<async::Task> tasks;
  tasks.reserve(system_indices_.size());

  std::unordered_map<ecs::SystemTypeId, size_t> system_id_to_task_index;
  system_id_to_task_index.reserve(system_indices_.size());

  for (size_t i = 0; i < system_indices_.size(); ++i) {
    const size_t storage_index = system_indices_[i];

    // Capture pointer to SystemStorage instead of reference to span to avoid dangling reference
    auto* storage_ptr = &system_storage[storage_index];
    auto task = execution_graph_.EmplaceTask([storage_ptr, &world](async::SubTaskGraph& sub_graph) {
      auto& storage = *storage_ptr;
      app::SystemContext ctx(world, storage.info, sub_graph, storage.local_storage);
      storage.system->Update(ctx);
      ++storage.info.execution_count;
    });

    task.Name(storage_ptr->info.name);
    tasks.push_back(task);
    system_id_to_task_index[storage_ptr->info.type_id] = i;
  }

  return {tasks, system_id_to_task_index};
}

void ScheduleExecutor::ApplyExplicitOrdering(
    std::vector<async::Task>& tasks, const std::unordered_map<ecs::SystemTypeId, size_t>& system_id_to_task_index,
    std::span<SystemStorage> system_storage) {
  for (const auto& [system_id, ordering] : system_orderings_) {
    const auto system_it = system_id_to_task_index.find(system_id);
    if (system_it == system_id_to_task_index.end()) {
      HELIOS_WARN("Scheduling: System with type ID '{}' has ordering constraints but is not in this schedule!",
                  system_id);
      continue;
    }

    const size_t system_idx = system_it->second;
    const size_t storage_index = system_indices_[system_idx];
    const auto& storage = system_storage[storage_index];

    // Handle "before" constraints
    for (const auto& before_id : ordering.before) {
      const auto before_it = system_id_to_task_index.find(before_id);
      if (before_it != system_id_to_task_index.end()) {
        const size_t before_idx = before_it->second;
        [[maybe_unused]] const size_t before_storage_index = system_indices_[before_idx];
        tasks[system_idx].Precede(tasks[before_idx]);

        HELIOS_TRACE("Scheduling: System '{}' explicitly ordered before '{}'", storage.info.name,
                     system_storage[before_storage_index].info.name);
      } else {
        HELIOS_WARN("Scheduling: System '{}' has before constraint on non-existent system with type ID '{}'!",
                    storage.info.name, before_id);
      }
    }

    // Handle "after" constraints
    for (const auto& after_id : ordering.after) {
      const auto after_it = system_id_to_task_index.find(after_id);
      if (after_it != system_id_to_task_index.end()) {
        const size_t after_idx = after_it->second;
        [[maybe_unused]] const size_t after_storage_index = system_indices_[after_idx];
        tasks[after_idx].Precede(tasks[system_idx]);

        HELIOS_TRACE("Scheduling: System '{}' explicitly ordered after '{}'", storage.info.name,
                     system_storage[after_storage_index].info.name);
      } else {
        HELIOS_WARN("Scheduling: System '{}' has after constraint on non-existent system with type ID '{}'!",
                    storage.info.name, after_id);
      }
    }
  }
}

void ScheduleExecutor::ApplySetOrdering(std::vector<async::Task>& tasks,
                                        const std::unordered_map<ecs::SystemTypeId, size_t>& system_id_to_task_index,
                                        const std::unordered_map<SystemSetId, SystemSetInfo>& system_sets) {
  if (system_sets.empty()) [[unlikely]] {
    return;
  }

  for (const auto& [set_id, set_info] : system_sets) {
    const auto& members = set_info.member_systems;
    if (members.empty()) {
      continue;
    }

    for (const SystemSetId after_set_id : set_info.before_sets) {
      const auto after_it = system_sets.find(after_set_id);
      if (after_it == system_sets.end()) {
        continue;
      }

      const auto& after_members = after_it->second.member_systems;
      if (after_members.empty()) {
        continue;
      }

      // For every pair (a, b) where a ∈ set, b ∈ after_set and both are in this schedule: a -> b
      for (const ecs::SystemTypeId before_system_id : members) {
        const auto before_idx_it = system_id_to_task_index.find(before_system_id);
        if (before_idx_it == system_id_to_task_index.end()) {
          continue;
        }

        for (const ecs::SystemTypeId after_system_id : after_members) {
          const auto after_idx_it = system_id_to_task_index.find(after_system_id);
          if (after_idx_it == system_id_to_task_index.end()) {
            continue;
          }

          const size_t before_local_idx = before_idx_it->second;
          const size_t after_local_idx = after_idx_it->second;

          // Update explicit ordering map so set-level constraints become explicit edges
          auto& ordering = system_orderings_[before_system_id];
          if (std::ranges::find(ordering.before, after_system_id) == ordering.before.end()) {
            ordering.before.push_back(after_system_id);
          }

          tasks[before_local_idx].Precede(tasks[after_local_idx]);
        }
      }
    }
  }
}

void ScheduleExecutor::ApplyAccessPolicyOrdering(std::vector<async::Task>& tasks,
                                                 std::span<SystemStorage> system_storage) {
  // Build dependencies based on access policy conflicts
  for (size_t i = 0; i < system_indices_.size(); ++i) {
    for (size_t j = i + 1; j < system_indices_.size(); ++j) {
      const size_t storage_index_i = system_indices_[i];
      const size_t storage_index_j = system_indices_[j];
      const auto& storage_i = system_storage[storage_index_i];
      const auto& storage_j = system_storage[storage_index_j];

      const bool query_conflict = storage_i.info.access_policy.HasQueryConflict(storage_j.info.access_policy);
      const bool resource_conflict = storage_i.info.access_policy.HasResourceConflict(storage_j.info.access_policy);

      if (query_conflict || resource_conflict) {
        tasks[i].Precede(tasks[j]);

#ifdef HELIOS_DEBUG_MODE
        if (query_conflict) {
          const auto component_conflicts =
              SystemDiagnostics::AnalyzeComponentConflicts(storage_i.info.access_policy, storage_j.info.access_policy);
          const std::string conflict_details = SystemDiagnostics::FormatComponentConflicts(
              storage_i.info.name, storage_j.info.name, component_conflicts);
          HELIOS_TRACE("Scheduling: System '{}' must run before '{}' due to component conflicts:\n{}",
                       storage_i.info.name, storage_j.info.name, conflict_details);
        }

        if (resource_conflict) {
          const auto resource_conflicts =
              SystemDiagnostics::AnalyzeResourceConflicts(storage_i.info.access_policy, storage_j.info.access_policy);
          const std::string conflict_details =
              SystemDiagnostics::FormatResourceConflicts(storage_i.info.name, storage_j.info.name, resource_conflicts);
          HELIOS_TRACE("Scheduling: System '{}' must run before '{}' due to resource conflicts:\n{}",
                       storage_i.info.name, storage_j.info.name, conflict_details);
        }
#else
        HELIOS_TRACE("Scheduling: System '{}' must run before '{}' due to data conflict", storage_i.info.name,
                     storage_j.info.name);
#endif
      }
    }
  }

  [[maybe_unused]] const size_t system_count = system_indices_.size();
  HELIOS_DEBUG("Built execution graph with {} {}", system_count, system_count == 1 ? "system" : "systems");
}

}  // namespace helios::app::details
