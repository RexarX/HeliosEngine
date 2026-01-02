#include <helios/core/app/app.hpp>

#include <helios/core/app/runners.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/time.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/ecs/events/builtin_events.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>

#include <atomic>
#include <cstddef>
#include <future>
#include <ranges>
#include <utility>

namespace helios::app {

App::App() : runner_(DefaultRunnerWrapper) {}

App::App(size_t worker_thread_count) : executor_(worker_thread_count), runner_(DefaultRunnerWrapper) {}

App::~App() {
  // Ensure we're not running (should already be false if properly shut down)
  is_running_.store(false, std::memory_order_release);

  // Wait for any pending overlapping sub-app updates
  WaitForOverlappingUpdates();

  // Wait for all pending executor tasks to complete
  executor_.WaitForAll();
}

void App::Clear() {
  HELIOS_ASSERT(!IsRunning(), "Failed to clear app: Cannot clear app while it is running!");

  WaitForOverlappingUpdates();  // Ensure all async work is done
  sub_app_overlapping_futures_.clear();
  main_sub_app_.Clear();
  sub_apps_.clear();
  sub_app_index_map_.clear();

  is_initialized_ = false;
}

AppExitCode App::Run() {
  HELIOS_ASSERT(!IsRunning(), "Failed to run: App is already running!");

  HELIOS_INFO("Starting application...");

  // Register built-in resources and events if not already registered
  RegisterBuiltins();

  BuildModules();
  FinishModules();
  Initialize();

  // Run the application using the provided runner function
  is_running_.store(true, std::memory_order_release);
  const AppExitCode exit_code = runner_(*this);
  is_running_.store(false, std::memory_order_release);

  HELIOS_INFO("Cleaning up application...");
  CleanUp();

  executor_.WaitForAll();

  DestroyModules();

  // Reset state
  is_initialized_ = false;
  sub_app_overlapping_futures_.clear();
  main_sub_app_.Clear();
  sub_apps_.clear();
  sub_app_index_map_.clear();

  HELIOS_INFO("Application exiting with code: {}", std::to_underlying(exit_code));
  return exit_code;
}

void App::Update() {
  // Clean up completed per-sub-app overlapping futures using cached ready flags
  for (auto& [index, futures] : sub_app_overlapping_futures_) {
    std::erase_if(futures, [](details::TrackedFuture& tracked) { return tracked.IsReady(); });
  }

  // Update main sub-app (synchronous)
  main_sub_app_.Update(executor_);

  // Process non-overlapping sub-apps
  auto non_overlapping_future = executor_.Run(update_graph_);

  // Launch overlapping sub-apps asynchronously (non-blocking)
  for (size_t i = 0; i < sub_apps_.size(); ++i) {
    auto& sub_app = sub_apps_[i];
    if (!sub_app.AllowsOverlappingUpdates()) {
      continue;
    }

    // Capture by reference is safe: App lifetime exceeds sub-app updates
    auto future = executor_.Async([this, &sub_app]() {
      sub_app.Extract(main_sub_app_.GetWorld());
      sub_app.Update(executor_);
    });

    // Store as TrackedFuture for optimized ready-state tracking
    sub_app_overlapping_futures_[i].push_back(details::TrackedFuture{.future = future.share(), .ready = false});
  }

  non_overlapping_future.Wait();
}

void App::Initialize() {
  HELIOS_ASSERT(!IsInitialized(), "Failed to initialize: App is already initialized!");
  HELIOS_ASSERT(!IsRunning(), "Failed to initialize: Cannot initialize while app is running!");

  async::TaskGraph initialization_graph;
  initialization_graph.EmplaceTask([this]() {
    main_sub_app_.BuildScheduler();
    main_sub_app_.ExecuteStage<helios::app::StartUpStage>(executor_);
    main_sub_app_.GetWorld().Update();
  });

  initialization_graph.ForEach(sub_apps_, [this](const SubApp& sub_app_ref) mutable {
    // Need non-const access to modify sub_app
    auto& sub_app = const_cast<SubApp&>(sub_app_ref);

    sub_app.BuildScheduler();
    sub_app.ExecuteStage<helios::app::StartUpStage>(executor_);
    sub_app.GetWorld().Update();
  });

  auto future = executor_.Run(initialization_graph);

  update_graph_.ForEach(sub_apps_, [this](const SubApp& sub_app_ref) mutable {
    if (sub_app_ref.AllowsOverlappingUpdates()) {
      return;
    }

    // Need non-const access to modify sub_app
    auto& sub_app = const_cast<SubApp&>(sub_app_ref);
    sub_app.Extract(main_sub_app_.GetWorld());
    sub_app.Update(executor_);
  });

  future.Wait();

  is_initialized_ = true;
}

void App::CleanUp() {
  HELIOS_ASSERT(IsInitialized(), "Failed to clean up: App is not initialized!");
  HELIOS_ASSERT(!IsRunning(), "Failed to clean up: Cannot clean up while app is running!");

  // Wait for all overlapping sub-app updates to complete
  WaitForOverlappingUpdates();

  async::TaskGraph cleanup_graph;
  cleanup_graph.EmplaceTask([this]() { main_sub_app_.ExecuteStage<helios::app::CleanUpStage>(executor_); });
  cleanup_graph.ForEach(sub_apps_, [this](const SubApp& sub_app_ref) mutable {
    // Need non-const access to modify sub_app
    auto& sub_app = const_cast<SubApp&>(sub_app_ref);
    sub_app.ExecuteStage<helios::app::CleanUpStage>(executor_);
  });

  executor_.Run(cleanup_graph).Wait();
}

void App::WaitForOverlappingUpdates() {
  for (auto& [index, futures] : sub_app_overlapping_futures_) {
    for (auto& tracked : futures) {
      tracked.Wait();
    }
    futures.clear();
  }
}

void App::WaitForOverlappingUpdates(const SubApp& sub_app) {
  // Find the sub-app index by comparing addresses
  auto sub_app_index = static_cast<size_t>(-1);
  for (size_t i = 0; i < sub_apps_.size(); ++i) {
    if (&sub_apps_[i] == &sub_app) {
      sub_app_index = i;
      break;
    }
  }

  if (sub_app_index == static_cast<size_t>(-1)) [[unlikely]] {
    return;  // Sub-app not found
  }

  const auto futures_it = sub_app_overlapping_futures_.find(sub_app_index);
  if (futures_it == sub_app_overlapping_futures_.end()) {
    return;  // No overlapping futures for this sub-app
  }

  for (auto& tracked : futures_it->second) {
    tracked.Wait();
  }
  futures_it->second.clear();
}

void App::TickTime() noexcept {
  if (main_sub_app_.GetWorld().HasResource<Time>()) [[likely]] {
    main_sub_app_.GetWorld().WriteResource<Time>().Tick();
  }
}

void App::BuildModules() {
  HELIOS_ASSERT(!IsRunning(), "Failed to build modules: Cannot build modules while app is running!");

  // Build static modules
  for (auto& module : modules_) {
    if (module) [[likely]] {
      module->Build(*this);
    }
  }

  // Build dynamic modules
  for (auto& dyn_module : dynamic_modules_) {
    if (dyn_module.Loaded()) [[likely]] {
      dyn_module.GetModule().Build(*this);
    }
  }
}

void App::FinishModules() {
  HELIOS_ASSERT(!IsRunning(), "Failed to finish modules: Cannot finish modules while app is running!");

  // Poll until all modules are ready
  bool all_ready = false;
  while (!all_ready) {
    all_ready = true;

    // Check static modules
    for (const auto& module : modules_) {
      if (module && !module->IsReady(*this)) {
        all_ready = false;
        break;
      }
    }

    // Check dynamic modules
    if (all_ready) {
      for (const auto& dyn_module : dynamic_modules_) {
        if (dyn_module.Loaded() && !dyn_module.GetModule().IsReady(*this)) {
          all_ready = false;
          break;
        }
      }
    }

    // If not all ready, yield to allow async operations to complete
    if (!all_ready) {
      std::this_thread::yield();
    }
  }

  // All modules are ready, call Finish on each
  for (auto& module : modules_) {
    if (module) [[likely]] {
      module->Finish(*this);
    }
  }

  for (auto& dyn_module : dynamic_modules_) {
    if (dyn_module.Loaded()) [[likely]] {
      dyn_module.GetModule().Finish(*this);
    }
  }
}

void App::DestroyModules() {
  HELIOS_ASSERT(!IsRunning(), "Failed to destroy modules: Cannot destroy modules while app is running!");

  // Destroy dynamic modules first (reverse order of addition for proper cleanup)
  for (auto& dyn_module : dynamic_modules_ | std::views::reverse) {
    if (dyn_module.Loaded()) [[likely]] {
      dyn_module.GetModule().Destroy(*this);
    }
  }
  dynamic_modules_.clear();
  dynamic_module_index_map_.clear();

  // Destroy static modules (reverse order of addition for proper cleanup)
  for (auto& module : modules_ | std::views::reverse) {
    if (module) [[likely]] {
      module->Destroy(*this);
    }
  }
  modules_.clear();
  module_index_map_.clear();
}

void App::RegisterBuiltins() {
  // Register Time resource if not already present
  if (!HasResource<Time>()) {
    EmplaceResource<Time>();
  }

  // Register ShutdownEvent if not already present
  if (!HasEvent<ecs::ShutdownEvent>()) {
    AddEvent<ecs::ShutdownEvent>();
  }
}

AppExitCode App::DefaultRunnerWrapper(App& app) {
  // Delegate to the DefaultRunner implementation
  return helios::app::DefaultRunner(app);
}

}  // namespace helios::app
