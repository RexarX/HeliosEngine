
#include <pch.hpp>

#include <helios/app/sub_app.hpp>

#include <helios/app/app.hpp>
#include <helios/app/details/profile.hpp>
#include <helios/app/runners.hpp>
#include <helios/assert.hpp>
#include <helios/async/executor.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/stage.hpp>
#include <helios/log/logger.hpp>

#include <mutex>
#include <thread>

#if defined(HELIOS_APP_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#include <format>
#endif

namespace helios::app {

SubApp::SubApp() : runner_(RunOnceSubApp) {
  RegisterBuiltinSubAppSchedules(scheduler_);
}

SubApp::SubApp(std::string name)
    : name_(std::move(name)), runner_(RunOnceSubApp) {
  RegisterBuiltinSubAppSchedules(scheduler_);
}

void SubApp::Clear() {
  HELIOS_ASSERT(!IsUpdating(), "Cannot clear while sub-app is updating!");

  world_.Clear();
  scheduler_.Clear();
  RegisterBuiltinSubAppSchedules(scheduler_);
  update_stage_ = ecs::StageTypeIndex::From(kUpdateStage);
  extract_fn_ = nullptr;
  runner_ = RunOnceSubApp;
  owner_app_ = nullptr;
  async_loop_stop_.store(false, std::memory_order_release);
}

void SubApp::WaitUntilFullyIdle() const noexcept {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_NAME(std::format(
      "helios::app::SubApp::WaitUntilFullyIdle{{name: {}}}", GetName()));
  HELIOS_APP_PROFILE_ZONE_TEXT(std::format(
      "allow_overlapping_updates: {}, max_extraction_skips: {}, is_async: {}",
      allow_overlapping_updates_, max_extraction_skips_, is_async_));

  while (IsUpdating()) {
    std::this_thread::yield();
  }
}

bool SubApp::ShouldExit() const noexcept {
  if (AsyncLoopStopRequested()) {
    return true;
  }

  if (owner_app_ == nullptr) [[unlikely]] {
    return false;
  }

  return owner_app_->ShouldExit().has_value();
}

bool SubApp::TryBeginUpdate() noexcept {
  bool expected = false;
  return is_updating_.compare_exchange_strong(expected, true,
                                              std::memory_order_acq_rel);
}

void SubApp::RunUpdatePass(async::Executor& executor) {
  if (!TryBeginUpdate()) [[unlikely]] {
    log::Warn("Failed to update sub-app, update already in progress!");
    return;
  }

  struct UpdateGuard {
    SubApp& sub_app;

    ~UpdateGuard() { sub_app.EndUpdate(); }
  } guard{*this};

  if (runner_) [[likely]] {
    runner_(*this, executor);
    return;
  }

  if (is_async_) {
    while (!ShouldExit()) {
      Update(executor);
    }
    return;
  }

  Update(executor);
}

void SubApp::RunStageUnchecked(async::Executor& executor,
                               ecs::StageTypeIndex stage, ecs::World& world) {
  const std::scoped_lock lock(stage_run_mutex_);
  BuildScheduler(executor);
  scheduler_.RunStage(stage, world);
}

}  // namespace helios::app
