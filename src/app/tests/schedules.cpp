#include <doctest/doctest.h>

#include <helios/app/schedules.hpp>
#include <helios/async/executor.hpp>
#include <helios/ecs/schedule/scheduler.hpp>

#include <string_view>

using namespace helios;
using namespace helios::app;
using namespace helios::ecs;

namespace {

struct CustomSchedule {
  static constexpr std::string_view kName = "CustomSchedule";
};

}  // namespace

TEST_SUITE("helios::app::RegisterBuiltinSchedules") {
  TEST_CASE("app::RegisterBuiltinSchedules") {
    SUBCASE("Registers all built-in schedules on empty scheduler") {
      ecs::Scheduler scheduler;
      RegisterBuiltinSchedules(scheduler);

      CHECK_NE(scheduler.TryGetSchedule(kMainStartup), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kPreStartup), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kStartup), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kPostStartup), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kFirst), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kPreUpdate), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kUpdate), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kPostUpdate), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kLast), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kExtract), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kPreShutdown), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kShutdown), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kPostShutdown), nullptr);
      CHECK(scheduler.HasStage(kStartupStage));
      CHECK(scheduler.HasStage(kUpdateStage));
      CHECK(scheduler.HasStage(kExtractStage));
      CHECK(scheduler.HasStage(kShutdownStage));
    }

    SUBCASE("Is idempotent when schedules already exist") {
      ecs::Scheduler scheduler;
      RegisterBuiltinSchedules(scheduler);
      RegisterBuiltinSchedules(scheduler);

      CHECK_NE(scheduler.TryGetSchedule(kUpdate), nullptr);
    }

    SUBCASE("Assigns expected executor kinds") {
      ecs::Scheduler scheduler;
      RegisterBuiltinSchedules(scheduler);

      CHECK_EQ(scheduler.TryGetSchedule(kFirst)->Settings().executor_kind,
               ExecutorKind::kMainThread);
      CHECK_EQ(scheduler.TryGetSchedule(kPreUpdate)->Settings().executor_kind,
               ExecutorKind::kMultiThreaded);
      CHECK_EQ(scheduler.TryGetSchedule(kUpdate)->Settings().executor_kind,
               ExecutorKind::kMultiThreaded);
      CHECK_EQ(scheduler.TryGetSchedule(kExtract)->Settings().executor_kind,
               ExecutorKind::kMultiThreaded);
      CHECK_EQ(scheduler.TryGetSchedule(kShutdown)->Settings().executor_kind,
               ExecutorKind::kMainThread);
    }
  }
}

TEST_SUITE("helios::app::RegisterBuiltinSubAppSchedules") {
  TEST_CASE("app::RegisterBuiltinSubAppSchedules") {
    SUBCASE("Registers sub-app schedules without extract stage") {
      ecs::Scheduler scheduler;
      RegisterBuiltinSubAppSchedules(scheduler);

      CHECK_NE(scheduler.TryGetSchedule(kStartup), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kUpdate), nullptr);
      CHECK_NE(scheduler.TryGetSchedule(kShutdown), nullptr);
      CHECK_EQ(scheduler.TryGetSchedule(kExtract), nullptr);
      CHECK_FALSE(scheduler.HasStage(kExtractStage));
    }

    SUBCASE("Is idempotent when schedules already exist") {
      ecs::Scheduler scheduler;
      RegisterBuiltinSubAppSchedules(scheduler);
      RegisterBuiltinSubAppSchedules(scheduler);

      CHECK_NE(scheduler.TryGetSchedule(kUpdate), nullptr);
    }

    SUBCASE("Preserves startup ordering constraints") {
      ecs::Scheduler scheduler;
      RegisterBuiltinSubAppSchedules(scheduler);

      async::Executor executor{1};
      scheduler.Build(executor);
      CHECK_FALSE(scheduler.IsDirty());
    }
  }
}
