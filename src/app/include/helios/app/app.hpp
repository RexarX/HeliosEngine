#pragma once

#include <helios/app/details/profile.hpp>
#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/scheduler.hpp>
#include <helios/app/schedules.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>
#include <helios/utils/common_traits.hpp>

#include <array>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#include <format>
#endif

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

namespace helios::app {

/// @brief Application exit codes.
enum class ExitCode : uint8_t {
  kSuccess = 0,  ///< Successful execution
  kFailure = 1,  ///< General failure
};

/**
 * @brief Message that requests the application to exit.
 * @details Written by systems; `RunDefault` stops when this message is present.
 * Uses manual clear policy so it survives per-schedule `World::Update()` sync
 * points within a frame.
 */
struct AppExit {
  static constexpr std::string_view kName = "AppExit";
  static constexpr ecs::MessageClearPolicy kClearPolicy =
      ecs::MessageClearPolicy::kManual;
  static constexpr bool kAsync = false;
  static constexpr bool kConsumable = false;

  ExitCode code = ExitCode::kSuccess;
};

/**
 * @brief Application class.
 * @details Manages the application lifecycle, including initialization,
 * updating, and shutdown.
 * @note Partially thread-safe.
 */
class App {
public:
#ifdef HELIOS_MOVEONLY_FUNCTION_AVAILABLE
  using RunnerFn = std::move_only_function<ExitCode(App&)>;
  using ExtractFn =
      std::move_only_function<void(const ecs::World&, ecs::World&)>;
#else
  using RunnerFn = std::function<ExitCode(App&)>;
  using ExtractFn = std::function<void(const ecs::World&, ecs::World&)>;
#endif

  App();

  /**
   * @brief Constructs an App with a specific number of worker threads.
   * @param worker_thread_count Number of worker threads for the executor
   */
  explicit App(size_t worker_thread_count);
  App(const App&) = delete;
  App(App&&) = delete;

  /**
   * @brief Destructor that ensures all async work is completed before
   * destruction.
   * @details Waits for all overlapping updates and pending executor tasks to
   * complete.
   */
  ~App();

  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;

  /**
   * @brief Clears the application state, removing all data.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is running.
   */
  void Clear();

  /**
   * @brief Initializes the application and its subsystems.
   * @details Called before the main loop starts.
   * Spawns tasks on the executor for parallel initialization.
   *
   * Automaticly called in `Run()`.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is already initialized or running.
   */
  void Initialize();

  /**
   * @brief Updates the application and its subsystems.
   * @details Calls Update on the main sub-app and all registered sub-apps.
   * Spawns async tasks as needed.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is not initialized.
   */
  void Update();

  /**
   * @brief Runs the application with the given arguments.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @return Exit code of the application.
   */
  ExitCode Run();

  /**
   * @brief Notifies the app that plugin readiness may have changed.
   * @details Async plugins should call this after completing background setup
   * so `WaitForPluginsReady` can resume without busy-waiting.
   * @note Not thread-safe.
   */
  void NotifyPluginReadinessChanged() noexcept {
    plugins_ready_cv_.notify_all();
  }

  /**
   * @brief Adds plugin instance, if plugin of the same type is not already
   * registered.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Plugin Plugin type
   * @param plugin Plugin instance to add
   * @return Reference to app for method chaining
   */
  template <PluginTrait Plugin>
  auto AddPlugins(this auto&& self, Plugin&& plugin)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds multiple plugin instances, if plugins of the same types are not
   * already registered.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Plugins Plugin types
   * @param plugins Plugin instances to add
   * @return Reference to app for method chaining
   */
  template <PluginTrait... Plugins>
    requires utils::UniqueTypes<Plugins...> && (sizeof...(Plugins) > 1)
  auto AddPlugins(this auto&& self, Plugins&&... plugins)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds one plugin, constructing it in-place with the given arguments
   * if a plugin of the same type is not already registered.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Plugin Plugin type
   * @param args Arguments to construct the plugin
   * @return Reference to app for method chaining
   */
  template <PluginTrait Plugin, typename... Args>
    requires std::constructible_from<Plugin, Args...>
  auto EmplacePlugin(this auto&& self, Args&&... args)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a dynamic plugin instance, if a plugin of the same type is not
   * already registered.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @param plugin Dynamic plugin instance to add
   * @return Reference to app for method chaining
   */
  auto AddDynamicPlugin(this auto&& self, DynamicPlugin plugin)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Inserts a sub-app under the given label, if a sub-app with the same
   * label is not already registered.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Sub-app label type
   * @param label Label value
   * @param sub_app Sub-app instance (executor is injected by `App`)
   * @return Reference to this app for chaining
   */
  template <SubAppTrait T>
  auto InsertSubApp(this auto&& self, const T& label, SubApp sub_app)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Removes a registered sub-app.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Sub-app label type
   * @return True if a sub-app was removed
   */
  template <SubAppTrait T>
  bool RemoveSubApp(const T& label = {});

