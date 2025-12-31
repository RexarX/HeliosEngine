#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/module.hpp>
#include <helios/core/app/schedule.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_config.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/app/system_set_config.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/core.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::app {

/**
 * @brief Application exit codes.
 */
enum class AppExitCode : uint8_t {
  Success = 0,  ///< Successful execution
  Failure = 1,  ///< General failure
};

/**
 * @brief Application class.
 * @details Manages the application lifecycle, including initialization, updating, and shutdown.
 * @note Not thread-safe.
 */
class App {
public:
#ifdef HELIOS_MOVEONLY_FUNCTION_AVALIABLE
  using RunnerFn = std::move_only_function<AppExitCode(App&)>;
  using ExtractFn = std::move_only_function<void(const ecs::World&, ecs::World&)>;
#else
  using RunnerFn = std::function<AppExitCode(App&)>;
  using ExtractFn = std::function<void(const ecs::World&, ecs::World&)>;
#endif

  App() = default;

  /**
   * @brief Constructs an App with a specific number of worker threads.
   * @param worker_thread_count Number of worker threads for the executor
   */
  explicit App(size_t worker_thread_count) : executor_(worker_thread_count) {}

  App(const App&) = delete;
  App(App&&) = delete;

  /**
   * @brief Destructor that ensures all async work is completed before destruction.
   * @details Waits for all overlapping updates and pending executor tasks to complete.
   */
  ~App();

  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;

  /**
   * @brief Clears the application state, removing all data.
   * @warning Triggers assertion if app is running.
   */
  void Clear();

  /**
   * @brief Initializes the application and its subsystems.
   * @details Called before the main loop starts.
   * Spawns tasks on the executor for parallel initialization.
   * @note Automaticly called in Run().
   * @warning Triggers assertion if app is already initialized or running.
   */
  void Initialize();

  /**
   * @brief Updates the application and its subsystems.
   * @details Calls Update on the main sub-app and all registered sub-apps.
   * Spawns async tasks as needed.
   * @note Should not be called directly - use the runner function instead.
   * @warning Triggers assertion if app is not initialized.
   */
  void Update();

  /**
   * @brief Runs the application with the given arguments.
   * @warning Triggers assertion if app is initialized or running.
   * @return Exit code of the application.
   */
  AppExitCode Run();

  /**
   * @brief Waits for all overlapping sub-app updates to complete.
   * @details Can be called explicitly when synchronization is needed.
   * Automatically called during CleanUp().
   */
  void WaitForOverlappingUpdates();

  /**
   * @brief Waits for overlapping updates of a specific sub-app type to complete.
   * @details Can be called explicitly when synchronization is needed for a specific sub-app.
   * @tparam T Sub-app type
   * @param sub_app Sub-app type tag
   */
  template <SubAppTrait T>
  void WaitForOverlappingUpdates(T sub_app = {});

  /**
   * @brief Waits for overlapping updates of a specific sub-app instance to complete.
   * @details Can be called explicitly when synchronization is needed for a specific sub-app.
   * @param sub_app Reference to the sub-app to wait for
   */
  void WaitForOverlappingUpdates(const SubApp& sub_app);

