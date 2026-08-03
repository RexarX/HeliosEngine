#include <pch.hpp>

#include <helios/app/scheduler.hpp>

#include <helios/app/app.hpp>
#include <helios/app/details/profile.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/assert.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/world.hpp>

#include <chrono>
#include <cstddef>

namespace helios::app {

void Scheduler::Clear() {
  HELIOS_ASSERT(async_loops_running_.load(std::memory_order_acquire) == 0,
                "Cannot clear scheduler while async update loops are running! "
                "Call Stop() or Shutdown() first.");
  HELIOS_ASSERT(async_loop_futures_.empty(),
                "Cannot clear scheduler while async loop futures remain! Call "
                "Stop() or Shutdown() first.");
  HELIOS_ASSERT(overlapping_update_futures_.empty(),
                "Cannot clear scheduler while overlapping updates remain! Call "
                "Stop() or Shutdown() first.");

  for (const SubAppFrameState& state : sub_app_states_) {
    HELIOS_ASSERT(!state.sub_app.get().IsUpdating(),
                  "Cannot clear scheduler while sub-apps are updating! Call "
                  "Stop() or Shutdown() first.");
  }

  HELIOS_ASSERT(!blocking_update_future_.has_value(),
                "Cannot clear scheduler while blocking sub-app updates are in "
                "flight! Call Stop() or Shutdown() first.");

  startup_graph_.Clear();
  blocking_update_graph_.Clear();
  shutdown_graph_.Clear();
  sub_app_states_.clear();
  blocking_update_future_.reset();
}

void Scheduler::Stop(async::Executor& /*executor*/) {
  StopAsyncUpdateLoops();
  WaitForOverlappingUpdates();
  WaitForSubApps();
}

void Scheduler::Build(App& app) {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::Build");

  auto& main = app.GetMainSubApp();
  auto& executor = app.GetExecutor();

  main.BuildScheduler(executor);

  startup_graph_.Clear();
  blocking_update_graph_.Clear();
  shutdown_graph_.Clear();
  sub_app_states_.clear();

  if (app.sub_apps_.empty()) {
    return;
  }

  sub_app_states_.reserve(app.sub_apps_.size());

  for (auto&& [index, sub_app] : app.sub_apps_) {
    sub_app.SetOwnerApp(app);
    sub_app_states_.push_back(SubAppFrameState{
        .sub_app = sub_app,
        .mode = ClassifySubApp(sub_app),
    });

    startup_graph_.EmplaceTask(
        [&sub_app, &executor]() { sub_app.RunStartupUnchecked(executor); });

    shutdown_graph_.EmplaceTask([&sub_app, &executor]() {
      sub_app.WaitUntilFullyIdle();
      sub_app.RunShutdownUnchecked(executor);
    });
  }

  for (auto&& [index, sub_app] : app.sub_apps_) {
    if (ClassifySubApp(sub_app) != SubAppMode::kBlocking) {
      continue;
    }

    blocking_update_graph_.EmplaceTask(
        [&sub_app, &executor]() { sub_app.RunUpdatePass(executor); });
  }
}

void Scheduler::RunStartup(App& app) {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::RunStartup");

  auto& main = app.GetMainSubApp();
  auto& executor = app.GetExecutor();

  RunMainStartup(main, executor);

  if (!app.sub_apps_.empty()) {
    executor.Run(startup_graph_).Wait();
    StartAsyncUpdateLoops(app);
  }
}

void Scheduler::RunFrame(App& app) {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::RunFrame");

  auto& executor = app.GetExecutor();
  auto& main = app.GetMainSubApp();

  for (SubAppFrameState& state : sub_app_states_) {
    state.fresh_extract_this_frame = false;
  }

  RunUpdateStage(main, executor);
  RunExtractStage(main, executor);
  LaunchSubAppUpdates(app);
  WaitForSubApps();
}

void Scheduler::Shutdown(App& app) {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::Shutdown");

  StopAsyncLoops(app);
  WaitForOverlappingUpdates();
  WaitForSubApps();

  auto& main = app.GetMainSubApp();
  auto& executor = app.GetExecutor();

  for (SubAppFrameState& state : sub_app_states_) {
    if (state.mode == SubAppMode::kAsync) {
      continue;
    }

    state.sub_app.get().WaitUntilFullyIdle();
  }

  if (!app.sub_apps_.empty()) {
    executor.Run(shutdown_graph_).Wait();
  }

  RunMainShutdown(main, executor);
}

void Scheduler::StopAsyncLoops(App& app) {
  for (auto&& [index, sub_app] : app.sub_apps_) {
    if (sub_app.IsAsync()) {
      sub_app.RequestAsyncLoopStop();
    }
  }

  StopAsyncUpdateLoops();
}

void Scheduler::WaitForSubApps() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::WaitForSubApps");

  if (blocking_update_future_.has_value()) {
    blocking_update_future_->Wait();
    blocking_update_future_.reset();
  }
}