  /**
   * @brief Adds a schedule to the main sub-app.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @param id Schedule type id
   * @param schedule Schedule instance
   * @return Ordering handle for configuring placement
   */
  auto AddSchedule(ecs::ScheduleTypeId id, ecs::Schedule&& schedule)
      -> ecs::ScheduleOrdering;

  /**
   * @brief Adds a schedule to the main sub-app.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Schedule label type
   * @param label Label value
   * @param schedule Schedule instance
   * @return Ordering handle for configuring placement
   */
  template <ecs::ScheduleTrait T>
  auto AddSchedule(const T& label, ecs::Schedule&& schedule)
      -> ecs::ScheduleOrdering;

  /**
   * @brief Creates an empty schedule on the main sub-app when missing.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam T Schedule label type
   * @param label Label value
   * @return Reference to this app for chaining
   */
  template <ecs::ScheduleTrait T>
  auto InitSchedule(this auto&& self, const T& label = {})
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Mutates a main sub-app schedule in place.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam L Schedule label type
   * @param label Target schedule label
   * @param fn Callable receiving `ecs::Schedule&`
   * @return Reference to this app for chaining
   */
  template <ecs::ScheduleTrait L, typename F>
    requires std::invocable<F, ecs::Schedule&>
  auto EditSchedule(this auto&& self, const L& label, const F& fn)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a functor system to a main sub-app schedule.
   * @details Derives name and `SystemId` from the functor type. Lambdas
   * are rejected; use the `AddSystem(label, name, system)` overload.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam L Schedule label type
   * @tparam S System type satisfying `FunctorSystemTrait`
   * @param label Target schedule label
   * @param system System instance
   * @param options Local data options
   * @return System configuration handle
   */
  template <ecs::ScheduleTrait L, ecs::FunctorSystemTrait S>
  auto AddSystem(const L& label, S&& system,
                 ecs::SystemLocalDataOptions options = {})
      -> ecs::SystemHandle {
    return main_sub_app_.AddSystem(label, std::forward<S>(system), options);
  }

  /**
   * @brief Adds a system to a main sub-app schedule with an explicit name.
   * @details Required for lambdas. Derives the `SystemId` from the
   * provided name, enabling multiple instances of the same type.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam L Schedule label type
   * @tparam S System type satisfying `SystemTrait`
   * @param label Target schedule label
   * @param name Human-readable system name
   * @param system System instance
   * @param options Local data options
   * @return System configuration handle
   */
  template <ecs::ScheduleTrait L, ecs::SystemTrait S>
  auto AddSystem(const L& label, std::string name, S&& system,
                 ecs::SystemLocalDataOptions options = {})
      -> ecs::SystemHandle {
    return main_sub_app_.AddSystem(label, std::move(name),
                                   std::forward<S>(system), options);
  }

  /**
   * @brief Adds multiple functor systems to the same main sub-app schedule.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam L Schedule label type
   * @tparam Systems System types satisfying `FunctorSystemTrait`
   * @param label Target schedule label
   * @param systems System instances
   * @return Handle for configuring the anonymous system set
   */
  template <ecs::ScheduleTrait L, ecs::FunctorSystemTrait... Systems>
    requires(sizeof...(Systems) > 1)
  ecs::SystemSetHandle AddSystems(const L& label, Systems&&... systems);

  /**
   * @brief Gets or creates a system set in a main sub-app schedule.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam L Schedule label type
   * @tparam S System set type
   * @param label Target schedule label
   * @param set System set instance
   * @return System set handle
   */
  template <ecs::ScheduleTrait L, ecs::SystemSetTrait S>
  auto ConfigureSet(const L& label, const S& set = {}) -> ecs::SystemSetHandle {
    return main_sub_app_.ConfigureSet(label, set);
  }