  /**
   * @brief Adds a new sub-application of type T.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam T Sub-application type
   * @param sub_app Sub-app type tag
   * @return Reference to app for method chaining
   */
  template <SubAppTrait T>
  auto AddSubApp(this auto&& self, T sub_app = {}) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an existing sub-application instance.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam T Sub-application type
   * @param sub_app Sub-application instance to add
   * @param sub_app_tag Sub-app type tag
   * @return Reference to app for method chaining
   */
  template <SubAppTrait T>
  auto AddSubApp(this auto&& self, SubApp sub_app, T sub_app_tag = {}) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a module to the main sub-app.
   * @details Modules can add their own systems, resources, and sub-apps.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam T Module type
   * @return Reference to app for method chaining
   */
  template <ModuleTrait T>
  auto AddModule(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds multiple modules to the main sub-app.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam Modules Module types
   * @return Reference to app for method chaining
   */
  template <ModuleTrait... Modules>
    requires(sizeof...(Modules) > 1 && utils::UniqueTypes<Modules...>)
  auto AddModules(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a system to the specified schedule in the main sub-app.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam T System type
   * @tparam Schedule Schedule type
   * @param schedule Schedule to add system to
   * @return Reference to app for method chaining
   */
  template <ecs::SystemTrait T, ScheduleTrait Schedule>
  auto AddSystem(this auto&& self, Schedule schedule = {}) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds multiple systems to the specified schedule in the main sub-app.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam Systems System types
   * @tparam Schedule Schedule type
   * @param schedule Schedule to add systems to
   * @return Reference to app for method chaining
   */
  template <ecs::SystemTrait... Systems, ScheduleTrait Schedule>
    requires(sizeof...(Systems) > 1 && utils::UniqueTypes<Systems...>)
  auto AddSystems(this auto&& self, Schedule schedule = {}) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds systems with fluent configuration builder.
   * @details Returns a builder that allows chaining configuration methods like
   * .After(), .Before(), .InSet(), and .Sequence().
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam Systems System types to add
   * @tparam Schedule Schedule type
   * @param schedule Schedule to add systems to
   * @return SystemConfig builder for fluent configuration
   *
   * @example
   * @code
   * app.AddSystemsBuilder<MovementSystem, CollisionSystem>(kUpdate)
   *     .After<InputSystem>()
   *     .Before<RenderSystem>()
   *     .InSet<PhysicsSet>()
   *     .Sequence();
   * @endcode
   */
  template <ecs::SystemTrait... Systems, ScheduleTrait Schedule>
    requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
  auto AddSystemsBuilder(this auto&& self, Schedule schedule = {}) -> SystemConfig<Schedule, Systems...>;

  /**
   * @brief Adds a single system with fluent configuration builder.
   * @details Returns a builder that allows chaining configuration methods.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam T System type to add
   * @tparam Schedule Schedule type
   * @param schedule Schedule to add system to
   * @return SystemConfig builder for fluent configuration
   */
  template <ecs::SystemTrait T, ScheduleTrait Schedule>
  auto AddSystemBuilder(this auto&& self, Schedule schedule = {}) -> SystemConfig<Schedule, T>;

  /**
   * @brief Configures a system set with fluent builder.
   * @details Returns a builder that allows configuring set ordering.
   * @warning Triggers an assertion if app is initialized or running.
   * @tparam Set System set type to configure
   * @tparam Schedule Schedule type
   * @param schedule Schedule where the set is configured
   * @return SystemSetConfig builder for fluent configuration
   *
   * @example
   * @code
   * app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>().Before<RenderSet>();
   * @endcode
   */
  template <SystemSetTrait Set, ScheduleTrait Schedule>
  auto ConfigureSet(this auto&& self, Schedule schedule = {}) -> SystemSetConfig<Schedule, Set>;

  /**
   * @brief Inserts a resource into the main sub-app.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Resource type
   * @param resource Resource to insert
   * @return Reference to app for method chaining
   */
  template <ecs::ResourceTrait T>
  auto InsertResource(this auto&& self, T&& resource) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Emplaces a resource into the main sub-app's world.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Resource type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to resource constructor
   * @return Reference to app for method chaining
   */
  template <ecs::ResourceTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  auto EmplaceResource(this auto&& self, Args&&... args) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Registers an event type for use in the main sub-app.
   * @details Events must be registered before they can be written or read.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Event type
   * @return Reference to app for method chaining
   */
  template <ecs::EventTrait T>
  auto AddEvent(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Registers multiple event types for use in the main sub-app.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Events Event types to register
   * @return Reference to app for method chaining
   */
  template <ecs::EventTrait... Events>
    requires(sizeof...(Events) > 1)
  auto AddEvents(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Sets runner function for the application.
   * @warning Triggers assertion if app is initialized, running, or runner is null.
   * @param runner A move-only function that takes an App reference and returns an AppExitCode.
   * @return Reference to app for method chaining
   */
  auto SetRunner(this auto&& self, RunnerFn runner) noexcept -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Sets extraction function for a sub-app.
   * @details The extraction function is called before the sub-app's Update to copy data from the main world.
   * @warning Triggers assertion if app is initialized, running, extraction function is null, or sub-app doesn't exist.
   * @tparam T Sub-application type
   * @param extract_fn Function that takes main world and sub-app world references
   * @return Reference to app for method chaining
   */
  template <SubAppTrait T>
  auto SetSubAppExtraction(this auto&& self, ExtractFn extract_fn) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Checks if a sub app of type T exists in main sub-app.
   * @tparam T SubApp type
   * @return True if sub app exists, false otherwise
   */
  template <SubAppTrait T>
  [[nodiscard]] bool ContainsSubApp() const noexcept;

  /**
   * @brief Checks if a module of type T exists in main sub-app.
   * @tparam T Module type
   * @return True if module exists, false otherwise
   */
  template <ModuleTrait T>
  [[nodiscard]] bool ContainsModule() const noexcept;

  /**
   * @brief Checks if a system of type T exists in any schedule of main sub-app.
   * @tparam T System type
   * @return True if system exists, false otherwise
   */
  template <ecs::SystemTrait T>
  [[nodiscard]] bool ContainsSystem() const noexcept {
    return main_sub_app_.ContainsSystem<T>();
  }

  /**
   * @brief Checks if a system of type T exists in the specified schedule of main sub-app.
   * @tparam T System type
   * @tparam Schedule Schedule type
   * @param schedule Schedule to check
   * @return True if system exists, false otherwise
   */
  template <ecs::SystemTrait T, ScheduleTrait Schedule>
  [[nodiscard]] bool ContainsSystem(Schedule schedule = {}) const noexcept {
    return main_sub_app_.ContainsSystem<T>(schedule);
  }

  /**
   * @brief Checks if the app has been initialized.
   * @return True if initialized, false otherwise
   */
  [[nodiscard]] bool IsInitialized() const noexcept { return is_initialized_; }

  /**
   * @brief Checks if the app is currently running.
   * @return True if running, false otherwise
   */
  [[nodiscard]] bool IsRunning() const noexcept { return is_running_.load(std::memory_order_acquire); }

  /**
   * @brief Checks if a resource exists in main sub_app.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] bool HasResource() const noexcept {
    return main_sub_app_.HasResource<T>();
  }

  /**
   * @brief Checks if a event is registered in main sub_app.
   * @tparam T Event type
   * @return True if event exists, false otherwise
   */
  template <ecs::EventTrait T>
  [[nodiscard]] bool HasEvent() const noexcept {
    return main_sub_app_.HasEvent<T>();
  }

  /**
   * @brief Gets the number of modules in main sub-app.
   * @return Number of modules
   */
  [[nodiscard]] size_t ModuleCount() const noexcept { return modules_.size(); }

  /**
   * @brief Gets the total number of systems across all schedules in main sub-app.
   * @return Total number of systems
   */
  [[nodiscard]] size_t SystemCount() const noexcept { return main_sub_app_.SystemCount(); }

  /**
   * @brief Gets the number of systems in the specified schedule of main sub-app.
   * @tparam Schedule Schedule type
   * @param schedule Schedule to query
   * @return Number of systems in the schedule
   */
  template <ScheduleTrait Schedule>
  [[nodiscard]] size_t SystemCount(Schedule schedule = {}) const noexcept {
    return main_sub_app_.SystemCount(schedule);
  }

  /**
   * @brief Gets a sub-application by type.
   * @warning Triggers an assertion if the sub-app does not exist.
   * Reference might be invalidated due to reallocation of storage after addition of new sub apps.
   * @tparam T Sub-application type
   * @return Reference to the sub-application
   */
  template <SubAppTrait T>
  [[nodiscard]] SubApp& GetSubApp() noexcept;

  /**
   * @brief Gets a sub-application by type (const version).
   * @warning Triggers an assertion if the sub-app does not exist.
   * @tparam T Sub-application type
   * @return Const reference to the sub-application
   */
  template <SubAppTrait T>
  [[nodiscard]] const SubApp& GetSubApp() const noexcept;

  /**
   * @brief Gets the main sub-application.
   * @return Reference to the main sub-application
   */
  [[nodiscard]] SubApp& GetMainSubApp() noexcept { return main_sub_app_; }

  /**
   * @brief Gets the main sub-application (const version).
   * @return Const reference to the main sub-application
   */
  [[nodiscard]] const SubApp& GetMainSubApp() const noexcept { return main_sub_app_; }

  /**
   * @brief Gets the main world.
   * @return Reference to the main world
   */
  [[nodiscard]] const ecs::World& GetMainWorld() const noexcept { return main_sub_app_.GetWorld(); }

  /**
   * @brief Gets the async executor.
   * @return Reference to the async executor
   */
  [[nodiscard]] async::Executor& GetExecutor() noexcept { return executor_; }

  /**
   * @brief Gets the async executor (const version).
   * @return Const reference to the async executor
   */
  [[nodiscard]] const async::Executor& GetExecutor() const noexcept { return executor_; }

private:
  /**
   * @brief Cleans up the application and its subsystems.
   * @details Called after the main loop ends.
   * Spawns tasks on the executor for parallel cleanup.
   */
  void CleanUp();

  /**
   * @brief Builds all registered modules.
   * @details Calls Build on each module after all modules have been added.
   */
  void BuildModules();

  /**
   * @brief Destroys all registered modules.
   * @details Calls Destroy on each module.
   */
  void DestroyModules();

  [[nodiscard]] static AppExitCode DefaultRunner(App& app);

  bool is_initialized_ = false;          ///< Whether the app has been initialized
  std::atomic<bool> is_running_{false};  ///< Whether the app is currently running

  SubApp main_sub_app_;                                         ///< The main sub-application
  std::vector<SubApp> sub_apps_;                                ///< List of additional sub-applications
  std::unordered_map<SubAppTypeId, size_t> sub_app_index_map_;  ///< Map of sub-application type IDs to their indices

  std::vector<std::unique_ptr<Module>> modules_;               ///< Owned modules
  std::unordered_map<ModuleTypeId, size_t> module_index_map_;  ///< Map of module type IDs to their indices

  async::Executor executor_;       ///< Async executor for parallel execution
  async::TaskGraph update_graph_;  ///< Task graph for managing updates

  RunnerFn runner_ = DefaultRunner;  ///< The runner function

  /// Map from sub-app index to their overlapping shared futures.
  std::unordered_map<size_t, std::vector<std::shared_future<void>>> sub_app_overlapping_futures_;
};

inline App::~App() {
  // Ensure we're not running (should already be false if properly shut down)
  is_running_.store(false, std::memory_order_release);

  // Wait for any pending overlapping sub-app updates
  WaitForOverlappingUpdates();

  // Wait for all pending executor tasks to complete
  executor_.WaitForAll();
}

inline void App::Clear() {
  HELIOS_ASSERT(!IsRunning(), "Failed to clear app: Cannot clear app while it is running!");

  WaitForOverlappingUpdates();  // Ensure all async work is done
  sub_app_overlapping_futures_.clear();
  main_sub_app_.Clear();
  sub_apps_.clear();
  sub_app_index_map_.clear();

  is_initialized_ = false;
}

template <SubAppTrait T>
inline auto App::AddSubApp(this auto&& self, T /*sub_app*/) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add sub app '{}': Cannot add sub apps after app initialization!",
                SubAppNameOf<T>());

  HELIOS_ASSERT(!self.IsRunning(), "Failed to add sub app '{}': Cannot add sub apps while app is running!",
                SubAppNameOf<T>());

  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  if (self.sub_app_index_map_.contains(id)) [[unlikely]] {
    HELIOS_WARN("Sub app '{}' is already exist in app!", SubAppNameOf<T>());
    return std::forward<decltype(self)>(self);
  }

  self.sub_app_index_map_[id] = self.sub_apps_.size();
  self.sub_apps_.emplace_back();

  // Set overlapping updates flag based on trait
  constexpr bool allow_overlapping = SubAppAllowsOverlappingUpdates<T>();
  self.sub_apps_.back().SetAllowOverlappingUpdates(allow_overlapping);

  // Set max overlapping updates based on trait
  constexpr size_t max_overlapping = SubAppMaxOverlappingUpdates<T>();
  self.sub_apps_.back().SetMaxOverlappingUpdates(max_overlapping);

  return std::forward<decltype(self)>(self);
}

template <SubAppTrait T>
inline auto App::AddSubApp(this auto&& self, SubApp sub_app, T /*sub_app_tag*/)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add sub app '{}': Cannot add sub apps after app initialization!",
                SubAppNameOf<T>());

