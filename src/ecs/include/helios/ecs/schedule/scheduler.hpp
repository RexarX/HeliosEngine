#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#if defined(HELIOS_ECS_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#include <helios/memory/temporary_storage_helpers.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#endif
#endif
#include <helios/assert.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/schedule/run_stage_options.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/stage.hpp>
#include <helios/ecs/schedule/stage_settings.hpp>

HELIOS_MODULE_EXPORT
namespace helios::async {

class Executor;

}

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class Scheduler;
class Executor;

/// @brief Handle returned when configuring stage ordering.
class StageOrdering {
public:
  StageOrdering(Scheduler& scheduler, size_t hash) noexcept
      : scheduler_(scheduler), hash_(hash) {}

  StageOrdering(Scheduler& scheduler, StageTypeIndex index) noexcept
      : StageOrdering(scheduler, index.Hash()) {}

  template <StageTrait T>
  explicit StageOrdering(const T& stage, Scheduler& scheduler) noexcept
      : StageOrdering(scheduler, StageTypeIndex::From(stage)) {}

  StageOrdering(StageOrdering&&) noexcept = default;
  StageOrdering(const StageOrdering&) = delete;
  ~StageOrdering() noexcept = default;

  StageOrdering& operator=(const StageOrdering&) = delete;
  StageOrdering& operator=(StageOrdering&&) noexcept = default;

  auto After(this auto&& self, StageTypeIndex index)
      -> decltype(std::forward<decltype(self)>(self));

  template <StageTrait T>
  auto After(this auto&& self, const T& stage = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(
        StageTypeIndex::From(stage));
  }

  auto Before(this auto&& self, StageTypeIndex index)
      -> decltype(std::forward<decltype(self)>(self));

  template <StageTrait T>
  auto Before(this auto&& self, const T& stage = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(
        StageTypeIndex::From(stage));
  }

  Scheduler& Done() noexcept { return scheduler_.get(); }

  [[nodiscard]] StageSettings& Settings() noexcept;

  [[nodiscard]] size_t Hash() const noexcept { return hash_; }

private:
  std::reference_wrapper<Scheduler> scheduler_;
  size_t hash_ = 0;
};

/// @brief Handle returned when adding a schedule to the scheduler for
/// configuring ordering.
class ScheduleOrdering {
public:
  ScheduleOrdering(Scheduler& scheduler, size_t hash) noexcept
      : scheduler_(scheduler), hash_(hash) {}

  ScheduleOrdering(Scheduler& scheduler, ScheduleTypeIndex index) noexcept
      : ScheduleOrdering(scheduler, index.Hash()) {}

  template <ScheduleTrait T>
  explicit ScheduleOrdering(const T& schedule, Scheduler& scheduler) noexcept
      : ScheduleOrdering(scheduler, ScheduleTypeIndex::From(schedule)) {}

  ScheduleOrdering(ScheduleOrdering&&) noexcept = default;
  ScheduleOrdering(const ScheduleOrdering&) = delete;
  ~ScheduleOrdering() noexcept = default;

  ScheduleOrdering& operator=(const ScheduleOrdering&) = delete;
  ScheduleOrdering& operator=(ScheduleOrdering&&) noexcept = default;

  auto After(this auto&& self, ScheduleTypeIndex index)
      -> decltype(std::forward<decltype(self)>(self));

  template <ScheduleTrait T>
  auto After(this auto&& self, const T& schedule = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(
        ScheduleTypeIndex::From(schedule));
  }

  auto Before(this auto&& self, ScheduleTypeIndex index)
      -> decltype(std::forward<decltype(self)>(self));

  template <ScheduleTrait T>
  auto Before(this auto&& self, const T& schedule = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(
        ScheduleTypeIndex::From(schedule));
  }

  auto InStage(this auto&& self, StageTypeIndex stage)
      -> decltype(std::forward<decltype(self)>(self));

  template <StageTrait T>
  auto InStage(this auto&& self, const T& stage = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).InStage(
        StageTypeIndex::From(stage));
  }

  Scheduler& Done() noexcept { return scheduler_.get(); }

private:
  [[nodiscard]] size_t Hash() const noexcept { return hash_; }

  std::reference_wrapper<Scheduler> scheduler_;
  size_t hash_ = 0;

  friend class Scheduler;
};

