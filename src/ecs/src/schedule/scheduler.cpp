#include <pch.hpp>

#include <helios/ecs/schedule/scheduler.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/executor/multi_threaded.hpp>
#include <helios/ecs/schedule/executor/single_threaded.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/log/logger.hpp>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <vector>

namespace helios::ecs {

namespace {

struct TopoNode {
  std::vector<size_t> after;
  std::vector<size_t> before;
};

[[nodiscard]] auto TopoSortHashes(
    const std::vector<size_t>& hashes,
    const std::unordered_map<size_t, TopoNode>& nodes) -> std::vector<size_t> {
  const size_t size = hashes.size();
  if (size == 0) [[unlikely]] {
    return {};
  }

  std::unordered_map<size_t, size_t> hash_to_idx;
  hash_to_idx.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    hash_to_idx[hashes[i]] = i;
  }

  std::vector<std::vector<size_t>> adjacency(size);
  std::vector<size_t> in_degree(size, 0);

  for (size_t i = 0; i < size; ++i) {
    const auto& entry = nodes.at(hashes[i]);

    for (const size_t after_hash : entry.after) {
      if (const auto it = hash_to_idx.find(after_hash);
          it != hash_to_idx.end()) {
        adjacency[it->second].push_back(i);
        ++in_degree[i];
      }
    }

    for (const size_t before_hash : entry.before) {
      if (const auto it = hash_to_idx.find(before_hash);
          it != hash_to_idx.end()) {
        adjacency[i].push_back(it->second);
        ++in_degree[it->second];
      }
    }
  }

  std::queue<size_t> queue;
  for (size_t i = 0; i < size; ++i) {
    if (in_degree[i] == 0) {
      queue.push(i);
    }
  }

  std::vector<size_t> order;
  order.reserve(size);

  while (!queue.empty()) {
    const size_t current = queue.front();
    queue.pop();
    order.push_back(hashes[current]);

    for (size_t succ : adjacency[current]) {
      if (--in_degree[succ] == 0) {
        queue.push(succ);
      }
    }
  }

  if (order.size() < size) {
    log::Warn(
        "Cycle detected in schedule ordering; {} items will execute in "
        "arbitrary order",
        size - order.size());
    for (size_t i = 0; i < size; ++i) {
      if (std::ranges::find(order, hashes[i]) == order.end()) {
        order.push_back(hashes[i]);
      }
    }
  }

  return order;
}

}  // namespace

void Scheduler::Run(World& world, Executor& executor) {
  HELIOS_ASSERT(!is_dirty_,
                "Scheduler::Run called but scheduler is dirty! "
                "Call Build() first.");

  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Scheduler::Run");
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  for (const size_t schedule_hash : execution_order_cache_) {
    const auto it = schedules_.find(schedule_hash);
    if (it == schedules_.end()) {
      continue;
    }

    it->second.schedule.RunAndWait(world, executor);
  }
}

void Scheduler::Run(World& world) {
  HELIOS_ASSERT(!is_dirty_,
                "Scheduler::Run called but scheduler is dirty! "
                "Call Scheduler::Build first.");

  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Scheduler::Run");
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  for (const size_t schedule_hash : execution_order_cache_) {
    const auto it = schedules_.find(schedule_hash);
    if (it == schedules_.end()) {
      continue;
    }

    it->second.schedule.RunAndWait(world);
  }
}

void Scheduler::RunStage(StageTypeIndex stage, World& world) {
  HELIOS_ASSERT(!is_dirty_,
                "Scheduler::RunStage called but scheduler is dirty! "
                "Call Scheduler::Build first.");

  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Scheduler::RunStage");
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  const size_t stage_hash = stage.Hash();
  HELIOS_ASSERT(stages_.contains(stage_hash),
                "Stage with hash '{}' is not registered!", stage_hash);

  const auto members_it = stage_member_order_.find(stage_hash);
  if (members_it == stage_member_order_.end()) {
    return;
  }

  for (const size_t member_hash : members_it->second) {
    const auto it = schedules_.find(member_hash);
    if (it == schedules_.end()) [[unlikely]] {
      continue;
    }

    it->second.schedule.RunAndWait(world);
  }
}

void Scheduler::RunStage(StageTypeIndex stage, World& world,
                         Executor& executor) {
  HELIOS_ASSERT(!is_dirty_,
                "Scheduler::RunStage called but scheduler is dirty! "
                "Call Scheduler::Build first.");

  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Scheduler::RunStage");
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  const size_t stage_hash = stage.Hash();
  HELIOS_ASSERT(stages_.contains(stage_hash),
                "Stage with hash '{}' is not registered!", stage_hash);

  const auto members_it = stage_member_order_.find(stage_hash);
  if (members_it == stage_member_order_.end()) {
    return;
  }

  for (const size_t member_hash : members_it->second) {
    const auto it = schedules_.find(member_hash);
    if (it == schedules_.end()) [[unlikely]] {
      continue;
    }

    it->second.schedule.RunAndWait(world, executor);
  }
}

void Scheduler::BuildImpl(async::Executor* async_executor) {
  for (auto& [hash, entry] : schedules_) {
    if (entry.schedule.GetExecutor() == nullptr) {
      auto kind = entry.schedule.Settings().executor_kind;
      if (async_executor == nullptr && kind != ExecutorKind::kMainThread) {
        kind = ExecutorKind::kMainThread;
      }
      entry.schedule.SetExecutor(CreateExecutor(kind, async_executor));
    }

    const auto result = entry.schedule.Build();
    HELIOS_ASSERT(result.has_value(), "Failed to build scheduler: {}!",
                  result.error().message);
  }

  const auto ordering_result = ValidateOrdering();
  HELIOS_ASSERT(ordering_result.has_value(),
                "Failed to validate schedule ordering: {}!",
                ordering_result.error().message);

  ComputeExecutionOrder();
  MarkClean();
}