  HELIOS_ASSERT(!self.IsRunning(), "Failed to add sub app '{}': Cannot add sub apps while app is running!",
                SubAppNameOf<T>());

  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  if (self.sub_app_index_map_.contains(id)) [[unlikely]] {
    HELIOS_WARN("Sub app '{}' is already exist in app!", SubAppNameOf<T>());
    return std::forward<decltype(self)>(self);
  }

  // Set overlapping updates flag based on trait before adding
  constexpr bool allow_overlapping = SubAppAllowsOverlappingUpdates<T>();
  sub_app.SetAllowOverlappingUpdates(allow_overlapping);

  // Set max overlapping updates based on trait
  constexpr size_t max_overlapping = SubAppMaxOverlappingUpdates<T>();
  sub_app.SetMaxOverlappingUpdates(max_overlapping);

  self.sub_app_index_map_[id] = self.sub_apps_.size();
  self.sub_apps_.push_back(std::move(sub_app));

  return std::forward<decltype(self)>(self);
}

template <ModuleTrait T>
inline auto App::AddModule(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add module '{}': Cannot add modules after app initialization!",
                ModuleNameOf<T>());

  HELIOS_ASSERT(!self.IsRunning(), "Failed to add module '{}': Cannot add modules while app is running!",
                ModuleNameOf<T>());

  // module->Build(self);  // Will be called after all modules are added
  constexpr ModuleTypeId id = ModuleTypeIdOf<T>();
  if (self.module_index_map_.contains(id)) [[unlikely]] {
    HELIOS_WARN("Module '{}' is already exist in app!", ModuleNameOf<T>());
    return std::forward<decltype(self)>(self);
  }

  self.module_index_map_[id] = self.modules_.size();
  auto module = std::make_unique<T>();
  self.modules_.push_back(std::move(module));
  return std::forward<decltype(self)>(self);
}

