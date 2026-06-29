#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/schedule/dag.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/schedule/system_storage.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>
#include <helios/utils/type_info.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#if defined(HELIOS_ECS_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#include <format>
#endif

#include <helios/ecs/schedule/system_group_handle.hpp>
#include <helios/ecs/schedule/system_handle.hpp>
#include <helios/ecs/schedule/system_set_handle.hpp>

namespace helios::ecs {

/// @brief Type index for schedules.
using ScheduleTypeIndex = utils::TypeIndex;

/// @brief Type id for schedules.
using ScheduleTypeId = utils::TypeId;

/**
 * @brief Concept for schedules.
 * @details A schedule type must be an object and be empty.
 */
template <typename T>
concept ScheduleTrait = std::is_object_v<std::remove_cvref_t<T>> &&
                        std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for schedules that provide a name.
 * @details A schedule with name trait must satisfy `ScheduleTrait` and provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept ScheduleWithNameTrait = ScheduleTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Schedule name for debugging and serialization.
 * @tparam T Schedule type
 * @param instance Schedule instance
 * @return Schedule name
 */
template <ScheduleTrait T>
[[nodiscard]] constexpr std::string_view ScheduleNameOf(
    const T& /*instance*/ = {}) noexcept {
  if constexpr (ScheduleWithNameTrait<T>) {
    return std::remove_cvref_t<T>::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

class Schedule;

/// @brief Metadata attached to a system entry for ordering and configuration.
struct ScheduleSystemMetadata {
  std::vector<SystemId> before_targets;
  std::vector<SystemId> after_targets;
  std::vector<SystemSetId> before_set_targets;
  std::vector<SystemSetId> after_set_targets;
  std::vector<SystemSetId> member_of_sets;
  std::vector<RunConditionStorage> conditions;

  /**
   * @brief Adds a system ID to the list of targets that must run before this
   * system.
   * @details If the system ID is already present, it is not added again.
   * @param id The system ID to add
   */
  constexpr void AddBefore(SystemId id) { AppendUnique(before_targets, id); }

  /**
   * @brief Adds a system ID to the list of targets that must run after this
   * system.
   * @details If the system ID is already present, it is not added again.
   * @param id The system ID to add
   */
  constexpr void AddAfter(SystemId id) { AppendUnique(after_targets, id); }

  /**
   * @brief Adds a system set ID to the list of targets that must run before
   * this system.
   * @details If the system set ID is already present, it is not added again.
   * @param id The system set ID to add
   */
  constexpr void AddBefore(SystemSetId id) {
    AppendUnique(before_set_targets, id);
  }

  /**
   * @brief Adds a system set ID to the list of targets that must run after this
   * system.
   * @details If the system set ID is already present, it is not added again.
   * @param id The system set ID to add
   */
  constexpr void AddAfter(SystemSetId id) {
    AppendUnique(after_set_targets, id);
  }

  /**
   * @brief Adds a system set membership if not already present.
   * @details If the set id is already present, it is not added again.
   * @param id Set to assign this system to
   */
  constexpr void AddMemberOfSet(SystemSetId id) {
    AppendUnique(member_of_sets, id);
  }

  /**
   * @brief Appends a value to a vector if it is not already present.
   * @tparam T The type of the value
   * @param vec The vector to append to
   * @param value The value to append
   */
  template <typename T>
  static void AppendUnique(std::vector<T>& vec, const T& value) {
    if (std::ranges::find(vec, value) == vec.end()) {
      vec.push_back(value);
    }
  }
};

/// @brief Settings that control how a schedule is compiled and executed.
struct ScheduleSettings {
  ExecutorKind executor_kind = ExecutorKind::kMultiThreaded;
};

/// @brief Kind of error produced during schedule compilation.
enum class ScheduleErrorKind : uint8_t {
  kUnknown,
  kCycleDetected,
  kAmbiguousAccess,
  kUnknownSet,
  kUnknownNode,
};

/// @brief Error type returned when schedule compilation fails.
struct ScheduleError {
  ScheduleErrorKind kind;
  std::string message;
  std::vector<SystemId> involved_systems;
};

/// @brief Result type for schedule operations that can fail.
template <typename T>
using ScheduleResult = std::expected<T, ScheduleError>;

/**
 * @brief A collection of systems, sets, and run conditions with ordering
 * constraints.
 * @details Compile the schedule with `Build()` to produce an executable plan,
 * then call `Run()` with an executor to execute it. Adding, removing, or
 * modifying any configuration invalidates the plan and requires recompilation.
 */
class Schedule {
public:
  Schedule() = default;

  /**
   * @brief Constructs a schedule with the given name.
   * @param name Human-readable schedule name
   */
  explicit Schedule(std::string name) : name_(std::move(name)) {}

  Schedule(const Schedule&) = delete;
  Schedule(Schedule&&) noexcept = default;
  ~Schedule() = default;

  Schedule& operator=(const Schedule&) = delete;
  Schedule& operator=(Schedule&&) noexcept = default;

  /**
   * @brief Creates a schedule named after the given label type.
   * @tparam T Schedule type satisfying `ScheduleTrait`
   * @param schedule Schedule type instance
   * @return Schedule with name derived from the type
   */
  template <ScheduleTrait T>
  [[nodiscard]] static Schedule From(const T& schedule = {}) {
    return Schedule(std::string(ScheduleNameOf(schedule)));
  }

  /**
   * @brief Submits the compiled schedule for execution using the provided
   * executor.
   * @details Resets per-system arenas and submits systems for execution. Does
   * **not** call `World::Flush()` or `SystemLocalData::Update`. When execution
   * finishes (`Executor::Wait()` on the same executor), call
   * `ApplyDeferred(world)` before the next `Run()` — otherwise pending commands
   * and messages are discarded on the next arena reset.
   * @warning Triggers assertion if the schedule has not been built, is dirty,
   * or still has unapplied local data from a prior `Run()`.
   * @param world World to pass to each system
   * @param executor Executor strategy to use
   */
  void Run(World& world, Executor& executor);

  /**
   * @brief Submits the compiled schedule for execution using the stored
   * executor.
   * @details Same semantics as `Run(world, executor)`; see that overload.
   * @warning Triggers assertion if the schedule has not been built, is dirty,
   * no executor has been set, or still has unapplied local data from a prior
   * `Run()`.
   * @param world World to pass to each system
   */
  void Run(World& world);

  /**
   * @brief Executes the compiled schedule and blocks until completion.
   * @details Equivalent to `Run()`, waiting on the executor, then
   * `ApplyDeferred(world)`.
   * @warning Triggers assertion if the schedule has not been built or is dirty.
   * @param world World to pass to each system
   * @param executor Executor strategy to use
   */
  void RunAndWait(World& world, Executor& executor);

  /**
   * @brief Executes the compiled schedule using the stored executor and blocks
   * until completion.
   * @details Same semantics as `RunAndWait(world, executor)`; see that
   * overload.
   * @warning Triggers assertion if the schedule has not been built, is dirty,
   * or no executor has been set.
   * @param world World to pass to each system
   */
  void RunAndWait(World& world);

  /**
   * @brief Flushes world state and applies all pending system-local data.
   * @details Calls `World::Flush()` (entity reservations and
   * `World::EnqueueCommand` queue), then runs `SystemLocalData::Update` for
   * every system in this schedule. Required after `Run()` once the executor has
   * finished; called automatically by `RunAndWait()`.
   * @param world World to flush and update
   */
  void ApplyDeferred(World& world);

  /**
   * @brief Compiles the schedule into an executable plan.
   * @details Validates for cycles, unknown references, and ambiguous access.
   * Produces a topological order and conflict matrix for parallel execution.
   * @return Empty on success, or a `ScheduleError` with diagnostics
   */
  [[nodiscard]] auto Build() -> ScheduleResult<void>;

  /**
   * @brief Clears all systems, sets, and compiled state from this schedule.
   * @details Preserves `Settings()` and the stored executor for reuse.
   */
  void Clear();

  /**
   * @brief Adds a param-style functor system to the schedule.
   * @details Auto-deduces the access policy from the system's parameter types
   * and derives the system name and `SystemId` from the functor type.
   * Lambdas are rejected; use `Add(std::string, T&&, ...)` to register them
   * with an explicit name.
   * @tparam T System type satisfying `FunctorSystemTrait`
   * @param system System instance
   * @param options Local data options
   * @return Handle for further configuration
   */
  template <FunctorSystemTrait T>
  SystemHandle Add(T&& system, SystemLocalDataOptions options = {});

  /**
   * @brief Adds a param-style system to the schedule with an explicit name.
   * @details Auto-deduces the access policy from the system's parameter types
   * and derives the `SystemId` from the provided name. Required for lambdas
   * and recommended when multiple instances of the same functor type need
   * distinct identities within a single schedule.
   * @tparam T System type satisfying `SystemTrait`
   * @param name Human-readable system name
   * @param system System instance
   * @param options Local data options
   * @return Handle for further configuration
   */
  template <SystemTrait T>
  SystemHandle Add(std::string name, T&& system,
                   SystemLocalDataOptions options = {});

  /**
   * @brief Adds multiple param-style functor systems as a system group.
   * @details Each system is added to the schedule and assigned to a unique
   * anonymous group. The returned handle configures all members collectively
   * and can assign them to a named set via `InSet`. Lambdas must be registered
   * individually with explicit names.
   * @tparam Ts System types satisfying `FunctorSystemTrait`
   * @param systems System instances
   * @return Handle for configuring the system group
   */
  template <FunctorSystemTrait... Ts>
    requires(sizeof...(Ts) > 1)
  SystemGroupHandle Add(Ts&&... systems);

  /**
   * @brief Adds a callable system to the schedule with explicit name and access
   * policy.
   * @param name System name
   * @param system Callable that accepts `(World&, SystemLocalData&)`
   * @param policy Access policy for the system
   * @param options Local data options
   * @return Handle for further configuration
   */
  SystemHandle Add(std::string name, SystemCallable system,
                   AccessPolicy policy = {},
                   SystemLocalDataOptions options = {});

  /**
   * @brief Gets or creates a system set with the given identifier.
   * @param id Set identifier
   * @return Handle for configuring the set
   */
  [[nodiscard]] SystemSetHandle Set(SystemSetId id);

  /**
   * @brief Gets or creates a system set with the given label type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Handle for configuring the set
   */
  template <SystemSetTrait T>
  [[nodiscard]] SystemSetHandle Set(const T& /*set*/ = {}) {
    return Set(SystemSetId::From<T>());
  }

  /**
   * @brief Sets the executor for this schedule.
   * @param executor Executor to use when Run/AndWait is called without an
   * explicit executor
   */
  void SetExecutor(std::unique_ptr<Executor> executor) noexcept {
    executor_ = std::move(executor);
  }

  /**
   * @brief Sets the human-readable schedule name.
   * @param name Schedule name
   */
  void SetName(std::string name) noexcept { name_ = std::move(name); }

  /**
   * @brief Checks whether the schedule requires recompilation.
   * @return True if any configuration has changed since the last `Build()`
   */
  [[nodiscard]] bool IsDirty() const noexcept { return is_dirty_; }

  /**
   * @brief Checks whether any system still has unapplied local commands or
   * messages.
   * @return True if `ApplyDeferred` is still required before the next `Run()`
   */
  [[nodiscard]] bool HasPendingLocalData() const noexcept {
    return std::ranges::any_of(system_entries_, [](const auto& entry) {
      return entry.storage.local_data.HasPendingWork();
    });
  }

  /**
   * @brief Gets the human-readable schedule name.
   * @return Schedule name
   */
  [[nodiscard]] const std::string& GetName() const noexcept { return name_; }

  /**
   * @brief Gets scheduler-local settings for this schedule.
   * @return Mutable reference to the settings
   */
  [[nodiscard]] ScheduleSettings& Settings() noexcept { return settings_; }

  /**
   * @brief Gets scheduler-local settings for this schedule.
   * @return Const reference to the settings
   */
  [[nodiscard]] const ScheduleSettings& Settings() const noexcept {
    return settings_;
  }

  /**
   * @brief Gets the stored executor.
   * @return Pointer to the stored executor, or nullptr if not set
   */
  [[nodiscard]] Executor* GetExecutor() noexcept { return executor_.get(); }

  /**
   * @brief Gets the stored executor.
   * @return Const pointer to the stored executor, or nullptr if not set
   */
  [[nodiscard]] const Executor* GetExecutor() const noexcept {
    return executor_.get();
  }

private:
  /// @brief Entry stored for each system added to a schedule.
  struct SystemEntry {
    SystemStorage storage;
    ScheduleSystemMetadata metadata;
    bool is_sync_point = false;
  };

  /**
   * @brief Holds the conflict data between systems in the compiled plan.
   *
   * @details Two representations are maintained:
   * - A dense `uint64_t` bitmask per system when `system_count <= 64` (fast
   *   path, avoids heap indirection on every executor dispatch).
   * - A full `vector<vector<bool>>` matrix for larger schedules.
   *
   * Callers always use `Conflicts(i, j)` and never inspect the fields
   * directly, so the switch between representations is invisible at use sites.
   */
  struct ConflictData {
    /// Active when system_count <= 64; bitmask[i] has bit j set iff i and j
    /// conflict.
    std::vector<uint64_t> bitmask;
    /// Active when system_count > 64; empty when bitmask is in use.
    std::vector<std::vector<bool>> matrix;

    /// Returns true when systems at indices `i` and `j` have a
    /// data-access conflict and no explicit ordering constraint between them.
    [[nodiscard]] constexpr bool Conflicts(size_t i, size_t j) const noexcept {
      if (!bitmask.empty()) {
        return static_cast<bool>((bitmask[i] >> j) & 1U);
      }
      return matrix[i][j];
    }
  };

  /// @brief The result of a successful schedule compilation.
  struct CompiledPlan {
    std::vector<SystemId> execution_order;
    /// Maps SystemId::id -> index in system_entries_ for O(1) lookup.
    std::unordered_map<size_t, size_t> system_id_to_entry_idx;
    ConflictData conflicts;
  };

  size_t AddEntry(SystemStorage&& storage);

  [[nodiscard]] auto ResolveSetReferences() -> ScheduleResult<void>;

  [[nodiscard]] auto BuildDag() -> ScheduleResult<Dag>;
  void BuildConflictData(CompiledPlan& plan);

  void MergeRunConditions(
      std::vector<std::vector<RunConditionStorage*>>& out_conditions,
      const std::vector<SystemId>& execution_order);

  void ApplySequenceOrdering(
      const std::unordered_map<size_t, std::vector<SystemId>>& set_members);

  void WarnAmbiguousAccess(const Dag& dag);

  /**
   * @brief Gets or creates a system set entry in the schedule.
   * @details Marks the schedule dirty when a new set is created.
   * @param set_id Set identifier
   */
  void EnsureSet(SystemSetId set_id);

  /**
   * @brief Assigns a system to a set if it is not already a member.
   * @details Creates the target set if needed and marks the schedule dirty only
   * when membership changes.
   * @param slot Index of the system entry to update
   * @param set_id Set to assign the system to
   * @return True if membership was added
   */
  bool AddSystemToSet(size_t slot, SystemSetId set_id);

  /**
   * @brief Assigns every member of an anonymous group to a named set.
   * @details Creates the target set if needed. Each matching member is updated
   * through `AddSystemToSet`.
   * @param group_id Anonymous group id returned by variadic `Add`
   * @param target_set_id Named set to assign group members to
   */
  void AssignGroupToSet(SystemSetId group_id, SystemSetId target_set_id);

  /**
   * @brief Allocates a unique id for an anonymous system group.
   * @details Each variadic `Add` call receives a distinct id so batches with
   * the same system types do not share configuration.
   * @return New anonymous group id
   */
  [[nodiscard]] SystemSetId AllocateAnonymousGroupId() noexcept;

  /// @brief Bumps the generation counter and marks the schedule dirty.
  void MarkDirty() noexcept {
    is_dirty_ = true;
    ++generation_;
  }

  /**
   * @brief Returns the system entry for the system at `slot`.
   * @warning Triggers asserion if `slot` is out of bounds.
   * @param slot Index of the system entry to retrieve
   * @return Reference to the system entry at the given slot
   */
  [[nodiscard]] SystemEntry& GetSystemEntry(size_t slot) {
    HELIOS_ASSERT(slot < system_entries_.size(), "Invalid system slot '{}'!",
                  slot);
    return system_entries_[slot];
  }

  /**
   * @brief Returns the system set with the given id.
   * @warning Triggers assertion if the set is not found.
   * @param id System set id
   * @return Reference to the system set with the given id
   */
  [[nodiscard]] SystemSet& GetSystemSet(size_t id) {
    const auto it = sets_.find(id);
    HELIOS_ASSERT(it != sets_.end(), "Unknown system set '{}'!", id);
    return it->second;
  }

  std::vector<SystemEntry> system_entries_;
  std::unordered_map<size_t, SystemSet> sets_;
  std::optional<CompiledPlan> plan_;
  std::vector<std::vector<RunConditionStorage*>> conditions_cache_;

  std::string name_;
  ScheduleSettings settings_;

  std::unique_ptr<Executor> executor_;

  size_t generation_ = 0;  ///< Bumped on every structural modification
  size_t next_anonymous_group_id_ = 1;
  bool is_dirty_ = true;

  friend class MainThreadExecutor;
  friend class SingleThreadedExecutor;
  friend class MultiThreadedExecutor;
  friend class SystemHandle;
  friend class SystemSetHandle;
  friend class SystemGroupHandle;
};

inline void Schedule::Run(World& world) {
  HELIOS_ASSERT(executor_ != nullptr,
                "Schedule::Run() called but no executor is set! "
                "Use SetExecutor() before running.");

  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Schedule::Run");
  HELIOS_ECS_PROFILE_ZONE_NAME(
      std::format("helios::ecs::Schedule::Run{{name: {}}}", name_));
  HELIOS_ECS_PROFILE_ZONE_VALUE(
      plan_.has_value() ? plan_->execution_order.size() : 0U);

  Run(world, *executor_);
}

inline void Schedule::RunAndWait(World& world) {
  HELIOS_ASSERT(executor_ != nullptr,
                "Schedule::RunAndWait() called but no executor is set! "
                "Use SetExecutor() before running.");

  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::Schedule::RunAndWait");
  HELIOS_ECS_PROFILE_ZONE_NAME(
      std::format("helios::ecs::Schedule::RunAndWait{{name: {}}}", name_));
  HELIOS_ECS_PROFILE_ZONE_VALUE(
      plan_.has_value() ? plan_->execution_order.size() : 0U);

  RunAndWait(world, *executor_);
}

template <FunctorSystemTrait T>
inline SystemHandle Schedule::Add(T&& system, SystemLocalDataOptions options) {
  const size_t slot =
      AddEntry(SystemStorage::FromParam(std::forward<T>(system), options));
  return SystemHandle(ScheduleSystemId{.id = system_entries_[slot].storage.id,
                                       .slot = slot,
                                       .generation = generation_},
                      *this);
}

template <SystemTrait T>
inline SystemHandle Schedule::Add(std::string name, T&& system,
                                  SystemLocalDataOptions options) {
  const size_t slot = AddEntry(SystemStorage::FromParamNamed(
      std::move(name), std::forward<T>(system), options));
  return SystemHandle(ScheduleSystemId{.id = system_entries_[slot].storage.id,
                                       .slot = slot,
                                       .generation = generation_},
                      *this);
}

template <FunctorSystemTrait... Ts>
  requires(sizeof...(Ts) > 1)
inline SystemGroupHandle Schedule::Add(Ts&&... systems) {
  std::array handles = {Add(std::forward<Ts>(systems))...};

  const SystemSetId group_id = AllocateAnonymousGroupId();
  EnsureSet(group_id);

  for (SystemHandle& handle : handles) {
    AddSystemToSet(handle.Id().slot, group_id);
  }

  return {group_id, *this};
}

inline SystemHandle Schedule::Add(std::string name, SystemCallable system,
                                  AccessPolicy policy,
                                  SystemLocalDataOptions options) {
  const size_t slot = AddEntry(SystemStorage::From(
      std::move(name), std::move(system), std::move(policy), options));
  return SystemHandle(ScheduleSystemId{.id = system_entries_[slot].storage.id,
                                       .slot = slot,
                                       .generation = generation_},
                      *this);
}

inline SystemSetHandle Schedule::Set(SystemSetId id) {
  EnsureSet(id);
  MarkDirty();
  return {id, *this};
}

inline void Schedule::EnsureSet(SystemSetId set_id) {
  if (sets_.contains(set_id.id)) {
    return;
  }

  sets_.emplace(set_id.id, SystemSet(set_id));
  MarkDirty();
}

inline bool Schedule::AddSystemToSet(size_t slot, SystemSetId set_id) {
  EnsureSet(set_id);

  auto& entry = GetSystemEntry(slot);
  const size_t before = entry.metadata.member_of_sets.size();
  entry.metadata.AddMemberOfSet(set_id);
  if (entry.metadata.member_of_sets.size() != before) {
    MarkDirty();
    return true;
  }
  return false;
}

inline void Schedule::AssignGroupToSet(SystemSetId group_id,
                                       SystemSetId target_set_id) {
  EnsureSet(target_set_id);

  for (size_t slot = 0; slot < system_entries_.size(); ++slot) {
    const auto& entry = system_entries_[slot];
    if (std::ranges::find(entry.metadata.member_of_sets, group_id) !=
        entry.metadata.member_of_sets.end()) {
      AddSystemToSet(slot, target_set_id);
    }
  }
}

inline SystemSetId Schedule::AllocateAnonymousGroupId() noexcept {
  static constexpr size_t kAnonymousGroupIdTag = static_cast<size_t>(1) << 63;
  return SystemSetId{.id = kAnonymousGroupIdTag ^ next_anonymous_group_id_++};
}

constexpr auto SystemHandle::Before(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.AddBefore(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemHandle::Before(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.AddBefore(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemHandle::Before(this auto&& self,
                                    const SystemSetHandle& target)
    -> decltype(std::forward<decltype(self)>(self)) {
  return std::forward<decltype(self)>(self).Before(target.Id());
}

constexpr auto SystemHandle::Before(this auto&& self,
                                    const SystemGroupHandle& target)
    -> decltype(std::forward<decltype(self)>(self)) {
  return std::forward<decltype(self)>(self).Before(target.Id());
}

constexpr auto SystemHandle::After(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.AddAfter(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemHandle::After(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.AddAfter(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemHandle::After(this auto&& self,
                                   const SystemSetHandle& target)
    -> decltype(std::forward<decltype(self)>(self)) {
  return std::forward<decltype(self)>(self).After(target.Id());
}

constexpr auto SystemHandle::After(this auto&& self,
                                   const SystemGroupHandle& target)
    -> decltype(std::forward<decltype(self)>(self)) {
  return std::forward<decltype(self)>(self).After(target.Id());
}

constexpr auto SystemHandle::RunIf(this auto&& self, RunCondition condition,
                                   AccessPolicy policy,
                                   SystemLocalDataOptions options)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.conditions.push_back(RunConditionStorage::From(
      std::move(condition), std::move(policy), options));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

template <FunctorSystemTrait T>
constexpr auto SystemHandle::RunIf(this auto&& self, T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.conditions.push_back(
      RunConditionStorage::FromParam(std::forward<T>(condition)));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

template <SystemTrait T>
constexpr auto SystemHandle::RunIf(this auto&& self, std::string name,
                                   T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& entry = schedule.GetSystemEntry(self.id_.slot);
  entry.metadata.conditions.push_back(RunConditionStorage::FromParamNamed(
      std::move(name), std::forward<T>(condition)));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemHandle::InSet(this auto&& self, SystemSetId set_id)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();
  schedule.AddSystemToSet(self.id_.slot, set_id);
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSetHandle::Before(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.Before(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSetHandle::Before(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.Before(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSetHandle::Before(this auto&& self,
                                       const SystemGroupHandle& target)
    -> decltype(std::forward<decltype(self)>(self)) {
  return std::forward<decltype(self)>(self).Before(target.Id());
}

constexpr auto SystemSetHandle::After(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.After(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSetHandle::After(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.After(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSetHandle::After(this auto&& self,
                                      const SystemGroupHandle& target)
    -> decltype(std::forward<decltype(self)>(self)) {
  return std::forward<decltype(self)>(self).After(target.Id());
}

constexpr auto SystemSetHandle::RunIf(this auto&& self, RunCondition condition,
                                      AccessPolicy policy,
                                      SystemLocalDataOptions options)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.RunIf(RunConditionStorage::From(std::move(condition),
                                            std::move(policy), options));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

template <FunctorSystemTrait T>
constexpr auto SystemSetHandle::RunIf(this auto&& self, T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.RunIf(RunConditionStorage::FromParam(std::forward<T>(condition)));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

template <SystemTrait T>
constexpr auto SystemSetHandle::RunIf(this auto&& self, std::string name,
                                      T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.RunIf(RunConditionStorage::FromParamNamed(
      std::move(name), std::forward<T>(condition)));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSetHandle::Sequence(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.Sequence();
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::Before(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.Before(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::Before(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.Before(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::After(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.After(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::After(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.After(target);
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::RunIf(this auto&& self,
                                        RunCondition condition,
                                        AccessPolicy policy,
                                        SystemLocalDataOptions options)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.RunIf(RunConditionStorage::From(std::move(condition),
                                            std::move(policy), options));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

template <FunctorSystemTrait T>
constexpr auto SystemGroupHandle::RunIf(this auto&& self, T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.RunIf(RunConditionStorage::FromParam(std::forward<T>(condition)));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

template <SystemTrait T>
constexpr auto SystemGroupHandle::RunIf(this auto&& self, std::string name,
                                        T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.RunIf(RunConditionStorage::FromParamNamed(
      std::move(name), std::forward<T>(condition)));
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::InSet(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();
  schedule.AssignGroupToSet(self.id_, target);
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemGroupHandle::Sequence(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& schedule = self.schedule_.get();

  auto& set_entry = schedule.GetSystemSet(self.id_.id);
  set_entry.Sequence();
  schedule.MarkDirty();

  return std::forward<decltype(self)>(self);
}

}  // namespace helios::ecs