  /**
   * @brief Inserts or replaces resources in the main world.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Ts Resource types
   * @param resources Resource values
   * @return Reference to this app for chaining
   */
  template <ecs::ResourceTrait... Ts>
  auto InsertResources(this auto&& self, Ts&&... resources)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Inserts resources in the main world only when absent.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Ts Resource types
   * @param resources Resource values
   * @return Reference to this app for chaining
   */
  template <ecs::ResourceTrait... Ts>
  auto TryInsertResources(this auto&& self, Ts&&... resources)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Registers multiple message types on the main world.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @tparam Ts Message types
   * @return Reference to this app for chaining
   */
  template <ecs::AnyMessageTrait... Ts>
    requires(sizeof...(Ts) > 0)
  auto AddMessages(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Sets runner function for the application.
   * @note Not thread-safe.
   * @warning Triggers assertion if app is initialized or running.
   * @warning Triggers assertion in next cases:
   * - App is initialized
   * - App is running
   * - Runner is null
   * @param runner A move-only function that takes an `App` reference and
   * returns an `ExitCode`.
   * @return Reference to app for method chaining
   */
  auto SetRunner(this auto&& self, RunnerFn runner) noexcept
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Returns an exit code if an `AppExit` message was written this frame.
   * @note Not thread-safe.
   * @return Exit code when `AppExit` is present, otherwise `std::nullopt`
   */
  [[nodiscard]] auto ShouldExit() const noexcept -> std::optional<ExitCode>;

  /**
   * @brief Checks if the app has been initialized.
   * @note Not thread-safe.
   * @return True if initialized, false otherwise
   */
  [[nodiscard]] bool IsInitialized() const noexcept { return is_initialized_; }

  /**
   * @brief Checks if the app is currently running.
   * @note Thread-safe.
   * @return True if running, false otherwise
   */
  [[nodiscard]] bool IsRunning() const noexcept {
    return is_running_.load(std::memory_order_acquire);
  }

  /**
   * @brief Checks if the app has a plugin of the given type.
   * @note Not thread-safe.
   * @return True if the plugin is registered, false otherwise
   */
  [[nodiscard]] bool HasPlugin(PluginTypeIndex index) const noexcept {
    return plugins_.contains(index) || dynamic_plugins_.contains(index);
  }

  /**
   * @brief Checks if the app has a plugin of the given type.
   * @note Not thread-safe.
   * @return True if the plugin is registered, false otherwise
   */
  template <PluginTrait Plugin>
  [[nodiscard]] bool HasPlugins() const noexcept {
    return HasPlugin(PluginTypeIndex::From<Plugin>());
  }

  /**
   * @brief Checks if the app has plugins of  given types.
   * @note Not thread-safe.
   * @return Array of bools indicating whether each plugin is registered
   */
  template <PluginTrait... Plugins>
    requires utils::UniqueTypes<Plugins...> && (sizeof...(Plugins) > 1)
  [[nodiscard]] auto HasPlugins() const noexcept
      -> std::array<bool, sizeof...(Plugins)> {
    return {HasPlugin(PluginTypeIndex::From<Plugins>())...};
  }

  /**
   * @brief Returns whether a sub-app is registered for the label.
   * @tparam T Sub-app label type
   * @param label Sub-app label to check
   * @return True if the sub-app is registered, false otherwise
   */
  template <SubAppTrait T>
  [[nodiscard]] bool HasSubApp(const T& label = {}) const noexcept {
    return sub_apps_.contains(SubAppTypeIndex::From(label));
  }

  /**
   * @brief Gets the main sub-application.
   * @note Thread-safe.
   * @return Reference to the main sub-application
   */
  [[nodiscard]] SubApp& GetMainSubApp() noexcept { return main_sub_app_; }

  /**
   * @brief Gets the main sub-application (const version).
   * @note Thread-safe.
   * @return Const reference to the main sub-application
   */
  [[nodiscard]] const SubApp& GetMainSubApp() const noexcept {
    return main_sub_app_;
  }

  /**
   * @brief Gets the sub-app of the given type.
   * @note Not thread-safe.
   * @warning Triggers assertion if the sub-app is not found.
   * @param index The sub-app type index
   * @return Reference to the sub-app
   */
  [[nodiscard]] SubApp& GetSubApp(SubAppTypeIndex index) noexcept;

  /**
   * @brief Gets the sub-app of the given type (const version).
   * @note Not thread-safe.
   * @warning Triggers assertion if the sub-app is not found.
   * @param index The sub-app type index
   * @return Const reference to the sub-app
   */
  [[nodiscard]] const SubApp& GetSubApp(SubAppTypeIndex index) const noexcept;

  /**
   * @brief Gets the sub-app of the given type.
   * @note Not thread-safe.
   * @warning Triggers assertion if the sub-app is not found.
   * @tparam T The sub-app type
   * @param sub_app The sub-app instance to find
   * @return Reference to the sub-app
   */
  template <SubAppTrait T>
  [[nodiscard]] SubApp& GetSubApp(const T& sub_app = {}) noexcept;

  /**
   * @brief Gets the sub-app of the given type (const version).
   * @note Not thread-safe.
   * @warning Triggers assertion if the sub-app is not found.
   * @tparam T The sub-app type
   * @param sub_app The sub-app instance to find
   * @return Const reference to the sub-app
   */
  template <SubAppTrait T>
  [[nodiscard]] const SubApp& GetSubApp(const T& sub_app = {}) const noexcept;

  /**
   * @brief Tries to get a main sub-app schedule by label.
   * @note Not thread-safe.
   * @tparam T Schedule label type
   * @param label Schedule label to search for
   * @return Pointer to the schedule, or `nullptr` if not found
   */
  template <ecs::ScheduleTrait T>
  [[nodiscard]] ecs::Schedule* TryGetSchedule(const T& label = {}) {
    return main_sub_app_.TryGetSchedule(label);
  }

  /**
   * @brief Tries to get a main sub-app schedule by label (const version).
   * @note Not thread-safe.
   * @tparam T Schedule label type
   * @param label Schedule label to search for
   * @return Pointer to the schedule, or `nullptr` if not found
   */
  template <ecs::ScheduleTrait T>
  [[nodiscard]] const ecs::Schedule* TryGetSchedule(const T& label = {}) const {
    return main_sub_app_.TryGetSchedule(label);
  }

  /**
   * @brief Gets the async executor.
   * @note Thread-safe.
   * @return Reference to the async executor
   */
  [[nodiscard]] async::Executor& GetExecutor() noexcept { return executor_; }

  /**
   * @brief Gets the async executor (const version).
   * @note Thread-safe.
   * @return Const reference to the async executor
   */
  [[nodiscard]] const async::Executor& GetExecutor() const noexcept {
    return executor_;
  }

  /**
   * @brief Gets the main sub-app's ECS world.
   * @note Thread-safe.
   * @return Reference to the main sub-app's ECS world
   */
  [[nodiscard]] ecs::World& GetWorld() noexcept {
    return main_sub_app_.GetWorld();
  }

  /**
   * @brief Gets the main sub-app's ECS world (const version).
   * @note Thread-safe.
   * @return Const reference to the main sub-app's ECS world
   */
  [[nodiscard]] const ecs::World& GetWorld() const noexcept {
    return main_sub_app_.GetWorld();
  }

  /**
   * @brief Gets the application frame scheduler.
   * @note Thread-safe.
   * @return Reference to the application frame scheduler
   */
  [[nodiscard]] Scheduler& GetScheduler() noexcept { return scheduler_; }

  /**
   * @brief Gets the application frame scheduler (const version).
   * @note Thread-safe.
   * @return Const reference to the application frame scheduler
   */
  [[nodiscard]] const Scheduler& GetScheduler() const noexcept {
    return scheduler_;
  }

private:
  struct MainSubAppLabel {};
  static constexpr MainSubAppLabel kMainSubApp{};

  struct PluginStorage {
    std::unique_ptr<Plugin> plugin;
    std::string_view name;

    template <PluginTrait Plugin, typename... Args>
      requires std::constructible_from<Plugin, Args...>
    [[nodiscard]] static PluginStorage From(Args&&... args) {
      return PluginStorage{
          .plugin = std::make_unique<Plugin>(std::forward<Args>(args)...),
          .name = PluginNameOf<Plugin>(),
      };
    }
  };

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  template <typename K, typename V>
  using FlatMap = std::flat_map<K, V>;
#else
  template <typename K, typename V>
  using FlatMap = boost::container::flat_map<K, V>;
#endif

  /**
   * @brief Cleans up the application and its subsystems.
   * @details Called after the main loop ends.
   * Spawns tasks on the executor for parallel cleanup.
   */
  void CleanUp();

  void RegisterMessages();

  void BuildPlugins();
  void PollPlugins();
  void WaitForPluginsReady();
  [[nodiscard]] bool PluginsReady() const;
  void FinishPlugins();
  void DestroyPlugins();

  template <SubAppTrait T>
  static void ApplySubAppLabelTraits(SubApp& sub_app, const T& label);

  bool is_initialized_ = false;  ///< Whether the app has been initialized

  /// Whether the app is currently running
  std::atomic<bool> is_running_{false};

  FlatMap<PluginTypeIndex, PluginStorage> plugins_;  ///< Plugins

  /// Dynamic plugins
  FlatMap<PluginTypeIndex, DynamicPlugin> dynamic_plugins_;

  async::Executor executor_;  ///< Async executor for parallel execution
  Scheduler scheduler_;       ///< Frame scheduler for main and sub-apps

  SubApp main_sub_app_;                        ///< The main sub-app
  FlatMap<SubAppTypeIndex, SubApp> sub_apps_;  ///< Additional sub-apps

  RunnerFn runner_;  ///< The runner function

  // Tracy lockables are incompatible with std::condition_variable.
  mutable std::mutex plugins_ready_mutex_;
  std::condition_variable plugins_ready_cv_;

  friend class Scheduler;
};

inline App::~App() {
  // Ensure we're not running (should already be false if properly shut down)
  is_running_.store(false, std::memory_order_release);

  // Wait for all pending executor tasks to complete
  executor_.WaitForAll();
}

inline void App::Update() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::App::Update");

