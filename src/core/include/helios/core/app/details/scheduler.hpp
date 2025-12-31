#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/details/system_info.hpp>
#include <helios/core/app/details/system_set_info.hpp>
#include <helios/core/app/schedule.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::app::details {

/**
 * @brief Storage for a system with its metadata and local storage.
 */
struct SystemStorage {
  std::unique_ptr<ecs::System> system;
  SystemInfo info;
  ecs::details::SystemLocalStorage local_storage;

  SystemStorage() = default;
  SystemStorage(const SystemStorage&) = delete;
  SystemStorage(SystemStorage&&) noexcept = default;
  ~SystemStorage() = default;

  SystemStorage& operator=(const SystemStorage&) = delete;
  SystemStorage& operator=(SystemStorage&&) noexcept = default;
};

/**
 * @brief Ordering constraints for a system.
 */
struct SystemOrdering {
  std::vector<ecs::SystemTypeId> before;
  std::vector<ecs::SystemTypeId> after;
};

/**
 * @brief Ordering constraints for a schedule.
 */
struct ScheduleOrdering {
  std::vector<ScheduleId> before;  ///< Schedules that must run before this schedule
  std::vector<ScheduleId> after;   ///< Schedules that must run after this schedule
  ScheduleId stage_id = 0;         ///< Stage that this schedule belongs to (0 if none)
};

/**
 * @brief Manages system scheduling and execution for a single schedule.
 * @details Builds a dependency graph based on AccessPolicy conflicts and executes systems
 * concurrently when possible using TaskGraph.
 * @note Not thread-safe.
 */
class ScheduleExecutor {
public:
  ScheduleExecutor() = default;
  explicit ScheduleExecutor(ScheduleId schedule_id) : schedule_id_(schedule_id) {}
  ScheduleExecutor(const ScheduleExecutor&) = delete;
  ScheduleExecutor(ScheduleExecutor&&) noexcept = default;
  ~ScheduleExecutor() = default;

  ScheduleExecutor& operator=(const ScheduleExecutor&) = delete;
  ScheduleExecutor& operator=(ScheduleExecutor&&) noexcept = default;

  /**
   * @brief Clears all systems and resets the schedule.
   */
  void Clear();

  /**
   * @brief Adds a system to this schedule.
   * @param system_storage_index Index in the global system storage
   */
  void AddSystem(size_t system_storage_index);

  /**
   * @brief Registers ordering constraints for a system.
   * @param system_id System type ID
   * @param ordering Ordering constraints (before/after)
   */
  void RegisterOrdering(ecs::SystemTypeId system_id, SystemOrdering ordering);

  /**
   * @brief Builds the execution graph based on system access policies.
   * @details Analyzes conflicts and creates dependency chains.
   * @param world World to execute systems on
   * @param system_storage Reference to global system storage
   */
  void BuildExecutionGraph(ecs::World& world, std::span<SystemStorage> system_storage,
                           const std::unordered_map<SystemSetId, SystemSetInfo>& system_sets);

  /**
   * @brief Executes all systems in this schedule.
   * @warning Triggers assertion if execution graph has not been built.
   * @param world World to execute systems on
   * @param executor Async executor for parallel execution (ignored if use_async_ is false)
   * @param system_storage Reference to global system storage
   */
  void Execute(ecs::World& world, async::Executor& executor, std::span<SystemStorage> system_storage);

  /**
   * @brief Finds the storage index of a system by type ID within this schedule.
   * @param system_id System type ID to find
   * @param system_storage Reference to global system storage
   * @return Index in system storage if found, std::nullopt otherwise
   */
  [[nodiscard]] auto FindSystemIndexByType(ecs::SystemTypeId system_id,
                                           std::span<const SystemStorage> system_storage) const noexcept
      -> std::optional<size_t>;

  /**
   * @brief Checks if this schedule is the Main stage.
   * @details Main stage executes synchronously on the main thread.
   * All other stages execute asynchronously via the executor.
   * @return True if this is the Main stage, false otherwise
   */
  [[nodiscard]] bool IsMainStage() const noexcept { return schedule_id_ == ScheduleIdOf<Main>(); }

