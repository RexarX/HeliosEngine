#pragma once

#include <helios/app/details/profile.hpp>
#include <helios/app/schedules.hpp>
#include <helios/assert.hpp>
#include <helios/async/executor.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/scheduler.hpp>
#include <helios/ecs/schedule/stage.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>
#include <helios/log/logger.hpp>
#include <helios/utils/type_info.hpp>

#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#include <format>
#endif

namespace helios::app {

class App;

/// @brief Type index for sub-apps.
using SubAppTypeIndex = utils::TypeIndex;

/// @brief Type id for sub-apps.
using SubAppTypeId = utils::TypeId;

/**
 * @brief Trait to identify valid sub-app types.
 * @details A valid sub-app type must be an empty struct or class.
 */
template <typename T>
concept SubAppTrait = std::is_object_v<std::remove_cvref_t<T>> &&
                      std::is_empty_v<std::remove_cvref_t<T>>;

/// @brief Concept for sub-apps that provide custom names.
template <typename T>
concept SubAppWithNameTrait = SubAppTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/// @brief Concept for sub-apps that allow skipping extraction while updating.
template <typename T>
concept OverlappingUpdatesSubAppTrait = SubAppTrait<T> && requires {
  requires std::remove_cvref_t<T>::kAllowOverlappingUpdates;
};

/// @brief Concept for sub-apps with a maximum extraction-skip budget.
template <typename T>
concept SubAppWithMaxOverlappingUpdatesTrait =
    OverlappingUpdatesSubAppTrait<T> && requires {
      {
        std::remove_cvref_t<T>::kMaxOverlappingUpdates
      } -> std::convertible_to<size_t>;
    };

/// @brief Concept for sub-apps that run updates on a background loop.
template <typename T>
concept AsyncSubAppTrait =
    SubAppTrait<T> && requires { requires std::remove_cvref_t<T>::kAsync; };

/**
 * @brief Returns the name of the sub-app.
 * @tparam T The type of the sub-app
 * @param sub_app The sub-app to get the name of
 * @return The name of the sub-app
 */
template <SubAppTrait T>
[[nodiscard]] constexpr std::string_view SubAppNameOf(
    const T& sub_app = {}) noexcept {
  if constexpr (SubAppWithNameTrait<T>) {
    return std::remove_cvref_t<T>::kName;
  } else {
    return utils::QualifiedTypeNameOf(sub_app);
  }
}

/**
 * @brief Returns whether the sub-app allows extraction skips while updating.
 * @tparam T The type of the sub-app
 * @return Whether overlapping extraction skips are enabled
 */
template <SubAppTrait T>
[[nodiscard]] consteval bool IsSubAppAllowsOverlappingUpdates(
    const T& /*sub_app*/ = {}) noexcept {
  return OverlappingUpdatesSubAppTrait<std::remove_cvref_t<T>>;
}

/**
 * @brief Returns the maximum consecutive extraction skips for the sub-app.
 * @details `0` means unlimited skips while an update is in flight.
 * @tparam T The type of the sub-app
 * @return Maximum extraction skip budget
 */
template <OverlappingUpdatesSubAppTrait T>
[[nodiscard]] consteval size_t SubAppMaxOverlappingUpdates(
    const T& /*sub_app*/ = {}) noexcept {
  if constexpr (SubAppWithMaxOverlappingUpdatesTrait<T>) {
    return std::remove_cvref_t<T>::kMaxOverlappingUpdates;
  } else {
    return 0;
  }
}

/**
 * @brief Returns whether the sub-app runs updates on a background loop.
 * @tparam T The type of the sub-app
 * @return True when `kAsync` is set on the label type
 */
template <SubAppTrait T>
[[nodiscard]] consteval bool IsSubAppAsync(const T& /*sub_app*/ = {}) noexcept {
  return AsyncSubAppTrait<std::remove_cvref_t<T>>;
}

/**
 * @brief A sub-application with its own ECS world and scheduler.
 * @details Use `SetExtractFunction` to copy data from the main world before
 * each update. Overlapping sub-apps may skip extraction while an update is in
 * flight, up to `kMaxOverlappingUpdates` consecutive frames (`0` = unlimited).
 * Async sub-apps run their update pass on a background loop; extraction runs
 * every main frame and thread safety is the user's responsibility.
 * @warning Async sub-apps must use a looping runner such as `RunDefaultSubApp`
 * or `RunFixedSubApp` via `SetRunner()` before initialization. The default
 * `RunOnceSubApp` runs a single update pass and is intended for blocking
 * sub-apps.
 * @note Partially thread-safe.
 */
class SubApp {
public:
  using RunnerFn = std::move_only_function<void(SubApp&, async::Executor&)>;
  using ExtractFn =
      std::move_only_function<void(const ecs::World&, ecs::World&)>;

