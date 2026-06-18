#include <pch.hpp>

#include <helios/app/scheduler.hpp>

#include <helios/app/app.hpp>
#include <helios/app/details/profile.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/assert.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/world.hpp>

#include <thread>

namespace helios::app {

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
    StartAsyncUpdateLoops(app, executor);
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
  LaunchSubAppUpdates(app, executor);
  WaitForSubApps();
}

void Scheduler::Shutdown(App& app) {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::Scheduler::Shutdown");

  StopAsyncUpdateLoops();

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

void Scheduler::LaunchSubAppUpdates(App& app, async::Executor& executor) {
  if (app.sub_apps_.empty()) {
    return;
  }

  if (blocking_update_graph_.TaskCount() > 0) {
    blocking_update_future_ = executor.Run(blocking_update_graph_);
  }

  for (SubAppFrameState& state : sub_app_states_) {
    if (state.mode != SubAppMode::kOverlapping ||
        !state.fresh_extract_this_frame) {
      continue;
    }

    SubApp& sub_app = state.sub_app.get();
    executor.SilentAsync(
        [&sub_app, &executor]() { sub_app.RunUpdatePass(executor); });  // +
  }
}

void Scheduler::StartAsyncUpdateLoops(App& app, async::Executor& executor) {
  for (auto&& [index, sub_app] : app.sub_apps_) {
    if (!sub_app.IsAsync()) {
      continue;
    }

    sub_app.ResetAsyncLoopStop();
    async_loops_running_.fetch_add(1, std::memory_order_acq_rel);

    executor.SilentAsync([&sub_app, &executor, this]() {  // +
      sub_app.RunUpdatePass(executor);
      async_loops_running_.fetch_sub(1, std::memory_order_acq_rel);
    });
  }
}

void Scheduler::StopAsyncUpdateLoops() {
  for (SubAppFrameState& state : sub_app_states_) {
    if (state.mode != SubAppMode::kAsync) {
      continue;
    }

    state.sub_app.get().RequestAsyncLoopStop();
  }

  while (async_loops_running_.load(std::memory_order_acquire) > 0) {
    std::this_thread::yield();
  }
}

void Scheduler::ExtractSubApp(SubAppFrameState& state,
                              const ecs::World& main_world) {
  SubApp& sub_app = state.sub_app.get();
  state.fresh_extract_this_frame = false;

  switch (state.mode) {
    case SubAppMode::kBlocking:
      sub_app.WaitUntilFullyIdle();
      sub_app.Extract(main_world);
      state.fresh_extract_this_frame = true;
      state.consecutive_extract_skips = 0;
      return;
    case SubAppMode::kAsync:
      sub_app.Extract(main_world, true);
      return;
    case SubAppMode::kOverlapping:
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