/**
 * @brief Orchestrates multiple named schedules, handles transitions, and
 * executes them in topological order.
 * @details Schedules are identified by types (empty structs satisfying
 * `ScheduleTrait`). Stages group schedules via `InStage` and are ordered with
 * `OrderStage`. When `Run()` is called, all schedules are executed in
 * topological order; system-local data (commands, messages) is flushed at each
 * schedule boundary.
 */
class Scheduler {
public:
  Scheduler() = default;
  Scheduler(const Scheduler&) = delete;
  Scheduler(Scheduler&&) noexcept = default;
  ~Scheduler() = default;

  Scheduler& operator=(const Scheduler&) = delete;
  Scheduler& operator=(Scheduler&&) noexcept = default;

  void Build();
  void Build(async::Executor& executor);

  void Run(World& world, Executor& executor);
  void Run(World& world);

  void RunStage(StageTypeIndex stage, World& world,
                RunStageOptions options = {});
  void RunStage(StageTypeIndex stage, World& world, Executor& executor,
                RunStageOptions options = {});

  template <StageTrait T>
  void RunStage(const T& stage, World& world, RunStageOptions options = {});

  template <StageTrait T>
  void RunStage(const T& stage, World& world, Executor& executor,
                RunStageOptions options = {});

  /**
   * @brief Applies deferred work for every schedule in a stage with explicit
   * flags.
   * @details Used after `RunStage` when stage settings request additional
   * command flush / message merge. Does **not** advance message buffers.
   * @param stage Stage whose member schedules to drain
   * @param world World to apply against
   * @param apply_commands Whether to flush and execute commands
   * @param merge_messages Whether to merge local messages
   */
  void ApplyStageDeferred(StageTypeIndex stage, World& world,
                          bool apply_commands, bool merge_messages);

  /**
   * @brief Applies deferred work for every schedule in a stage with explicit
   * flags.
   * @details Used after `RunStage` when stage settings request additional
   * command flush / message merge. Does **not** advance message buffers.
   * @param stage Stage whose member schedules to drain
   * @param world World to apply against
   * @param apply_commands Whether to flush and execute commands
   * @param merge_messages Whether to merge local messages
   */
  template <StageTrait T>
  void ApplyStageDeferred(const T& stage, World& world, bool apply_commands,
                          bool merge_messages);

  void Clear();

  [[nodiscard]] ScheduleOrdering Order(ScheduleTypeIndex index);

  template <ScheduleTrait T>
  [[nodiscard]] ScheduleOrdering Order(const T& schedule = {}) {
    return Order(ScheduleTypeIndex::From(schedule));
  }

  [[nodiscard]] StageOrdering OrderStage(StageTypeIndex index);

  template <StageTrait T>
  [[nodiscard]] StageOrdering OrderStage(const T& stage = {}) {
    return OrderStage(StageTypeIndex::From(stage));
  }

  Schedule& In(size_t schedule_hash);
  Schedule& In(ScheduleTypeIndex index) { return In(index.Hash()); }

  template <ScheduleTrait T>
  Schedule& In(const T& schedule = {});

  ScheduleOrdering Add(ScheduleTypeId id, Schedule&& schedule);

  template <ScheduleTrait T>
  ScheduleOrdering Add(const T& schedule, ecs::Schedule&& sched) {
    return Add(ScheduleTypeId::From(schedule), std::move(sched));
  }

  StageOrdering AddStage(StageTypeId id);

  template <StageTrait T>
  StageOrdering AddStage(const T& stage = {}) {
    return AddStage(StageTypeId::From(stage));
  }

  bool Remove(ScheduleTypeIndex index);

  template <ScheduleTrait T>
  bool Remove(const T& schedule = {}) {
    return Remove(ScheduleTypeIndex::From(schedule));
  }

  [[nodiscard]] Schedule* TryGetSchedule(ScheduleTypeIndex index);
  [[nodiscard]] const Schedule* TryGetSchedule(ScheduleTypeIndex index) const;

  template <ScheduleTrait T>
  [[nodiscard]] Schedule* TryGetSchedule(const T& schedule = {}) {
    return TryGetSchedule(ScheduleTypeIndex::From(schedule));
  }

