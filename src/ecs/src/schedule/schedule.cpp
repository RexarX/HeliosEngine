#include <pch.hpp>

#include <helios/ecs/schedule/schedule.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/log/logger.hpp>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <iterator>
#include <ranges>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs {

namespace {

[[nodiscard]] constexpr ScheduleErrorKind DagToScheduleErrorKind(
    DagErrorKind kind) noexcept {
  switch (kind) {
    case DagErrorKind::kCycleDetected:
      return ScheduleErrorKind::kCycleDetected;
    case DagErrorKind::kUnknownNode:
      return ScheduleErrorKind::kUnknownNode;
    default:
      return ScheduleErrorKind::kUnknown;
  }
}

[[nodiscard]] constexpr ScheduleError DagToScheduleError(
    DagError error) noexcept {
  return ScheduleError{.kind = DagToScheduleErrorKind(error.kind),
                       .message = std::move(error.message),
                       .involved_systems = std::move(error.involved_nodes)};
}

template <typename T, std::ranges::input_range R>
void AppendTargetRange(std::vector<T>& targets, R&& range) {
  if constexpr (requires { targets.append_range(range); }) {
    targets.append_range(std::forward<R>(range));
  } else {
    targets.insert(targets.end(), std::ranges::begin(range),
                   std::ranges::end(range));
  }
}

}  // namespace

void Schedule::Run(World& world, Executor& executor) {
  HELIOS_ASSERT(!is_dirty_,
                "Schedule::Run called but schedule is dirty! Call "
                "Schedule::Build first.");
  HELIOS_ASSERT(plan_.has_value(),
                "Schedule::Run called but no plan exists! Call Build first.");
  HELIOS_ASSERT(
      !HasPendingLocalData(),
      "Schedule '{}' still has unapplied commands or messages from a prior "
      "Run(). Call ApplyDeferred(world) or use RunAndWait() before running "
      "again.",
      name_);

  HELIOS_ECS_PROFILE_SCOPE();
  HELIOS_ECS_PROFILE_ZONE_NAME(
      std::format("helios::ecs::Schedule::Run{{name: {}}}", name_));
  HELIOS_ECS_PROFILE_ZONE_VALUE(plan_->execution_order.size());

  for (auto& entry : system_entries_) {
    entry.storage.local_data.ResetArena();
  }

  executor.Execute(*this, world);
}

void Schedule::RunAndWait(World& world, Executor& executor) {
  HELIOS_ASSERT(!is_dirty_,
                "Schedule::RunAndWait called but schedule is dirty! Call "
                "Schedule::Build first.");
  HELIOS_ASSERT(plan_.has_value(),
                "Schedule::RunAndWait called but no plan exists! Call "
                "Schedule::Build first.");
  HELIOS_ASSERT(
      !HasPendingLocalData(),
      "Schedule '{}' still has unapplied commands or messages from a prior "
      "Run(). Call ApplyDeferred(world) before RunAndWait().",
      name_);

  HELIOS_ECS_PROFILE_SCOPE();
  HELIOS_ECS_PROFILE_ZONE_NAME(
      std::format("helios::ecs::Schedule::RunAndWait{{name: {}}}", name_));
  HELIOS_ECS_PROFILE_ZONE_VALUE(plan_->execution_order.size());

  for (auto& entry : system_entries_) {
    entry.storage.local_data.ResetArena();
  }

  executor.ExecuteAndWait(*this, world);
  ApplyDeferred(world);
}

void Schedule::ApplyDeferred(World& world) {
  HELIOS_ECS_PROFILE_SCOPE();
  HELIOS_ECS_PROFILE_ZONE_NAME(
      std::format("helios::ecs::Schedule::ApplyDeferred{{name: {}}}", name_));

  world.Flush();
  for (auto& entry : system_entries_) {
    entry.storage.local_data.Update(world);
  }
}

auto Schedule::Build() -> ScheduleResult<void> {
  if (!is_dirty_ && plan_.has_value()) {
    return {};
  }

  HELIOS_ECS_PROFILE_SCOPE();
  HELIOS_ECS_PROFILE_ZONE_NAME(
      std::format("helios::ecs::Schedule::Build{{name: {}}}", name_));
  HELIOS_ECS_PROFILE_ZONE_VALUE(system_entries_.size());

  if (auto result = ResolveSetReferences(); !result) [[unlikely]] {
    return std::unexpected(std::move(result.error()));
  }

  auto dag_result = BuildDag();
  if (!dag_result) [[unlikely]] {
    return std::unexpected(std::move(dag_result.error()));
  }

  auto sort_result = dag_result->Sort();
  if (!sort_result) [[unlikely]] {
    return std::unexpected(DagToScheduleError(sort_result.error()));
  }

  WarnAmbiguousAccess(*dag_result);

  CompiledPlan plan;
  plan.execution_order = std::move(*sort_result);

  for (size_t i = 0; i < system_entries_.size(); ++i) {
    plan.system_id_to_entry_idx[system_entries_[i].storage.id.id] = i;
  }

  BuildConflictData(plan);

  plan_ = std::move(plan);

  MergeRunConditions(conditions_cache_, plan_->execution_order);

  is_dirty_ = false;
  return {};
}

