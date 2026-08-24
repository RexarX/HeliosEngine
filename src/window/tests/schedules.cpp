#include <doctest/doctest.h>

#include <helios/app/schedules.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/schedule/scheduler.hpp>
#include <helios/window/schedules.hpp>

using namespace helios;
using namespace helios::window;

TEST_SUITE("helios::window::RegisterEventsSchedule") {
  TEST_CASE("helios::window::RegisterEventsSchedule") {
    SUBCASE("Adds a main-thread events schedule before update") {
      ecs::Scheduler scheduler;
      app::RegisterBuiltinSchedules(scheduler);
      RegisterEventsSchedule(scheduler);

      CHECK(scheduler.HasStage(kWindowStage));
      CHECK_NE(scheduler.TryGetSchedule(kEvents), nullptr);
      CHECK_EQ(scheduler.TryGetSchedule(kEvents)->Settings().executor_kind,
               ecs::ExecutorKind::kMainThread);
      CHECK_FALSE(scheduler.GetStageSettings(kWindowStage).advance_messages);
      CHECK_FALSE(scheduler.GetStageSettings(kWindowStage).apply_commands);
      CHECK_FALSE(scheduler.GetStageSettings(kWindowStage).merge_messages);
    }

    SUBCASE("Is idempotent") {
      ecs::Scheduler scheduler;
      app::RegisterBuiltinSchedules(scheduler);
      RegisterEventsSchedule(scheduler);
      RegisterEventsSchedule(scheduler);

      CHECK(scheduler.HasStage(kWindowStage));
      CHECK_NE(scheduler.TryGetSchedule(kEvents), nullptr);
    }

    SUBCASE("Creates the update stage when it is missing") {
      ecs::Scheduler scheduler;
      RegisterEventsSchedule(scheduler);

      CHECK(scheduler.HasStage(kWindowStage));
      CHECK(scheduler.HasStage(app::kUpdateStage));
      CHECK_NE(scheduler.TryGetSchedule(kEvents), nullptr);
    }
  }
}