  template <ScheduleTrait T>
  [[nodiscard]] const Schedule* TryGetSchedule(const T& schedule = {}) const {
    return TryGetSchedule(ScheduleTypeIndex::From(schedule));
  }

  [[nodiscard]] bool HasStage(StageTypeIndex index) const noexcept {
    return stages_.contains(index.Hash());
  }

  template <StageTrait T>
  [[nodiscard]] bool HasStage(const T& stage = {}) const noexcept {
    return HasStage(StageTypeIndex::From(stage));
  }

  [[nodiscard]] StageSettings& GetStageSettings(StageTypeIndex index) {
    return GetStageEntry(index.Hash()).settings;
  }

  template <StageTrait T>
  [[nodiscard]] StageSettings& GetStageSettings(const T& stage = {}) {
    return GetStageSettings(StageTypeIndex::From(stage));
  }

  [[nodiscard]] const StageSettings& GetStageSettings(
      StageTypeIndex index) const;

  template <StageTrait T>
  [[nodiscard]] const StageSettings& GetStageSettings(
      const T& stage = {}) const {
    return GetStageSettings(StageTypeIndex::From(stage));
  }

  [[nodiscard]] bool IsDirty() const noexcept;

private:
  struct ScheduleEntry {
    Schedule schedule;
    std::vector<size_t> after_schedules;
    std::vector<size_t> before_schedules;
    std::string_view name;
    std::optional<size_t> stage_hash;
  };

  struct StageEntry {
    std::vector<size_t> after_stages;
    std::vector<size_t> before_stages;
    std::string_view name;
    StageSettings settings;
  };

  [[nodiscard]] ScheduleEntry& GetEntry(size_t hash);
  [[nodiscard]] const ScheduleEntry& GetEntry(size_t hash) const;
  [[nodiscard]] StageEntry& GetStageEntry(size_t hash);

  void MarkDirty() noexcept { is_dirty_ = true; }
  void MarkClean() noexcept { is_dirty_ = false; }

  void BuildImpl(async::Executor* async_executor);

  [[nodiscard]] auto ValidateOrdering() -> ScheduleResult<void>;
  void ComputeExecutionOrder();

  [[nodiscard]] auto CreateExecutor(ExecutorKind kind,
                                    async::Executor* async_executor)
      -> std::unique_ptr<Executor>;

  std::unordered_map<size_t, ScheduleEntry> schedules_;
  std::unordered_map<size_t, StageEntry> stages_;
  std::vector<size_t> execution_order_cache_;
  std::vector<size_t> stage_order_;
  std::unordered_map<size_t, std::vector<size_t>> stage_member_order_;
  bool is_dirty_ = true;

  friend class ScheduleOrdering;
  friend class StageOrdering;
};

inline void Scheduler::Build() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Scheduler::Build");
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  BuildImpl(nullptr);
}

inline void Scheduler::Build(async::Executor& executor) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Scheduler::Build");
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  BuildImpl(&executor);
}

template <StageTrait T>
inline void Scheduler::RunStage(const T& stage, World& world,
                                RunStageOptions options) {
  HELIOS_ECS_PROFILE_SCOPE();
  HELIOS_ECS_PROFILE_ZONE_NAME(utils::TempFormat(
      "helios::ecs::Scheduler::RunStage{{name: {}}}", StageNameOf(stage)));
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  RunStage(StageTypeIndex::From(stage), world, options);
}

template <StageTrait T>
inline void Scheduler::RunStage(const T& stage, World& world,
                                Executor& executor, RunStageOptions options) {
  HELIOS_ECS_PROFILE_SCOPE();
  HELIOS_ECS_PROFILE_ZONE_NAME(utils::TempFormat(
      "helios::ecs::Scheduler::RunStage{{name: {}}}", StageNameOf(stage)));
  HELIOS_ECS_PROFILE_ZONE_VALUE(schedules_.size());

  RunStage(StageTypeIndex::From(stage), world, executor, options);
}

inline Schedule& Scheduler::In(size_t schedule_hash) {
  const auto it = schedules_.find(schedule_hash);
  HELIOS_ASSERT(it != schedules_.end(),
                "Schedule with hash '{}' does not exist!", schedule_hash);
  return it->second.schedule;
}