void Schedule::Clear() {
  system_entries_.clear();
  sets_.clear();
  conditions_cache_.clear();
  plan_.reset();
  next_anonymous_group_id_ = 1;
  is_dirty_ = true;
  ++generation_;
}

size_t Schedule::AddEntry(SystemStorage&& storage) {
  const size_t index = system_entries_.size();

  for (const auto& entry : system_entries_) {
    if (entry.storage.id == storage.id) {
      storage.id = SystemId::From(std::format("{}#{}", storage.name, index));
      break;
    }
  }

  system_entries_.push_back(SystemEntry{
      .storage = std::move(storage), .metadata = {}, .is_sync_point = false});
  MarkDirty();
  return index;
}

auto Schedule::BuildDag() -> ScheduleResult<Dag> {
  Dag dag;

  for (size_t i = 0; i < system_entries_.size(); ++i) {
    const SystemId id = system_entries_[i].storage.id;
    dag.AddNode(id);
  }

  for (size_t i = 0; i < system_entries_.size(); ++i) {
    const auto& entry = system_entries_[i];
    const SystemId from_id = entry.storage.id;

    for (const SystemId target : entry.metadata.before_targets) {
      if (auto result = dag.AddEdge(from_id, target); !result) [[unlikely]] {
        return std::unexpected(DagToScheduleError(std::move(result.error())));
      }
    }

    for (const SystemId target : entry.metadata.after_targets) {
      if (auto result = dag.AddEdge(target, from_id); !result) [[unlikely]] {
        return std::unexpected(DagToScheduleError(std::move(result.error())));
      }
    }
  }

  return dag;
}

auto Schedule::ResolveSetReferences() -> ScheduleResult<void> {
  std::unordered_map<size_t, std::vector<SystemId>> set_members;
  for (const auto& entry : system_entries_) {
    for (const SystemSetId set_id : entry.metadata.member_of_sets) {
      set_members[set_id.id].push_back(entry.storage.id);
    }
  }

  const auto resolve_set =
      [this, &set_members](
          SystemSetId set_id) -> std::optional<std::span<const SystemId>> {
    if (!sets_.contains(set_id.id)) [[unlikely]] {
      return std::nullopt;
    }

    const auto it = set_members.find(set_id.id);
    if (it == set_members.end()) {
      return std::span<const SystemId>{};
    }
    return std::span<const SystemId>{it->second};
  };

  for (auto& entry : system_entries_) {
    for (const SystemSetId set_id : entry.metadata.before_set_targets) {
      const auto members = resolve_set(set_id);
      if (!members) [[unlikely]] {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknownSet,
            .message = std::format("Unknown set referenced by system '{}'",
                                   entry.storage.name),
            .involved_systems = {entry.storage.id}});
      }
      AppendTargetRange(entry.metadata.before_targets, *members);
    }
    entry.metadata.before_set_targets.clear();

    for (const SystemSetId set_id : entry.metadata.after_set_targets) {
      const auto members = resolve_set(set_id);
      if (!members) [[unlikely]] {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknownSet,
            .message = std::format("Unknown set referenced by system '{}'",
                                   entry.storage.name),
            .involved_systems = {entry.storage.id}});
      }
      AppendTargetRange(entry.metadata.after_targets, *members);
    }
    entry.metadata.after_set_targets.clear();
  }

  for (auto& entry : system_entries_) {
    for (const SystemSetId set_id : entry.metadata.member_of_sets) {
      const auto set_it = sets_.find(set_id.id);
      if (set_it == sets_.end()) {
        continue;
      }

      const auto& before = set_it->second.BeforeTargets();
      const auto& after = set_it->second.AfterTargets();
      const auto& before_sets = set_it->second.BeforeSetTargets();
      const auto& after_sets = set_it->second.AfterSetTargets();

      AppendTargetRange(entry.metadata.before_targets, before);
      AppendTargetRange(entry.metadata.after_targets, after);
      AppendTargetRange(entry.metadata.before_set_targets, before_sets);
      AppendTargetRange(entry.metadata.after_set_targets, after_sets);
    }
  }

  for (auto& entry : system_entries_) {
    for (const SystemSetId set_id : entry.metadata.before_set_targets) {
      const auto members = resolve_set(set_id);
      if (!members) [[unlikely]] {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknownSet,
            .message = std::format("Unknown set referenced by system '{}'",
                                   entry.storage.name),
            .involved_systems = {entry.storage.id}});
      }
      AppendTargetRange(entry.metadata.before_targets, *members);
    }
    entry.metadata.before_set_targets.clear();

    for (const SystemSetId set_id : entry.metadata.after_set_targets) {
      const auto members = resolve_set(set_id);
      if (!members) [[unlikely]] {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknownSet,
            .message = std::format("Unknown set referenced by system '{}'",
                                   entry.storage.name),
            .involved_systems = {entry.storage.id}});
      }
      AppendTargetRange(entry.metadata.after_targets, *members);
    }
    entry.metadata.after_set_targets.clear();
  }

  ApplySequenceOrdering(set_members);

  return {};
}

