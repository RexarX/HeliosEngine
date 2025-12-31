#include <helios/core/app/app.hpp>

#include <helios/core/app/schedules.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/logger.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <future>
#include <ranges>
#include <utility>

namespace helios::app {

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

  BuildModules();
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
  // Clean up completed per-sub-app overlapping futures
  for (auto& [index, futures] : sub_app_overlapping_futures_) {
    std::erase_if(futures,
                  [](auto& future) { return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
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

    // Store as shared_future for per-sub-app tracking (allows multiple references)
    sub_app_overlapping_futures_[i].push_back(future.share());
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
    for (auto& future : futures) {
      if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        future.wait();
      }
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

  for (auto& future : futures_it->second) {
    if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      future.wait();
    }
  }
  futures_it->second.clear();
}

void App::BuildModules() {
  HELIOS_ASSERT(!IsRunning(), "Failed to build modules: Cannot build modules while app is running!");

  for (auto& module : modules_) {
    if (module) [[likely]] {
      module->Build(*this);
    }
  }
}

void App::DestroyModules() {
  HELIOS_ASSERT(!IsRunning(), "Failed to destroy modules: Cannot destroy modules while app is running!");

  for (auto& module : modules_) {
    if (module) [[likely]] {
      module->Destroy(*this);
    }
  }
  modules_.clear();
  module_index_map_.clear();
}

AppExitCode App::DefaultRunner(App& app) {
  // Simple loop that runs until interrupted
  // Users should provide their own runner for more complex behavior
  try {
    while (app.IsRunning()) {
      app.Update();
    }
  } catch (std::exception& exception) {
    HELIOS_CRITICAL("Application encountered an unhandled exception and will exit: {}!", exception.what());
    return AppExitCode::Failure;
  }

  return AppExitCode::Success;
}

}  // namespace helios::app