  /// @brief Constructs a sub-app with built-in schedules registered.
  SubApp();

  /**
   * @brief Constructs a sub-app with the given name and built-in schedules
   * registered.
   * @param name Human-readable sub-app name
   */
  explicit SubApp(std::string name);

  SubApp(const SubApp&) = delete;

  /**
   * @brief Move-constructs a sub-app from another.
   * @warning Triggers assertion if an update is in progress.
   * @param other The sub-app to move from
   */
  SubApp(SubApp&& other) noexcept;
  ~SubApp() = default;

  SubApp& operator=(const SubApp&) = delete;

  /**
   * @brief Move-assigns a sub-app from another.
   * @warning Triggers assertion if an update is in progress.
   * @param other The sub-app to move from
   */
  SubApp& operator=(SubApp&& other) noexcept;

  /**
   * @brief Creates a sub-app named after the given label type.
   * @tparam T Sub-app label type satisfying `SubAppTrait`
   * @param sub_app Sub-app label instance
   * @return Sub-app with name derived from the label
   */
  template <SubAppTrait T>
  [[nodiscard]] static SubApp From(const T& sub_app = {}) {
    return SubApp(std::string(SubAppNameOf(sub_app)));
  }

  /**
   * @brief Clears the sub-app world, ECS scheduler, and extract function.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   */
  void Clear();

  /**
   * @brief Runs the configured update stage.
   * @note Only valid within an active update pass (IsUpdating() is true).
   * @warning Triggers assertion if not called from within an active update
   * pass.
   * @param executor Async executor used to build and run schedules
   */
  void Update(async::Executor& executor);

  /**
   * @brief Copies data from the main world into this sub-app's world.
   * @details Calls the function passed to `SetExtractFunction` when set.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress and @p
   * allow_while_updating is false.
   * @param main_world Main application world to read from
   * @param allow_while_updating When true, extraction may run during an update
   */
  void Extract(const ecs::World& main_world, bool allow_while_updating = false);

  /**
   * @brief Blocks until no update is in flight.
   * @note Thread-safe.
   */
  void WaitUntilFullyIdle() const noexcept;

  /**
   * @brief Builds all ECS schedules when any are dirty.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param executor Async executor used to create threaded schedule executors
   */
  void BuildScheduler(async::Executor& executor);

  /**
   * @brief Adds a schedule with the given label type id.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param id Schedule type id
   * @param schedule Schedule instance
   * @return Ordering handle for configuring placement
   */
  ecs::ScheduleOrdering AddSchedule(ecs::ScheduleTypeId id,
                                    ecs::Schedule&& schedule) {
    return scheduler_.Add(id, std::move(schedule));
  }

  /**
   * @brief Adds a schedule with the given label type.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam T Schedule label type
   * @param schedule Schedule instance
   * @param label Label value
   * @return Ordering handle for configuring placement
   */
  template <ecs::ScheduleTrait T>
  ecs::ScheduleOrdering AddSchedule(const T& label, ecs::Schedule&& schedule) {
    return scheduler_.Add(label, std::move(schedule));
  }

  /**
   * @brief Creates an empty schedule for the label when it does not exist.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam T Schedule label type
   * @param label Label value
   * @return Reference to this sub-app for chaining
   */
  template <ecs::ScheduleTrait T>
  auto InitSchedule(this auto&& self, const T& label = {})
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Mutates an existing schedule in place.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam L Schedule label type
   * @param label Target schedule label
   * @param fn Callable receiving `ecs::Schedule&`
   * @return Reference to this sub-app for chaining
   */
  template <ecs::ScheduleTrait L, typename F>
    requires std::invocable<F, ecs::Schedule&>
  auto EditSchedule(this auto&& self, const L& label, const F& fn)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a functor system to the given schedule.
   * @details Derives name and `SystemId` from the functor type. Lambdas
   * are rejected; use the `AddSystem(label, name, system)` overload.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam L Schedule label type
   * @tparam S System type satisfying `FunctorSystemTrait`
   * @param label Target schedule label
   * @param system System instance
   * @param options Local data options for the system
   * @return Handle for ordering and run conditions
   */
  template <ecs::ScheduleTrait L, ecs::FunctorSystemTrait S>
  ecs::SystemHandle AddSystem(const L& label, S&& system,
                              ecs::SystemLocalDataOptions options = {}) {
    return scheduler_.In(label).Add(std::forward<S>(system), options);
  }