  /**
   * @brief Checks if a system is in this schedule by storage index.
   * @param system_storage_index Index in the global system storage
   * @return True if system is present, false otherwise
   */
  [[nodiscard]] bool Contains(size_t system_storage_index) const noexcept {
    return std::ranges::find(system_indices_, system_storage_index) != system_indices_.end();
  }

  /**
   * @brief Checks if a system of given type is in this schedule.
   * @param system_id System type ID to check
   * @param system_storage Reference to global system storage
   * @return True if system of this type is present, false otherwise
   */
  [[nodiscard]] bool ContainsSystemOfType(ecs::SystemTypeId system_id,
                                          std::span<const SystemStorage> system_storage) const noexcept {
    return std::ranges::any_of(system_indices_, [system_storage, system_id](size_t index) {
      return index < system_storage.size() && system_storage[index].info.type_id == system_id;
    });
  }

  /**
   * @brief Checks if this schedule has any systems.
   * @return True if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return system_indices_.empty(); }

  /**
   * @brief Gets the schedule ID.
   * @return Schedule ID
   */
  [[nodiscard]] ScheduleId GetScheduleId() const noexcept { return schedule_id_; }

  /**
   * @brief Gets the number of systems in this schedule.
   * @return Number of systems
   */
  [[nodiscard]] size_t SystemCount() const noexcept { return system_indices_.size(); }

  /**
   * @brief Gets const reference to system indices in this schedule.
   * @return Const reference to vector of system storage indices
   */
  [[nodiscard]] const std::vector<size_t>& SystemIndices() const noexcept { return system_indices_; }

private:
  /**
   * @brief Creates tasks for all systems in this schedule.
   * @param system_storage Global system storage
   * @param world World to execute systems on
   * @return Vector of created tasks and map from system ID to task index
   */
  auto CreateSystemTasks(std::span<SystemStorage> system_storage, ecs::World& world)
      -> std::pair<std::vector<async::Task>, std::unordered_map<ecs::SystemTypeId, size_t>>;

  /**
   * @brief Applies explicit ordering constraints between systems.
   * @param tasks Vector of system tasks
   * @param system_id_to_task_index Map from system ID to task index
   * @param system_storage Global system storage
   */
  void ApplyExplicitOrdering(std::vector<async::Task>& tasks,
                             const std::unordered_map<ecs::SystemTypeId, size_t>& system_id_to_task_index,
                             std::span<SystemStorage> system_storage);

  /**
   * @brief Applies set-level ordering constraints.
   * @param tasks Vector of system tasks
   * @param system_id_to_task_index Map from system ID to task index
   * @param system_sets System set information
   */
  void ApplySetOrdering(std::vector<async::Task>& tasks,
                        const std::unordered_map<ecs::SystemTypeId, size_t>& system_id_to_task_index,
                        const std::unordered_map<SystemSetId, SystemSetInfo>& system_sets);

  /**
   * @brief Applies access policy based ordering for conflicting systems.
   * @param tasks Vector of system tasks
   * @param system_storage Global system storage
   */
  void ApplyAccessPolicyOrdering(std::vector<async::Task>& tasks, std::span<SystemStorage> system_storage);

  ScheduleId schedule_id_ = 0;                                              ///< ID of this schedule
  std::vector<size_t> system_indices_;                                      ///< Indices into global system storage
  std::unordered_map<ecs::SystemTypeId, SystemOrdering> system_orderings_;  ///< Explicit ordering constraints
  async::TaskGraph execution_graph_;                                        ///< Task graph for executing systems

  bool graph_built_ = false;  ///< Whether the execution graph has been built
};

inline void ScheduleExecutor::Clear() {
  system_indices_.clear();
  system_orderings_.clear();
  execution_graph_.Clear();
  graph_built_ = false;
}

inline void ScheduleExecutor::AddSystem(size_t system_storage_index) {
  system_indices_.push_back(system_storage_index);
  graph_built_ = false;
}

