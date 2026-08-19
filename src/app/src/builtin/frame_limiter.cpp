#include <pch.hpp>

#include <helios/app/builtin/frame_limiter.hpp>

#include <helios/app/application.hpp>
#include <helios/app/details/profile.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/assert.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/scheduler.hpp>
#include <helios/utils/sleep.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <utility>

namespace helios::app {

namespace {

[[nodiscard]] bool RegisterFramePaceSchedule(ecs::Scheduler& scheduler,
                                             bool in_update_stage) {
  if (scheduler.TryGetSchedule(kFramePace) != nullptr) {
    return false;
  }

  auto schedule = ecs::Schedule::From(kFramePace);
  schedule.Settings().executor_kind = ecs::ExecutorKind::kMainThread;

  if (in_update_stage) {
    scheduler.Add(kFramePace, std::move(schedule))
        .InStage(kUpdateStage)
        .Before(kFirst);
    return true;
  }

  if (!scheduler.HasStage(kFramePaceStage)) {
    scheduler.AddStage(kFramePaceStage);
  }
  scheduler.OrderStage(kFramePaceStage).Before(kUpdateStage);
  scheduler.Add(kFramePace, std::move(schedule)).InStage(kFramePaceStage);
  return true;
}

}  // namespace

auto FrameLimiter::ResolveInterval() const noexcept
    -> std::optional<std::chrono::nanoseconds> {
  const auto mode = Mode();
  if (mode == FrameLimiterMode::kOff) {
    return std::nullopt;
  }

  if (mode == FrameLimiterMode::kAuto) {
    const uint32_t hz = RefreshRate();
    if (hz == 0) {
      return std::nullopt;
    }
    constexpr int64_t kNanosecsInSec = 1'000'000'000;
    return std::chrono::nanoseconds{kNanosecsInSec / hz};
  }

  const auto interval = Interval();
  if (interval.count() <= 0) {
    return std::nullopt;
  }
  return interval;
}

void FrameLimiter::Wait() {
  HELIOS_APP_PROFILE_SCOPE_N("helios::app::FrameLimiter::Wait");

  const auto interval = ResolveInterval();
  if (!interval.has_value()) {
    next_tick_.reset();
    return;
  }

  const auto now = Clock::now();
  if (!next_tick_.has_value()) {
    next_tick_ = now + *interval;
    return;
  }

  utils::PreciseSleepUntil(*next_tick_);
  next_tick_ = std::max(*next_tick_, Clock::now()) + *interval;
}

void InstallFrameLimiter(App& app, FrameLimiterSettings settings) {
  app.TryInsertResources(FrameLimiter{settings});

  auto& scheduler = app.GetMainSubApp().GetScheduler();
  if (RegisterFramePaceSchedule(scheduler, false)) {
    app.AddSystem(kFramePace, LimitFrameRate{});
  }

  app.GetWorld().WriteResource<MainFrameOrder>().TryPushFront(kFramePaceStage);
}

void InstallFrameLimiter(SubApp& sub_app, FrameLimiterSettings settings) {
  HELIOS_ASSERT(!sub_app.AllowsOverlappingUpdates(),
                "Frame limiter cannot be installed on overlapping sub-apps; "
                "sleeping would park a shared executor worker. Use an async "
                "sub-app for an independent rate.");

  sub_app.TryInsertResources(FrameLimiter{settings});

  if (RegisterFramePaceSchedule(sub_app.GetScheduler(), true)) {
    sub_app.AddSystem(kFramePace, LimitFrameRate{});
  }
}

}  // namespace helios::app