  HELIOS_ASSERT(is_initialized_, "App is not initialized!");
  scheduler_.RunFrame(*this);

  HELIOS_APP_PROFILE_FRAME();
}

template <PluginTrait Plugin>
inline auto App::AddPlugins(this auto&& self, Plugin&& plugin)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Cannot add plugin after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Cannot add plugin while app is running!");

  auto storage = PluginStorage::From<std::remove_cvref_t<Plugin>>(
      std::forward<Plugin>(plugin));
  self.plugins_.try_emplace(PluginTypeIndex::From(plugin), std::move(storage));
  return std::forward<decltype(self)>(self);
}

template <PluginTrait... Plugins>
  requires utils::UniqueTypes<Plugins...> && (sizeof...(Plugins) > 1)
inline auto App::AddPlugins(this auto&& self, Plugins&&... plugins)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Cannot add plugin after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Cannot add plugin while app is running!");

  (self.AddPlugins(std::forward<Plugins>(plugins)), ...);
  return std::forward<decltype(self)>(self);
}

template <PluginTrait Plugin, typename... Args>
  requires std::constructible_from<Plugin, Args...>
inline auto App::EmplacePlugin(this auto&& self, Args&&... args)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Cannot add plugin after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Cannot add plugin while app is running!");

  auto storage = PluginStorage::From<Plugin>(std::forward<Args>(args)...);
  self.plugins_.try_emplace(PluginTypeIndex::From<Plugin>(),
                            std::move(storage));
  return std::forward<decltype(self)>(self);
}