inline void ScheduleExecutor::RegisterOrdering(ecs::SystemTypeId system_id, SystemOrdering ordering) {
  system_orderings_[system_id] = std::move(ordering);
  graph_built_ = false;
}

inline auto ScheduleExecutor::FindSystemIndexByType(ecs::SystemTypeId system_id,
                                                    std::span<const SystemStorage> system_storage) const noexcept
    -> std::optional<size_t> {
  std::optional<size_t> result;
  const auto it = std::ranges::find_if(system_indices_, [system_storage, system_id](size_t index) {
    return index < system_storage.size() && system_storage[index].info.type_id == system_id;
  });

  if (it != system_indices_.end()) {
    result = *it;
  }
  return result;
}

/**
 * @brief Main scheduler that manages all schedules.
 * @details Holds multiple ScheduleExecutors for different execution stages.
 * @note Not thread-safe.
 */
class Scheduler {
public:
  Scheduler() = default;
  Scheduler(const Scheduler&) = delete;
  Scheduler(Scheduler&&) noexcept = default;
  ~Scheduler() = default;

  Scheduler& operator=(const Scheduler&) = delete;
  Scheduler& operator=(Scheduler&&) noexcept = default;

  /**
   * @brief Clears all schedules and systems.
   */
  void Clear();

  /**
   * @brief Registers a schedule type.
   * @tparam T Schedule type
   */
  template <ScheduleTrait T>
  void RegisterSchedule();

  /**
   * @brief Adds a system to the specified schedule.
   * @warning Triggers assertion if system T is already added to the schedule.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule to add system to
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  void AddSystem(S schedule = {});

  /**
   * @brief Registers ordering constraint for a system.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule to register ordering for
   * @param ordering Ordering constraints
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  void RegisterOrdering(S schedule, SystemOrdering ordering);

  /**
   * @brief Executes all systems in the specified schedule.
   * @tparam S Schedule type
   * @param world World to execute systems on
   * @param executor Async executor for parallel execution
   */
  template <ScheduleTrait S>
  void ExecuteSchedule(ecs::World& world, async::Executor& executor);

  /**
   * @brief Executes all systems in the specified schedule by ID.
   * @param schedule_id Schedule ID to execute
   * @param world World to execute systems on
   * @param executor Async executor for parallel execution
   */
  void ExecuteScheduleById(ScheduleId schedule_id, ecs::World& world, async::Executor& executor);

  /**
   * @brief Executes all schedules in the specified stage.
   * @details Executes schedules in topologically sorted order based on Before/After relationships.
   * @tparam S Stage type (must satisfy StageTrait)
   * @param world World to execute systems on
   * @param executor Async executor for parallel execution
   */
  template <StageTrait S>
  void ExecuteStage(ecs::World& world, async::Executor& executor);

  /**
   * @brief Merges all system local commands into the world's main command queue.
   * @details Should be called after Extract schedule to flush all pending commands.
   */
  void MergeCommandsToWorld(ecs::World& world);

  /**
   * @brief Resets all system frame allocators.
   * @details Call this at frame boundaries to reclaim all temporary per-system allocations.
   * Should typically be called at the end of App::Update() after all schedules have executed.
   */
  void ResetFrameAllocators() noexcept;

  /**
   * @brief Builds execution graphs for all schedules.
   * @details Should be called after all systems are added and before execution.
   * This method also propagates system set ordering constraints into explicit
   * system-to-system ordering edges for each schedule.
   * @param world World to execute systems on
   */
  void BuildAllGraphs(ecs::World& world);