auto Scheduler::ValidateOrdering() -> ScheduleResult<void> {
  for (const auto& [hash, entry] : schedules_) {
    if (entry.stage_hash.has_value() &&
        !stages_.contains(entry.stage_hash.value())) {
      return std::unexpected(ScheduleError{
          .kind = ScheduleErrorKind::kUnknown,
          .message = std::format(
              "Schedule '{}' is assigned to unregistered stage hash '{}'",
              entry.name, entry.stage_hash.value()),
          .involved_systems = {}});
    }

    const auto validate_schedule_target =
        [&](size_t other_hash) -> ScheduleResult<void> {
      const auto other_it = schedules_.find(other_hash);
      if (other_it == schedules_.end()) [[unlikely]] {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknown,
            .message =
                std::format("Unknown schedule referenced by '{}'", entry.name),
            .involved_systems = {}});
      }

      const ScheduleEntry& other = other_it->second;

      if (entry.stage_hash != other.stage_hash) {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknown,
            .message = std::format(
                "Schedule '{}' cannot depend on schedule '{}' from a "
                "different stage",
                entry.name, other.name),
            .involved_systems = {}});
      }

      return {};
    };

    for (const size_t after_hash : entry.after_schedules) {
      if (auto result = validate_schedule_target(after_hash); !result) {
        return result;
      }
    }
    for (const size_t before_hash : entry.before_schedules) {
      if (auto result = validate_schedule_target(before_hash); !result) {
        return result;
      }
    }
  }

  for (const auto& [hash, entry] : stages_) {
    const auto validate_stage_target =
        [&](size_t other_hash) -> ScheduleResult<void> {
      if (!stages_.contains(other_hash)) [[unlikely]] {
        return std::unexpected(ScheduleError{
            .kind = ScheduleErrorKind::kUnknown,
            .message =
                std::format("Unknown stage referenced by '{}'", entry.name),
            .involved_systems = {}});
      }
      return {};
    };

    for (const size_t after_hash : entry.after_stages) {
      if (auto result = validate_stage_target(after_hash); !result) {
        return result;
      }
    }
    for (const size_t before_hash : entry.before_stages) {
      if (auto result = validate_stage_target(before_hash); !result) {
        return result;
      }
    }
  }

  return {};
}

void Scheduler::ComputeExecutionOrder() {
  stage_order_.clear();
  stage_member_order_.clear();
  execution_order_cache_.clear();

  if (schedules_.empty() && stages_.empty()) [[unlikely]] {
    return;
  }

  std::unordered_map<size_t, TopoNode> schedule_topo_nodes;
  schedule_topo_nodes.reserve(schedules_.size());
  for (const auto& [hash, entry] : schedules_) {
    schedule_topo_nodes.emplace(hash,
                                TopoNode{.after = entry.after_schedules,
                                         .before = entry.before_schedules});
  }

  std::unordered_map<size_t, TopoNode> stage_topo_nodes;
  stage_topo_nodes.reserve(stages_.size());
  for (const auto& [hash, entry] : stages_) {
    stage_topo_nodes.emplace(hash, TopoNode{.after = entry.after_stages,
                                            .before = entry.before_stages});
  }

  std::vector<size_t> stage_hashes;
  stage_hashes.reserve(stages_.size());
  for (const auto& [hash, entry] : stages_) {
    stage_hashes.push_back(hash);
  }

  stage_order_ = TopoSortHashes(stage_hashes, stage_topo_nodes);

  for (const size_t stage_hash : stage_order_) {
    std::vector<size_t> members;
    members.reserve(schedules_.size());

    for (const auto& [hash, entry] : schedules_) {
      if (entry.stage_hash.has_value() &&
          entry.stage_hash.value() == stage_hash) {
        members.push_back(hash);
      }
    }

    stage_member_order_[stage_hash] =
        TopoSortHashes(members, schedule_topo_nodes);
    execution_order_cache_.insert(execution_order_cache_.end(),
                                  stage_member_order_[stage_hash].begin(),
                                  stage_member_order_[stage_hash].end());
  }

  std::vector<size_t> ungrouped_hashes;
  ungrouped_hashes.reserve(schedules_.size());
  for (const auto& [hash, entry] : schedules_) {
    if (!entry.stage_hash.has_value()) {
      ungrouped_hashes.push_back(hash);
    }
  }

  const std::vector<size_t> ungrouped_order =
      TopoSortHashes(ungrouped_hashes, schedule_topo_nodes);
  execution_order_cache_.insert(execution_order_cache_.end(),
                                ungrouped_order.begin(), ungrouped_order.end());
}

auto Scheduler::CreateExecutor(ExecutorKind kind,
                               async::Executor* async_executor)
    -> std::unique_ptr<Executor> {
  switch (kind) {
    using enum ExecutorKind;
    case kMainThread:
      return std::make_unique<MainThreadExecutor>();
    case kSingleThreaded: {
      HELIOS_ASSERT(async_executor != nullptr,
                    "Scheduler: cannot create SingleThreadedExecutor without "
                    "an async::Executor! Call Build(async::Executor&).");
      return std::make_unique<SingleThreadedExecutor>(*async_executor);
    }
    case kMultiThreaded: {
      HELIOS_ASSERT(async_executor != nullptr,
                    "Scheduler: cannot create MultiThreadedExecutor without "
                    "an async::Executor! Call Build(async::Executor&).");
      return std::make_unique<MultiThreadedExecutor>(*async_executor);
    }
  }

  HELIOS_ASSERT(false, "Scheduler: unknown ExecutorKind!");
  return nullptr;
}

}  // namespace helios::ecs
