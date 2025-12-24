#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/details/scheduler.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/app/schedule.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/core.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace helios::app {

// Forward declarations
template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
class SystemConfig;

template <ScheduleTrait Schedule, SystemSetTrait Set>
class SystemSetConfig;

/**
 * @brief Trait to identify valid sub-app types.
 * @details A valid sub-app type must be an empty struct or class.
 */
template <typename T>
concept SubAppTrait = std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Trait to identify sub-apps with custom names.
 * @details A sub-app with name trait must satisfy SubAppTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 */
template <typename T>
concept SubAppWithNameTrait = SubAppTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Trait to identify sub-apps that support async/overlapping updates.
 * @details A sub-app with async trait must satisfy SubAppTrait and provide:
 * - `static constexpr bool AllowOverlappingUpdates() noexcept`
 *
 * When true, the sub-app can be updated concurrently with other sub-apps.
 * When false (default), sub-app updates are serialized.
 * @tparam T Sub-app type
 *
 * @example
 * @code
 * struct RenderSubApp {
 *   static constexpr bool AllowOverlappingUpdates() noexcept { return true; }
 * };
 * @endcode
 */
template <typename T>
concept SubAppWithAsyncTrait = SubAppTrait<T> && requires {
  { T::AllowOverlappingUpdates() } -> std::same_as<bool>;
};

/**
 * @brief Trait to identify sub-apps with maximum overlapping updates.
 * @details A sub-app with max overlapping updates trait must satisfy SubAppTrait and provide:
 * - `static constexpr size_t GetMaxOverlappingUpdates() noexcept`
 *
 * This is useful for sub-apps like renderers that need to limit concurrent updates
 * (e.g., max frames in flight).
 * @tparam T Sub-app type
 *
 * @example
 * @code
 * struct RenderSubApp {
 *   static constexpr size_t GetMaxOverlappingUpdates() noexcept { return 3; }
 * };
 * @endcode
 */
template <typename T>
concept SubAppWithMaxOverlappingUpdatesTrait = SubAppTrait<T> && requires {
  { T::GetMaxOverlappingUpdates() } -> std::same_as<size_t>;
};

/**
 * @brief Gets whether a sub-app allows overlapping updates.
 * @tparam T Sub-app type
 * @return True if overlapping updates are allowed, false otherwise
 */
template <SubAppTrait T>
constexpr bool SubAppAllowsOverlappingUpdates() noexcept {
  if constexpr (SubAppWithAsyncTrait<T>) {
    return T::AllowOverlappingUpdates();
  } else {
    return false;  // Default: no overlapping updates
  }
}

/**
 * @brief Gets the maximum number of overlapping updates for a sub-app.
 * @tparam T Sub-app type
 * @return Maximum overlapping updates, or 0 if not specified
 */
template <SubAppTrait T>
constexpr size_t SubAppMaxOverlappingUpdates() noexcept {
  if constexpr (SubAppWithMaxOverlappingUpdatesTrait<T>) {
    return T::GetMaxOverlappingUpdates();
  } else {
    return 0;  // Default: no limit
  }
}

/**
 * @brief Type alias for sub-app type IDs.
 */
using SubAppTypeId = size_t;

/**
 * @brief Type alias for module type IDs.
 * @tparam T Module type
 * @return Unique type ID for the module
 */
template <SubAppTrait T>
constexpr SubAppTypeId SubAppTypeIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of a sub-app type.
 * @tparam T Sub-app type
 * @return Name of the sub-app
 */