void Schedule::ApplySequenceOrdering(
    const std::unordered_map<size_t, std::vector<SystemId>>& set_members) {
  for (const auto& [set_id, set] : sets_) {
    if (!set.IsSequence()) {
      continue;
    }

    const auto members_it = set_members.find(set_id);
    if (members_it == set_members.end() || members_it->second.size() < 2) {
      continue;
    }

    const auto& members = members_it->second;
    for (size_t i = 1; i < members.size(); ++i) {
      for (auto& entry : system_entries_) {
        if (entry.storage.id == members[i]) {
          entry.metadata.AddAfter(members[i - 1]);
          break;
        }
      }
    }
  }
}

void Schedule::WarnAmbiguousAccess(const Dag& dag) {
  for (size_t i = 0; i < system_entries_.size(); ++i) {
    for (size_t j = i + 1; j < system_entries_.size(); ++j) {
      const auto& policy_i = system_entries_[i].storage.access_policy;
      const auto& policy_j = system_entries_[j].storage.access_policy;

      if (!policy_i.ConflictsWith(policy_j)) {
        continue;
      }

      const SystemId id_i = system_entries_[i].storage.id;
      const SystemId id_j = system_entries_[j].storage.id;

      if (dag.Reachable(id_i, id_j) || dag.Reachable(id_j, id_i)) {
        continue;
      }

      log::Warn("Ambiguous access between '{}' and '{}'!",
                system_entries_[i].storage.name,
                system_entries_[j].storage.name);
    }
  }
}

void Schedule::BuildConflictData(CompiledPlan& plan) {
  const size_t size = plan.execution_order.size();

  if (size <= 64) {
    plan.conflicts.bitmask.assign(size, 0);

    for (size_t i = 0; i < size; ++i) {
      const SystemId id_i = plan.execution_order[i];
      const auto it_i = plan.system_id_to_entry_idx.find(id_i.id);
      if (it_i == plan.system_id_to_entry_idx.end()) {
        continue;
      }

      const auto& policy_i =
          system_entries_[it_i->second].storage.access_policy;

      for (size_t j = i + 1; j < size; ++j) {
        const SystemId id_j = plan.execution_order[j];
        const auto it_j = plan.system_id_to_entry_idx.find(id_j.id);
        if (it_j == plan.system_id_to_entry_idx.end()) {
          continue;
        }

        const auto& policy_j =
            system_entries_[it_j->second].storage.access_policy;

        if (policy_i.ConflictsWith(policy_j)) {
          plan.conflicts.bitmask[i] |= (1ULL << j);
          plan.conflicts.bitmask[j] |= (1ULL << i);
        }
      }
    }
  } else {
    plan.conflicts.matrix.assign(size, std::vector<bool>(size, false));

    for (size_t i = 0; i < size; ++i) {
      const SystemId id_i = plan.execution_order[i];
      const auto it_i = plan.system_id_to_entry_idx.find(id_i.id);
      if (it_i == plan.system_id_to_entry_idx.end()) {
        continue;
      }

      const auto& policy_i =
          system_entries_[it_i->second].storage.access_policy;

      for (size_t j = i + 1; j < size; ++j) {
        const SystemId id_j = plan.execution_order[j];
        const auto it_j = plan.system_id_to_entry_idx.find(id_j.id);
        if (it_j == plan.system_id_to_entry_idx.end()) {
          continue;
        }

        const auto& policy_j =
            system_entries_[it_j->second].storage.access_policy;

        const bool conflict = policy_i.ConflictsWith(policy_j);
        plan.conflicts.matrix[i][j] = conflict;
        plan.conflicts.matrix[j][i] = conflict;
      }
    }
  }
}

void Schedule::MergeRunConditions(
    std::vector<std::vector<RunConditionStorage*>>& out_conditions,
    const std::vector<SystemId>& execution_order) {
  out_conditions.clear();
  out_conditions.resize(execution_order.size());

  HELIOS_ASSERT(plan_.has_value(),
                "MergeRunConditions: schedule has no compiled plan!");

  for (size_t exec_idx = 0; exec_idx < execution_order.size(); ++exec_idx) {
    const SystemId id = execution_order[exec_idx];
    const auto it = plan_->system_id_to_entry_idx.find(id.id);
    if (it == plan_->system_id_to_entry_idx.end()) {
      continue;
    }

    auto& entry = system_entries_[it->second];
    auto& conditions = out_conditions[exec_idx];

    for (auto& condition : entry.metadata.conditions) {
      conditions.push_back(&condition);
    }

    for (const SystemSetId set_id : entry.metadata.member_of_sets) {
      const auto set_it = sets_.find(set_id.id);
      if (set_it == sets_.end()) {
        continue;
      }

      for (auto& condition : set_it->second.Conditions()) {
        conditions.push_back(const_cast<RunConditionStorage*>(&condition));
      }
    }
  }
}

}  // namespace helios::ecs