inline auto App::AddDynamicPlugin(this auto&& self, DynamicPlugin plugin)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Cannot add dynamic plugin after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(),
                "Cannot add dynamic plugin while app is running!");

  self.dynamic_plugins_.try_emplace(PluginTypeIndex::From(plugin),
                                    std::move(plugin));
  return std::forward<decltype(self)>(self);
}

template <SubAppTrait T>
inline auto App::InsertSubApp(this auto&& self, const T& label, SubApp sub_app)
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Cannot insert sub-app after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(),
                "Cannot insert sub-app while app is running!");

  ApplySubAppLabelTraits(sub_app, label);
  self.sub_apps_.try_emplace(SubAppTypeIndex::From(label), std::move(sub_app));
  return std::forward<decltype(self)>(self);
}

template <SubAppTrait T>
inline bool App::RemoveSubApp(const T& label) {
  HELIOS_ASSERT(!IsInitialized(),
                "Cannot remove sub-app after app initialization!");
  HELIOS_ASSERT(!IsRunning(), "Cannot remove sub-app while app is running!");

  return sub_apps_.erase(SubAppTypeIndex::From(label)) > 0;
}

inline auto App::AddSchedule(ecs::ScheduleTypeId id, ecs::Schedule&& schedule)
    -> ecs::ScheduleOrdering {
  HELIOS_ASSERT(!IsInitialized(),
                "Cannot add schedule after app initialization!");
  HELIOS_ASSERT(!IsRunning(), "Cannot add schedule while app is running!");

  return main_sub_app_.AddSchedule(id, std::move(schedule));
}

