#include <pch.hpp>

#include <helios/app/schedules.hpp>

#include <helios/app/frame_order.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/scheduler.hpp>
#include <helios/ecs/schedule/stage.hpp>
#include <helios/ecs/world.hpp>

namespace helios::app {

void RegisterBuiltinSchedules(ecs::Scheduler& scheduler) {
  const auto add_in_stage = []<ecs::ScheduleTrait T, ecs::StageTrait S>(
                                ecs::Scheduler& sched, const T& schedule_type,
                                const S& stage, ecs::ExecutorKind kind) {
    if (sched.TryGetSchedule(schedule_type) != nullptr) {
      return;
    }
    auto schedule = ecs::Schedule::From(schedule_type);
    schedule.Settings().executor_kind = kind;
    sched.Add(schedule_type, std::move(schedule)).InStage(stage);
  };

  if (!scheduler.HasStage(kStartupStage)) {
    scheduler.AddStage(kStartupStage);
  }

  // FramePumpOrder ends at Update — advance message buffers there for nested
  // pumps. MainFrameOrder also includes Update but only advances on its last
  // present stage (Extract); see Scheduler::RunFrameOrder.
  if (!scheduler.HasStage(kUpdateStage)) {
    auto& settings = scheduler.AddStage(kUpdateStage).Settings();
    settings.advance_messages = true;
  } else {
    auto& settings = scheduler.GetStageSettings(kUpdateStage);
    settings.advance_messages = true;
  }

  // MainFrameOrder ends at Extract: flush after extract, advance message
  // buffers once at end of the main frame order.
  if (!scheduler.HasStage(kExtractStage)) {
    auto& settings = scheduler.AddStage(kExtractStage).Settings();
    settings.apply_commands = true;
    settings.advance_messages = true;
  } else {
    auto& settings = scheduler.GetStageSettings(kExtractStage);
    settings.apply_commands = true;
    settings.advance_messages = true;
  }

  if (!scheduler.HasStage(kShutdownStage)) {
    scheduler.AddStage(kShutdownStage);
  }

  add_in_stage(scheduler, kMainStartup, kStartupStage,
               ecs::ExecutorKind::kMainThread);
  add_in_stage(scheduler, kPreStartup, kStartupStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kStartup, kStartupStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPostStartup, kStartupStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kFirst, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPreUpdate, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kUpdate, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPostUpdate, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kLast, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kExtract, kExtractStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPreShutdown, kShutdownStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kShutdown, kShutdownStage,
               ecs::ExecutorKind::kMainThread);
  add_in_stage(scheduler, kPostShutdown, kShutdownStage,
               ecs::ExecutorKind::kMultiThreaded);

  scheduler.Order(kPreStartup).After(kMainStartup);
  scheduler.Order(kStartup).After(kPreStartup);
  scheduler.Order(kPostStartup).After(kStartup);
  scheduler.Order(kPreUpdate).After(kFirst);
  scheduler.Order(kUpdate).After(kPreUpdate);
  scheduler.Order(kPostUpdate).After(kUpdate);
  scheduler.Order(kLast).After(kPostUpdate);
  scheduler.Order(kShutdown).After(kPreShutdown);
  scheduler.Order(kPostShutdown).After(kShutdown);
  scheduler.OrderStage(kExtractStage).After(kUpdateStage);
}

void RegisterBuiltinFrameOrders(ecs::World& world) {
  MainFrameOrder main_order;
  main_order.TryPushBack(kUpdateStage);
  main_order.TryPushBack(kExtractStage);

  FramePumpOrder pump_order;
  pump_order.TryPushBack(kUpdateStage);

  world.TryInsertResources(std::move(main_order), std::move(pump_order));
}

void RegisterBuiltinSubAppSchedules(ecs::Scheduler& scheduler) {
  const auto add_in_stage = []<ecs::ScheduleTrait T, ecs::StageTrait S>(
                                ecs::Scheduler& sched, const T& schedule_type,
                                const S& stage, ecs::ExecutorKind kind) {
    if (sched.TryGetSchedule(schedule_type) != nullptr) {
      return;
    }
    auto schedule = ecs::Schedule::From(schedule_type);
    schedule.Settings().executor_kind = kind;
    sched.Add(schedule_type, std::move(schedule)).InStage(stage);
  };

  if (!scheduler.HasStage(kStartupStage)) {
    scheduler.AddStage(kStartupStage);
  }

  // Sub-apps and FramePumpOrder end at Update: advance message buffers there.
  if (!scheduler.HasStage(kUpdateStage)) {
    auto& settings = scheduler.AddStage(kUpdateStage).Settings();
    settings.advance_messages = true;
  } else {
    auto& settings = scheduler.GetStageSettings(kUpdateStage);
    settings.advance_messages = true;
  }

  if (!scheduler.HasStage(kShutdownStage)) {
    scheduler.AddStage(kShutdownStage);
  }

  add_in_stage(scheduler, kMainStartup, kStartupStage,
               ecs::ExecutorKind::kMainThread);
  add_in_stage(scheduler, kPreStartup, kStartupStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kStartup, kStartupStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPostStartup, kStartupStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kFirst, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPreUpdate, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kUpdate, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPostUpdate, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kLast, kUpdateStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kPreShutdown, kShutdownStage,
               ecs::ExecutorKind::kMultiThreaded);
  add_in_stage(scheduler, kShutdown, kShutdownStage,
               ecs::ExecutorKind::kMainThread);
  add_in_stage(scheduler, kPostShutdown, kShutdownStage,
               ecs::ExecutorKind::kMultiThreaded);

  scheduler.Order(kPreStartup).After(kMainStartup);
  scheduler.Order(kStartup).After(kPreStartup);
  scheduler.Order(kPostStartup).After(kStartup);
  scheduler.Order(kPreUpdate).After(kFirst);
  scheduler.Order(kUpdate).After(kPreUpdate);
  scheduler.Order(kPostUpdate).After(kUpdate);
  scheduler.Order(kLast).After(kPostUpdate);
  scheduler.Order(kShutdown).After(kPreShutdown);
  scheduler.Order(kPostShutdown).After(kShutdown);
}

}  // namespace helios::app