  /**
   * @brief Appends system ordering constraints to a system's metadata in a specific schedule.
   * @details This updates SystemInfo.before_systems and SystemInfo.after_systems for the given system type
   * within the specified schedule. If the system has not been added to the schedule yet, this call is a no-op.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule where the system is registered
   * @param before Systems that must run after T in this schedule
   * @param after Systems that must run before T in this schedule
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  void AppendSystemOrderingMetadata(S schedule, std::span<const ecs::SystemTypeId> before,
                                    std::span<const ecs::SystemTypeId> after);

  /**
   * @brief Appends system set membership to a system's metadata in a specific schedule.
   * @details This updates SystemInfo.system_sets for the given system type within the specified schedule.
   * If the system has not been added to the schedule yet, this call is a no-op.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule where the system is registered
   * @param sets System set identifiers to append
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  void AppendSystemSetMetadata(S schedule, std::span<const SystemSetId> sets);

  /**
   * @brief Gets or registers a system set in the global registry.
   * @details If the set is not present, it will be created and initialized with its static name and ID.
   * @tparam Set System set type
   * @return Reference to the SystemSetInfo for this set
   */
  template <SystemSetTrait Set>
  SystemSetInfo& GetOrRegisterSystemSet();

  /**
   * @brief Adds a system to a system set's membership list.
   * @details If the set is not present in the registry this call is a no-op.
   * @param set_id Identifier of the system set
   * @param system_id Identifier of the system to add
   */
  void AddSystemToSet(SystemSetId set_id, ecs::SystemTypeId system_id);

  /**
   * @brief Adds a set-level ordering constraint: set A runs before set B.
   * @details All systems that are members of set A must run before systems that are members of set B.
   * This relationship is schedule-agnostic at registration time; it is applied when building graphs
   * for each schedule based on actual system membership in that schedule.
   * @param before_id Identifier of the set that must run before
   * @param after_id Identifier of the set that must run after
   */
  void AddSetRunsBefore(SystemSetId before_id, SystemSetId after_id);

  /**
   * @brief Adds a set-level ordering constraint: set A runs after set B.
   * @details Convenience wrapper around AddSetRunsBefore(B, A).
   * @param after_id Identifier of the set that must run after
   * @param before_id Identifier of the set that must run before
   */
  void AddSetRunsAfter(SystemSetId after_id, SystemSetId before_id) { AddSetRunsBefore(before_id, after_id); }

  /**
   * @brief Checks if a system of type T is in any schedule.
   * @tparam T System type
   * @return True if system is present, false otherwise
   */
  template <ecs::SystemTrait T>
  [[nodiscard]] bool ContainsSystem() const noexcept;

  /**
   * @brief Checks if a system of type T is in the specified schedule.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule to check
   * @return True if system is present, false otherwise
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  [[nodiscard]] bool ContainsSystem(S schedule = {}) const noexcept;

  /**
   * @brief Gets the total number of systems across all schedules.
   * @return Total number of systems
   */
  [[nodiscard]] size_t SystemCount() const noexcept { return system_storage_.size(); }

  /**
   * @brief Gets the number of systems in the specified schedule.
   * @tparam S Schedule type
   * @param schedule Schedule to query
   * @return Number of systems in the schedule
   */
  template <ScheduleTrait S>
  [[nodiscard]] size_t SystemCount(S schedule = {}) const noexcept;

  /**
   * @brief Gets const reference to system storage.
   * @return Const reference to vector of SystemStorage
   */
  [[nodiscard]] auto GetSystemStorage() const noexcept -> const std::vector<SystemStorage>& { return system_storage_; }

  /**
   * @brief Gets the schedule execution order (topologically sorted).
   * @return Const reference to vector of Schedule IDs in execution order
   */
  [[nodiscard]] auto GetScheduleOrder() const noexcept -> const std::vector<ScheduleId>& { return schedule_order_; }

  /**
   * @brief Gets schedules that belong to a specific stage.
   * @tparam S Stage type
   * @return Vector of schedule IDs that execute within the stage
   */
  template <StageTrait S>
  [[nodiscard]] auto GetSchedulesInStage() const noexcept -> std::vector<ScheduleId>;

private:
  /**
   * @brief Collects all schedule IDs from the schedules map.
   * @return Vector of all schedule IDs
   */
  auto CollectAllScheduleIds() const -> std::vector<ScheduleId>;