template <ModuleTrait... Modules>
  requires(sizeof...(Modules) > 1 && utils::UniqueTypes<Modules...>)
inline auto App::AddModules(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add modules: Cannot add modules after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Failed to add modules: Cannot add modules while app is running!");

  (self.template AddModule<Modules>(), ...);
  return std::forward<decltype(self)>(self);
}

template <ecs::SystemTrait T, ScheduleTrait Schedule>
inline auto App::AddSystem(this auto&& self, Schedule schedule) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add system '{}': Cannot add systems after app initialization!",
                ecs::SystemNameOf<T>());

  HELIOS_ASSERT(!self.IsRunning(), "Failed to add system '{}': Cannot add systems while app is running!",
                ecs::SystemNameOf<T>());

  if (self.template ContainsSystem<T>(schedule)) [[unlikely]] {
    HELIOS_WARN("System '{}' is already exist in app schedule '{}'!", ecs::SystemNameOf<T>(),
                ScheduleNameOf<Schedule>());
    return std::forward<decltype(self)>(self);
  }

  self.GetMainSubApp().template AddSystem<T>(schedule);
  return std::forward<decltype(self)>(self);
}

template <ecs::SystemTrait... Systems, ScheduleTrait Schedule>
  requires(sizeof...(Systems) > 1 && utils::UniqueTypes<Systems...>)