  /**
   * @brief Adds a system to the given schedule with an explicit name.
   * @details Required for lambdas. Derives the `SystemId` from the
   * provided name, enabling multiple instances of the same type.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam L Schedule label type
   * @tparam S System type satisfying `SystemTrait`
   * @param label Target schedule label
   * @param name Human-readable system name
   * @param system System instance
   * @param options Local data options for the system
   * @return Handle for ordering and run conditions
   */
  template <ecs::ScheduleTrait L, ecs::SystemTrait S>
  ecs::SystemHandle AddSystem(const L& label, std::string name, S&& system,
                              ecs::SystemLocalDataOptions options = {}) {
    return scheduler_.In(label).Add(std::move(name), std::forward<S>(system),
                                    options);
  }

  /**
   * @brief Adds multiple functor systems to the same schedule as an anonymous
   * set.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam L Schedule label type
   * @tparam Systems System types satisfying `FunctorSystemTrait`
   * @param label Target schedule label
   * @param systems System instances
   * @return Handle for configuring the anonymous system set
   */
  template <ecs::ScheduleTrait L, ecs::FunctorSystemTrait... Systems>
    requires(sizeof...(Systems) > 1)
  ecs::SystemSetHandle AddSystems(const L& label, Systems&&... systems) {
    return scheduler_.In(label).Add(std::forward<Systems>(systems)...);
  }

  /**
   * @brief Gets or creates a system set in the given schedule.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam L Schedule label type
   * @tparam S System set type
   * @param label Target schedule label
   * @param set System set instance
   * @return Handle for configuring the set
   */
  template <ecs::ScheduleTrait L, ecs::SystemSetTrait S>
  ecs::SystemSetHandle ConfigureSet(const L& label = {}, const S& set = {}) {
    return scheduler_.In(label).Set(set);
  }