  /**
   * @brief Builds the schedule dependency graph for topological sorting.
   * @param all_schedules Vector of all schedule IDs
   * @return Pair of adjacency list and in-degree map
   */
  auto BuildScheduleDependencyGraph(const std::vector<ScheduleId>& all_schedules) const
      -> std::pair<std::unordered_map<ScheduleId, std::vector<ScheduleId>>, std::unordered_map<ScheduleId, size_t>>;

  /**
   * @brief Finds the index of a system by its type ID.</parameter>
   * @param system_id System type ID
   * @return Index in system_storage_ or -1 if not found
   */
  [[nodiscard]] auto FindSystemIndex(ecs::SystemTypeId system_id) const noexcept -> std::optional<size_t>;

  /**
   * @brief Performs topological sort using Kahn's algorithm.
   * @param all_schedules Vector of all schedule IDs
   * @param adjacency Adjacency list for schedule dependencies
   * @param in_degree In-degree map for each schedule
   * @return Topologically sorted vector of schedule IDs
   */
  static auto TopologicalSort(const std::vector<ScheduleId>& all_schedules,
                              const std::unordered_map<ScheduleId, std::vector<ScheduleId>>& adjacency,
                              std::unordered_map<ScheduleId, size_t>& in_degree) -> std::vector<ScheduleId>;

  std::unordered_map<ScheduleId, ScheduleExecutor> schedules_;             ///< Executors for each schedule
  std::unordered_map<ScheduleId, ScheduleOrdering> schedule_constraints_;  ///< Ordering constraints for schedules
  std::vector<ScheduleId> schedule_order_;                                 ///< Topologically sorted schedule IDs
  std::vector<SystemStorage> system_storage_;                              ///< Global storage for all systems
  std::unordered_map<SystemSetId, SystemSetInfo> system_sets_;             ///< Registry of all system sets
};

inline void Scheduler::Clear() {
  system_storage_.clear();
  for (auto& [id, schedule] : schedules_) {
    schedule.Clear();
  }
  schedules_.clear();
  schedule_constraints_.clear();
  schedule_order_.clear();
  system_sets_.clear();
}