template <ScheduleTrait T>
inline Schedule& Scheduler::In(const T& schedule) {
  const auto it = schedules_.find(ScheduleTypeIndex::From(schedule).Hash());
  HELIOS_ASSERT(it != schedules_.end(), "Schedule '{}' does not exist!",
                ScheduleNameOf(schedule));
  return it->second.schedule;
}

inline ScheduleOrdering Scheduler::Order(ScheduleTypeIndex index) {
  HELIOS_ASSERT(schedules_.contains(index.Hash()),
                "Schedule with hash '{}' does not exist!", index.Hash());
  return {*this, index.Hash()};
}

inline StageOrdering Scheduler::OrderStage(StageTypeIndex index) {
  HELIOS_ASSERT(stages_.contains(index.Hash()),
                "Stage with hash '{}' does not exist!", index.Hash());
  return {*this, index.Hash()};
}

inline StageSettings& StageOrdering::Settings() noexcept {
  return scheduler_.get().GetStageEntry(hash_).settings;
}

inline Schedule* Scheduler::TryGetSchedule(ScheduleTypeIndex index) {
  const auto it = schedules_.find(index.Hash());
  if (it == schedules_.end()) {
    return nullptr;
  }
  return &it->second.schedule;
}

inline const Schedule* Scheduler::TryGetSchedule(
    ScheduleTypeIndex index) const {
  const auto it = schedules_.find(index.Hash());
  if (it == schedules_.end()) {
    return nullptr;
  }
  return &it->second.schedule;
}

inline const StageSettings& Scheduler::GetStageSettings(
    StageTypeIndex index) const {
  HELIOS_ASSERT(stages_.contains(index.Hash()),
                "Stage with hash '{}' not found!", index.Hash());
  return stages_.at(index.Hash()).settings;
}

inline auto StageOrdering::Before(this auto&& self, StageTypeIndex index)
    -> decltype(std::forward<decltype(self)>(self)) {
  const size_t other_hash = index.Hash();
  auto& scheduler = self.scheduler_.get();
  auto& entry = scheduler.GetStageEntry(self.Hash());

  if (std::ranges::find(entry.before_stages, other_hash) ==
      entry.before_stages.end()) {
    entry.before_stages.push_back(other_hash);
  }
  scheduler.MarkDirty();
  return std::forward<decltype(self)>(self);
}

inline auto StageOrdering::After(this auto&& self, StageTypeIndex index)
    -> decltype(std::forward<decltype(self)>(self)) {
  const size_t other_hash = index.Hash();
  auto& scheduler = self.scheduler_.get();
  auto& entry = scheduler.GetStageEntry(self.Hash());

  if (std::ranges::find(entry.after_stages, other_hash) ==
      entry.after_stages.end()) {
    entry.after_stages.push_back(other_hash);
  }
  scheduler.MarkDirty();
  return std::forward<decltype(self)>(self);
}

inline auto ScheduleOrdering::After(this auto&& self, ScheduleTypeIndex index)
    -> decltype(std::forward<decltype(self)>(self)) {
  const size_t other_hash = index.Hash();
  auto& scheduler = self.scheduler_.get();
  auto& entry = scheduler.GetEntry(self.Hash());

  if (std::ranges::find(entry.after_schedules, other_hash) ==
      entry.after_schedules.end()) {
    entry.after_schedules.push_back(other_hash);
  }
  scheduler.MarkDirty();
  return std::forward<decltype(self)>(self);
}

inline auto ScheduleOrdering::Before(this auto&& self, ScheduleTypeIndex index)
    -> decltype(std::forward<decltype(self)>(self)) {
  const size_t other_hash = index.Hash();
  auto& scheduler = self.scheduler_.get();
  auto& entry = scheduler.GetEntry(self.Hash());

  if (std::ranges::find(entry.before_schedules, other_hash) ==
      entry.before_schedules.end()) {
    entry.before_schedules.push_back(other_hash);
  }
  scheduler.MarkDirty();
  return std::forward<decltype(self)>(self);
}

inline auto ScheduleOrdering::InStage(this auto&& self, StageTypeIndex stage)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& scheduler = self.scheduler_.get();
  auto& entry = scheduler.GetEntry(self.Hash());
  entry.stage_hash = stage.Hash();
  scheduler.MarkDirty();
  return std::forward<decltype(self)>(self);
}

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
