#include <doctest/doctest.h>

#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/schedule.hpp>

using namespace helios::ecs;

namespace {

struct CounterSystem {
  void operator()() {}

  int* counter = nullptr;
};

}  // namespace

TEST_SUITE("helios::ecs::MainThreadExecutor") {
  TEST_CASE("ecs::MainThreadExecutor::Execute") {
    SUBCASE(
        "Execute dispatches all systems in a built schedule to completion") {
      int counter = 0;

      Schedule schedule;
      CounterSystem system{.counter = &counter};
      schedule.Add(std::move(system));
      const auto build_result = schedule.Build();
      CHECK(build_result.has_value());

      MainThreadExecutor executor;
      World world;

      executor.Execute(schedule, world);

      CHECK_EQ(counter, 0);
    }

    SUBCASE("ExecuteAndWait is equivalent to Execute for main thread") {
      int counter = 0;

      Schedule schedule;
      CounterSystem system{.counter = &counter};
      schedule.Add(std::move(system));
      const auto build_result = schedule.Build();
      CHECK(build_result.has_value());

      MainThreadExecutor executor;
      World world;

      executor.ExecuteAndWait(schedule, world);

      CHECK_EQ(counter, 0);
    }

    SUBCASE("Wait does nothing for main thread executor") {
      MainThreadExecutor executor;
      executor.Wait();
    }
  }
}
