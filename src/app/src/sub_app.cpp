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
#include <helios/utils/defer.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

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

SubApp::SubApp(SubApp&& other) noexcept
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

SubApp& SubApp::operator=(SubApp&& other) noexcept {
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

void SubApp::Update(async::Executor& executor) {
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

void SubApp::Extract(const ecs::World& main_world,
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

void SubApp::BuildScheduler(async::Executor& executor) {
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
    is_updating_.wait(true, std::memory_order_acquire);
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

void SubApp::EndUpdate() noexcept {
  is_updating_.store(false, std::memory_order_release);
  is_updating_.notify_all();
}

void SubApp::RunUpdatePass(async::Executor& executor) {
  if (!TryBeginUpdate()) [[unlikely]] {
    log::Warn("Failed to update sub-app, update already in progress!");
    return;
  }

  HELIOS_DEFER {
    EndUpdate();
  };

  if (runner_) [[likely]] {
    runner_(*this, executor);
    return;
  }

  if (is_async_) {
    while (!ShouldExit()) {
      Update(executor);
      std::this_thread::yield();
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