template <ScheduleTrait S>
inline void Scheduler::RegisterSchedule() {
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  if (schedules_.contains(schedule_id)) [[unlikely]] {
    return;  // Already registered
  }

  schedules_.emplace(schedule_id, ScheduleExecutor(schedule_id));

  // Store schedule ordering constraints
  ScheduleOrdering ordering;

  // Extract Before constraints from schedule type
  if constexpr (ScheduleWithBeforeTrait<S>) {
    constexpr auto before = ScheduleBeforeOf<S>();
    ordering.before.reserve(before.size());
    for (const ScheduleId id : before) {
      ordering.before.push_back(id);
    }
  }

  // Extract After constraints from schedule type
  if constexpr (ScheduleWithAfterTrait<S>) {
    constexpr auto after = ScheduleAfterOf<S>();
    ordering.after.reserve(after.size());
    for (const ScheduleId id : after) {
      ordering.after.push_back(id);
    }
  }

  // Store stage membership
  ordering.stage_id = ScheduleStageOf<S>();

  schedule_constraints_[schedule_id] = std::move(ordering);
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline void Scheduler::RegisterOrdering(S /*schedule*/, SystemOrdering ordering) {
  RegisterSchedule<S>();
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();

  schedules_.at(schedule_id).RegisterOrdering(system_id, std::move(ordering));
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline void Scheduler::AddSystem(S /*schedule*/) {
  HELIOS_ASSERT((!ContainsSystem<T>(S{})), "Failed to add system '{}': System already exists in schedule '{}'!",
                ecs::SystemNameOf<T>(), ScheduleNameOf<S>());

  RegisterSchedule<S>();
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();

  // Create system storage
  SystemStorage storage{};
  storage.system = std::make_unique<T>();
  storage.info.name = std::string(ecs::SystemNameOf<T>());
  storage.info.type_id = system_id;
  storage.info.access_policy = T::GetAccessPolicy();
  storage.info.execution_count = 0;

  const size_t index = system_storage_.size();
  system_storage_.push_back(std::move(storage));

  schedules_.at(schedule_id).AddSystem(index);
}

template <ScheduleTrait S>
inline void Scheduler::ExecuteSchedule(ecs::World& world, async::Executor& executor) {
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  ExecuteScheduleById(schedule_id, world, executor);
}

inline void Scheduler::ExecuteScheduleById(ScheduleId schedule_id, ecs::World& world, async::Executor& executor) {
  const auto it = schedules_.find(schedule_id);
  if (it == schedules_.end()) [[unlikely]] {
    return;  // Schedule not registered or has no systems
  }

  it->second.Execute(world, executor, system_storage_);

  // After each schedule, merge all commands into world's main queue
  MergeCommandsToWorld(world);
}

inline void Scheduler::MergeCommandsToWorld(ecs::World& world) {
  for (auto& storage : system_storage_) {
    auto& commands = storage.local_storage.GetCommands();
    if (!commands.empty()) {
      world.MergeCommands(std::move(commands));
    }
  }
}

inline void Scheduler::ResetFrameAllocators() noexcept {
  for (auto& storage : system_storage_) {
    storage.local_storage.ResetFrameAllocator();
  }
}

template <StageTrait S>
inline void Scheduler::ExecuteStage(ecs::World& world, async::Executor& executor) {
  constexpr ScheduleId stage_id = ScheduleIdOf<S>();

  // Execute all schedules that belong to this stage in topological order
  for (const ScheduleId schedule_id : schedule_order_) {
    const auto constraint_it = schedule_constraints_.find(schedule_id);
    if (constraint_it == schedule_constraints_.end()) {
      continue;
    }

    // Check if this schedule belongs to this stage
    if (constraint_it->second.stage_id == stage_id) {
      ExecuteScheduleById(schedule_id, world, executor);
    }
  }
}

inline void Scheduler::AddSetRunsBefore(SystemSetId before_id, SystemSetId after_id) {
  if (before_id == after_id) [[unlikely]] {
    return;
  }

  auto before_it = system_sets_.find(before_id);
  if (before_it == system_sets_.end()) {
    // Set may not have been explicitly registered yet; create a minimal entry.
    SystemSetInfo info{};
    info.id = before_id;
    before_it = system_sets_.emplace(before_id, std::move(info)).first;
  }

  auto after_it = system_sets_.find(after_id);
  if (after_it == system_sets_.end()) {
    SystemSetInfo info{};
    info.id = after_id;
    after_it = system_sets_.emplace(after_id, std::move(info)).first;
  }

  auto& before_info = before_it->second;
  auto& after_info = after_it->second;

  // Encode relationship:
  //  - before_info.before_sets contains after_id (before runs before after)
  //  - after_info.after_sets contains before_id
  if (std::ranges::find(before_info.before_sets, after_id) == before_info.before_sets.end()) {
    before_info.before_sets.push_back(after_id);
  }
  if (std::ranges::find(after_info.after_sets, before_id) == after_info.after_sets.end()) {
    after_info.after_sets.push_back(before_id);
  }
}

template <SystemSetTrait Set>
inline SystemSetInfo& Scheduler::GetOrRegisterSystemSet() {
  constexpr SystemSetId id = SystemSetIdOf<Set>();
  const auto it = system_sets_.find(id);
  if (it != system_sets_.end()) {
    return it->second;
  }

  SystemSetInfo info{};
  info.id = id;
  info.name = std::string(SystemSetNameOf<Set>());

  const auto [insert_it, _] = system_sets_.emplace(id, std::move(info));
  return insert_it->second;
}

inline void Scheduler::AddSystemToSet(SystemSetId set_id, ecs::SystemTypeId system_id) {
  const auto it = system_sets_.find(set_id);
  if (it == system_sets_.end()) {
    return;
  }

  auto& members = it->second.member_systems;
  if (std::ranges::find(members, system_id) == members.end()) {
    members.push_back(system_id);
  }
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline void Scheduler::AppendSystemOrderingMetadata(S /*schedule*/, std::span<const ecs::SystemTypeId> before,
                                                    std::span<const ecs::SystemTypeId> after) {
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();

  // Find the schedule
  const auto schedule_it = schedules_.find(schedule_id);
  if (schedule_it == schedules_.end()) {
    return;
  }

  // Find the system index within this specific schedule
  const auto index_opt = schedule_it->second.FindSystemIndexByType(system_id, system_storage_);
  if (!index_opt.has_value()) {
    return;
  }

  auto& info = system_storage_[index_opt.value()].info;
  if (!before.empty()) {
#ifdef HELIOS_CONTAINERS_RANGES_AVALIABLE
    info.before_systems.append_range(before);
#else
    info.before_systems.insert(info.before_systems.end(), before.begin(), before.end());
#endif
  }
  if (!after.empty()) {
#ifdef HELIOS_CONTAINERS_RANGES_AVALIABLE
    info.after_systems.append_range(after);
#else
    info.after_systems.insert(info.after_systems.end(), after.begin(), after.end());
#endif
  }
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline void Scheduler::AppendSystemSetMetadata(S /*schedule*/, std::span<const SystemSetId> sets) {
  if (sets.empty()) [[unlikely]] {
    return;
  }

  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();

  // Find the schedule
  const auto schedule_it = schedules_.find(schedule_id);
  if (schedule_it == schedules_.end()) {
    return;
  }

  // Find the system index within this specific schedule
  const auto index_opt = schedule_it->second.FindSystemIndexByType(system_id, system_storage_);
  if (!index_opt.has_value()) {
    return;
  }

  auto& info = system_storage_[index_opt.value()].info;
#ifdef HELIOS_CONTAINERS_RANGES_AVALIABLE
  info.system_sets.append_range(sets);
#else
  info.system_sets.insert(info.system_sets.end(), sets.begin(), sets.end());
#endif
}

inline auto Scheduler::FindSystemIndex(ecs::SystemTypeId system_id) const noexcept -> std::optional<size_t> {
  for (size_t i = 0; i < system_storage_.size(); ++i) {
    if (system_storage_[i].info.type_id == system_id) {
      return i;
    }
  }
  return std::nullopt;
}

template <ecs::SystemTrait T>
inline bool Scheduler::ContainsSystem() const noexcept {
  constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();
  // Check if any schedule contains a system of this type
  return std::ranges::any_of(schedules_, [this, system_id](const auto& pair) {
    return pair.second.ContainsSystemOfType(system_id, system_storage_);
  });
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline bool Scheduler::ContainsSystem(S /*schedule*/) const noexcept {
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  const auto it = schedules_.find(schedule_id);
  if (it == schedules_.end()) {
    return false;
  }

  constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();
  return it->second.ContainsSystemOfType(system_id, system_storage_);
}

template <ScheduleTrait S>
inline size_t Scheduler::SystemCount(S /*schedule*/) const noexcept {
  constexpr ScheduleId schedule_id = ScheduleIdOf<S>();
  const auto it = schedules_.find(schedule_id);
  if (it == schedules_.end()) [[unlikely]] {
    return 0;
  }
  return it->second.SystemCount();
}

template <StageTrait S>
inline auto Scheduler::GetSchedulesInStage() const noexcept -> std::vector<ScheduleId> {
  constexpr ScheduleId stage_id = ScheduleIdOf<S>();
  std::vector<ScheduleId> result;

  // Iterate through all schedules in topological order and collect those belonging to this stage
  for (const ScheduleId schedule_id : schedule_order_) {
    const auto constraint_it = schedule_constraints_.find(schedule_id);
    if (constraint_it == schedule_constraints_.end()) {
      continue;
    }

    // Check if this schedule belongs to this stage
    if (constraint_it->second.stage_id == stage_id) {
      result.push_back(schedule_id);
    }
  }

  return result;
}

}  // namespace helios::app::details