  /**
   * @brief Inserts or replaces resources in this sub-app's world.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam Ts Resource types
   * @param resources Resource values
   * @return Reference to this sub-app for chaining
   */
  template <ecs::ResourceTrait... Ts>
  auto InsertResources(this auto&& self, Ts&&... resources)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Inserts resources only when no instance exists yet.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam Ts Resource types
   * @param resources Resource values
   * @return Reference to this sub-app for chaining
   */
  template <ecs::ResourceTrait... Ts>
  auto TryInsertResources(this auto&& self, Ts&&... resources)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Registers multiple message types on this sub-app's world.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam Ts Message types
   * @return Reference to this sub-app for chaining
   */
  template <ecs::AnyMessageTrait... Ts>
    requires(sizeof...(Ts) > 0)
  auto AddMessages(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Sets the human-readable sub-app name.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param name Sub-app name
   */
  void SetName(std::string name) noexcept;

  /**
   * @brief Gets the human-readable sub-app name.
   * @note Not thread-safe.
   * @return Sub-app name
   */
  [[nodiscard]] const std::string& GetName() const noexcept { return name_; }

  /**
   * @brief Sets the function invoked each frame to run this sub-app's update
   * pass.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param runner Callable receiving `SubApp&` and `async::Executor&`; when
   * set, this is called instead of running the update stage directly
   */
  void SetRunner(RunnerFn runner) noexcept;

  /**
   * @brief Sets the function invoked by `Extract`.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam F Callable receiving `(const ecs::World&, ecs::World&)`
   * @param fn The function to set
   */
  template <typename F>
    requires std::invocable<F, const ecs::World&, ecs::World&>
  void SetExtractFunction(F&& fn);

  /**
   * @brief Sets whether extraction may be skipped while an update is in flight.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param allow_overlapping_updates True to permit extraction skips
   */
  void SetAllowOverlappingUpdates(bool allow_overlapping_updates) noexcept;

  /**
   * @brief Sets the maximum consecutive extraction skips while updating.
   * @details `0` means unlimited skips. Values `1`, `2`, ... cap how many main
   * frames may skip extraction before waiting for the in-flight update.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param max_skips Maximum consecutive extraction skips
   */
  void SetMaxExtractionSkips(size_t max_skips) noexcept;

  /**
   * @brief Sets whether this sub-app runs updates on a background loop.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @warning Does not change the runner. When @p is_async is true, call
   * `SetRunner(RunDefaultSubApp)` or `SetRunner(RunFixedSubApp)` (or a custom
   * equivalent) before `App::Initialize()` so the background loop keeps
   * updating until `ShouldExit()` becomes true.
   * @param is_async True for continuous background updates
   */
  void SetAsync(bool is_async) noexcept;

  /**
   * @brief Sets which stage label `Update` executes.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @param index Stage label type index
   */
  void SetUpdateStage(ecs::StageTypeIndex index) noexcept;

  /**
   * @brief Sets which stage label `Update` executes.
   * @note Not thread-safe.
   * @warning Triggers assertion if an update is in progress.
   * @tparam T Stage label type
   * @param stage Stage label instance
   */
  template <ecs::StageTrait T>
  void SetUpdateStage(const T& stage = {}) noexcept;

  /**
   * @brief Tries to get the schedule for a label.
   * @note Not thread-safe.
   * @tparam T Schedule label type
   * @param label Target schedule label
   * @return Pointer to the schedule, or `nullptr` if missing
   */
  template <ecs::ScheduleTrait T>
  [[nodiscard]] ecs::Schedule* TryGetSchedule(const T& label = {}) {
    return scheduler_.TryGetSchedule(label);
  }

  /**
   * @brief Tries to get the schedule for a label.
   * @note Not thread-safe.
   * @tparam T Schedule label type
   * @param label Target schedule label
   * @return Pointer to the schedule, or `nullptr` if missing
   */
  template <ecs::ScheduleTrait T>
  [[nodiscard]] const ecs::Schedule* TryGetSchedule(const T& label = {}) const {
    return scheduler_.TryGetSchedule(label);
  }

  /**
   * @brief Returns whether this sub-app should stop its update loop.
   * @details Propagates `App::ShouldExit()` from the owning application when
   * set. Also returns true when an async loop stop was requested during
   * shutdown.
   * @note Thread-safe.
   * @return True when the sub-app should exit its runner loop
   */
  [[nodiscard]] bool ShouldExit() const noexcept;

  /**
   * @brief Returns whether an update is in flight.
   * @note Thread-safe.
   * @return True while an update pass is running, false otherwise
   */
  [[nodiscard]] bool IsUpdating() const noexcept {
    return is_updating_.load(std::memory_order_acquire);
  }

  /**
   * @brief Returns whether extraction may be skipped while updating.
   * @note Not thread-safe.
   * @return True if overlapping extraction skips are enabled
   */
  [[nodiscard]] bool AllowsOverlappingUpdates() const noexcept {
    return allow_overlapping_updates_;
  }

  /**
   * @brief Returns whether this sub-app uses a background update loop.
   * @note Not thread-safe.
   * @return True for async sub-apps
   */
  [[nodiscard]] bool IsAsync() const noexcept { return is_async_; }

  /**
   * @brief Returns the configured maximum consecutive extraction skips.
   * @note Not thread-safe.
   * @return Skip budget; `0` means unlimited
   */
  [[nodiscard]] size_t MaxExtractionSkips() const noexcept {
    return max_extraction_skips_;
  }

  /**
   * @brief Gets this sub-app's ECS world.
   * @note Thread-safe for read access to returned world.
   * @return ECS world reference
   */
  [[nodiscard]] ecs::World& GetWorld() noexcept { return world_; }

  /**
   * @brief Gets this sub-app's ECS world.
   * @return ECS world reference
   */
  [[nodiscard]] const ecs::World& GetWorld() const noexcept { return world_; }

  /**
   * @brief Gets this sub-app's ECS schedule collection.
   * @note Thread-safe.
   * @return ECS scheduler reference
   */
  [[nodiscard]] ecs::Scheduler& GetScheduler() noexcept { return scheduler_; }

  /**
   * @brief Gets this sub-app's ECS schedule collection.
   * @note Thread-safe.
   * @return ECS scheduler reference
   */
  [[nodiscard]] const ecs::Scheduler& GetScheduler() const noexcept {
    return scheduler_;
  }

private:
  [[nodiscard]] bool TryBeginUpdate() noexcept;
  void EndUpdate() noexcept {
    is_updating_.store(false, std::memory_order_release);
  }

  void RunStartupUnchecked(async::Executor& executor) {
    RunStageUnchecked(executor, ecs::StageTypeIndex::From(kStartupStage),
                      world_);
  }

  void RunShutdownUnchecked(async::Executor& executor) {
    RunStageUnchecked(executor, ecs::StageTypeIndex::From(kShutdownStage),
                      world_);
  }

  void RunUpdatePass(async::Executor& executor);
  void RunStageUnchecked(async::Executor& executor, ecs::StageTypeIndex stage,
                         ecs::World& world);

  void RequestAsyncLoopStop() noexcept {
    async_loop_stop_.store(true, std::memory_order_release);
  }

  void ResetAsyncLoopStop() noexcept {
    async_loop_stop_.store(false, std::memory_order_release);
  }

  void SetOwnerApp(const App& app) noexcept { owner_app_ = &app; }

  [[nodiscard]] bool AsyncLoopStopRequested() const noexcept {
    return async_loop_stop_.load(std::memory_order_acquire);
  }

  std::string name_;

  ecs::World world_;
  ecs::Scheduler scheduler_;
  std::optional<ecs::StageTypeIndex> update_stage_ =
      ecs::StageTypeIndex::From(kUpdateStage);

  RunnerFn runner_;
  ExtractFn extract_fn_;

  std::atomic<bool> is_updating_{false};
  std::atomic<bool> async_loop_stop_{false};
  bool allow_overlapping_updates_ = false;
  bool is_async_ = false;
  size_t max_extraction_skips_ = 0;

  const App* owner_app_ = nullptr;

  mutable std::mutex stage_run_mutex_;

  friend class App;
  friend class Scheduler;
};

inline SubApp::SubApp(SubApp&& other) noexcept
    : name_(std::move(other.name_)),
      world_(std::move(other.world_)),
      scheduler_(std::move(other.scheduler_)),
      update_stage_(other.update_stage_),
      runner_(std::move(other.runner_)),
      extract_fn_(std::move(other.extract_fn_)),
      is_updating_(other.is_updating_.load(std::memory_order_acquire)),
      async_loop_stop_(other.async_loop_stop_.load(std::memory_order_acquire)),
      allow_overlapping_updates_(other.allow_overlapping_updates_),
      is_async_(other.is_async_),
      max_extraction_skips_(other.max_extraction_skips_),
      owner_app_(other.owner_app_) {
  HELIOS_ASSERT(!other.IsUpdating(), "Cannot move from updating sub-app!");
}

inline SubApp& SubApp::operator=(SubApp&& other) noexcept {
  HELIOS_ASSERT(!IsUpdating(), "Cannot assign while updating!");
  HELIOS_ASSERT(!other.IsUpdating(), "Cannot assign from updating sub-app!");

  if (this == &other) [[unlikely]] {
    return *this;
  }

  name_ = std::move(other.name_);
  world_ = std::move(other.world_);
  scheduler_ = std::move(other.scheduler_);
  update_stage_ = other.update_stage_;
  extract_fn_ = std::move(other.extract_fn_);
  runner_ = std::move(other.runner_);
  is_updating_.store(other.is_updating_.load(std::memory_order_acquire),
                     std::memory_order_release);
  async_loop_stop_.store(other.async_loop_stop_.load(std::memory_order_acquire),
                         std::memory_order_release);
  allow_overlapping_updates_ = other.allow_overlapping_updates_;
  is_async_ = other.is_async_;
  max_extraction_skips_ = other.max_extraction_skips_;
  owner_app_ = other.owner_app_;

  return *this;
}

inline void SubApp::Update(async::Executor& executor) {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_NAME(
      std::format("helios::app::SubApp::Update{{name: {}}}", GetName()));
  HELIOS_APP_PROFILE_ZONE_TEXT(std::format(
      "allow_overlapping_updates: {}, max_extraction_skips: {}, is_async: "
      "{}",
      allow_overlapping_updates_, max_extraction_skips_, is_async_));

  HELIOS_ASSERT(IsUpdating(),
                "Update must be called within an active update pass!");
  if (!update_stage_.has_value()) [[unlikely]] {
    log::Warn("Sub-app update stage is not set!");
    return;
  }

  RunStageUnchecked(executor, *update_stage_, world_);
}

inline void SubApp::Extract(const ecs::World& main_world,
                            [[maybe_unused]] bool allow_while_updating) {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_NAME(
      std::format("helios::app::SubApp::Extract{{name: {}}}", GetName()));
  HELIOS_APP_PROFILE_ZONE_TEXT(std::format(
      "allow_overlapping_updates: {}, max_extraction_skips: {}, is_async: {}",
      allow_overlapping_updates_, max_extraction_skips_, is_async_));

  HELIOS_ASSERT(!IsUpdating() || allow_while_updating,
                "Cannot extract while sub-app is updating!");

  if (extract_fn_) [[likely]] {
    extract_fn_(main_world, world_);
  }
}

inline void SubApp::BuildScheduler(async::Executor& executor) {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_NAME(std::format(
      "helios::app::SubApp::BuildScheduler{{name: {}}}", GetName()));
  HELIOS_APP_PROFILE_ZONE_TEXT(std::format(
      "allow_overlapping_updates: {}, max_extraction_skips: {}, is_async: {}",
      allow_overlapping_updates_, max_extraction_skips_, is_async_));

  if (scheduler_.IsDirty()) {
    scheduler_.Build(executor);
  }
}

template <ecs::ScheduleTrait T>
inline auto SubApp::InitSchedule(this auto&& self, const T& label)
    -> decltype(std::forward<decltype(self)>(self)) {
  if (self.scheduler_.TryGetSchedule(label) == nullptr) {
    self.scheduler_.Add(label, ecs::Schedule::From(label)).Done();
  }
  return std::forward<decltype(self)>(self);
}

template <ecs::ScheduleTrait L, typename F>
  requires std::invocable<F, ecs::Schedule&>
inline auto SubApp::EditSchedule(this auto&& self, const L& label, const F& fn)
    -> decltype(std::forward<decltype(self)>(self)) {
  fn(self.scheduler_.In(label));
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait... Ts>
inline auto SubApp::InsertResources(this auto&& self, Ts&&... resources)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.world_.InsertResources(std::forward<Ts>(resources)...);
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait... Ts>
inline auto SubApp::TryInsertResources(this auto&& self, Ts&&... resources)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.world_.TryInsertResources(std::forward<Ts>(resources)...);
  return std::forward<decltype(self)>(self);
}

template <ecs::AnyMessageTrait... Ts>
  requires(sizeof...(Ts) > 0)
inline auto SubApp::AddMessages(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.world_.template AddMessages<Ts...>();
  return std::forward<decltype(self)>(self);
}

inline void SubApp::SetName(std::string name) noexcept {
  HELIOS_ASSERT(!IsUpdating(), "Cannot set name while updating!");
  name_ = std::move(name);
}

inline void SubApp::SetRunner(RunnerFn runner) noexcept {
  HELIOS_ASSERT(!IsUpdating(), "Cannot set runner while updating!");
  HELIOS_ASSERT(runner, "Sub-app runner cannot be null!");
  runner_ = std::move(runner);
}

template <typename F>
  requires std::invocable<F, const ecs::World&, ecs::World&>
inline void SubApp::SetExtractFunction(F&& fn) {
  HELIOS_ASSERT(!IsUpdating(), "Cannot set extract function while updating!");
  extract_fn_ = std::forward<F>(fn);
}

inline void SubApp::SetAllowOverlappingUpdates(
    bool allow_overlapping_updates) noexcept {
  HELIOS_ASSERT(!IsUpdating(),
                "Cannot set allow overlapping updates while updating!");
  allow_overlapping_updates_ = allow_overlapping_updates;
}

inline void SubApp::SetMaxExtractionSkips(size_t max_skips) noexcept {
  HELIOS_ASSERT(!IsUpdating(),
                "Cannot set max extraction skips while updating!");
  max_extraction_skips_ = max_skips;
}

inline void SubApp::SetAsync(bool is_async) noexcept {
  HELIOS_ASSERT(!IsUpdating(), "Cannot set async while updating!");
  is_async_ = is_async;
}

inline void SubApp::SetUpdateStage(ecs::StageTypeIndex index) noexcept {
  HELIOS_ASSERT(!IsUpdating(), "Cannot set update stage while updating!");
  update_stage_ = index;
}

template <ecs::StageTrait T>
inline void SubApp::SetUpdateStage(const T& stage) noexcept {
  HELIOS_ASSERT(!IsUpdating(), "Cannot set update stage while updating!");
  update_stage_ = ecs::StageTypeIndex::From(stage);
}

}  // namespace helios::app