inline auto App::AddSystems(this auto&& self, Schedule schedule) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add systems: Cannot add systems after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Failed to add systems: Cannot add systems while app is running!");

  self.GetMainSubApp().template AddSystems<Systems...>(schedule);
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait T>
inline auto App::InsertResource(this auto&& self, T&& resource) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to insert resource '{}': Cannot add resources after app initialization!",
                ecs::ResourceNameOf<T>());

  self.GetMainSubApp().InsertResource(std::forward<T>(resource));
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline auto App::EmplaceResource(this auto&& self, Args&&... args) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Failed to emplace resource '{}': Cannot add resources after app initialization!",
                ecs::ResourceNameOf<T>());

  HELIOS_ASSERT(!self.IsRunning(), "Failed to emplace resource '{}': Cannot add resources while app is running!",
                ecs::ResourceNameOf<T>());

  self.GetMainSubApp().template EmplaceResource<T>(std::forward<Args>(args)...);
  return std::forward<decltype(self)>(self);
}

template <ecs::EventTrait T>
inline auto App::AddEvent(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add event '{}': Cannot add events after app initialization!",
                ecs::EventNameOf<T>());

  HELIOS_ASSERT(!self.IsRunning(), "Failed to add event '{}': Cannot add events while app is running!",
                ecs::EventNameOf<T>());

  if (self.template HasEvent<T>()) [[unlikely]] {
    HELIOS_WARN("Event '{}' is already exist in app!", ecs::EventNameOf<T>());
    return std::forward<decltype(self)>(self);
  }

  self.GetMainSubApp().template AddEvent<T>();
  return std::forward<decltype(self)>(self);
}

template <ecs::EventTrait... Events>
  requires(sizeof...(Events) > 1)
inline auto App::AddEvents(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add events: Cannot add events after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Failed to add events: Cannot add events while app is running!");

  self.GetMainSubApp().template AddEvents<Events...>();
  return std::forward<decltype(self)>(self);
}

inline auto App::SetRunner(this auto&& self, RunnerFn runner) noexcept -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to set runner: Cannot set runner after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Failed to set runner: Cannot set runner while app is running!");

  HELIOS_ASSERT(runner, "Failed to set runner: Runner function cannot be null!");

  self.runner_ = std::move(runner);
  return std::forward<decltype(self)>(self);
}