template <ecs::ScheduleTrait T>
inline auto App::AddSchedule(const T& label, ecs::Schedule&& schedule)
    -> ecs::ScheduleOrdering {
  HELIOS_ASSERT(!IsInitialized(),
                "Cannot add schedule after app initialization!");
  HELIOS_ASSERT(!IsRunning(), "Cannot add schedule while app is running!");

  return main_sub_app_.AddSchedule(label, std::move(schedule));
}

template <ecs::ScheduleTrait T>
inline auto App::InitSchedule(this auto&& self, const T& label)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.main_sub_app_.InitSchedule(label);
  return std::forward<decltype(self)>(self);
}

template <ecs::ScheduleTrait L, ecs::FunctorSystemTrait... Systems>
  requires(sizeof...(Systems) > 1)
inline ecs::SystemSetHandle App::AddSystems(const L& label,
                                            Systems&&... systems) {
  return main_sub_app_.AddSystems(label, std::forward<Systems>(systems)...);
}

template <ecs::ScheduleTrait L, typename F>
  requires std::invocable<F, ecs::Schedule&>
inline auto App::EditSchedule(this auto&& self, const L& label, const F& fn)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.main_sub_app_.EditSchedule(label, fn);
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait... Ts>
inline auto App::InsertResources(this auto&& self, Ts&&... resources)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.main_sub_app_.InsertResources(std::forward<Ts>(resources)...);
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait... Ts>
inline auto App::TryInsertResources(this auto&& self, Ts&&... resources)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.main_sub_app_.TryInsertResources(std::forward<Ts>(resources)...);
  return std::forward<decltype(self)>(self);
}

template <ecs::AnyMessageTrait... Ts>
  requires(sizeof...(Ts) > 0)
inline auto App::AddMessages(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.main_sub_app_.template AddMessages<Ts...>();
  return std::forward<decltype(self)>(self);
}

inline auto App::SetRunner(this auto&& self, RunnerFn runner) noexcept
    -> decltype(std::forward<decltype(self)>(self)) {
  HELIOS_ASSERT(!self.IsInitialized(),
                "Cannot set runner after app initialization!");
  HELIOS_ASSERT(!self.IsRunning(), "Cannot set runner while app is running!");
  HELIOS_ASSERT(runner, "Runner function cannot be null!");

  self.runner_ = std::move(runner);
  return std::forward<decltype(self)>(self);
}

inline SubApp& App::GetSubApp(SubAppTypeIndex index) noexcept {
  const auto it = sub_apps_.find(index);
  HELIOS_ASSERT(it != sub_apps_.end(), "Sub-app not found!");
  return it->second;
}

inline const SubApp& App::GetSubApp(SubAppTypeIndex index) const noexcept {
  const auto it = sub_apps_.find(index);
  HELIOS_ASSERT(it != sub_apps_.end(), "Sub-app not found!");
  return it->second;
}

template <SubAppTrait T>
inline SubApp& App::GetSubApp(const T& sub_app) noexcept {
  const auto it = sub_apps_.find(SubAppTypeIndex::From(sub_app));
  HELIOS_ASSERT(it != sub_apps_.end(), "Sub-app '{}' not found!",
                SubAppNameOf(sub_app));
  return it->second;
}

template <SubAppTrait T>
inline const SubApp& App::GetSubApp(const T& sub_app) const noexcept {
  const auto it = sub_apps_.find(SubAppTypeIndex::From(sub_app));
  HELIOS_ASSERT(it != sub_apps_.end(), "Sub-app '{}' not found!",
                SubAppNameOf(sub_app));
  return it->second;
}

template <SubAppTrait T>
inline void App::ApplySubAppLabelTraits(SubApp& sub_app, const T& label) {
  static_assert(!(AsyncSubAppTrait<T> && OverlappingUpdatesSubAppTrait<T>),
                "kAsync and kAllowOverlappingUpdates cannot be used together!");

  sub_app.SetName(std::string(SubAppNameOf(label)));

  if constexpr (AsyncSubAppTrait<T>) {
    sub_app.SetAsync(true);
  } else if constexpr (OverlappingUpdatesSubAppTrait<T>) {
    sub_app.SetAllowOverlappingUpdates(true);
    sub_app.SetMaxExtractionSkips(SubAppMaxOverlappingUpdates<T>());
  }
}

}  // namespace helios::app