template <SubAppTrait T>
constexpr std::string_view SubAppNameOf() noexcept {
  if constexpr (SubAppWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

/**
 * @brief A sub-application with its own ECS world, systems, and resources.
 * @details SubApp encapsulates an ECS world and manages its own systems and resources.
 * It allows for modular separation of functionality within an application,
 * such as having distinct simulation and rendering sub-apps.
 * Each SubApp can have its own execution schedules and can extract data from a main world if needed.
 *
 * SubApps can be added to the main App instance and can be updated independently.
 * They support adding modules that can register their own systems and resources.
 *
 * @note Not thread-safe.
 * Use AllowOverlappingUpdates() trait for controlled concurrent access.
 */
class SubApp {
public:
#ifdef HELIOS_MOVEONLY_FUNCTION_AVALIABLE
  using ExtractFn = std::move_only_function<void(const ecs::World&, ecs::World&)>;
#else
  using ExtractFn = std::function<void(const ecs::World&, ecs::World&)>;
#endif

  SubApp() = default;
  SubApp(const SubApp&) = delete;
  SubApp(SubApp&& other) noexcept;
  ~SubApp() = default;

  SubApp& operator=(const SubApp&) = delete;
  SubApp& operator=(SubApp&& other) noexcept;

  /**
   * @brief Clears the sub-app, removing all data.
   * @warning Triggers assertion if sub app is updating.
   */
  void Clear();

  /**
   * @brief Updates this sub-app by executing all scheduled systems.
   * @details This method checks the updating flag to prevent concurrent updates when overlapping
   * updates are not allowed. SubApps with AllowOverlappingUpdates() can bypass this check.
   * @warning Triggers assertion if scheduler was not built.
   * @param executor Async executor for parallel execution
   */
  void Update(async::Executor& executor);

  /**
   * @brief Extracts data from the main world into this sub-app.
   * @warning Triggers assertion if sub app is updating.
   * @param main_world Const reference to the main world to extract data from
   */
  void Extract(const ecs::World& main_world);

  /**
   * @brief Builds execution graphs for all schedules.
   * @details Should be called after all systems are added and before first execution.
   * @warning Triggers assertion if sub app is updating.
   */
  void BuildScheduler();

  /**
   * @brief Executes a specific schedule.
   * @warning Triggers assertion if scheduler was not built.
   * @tparam S Schedule type
   * @param executor Async executor for parallel execution
   * @param schedule Schedule to execute
   */
  template <ScheduleTrait S>
  void ExecuteSchedule(async::Executor& executor, S schedule = {});

  /**
   * @brief Executes all schedules in a specific stage.
   * @warning Triggers assertion if scheduler was not built.
   * @tparam S Stage type (must satisfy StageTrait)
   * @param executor Async executor for parallel execution
   */
  template <StageTrait S>
  void ExecuteStage(async::Executor& executor);

  /**
   * @brief Adds a system to the specified schedule in the sub-app.
   * @warning Triggers assertion if sub app is updating.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule to add system to
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  void AddSystem(S schedule = {});

  /**
   * @brief Adds multiple systems to the specified schedule in the sub-app.
   * @warning Triggers assertion if sub app is updating.
   * @tparam Systems System types
   * @tparam S Schedule type
   * @param schedule Schedule to add systems to
   */
  template <ecs::SystemTrait... Systems, ScheduleTrait S>
    requires(sizeof...(Systems) > 1 && utils::UniqueTypes<Systems...>)
  void AddSystems(S schedule = {});

  /**
   * @brief Adds systems with fluent configuration builder.
   * @details Returns a builder that allows chaining configuration methods like
   * .After(), .Before(), .InSet(), and .Sequence().
   * @warning Triggers assertion if sub app is updating.
   * @tparam Systems System types to add
   * @tparam S Schedule type
   * @param schedule Schedule to add systems to
   * @return SystemConfig builder for fluent configuration
   *
   * @example
   * @code
   * sub_app.AddSystemsBuilder<MovementSystem, CollisionSystem>(kUpdate)
   *     .After<InputSystem>()
   *     .Before<RenderSystem>()
   *     .InSet<PhysicsSet>()
   *     .Sequence();
   * @endcode
   */
  template <ecs::SystemTrait... Systems, ScheduleTrait S>
    requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
  auto AddSystemsBuilder(S schedule = {}) -> SystemConfig<S, Systems...>;

  /**
   * @brief Adds a single system with fluent configuration builder.
   * @details Returns a builder that allows chaining configuration methods.
   * @warning Triggers assertion if sub app is updating.
   * @tparam T System type to add
   * @tparam S Schedule type
   * @param schedule Schedule to add system to
   * @return SystemConfig builder for fluent configuration
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  auto AddSystemBuilder(S schedule = {}) -> SystemConfig<S, T>;

  /**
   * @brief Configures a system set with fluent builder.
   * @details Returns a builder that allows configuring set ordering.
   * @warning Triggers assertion if sub app is updating.
   * @tparam Set System set type to configure
   * @tparam S Schedule type
   * @param schedule Schedule where the set is configured
   * @return SystemSetConfig builder for fluent configuration
   *
   * @example
   * @code
   * sub_app.ConfigureSet<PhysicsSet>(kUpdate)
   *     .After<InputSet>()
   *     .Before<RenderSet>();
   * @endcode
   */
  template <SystemSetTrait Set, ScheduleTrait S>
  auto ConfigureSet(S schedule = {}) -> SystemSetConfig<S, Set>;

  /**
   * @brief Inserts a resource into the sub-app's world.
   * @warning Triggers assertion if sub app is updating.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ecs::ResourceTrait T>
  void InsertResource(T&& resource) {
    world_.InsertResource(std::forward<T>(resource));
  }

  /**
   * @brief Emplaces a resource into the sub-app's world.
   * @warning Triggers assertion if sub app is updating.
   * @tparam T Resource type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to resource constructor
   */
  template <ecs::ResourceTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceResource(Args&&... args) {
    world_.EmplaceResource<T>(std::forward<Args>(args)...);
  }

  /**
   * @brief Registers an event type for use in this sub-app.
   * @details Events must be registered before they can be written or read.
   * @warning Triggers assertion if sub app is updating.
   * @tparam T Event type
   */
  template <ecs::EventTrait T>
  void AddEvent() {
    world_.AddEvent<T>();
  }

  /**
   * @brief Registers multiple event types for use in this sub-app.
   * @warning Triggers assertion if sub app is updating.
   * @tparam Events Event types to register
   */
  template <ecs::EventTrait... Events>
    requires(sizeof...(Events) > 1)
  void AddEvents() {
    world_.AddEvents<Events...>();
  }

  /**
   * @brief Sets custom extraction function for this sub-app.
   * @details The extraction function is called before the sub-app's Update to copy
   * data from the main world.
   * This is useful for example for  render sub-app that need to extract transform, mesh,
   * and camera data from the main simulation world.
   * @param extract_fn Function that takes main world and sub-app world references
   */
  void SetExtractFunction(ExtractFn extract_fn) noexcept { extract_fn_ = std::move(extract_fn); }

  /**
   * @brief Sets whether this sub-app allows overlapping updates.
   * @param allow True to allow concurrent updates, false otherwise
   */
  void SetAllowOverlappingUpdates(bool allow) noexcept { allow_overlapping_updates_ = allow; }

  /**
   * @brief Sets the maximum number of overlapping updates.
   * @param max Maximum number of concurrent updates (0 = unlimited)
   */
  void SetMaxOverlappingUpdates(size_t max) noexcept { max_overlapping_updates_ = max; }

  /**
   * @brief Checks if a system of type T is in any schedule.
   * @tparam T System type
   * @return True if system is present, false otherwise
   */
  template <ecs::SystemTrait T>
  [[nodiscard]] bool ContainsSystem() const noexcept {
    return scheduler_.ContainsSystem<T>();
  }

  /**
   * @brief Checks if a system of type T is in the specified schedule.
   * @tparam T System type
   * @tparam S Schedule type
   * @param schedule Schedule to check
   * @return True if system is present, false otherwise
   */
  template <ecs::SystemTrait T, ScheduleTrait S>
  [[nodiscard]] bool ContainsSystem(S schedule = {}) const noexcept {
    return scheduler_.ContainsSystem<T>(schedule);
  }

  /**
   * @brief Checks if a resource of type T exists in this sub-app.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] bool HasResource() const noexcept {
    return world_.HasResource<T>();
  }

  /**
   * @brief Checks if an event type is registered in this sub-app.
   * @tparam T Event type
   * @return True if event type is registered, false otherwise
   */
  template <ecs::EventTrait T>
  [[nodiscard]] bool HasEvent() const noexcept {
    return world_.HasEvent<T>();
  }

  /**
   * @brief Checks if the sub-app is currently updating.
   * @return True if update is in progress, false otherwise
   */
  [[nodiscard]] bool IsUpdating() const noexcept { return is_updating_.load(std::memory_order_acquire); }

  /**
   * @brief Checks if this sub-app allows overlapping updates.
   * @return True if overlapping updates are allowed, false otherwise
   */
  [[nodiscard]] bool AllowsOverlappingUpdates() const noexcept { return allow_overlapping_updates_; }

  /**
   * @brief Gets the total number of systems across all schedules.
   * @return Total number of systems
   */
  [[nodiscard]] size_t SystemCount() const noexcept { return scheduler_.SystemCount(); }

  /**
   * @brief Gets the number of systems in the specified schedule of this sub-app.
   * @tparam S Schedule type
   * @param schedule Schedule to query
   * @return Number of systems in the schedule
   */
  template <ScheduleTrait S>
  [[nodiscard]] size_t SystemCount(S schedule = {}) const noexcept {
    return scheduler_.SystemCount(schedule);
  }

  /**
   * @brief Gets const reference to this sub-app's world.
   * @return Const reference to the ECS world
   */
  [[nodiscard]] const ecs::World& GetWorld() const noexcept { return world_; }

private:
  /**
   * @brief Gets mutable reference to this sub-app's world.
   * @return Reference to the ECS world
   */
  [[nodiscard]] ecs::World& GetWorld() noexcept { return world_; }

  /**
   * @brief Gets mutable reference to the scheduler.
   * @return Reference to the scheduler
   */
  [[nodiscard]] details::Scheduler& GetScheduler() noexcept { return scheduler_; }

  ecs::World world_;              ///< The ECS world for this sub-app
  details::Scheduler scheduler_;  ///< Scheduler managing system execution

  /// Optional extraction function for data transfer from main world
  ExtractFn extract_fn_;

  std::atomic<bool> is_updating_{false};  ///< Flag to track if update is in progress
  bool graphs_built_ = false;             ///< Flag to track if execution graphs are built

  bool allow_overlapping_updates_ = false;              ///< Whether concurrent updates are allowed
  size_t max_overlapping_updates_ = 0;                  ///< Maximum concurrent updates (0 = unlimited)
  std::atomic<size_t> current_overlapping_updates_{0};  ///< Current number of overlapping updates

  friend class App;

  template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
    requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
  friend class SystemConfig;

  template <ScheduleTrait Schedule, SystemSetTrait Set>
  friend class SystemSetConfig;
};

inline SubApp::SubApp(SubApp&& other) noexcept
    : world_(std::move(other.world_)),
      scheduler_(std::move(other.scheduler_)),
      extract_fn_(std::move(other.extract_fn_)),
      is_updating_(other.is_updating_.load(std::memory_order_acquire)),
      graphs_built_(other.graphs_built_),
      allow_overlapping_updates_(other.allow_overlapping_updates_),
      max_overlapping_updates_(other.max_overlapping_updates_),
      current_overlapping_updates_(other.current_overlapping_updates_.load(std::memory_order_acquire)) {}

inline SubApp& SubApp::operator=(SubApp&& other) noexcept {
  if (this != &other) {
    world_ = std::move(other.world_);
    scheduler_ = std::move(other.scheduler_);
    extract_fn_ = std::move(other.extract_fn_);
    is_updating_.store(other.is_updating_.load(std::memory_order_acquire), std::memory_order_release);
    graphs_built_ = other.graphs_built_;
    allow_overlapping_updates_ = other.allow_overlapping_updates_;
    max_overlapping_updates_ = other.max_overlapping_updates_;
    current_overlapping_updates_.store(other.current_overlapping_updates_.load(std::memory_order_acquire),
                                       std::memory_order_release);
  }
  return *this;
}

inline void SubApp::Clear() {
  HELIOS_ASSERT(!IsUpdating(), "Failed to clear sub app: Cannot clear while app is running!");

  world_.Clear();
  scheduler_.Clear();
  extract_fn_ = nullptr;
  graphs_built_ = false;
}

inline void SubApp::Update(async::Executor& executor) {
  HELIOS_ASSERT(graphs_built_, "Failed to update sub app: Scheduler must be built before update!");

  if (!allow_overlapping_updates_) {
    // Enforce single update at a time
    bool expected = false;
    if (!is_updating_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
      HELIOS_ERROR("Failed to update sub app: Overlapping updates not allowed!");
      return;
    }
  } else {
    // Track overlapping updates if limit is set
    if (max_overlapping_updates_ > 0) {
      size_t current = current_overlapping_updates_.fetch_add(1, std::memory_order_acq_rel);
      if (current >= max_overlapping_updates_) {
        current_overlapping_updates_.fetch_sub(1, std::memory_order_acq_rel);
        HELIOS_WARN("Failed to update sub app: Max overlapping updates ({}) reached!", max_overlapping_updates_);
        return;
      }
    }

    is_updating_.store(true, std::memory_order_release);
  }

  // Execute all stages in order (StartUpStage only runs during Initialize, not Update)
  scheduler_.ExecuteStage<MainStage>(world_, executor);
  scheduler_.ExecuteStage<UpdateStage>(world_, executor);
  scheduler_.ExecuteStage<CleanUpStage>(world_, executor);

  // Merge all deferred commands into world after execution
  scheduler_.MergeCommandsToWorld(world_);

  // Execute all commands (create entities, add/remove components, etc.)
  world_.Update();

  // Reset all per-system frame allocators to reclaim temporary memory
  scheduler_.ResetFrameAllocators();

  // Clear update flags
  if (allow_overlapping_updates_ && max_overlapping_updates_ > 0) {
    current_overlapping_updates_.fetch_sub(1, std::memory_order_acq_rel);
  }

  is_updating_.store(false, std::memory_order_release);
}

inline void SubApp::Extract(const ecs::World& main_world) {
  HELIOS_ASSERT(!IsUpdating(), "Failed to extract: Cannot extract while app is running!");

  if (extract_fn_) {
    extract_fn_(main_world, world_);
  }
}

inline void SubApp::BuildScheduler() {
  HELIOS_ASSERT(!IsUpdating(), "Failed to build scheduler: Cannot build while app is running!");

  scheduler_.BuildAllGraphs(world_);
  graphs_built_ = true;
}

template <ScheduleTrait S>
inline void SubApp::ExecuteSchedule(async::Executor& executor, S /*schedule*/) {
  HELIOS_ASSERT(graphs_built_, "Failed to execute schedule: Scheduler must be built before update!");
  scheduler_.ExecuteSchedule<S>(world_, executor);
}

template <StageTrait S>
inline void SubApp::ExecuteStage(async::Executor& executor) {
  HELIOS_ASSERT(graphs_built_, "Failed to execute stage: Scheduler must be built before update!");
  scheduler_.ExecuteStage<S>(world_, executor);
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline void SubApp::AddSystem(S schedule) {
  HELIOS_ASSERT(!IsUpdating(), "Failed to add system '{}': Cannot add systems while app is running!",
                ecs::SystemNameOf<T>());

  if (ContainsSystem<T>(schedule)) [[unlikely]] {
    HELIOS_WARN("System '{}' is already exist in app schedule '{}'!", ecs::SystemNameOf<T>(), ScheduleNameOf<S>());
    return;
  }

  scheduler_.AddSystem<T>(schedule);
  graphs_built_ = false;
}

template <ecs::SystemTrait... Systems, ScheduleTrait S>
  requires(sizeof...(Systems) > 1 && utils::UniqueTypes<Systems...>)
inline void SubApp::AddSystems(S schedule) {
  HELIOS_ASSERT(!IsUpdating(), "Failed to add systems: Cannot add systems while app is running!");

  (scheduler_.AddSystem<Systems>(schedule), ...);
  graphs_built_ = false;
}

template <ecs::SystemTrait... Systems, ScheduleTrait S>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
inline auto SubApp::AddSystemsBuilder(S schedule) -> SystemConfig<S, Systems...> {
  HELIOS_ASSERT(!IsUpdating(), "Failed to add systems: Cannot add systems while app is running!");
  return SystemConfig<S, Systems...>(*this, schedule);
}

template <ecs::SystemTrait T, ScheduleTrait S>
inline auto SubApp::AddSystemBuilder(S schedule) -> SystemConfig<S, T> {
  HELIOS_ASSERT(!IsUpdating(), "Failed to add system '{}': Cannot add systems while app is running!",
                ecs::SystemNameOf<T>());
  return SystemConfig<S, T>(*this, schedule);
}

template <SystemSetTrait Set, ScheduleTrait S>
inline auto SubApp::ConfigureSet(S schedule) -> SystemSetConfig<S, Set> {
  HELIOS_ASSERT(!IsUpdating(), "Failed to configure set '{}': Cannot configure sets while app is running!",
                SystemSetNameOf<Set>());
  return SystemSetConfig<S, Set>(*this, schedule);
}

}  // namespace helios::app

#include <helios/core/app/system_config.hpp>
#include <helios/core/app/system_set_config.hpp>