template <SubAppTrait T>
inline auto App::SetSubAppExtraction(this auto&& self, ExtractFn extract_fn)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(
      !self.IsInitialized(),
      "Failed to set extraction function for sub app '{}': Cannot set extraction function after app initialization!",
      SubAppNameOf<T>());

  HELIOS_ASSERT(
      !self.IsRunning(),
      "Failed to set extraction function for sub app '{}': Cannot set extraction function while app is running!",
      SubAppNameOf<T>());

  HELIOS_ASSERT(extract_fn, "Failed to set extraction function for sub app '{}': Extraction function cannot be null!",
                SubAppNameOf<T>());

  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  const auto it = self.sub_app_index_map_.find(id);
  HELIOS_ASSERT(it != self.sub_app_index_map_.end(),
                "Failed to set extraction function for sub app '{}': Sub app does not exist!", SubAppNameOf<T>());

  self.sub_apps_.at(it->second).SetExtractFunction(std::move(extract_fn));
  return std::forward<decltype(self)>(self);
}

template <SubAppTrait T>
inline bool App::ContainsSubApp() const noexcept {
  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  return sub_app_index_map_.contains(id);
}

template <ModuleTrait T>
inline bool App::ContainsModule() const noexcept {
  constexpr ModuleTypeId id = ModuleTypeIdOf<T>();
  return module_index_map_.contains(id);
}

template <SubAppTrait T>
inline SubApp& App::GetSubApp() noexcept {
  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  const auto it = sub_app_index_map_.find(id);
  HELIOS_ASSERT(it != sub_app_index_map_.end(), "Failed to get sub app '{}': Sub app does not exist!",
                SubAppNameOf<T>());
  return sub_apps_.at(it->second);
}

template <SubAppTrait T>
inline const SubApp& App::GetSubApp() const noexcept {
  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  const auto it = sub_app_index_map_.find(id);
  HELIOS_ASSERT(it != sub_app_index_map_.end(), "Failed to get sub app '{}': Sub app does not exist!",
                SubAppNameOf<T>());
  return sub_apps_.at(it->second);
}

template <SubAppTrait T>
inline void App::WaitForOverlappingUpdates(T /*sub_app*/) {
  constexpr SubAppTypeId id = SubAppTypeIdOf<T>();
  const auto it = sub_app_index_map_.find(id);
  if (it == sub_app_index_map_.end()) [[unlikely]] {
    return;  // Sub-app doesn't exist
  }

  const size_t index = it->second;
  const auto futures_it = sub_app_overlapping_futures_.find(index);
  if (futures_it == sub_app_overlapping_futures_.end()) {
    return;  // No overlapping futures for this sub-app
  }

  for (auto& future : futures_it->second) {
    if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      future.wait();
    }
  }
  futures_it->second.clear();
}

template <ecs::SystemTrait... Systems, ScheduleTrait Schedule>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
inline auto App::AddSystemsBuilder(this auto&& self, Schedule schedule) -> SystemConfig<Schedule, Systems...> {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add systems: Cannot add systems after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Failed to add systems: Cannot add systems while app is running!");

  return self.GetMainSubApp().template AddSystemsBuilder<Systems...>(schedule);
}

template <ecs::SystemTrait T, ScheduleTrait Schedule>
inline auto App::AddSystemBuilder(this auto&& self, Schedule schedule) -> SystemConfig<Schedule, T> {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to add system '{}': Cannot add system after app initialization!",
                ecs::SystemNameOf<T>());
  HELIOS_ASSERT(!self.IsRunning(), "Failed to add system '{}': Cannot add system while app is running!",
                ecs::SystemNameOf<T>());

  return self.GetMainSubApp().template AddSystemBuilder<T>(schedule);
}

template <SystemSetTrait Set, ScheduleTrait Schedule>
inline auto App::ConfigureSet(this auto&& self, Schedule schedule) -> SystemSetConfig<Schedule, Set> {
  HELIOS_ASSERT(!self.IsInitialized(), "Failed to configure set '{}': Cannot configure set after app initialization!",
                SystemSetNameOf<Set>());
  HELIOS_ASSERT(!self.IsRunning(), "Failed to configure set '{}': Cannot configure set while app is running!",
                SystemSetNameOf<Set>());

  return self.GetMainSubApp().template ConfigureSet<Set>(schedule);
}

}  // namespace helios::app