void Scheduler::RunMainStartup(SubApp& main, async::Executor& executor) {
  main.BuildScheduler(executor);
  main.GetScheduler().RunStage(kStartupStage, main.GetWorld());
}

void Scheduler::RunUpdateStage(SubApp& main, async::Executor& executor) {
  main.BuildScheduler(executor);
  auto& world = main.GetWorld();
  main.GetScheduler().RunStage(kUpdateStage, world);
  world.Update();
}

void Scheduler::RunMainShutdown(SubApp& main, async::Executor& executor) {
  main.BuildScheduler(executor);
  main.GetScheduler().RunStage(kShutdownStage, main.GetWorld());
}

void Scheduler::RunExtractStage(SubApp& main, async::Executor& executor) {
  main.BuildScheduler(executor);

  auto& world = main.GetWorld();
  main.GetScheduler().RunStage(kExtractStage, world);

  const auto& main_world = main.GetWorld();
  for (SubAppFrameState& state : sub_app_states_) {
    ExtractSubApp(state, main_world);
  }
}

void Scheduler::LaunchSubAppUpdates(App& app) {
  if (app.sub_apps_.empty()) {
    return;
  }

  auto& executor = app.GetExecutor();

  if (blocking_update_graph_.TaskCount() > 0) {
    blocking_update_future_ = executor.Run(blocking_update_graph_);
  }

  PruneCompletedOverlappingUpdates();

  for (SubAppFrameState& state : sub_app_states_) {
    if (state.mode != SubAppMode::kOverlapping ||
        !state.fresh_extract_this_frame) {
      continue;
    }

    SubApp& sub_app = state.sub_app.get();
    overlapping_update_futures_.push_back(executor.Async(
        [&sub_app, &executor]() { sub_app.RunUpdatePass(executor); }));
  }
}

void Scheduler::StartAsyncUpdateLoops(App& app) {
  auto& executor = app.GetExecutor();

  for (auto&& [index, sub_app] : app.sub_apps_) {
    if (!sub_app.IsAsync()) {
      continue;
    }

    sub_app.ResetAsyncLoopStop();
    async_loops_running_.fetch_add(1, std::memory_order_acq_rel);

    async_loop_futures_.push_back(executor.Async([&sub_app, &executor, this]() {
      struct LoopGuard {
        Scheduler& self;

        ~LoopGuard() {
          self.async_loops_running_.fetch_sub(1, std::memory_order_acq_rel);
        }
      } guard{*this};

      sub_app.RunUpdatePass(executor);
    }));
  }
}

void Scheduler::StopAsyncUpdateLoops() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::StopAsyncUpdateLoops");

  for (SubAppFrameState& state : sub_app_states_) {
    if (state.mode != SubAppMode::kAsync) {
      continue;
    }

    state.sub_app.get().RequestAsyncLoopStop();
  }

  for (auto& future : async_loop_futures_) {
    if (future.valid()) {
      future.wait();
    }
  }
  async_loop_futures_.clear();
  async_loops_running_.store(0, std::memory_order_release);
}

void Scheduler::WaitForOverlappingUpdates() {
  HELIOS_APP_PROFILE_SCOPE_N(
      "helios::app::Scheduler::WaitForOverlappingUpdates");

  for (auto& future : overlapping_update_futures_) {
    if (future.valid()) {
      future.wait();
    }
  }
  overlapping_update_futures_.clear();
}

void Scheduler::PruneCompletedOverlappingUpdates() {
  std::erase_if(overlapping_update_futures_, [](std::future<void>& future) {
    return !future.valid() || future.wait_for(std::chrono::seconds{0}) ==
                                  std::future_status::ready;
  });
}

void Scheduler::ExtractSubApp(SubAppFrameState& state,
                              const ecs::World& main_world) {
  SubApp& sub_app = state.sub_app.get();
  state.fresh_extract_this_frame = false;

  switch (state.mode) {
    using enum SubAppMode;
    case kBlocking:
      sub_app.WaitUntilFullyIdle();
      sub_app.Extract(main_world);
      state.fresh_extract_this_frame = true;
      state.consecutive_extract_skips = 0;
      return;
    case kAsync:
      sub_app.Extract(main_world, true);
      return;
    case kOverlapping:
      break;
  }

  if (sub_app.IsUpdating()) {
    const size_t max_skips = sub_app.MaxExtractionSkips();
    if (max_skips == 0 || state.consecutive_extract_skips < max_skips) {
      ++state.consecutive_extract_skips;
      return;
    }

    sub_app.WaitUntilFullyIdle();
  }

  sub_app.Extract(main_world);
  state.fresh_extract_this_frame = true;
  state.consecutive_extract_skips = 0;
}

auto Scheduler::ClassifySubApp(const SubApp& sub_app) noexcept
    -> Scheduler::SubAppMode {
  if (sub_app.IsAsync()) {
    return SubAppMode::kAsync;
  }

  if (sub_app.AllowsOverlappingUpdates()) {
    return SubAppMode::kOverlapping;
  }

  return SubAppMode::kBlocking;
}

}  // namespace helios::app
