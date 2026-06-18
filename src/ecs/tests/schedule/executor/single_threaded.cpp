#include <doctest/doctest.h>

#include <helios/async/executor.hpp>
#include <helios/ecs/schedule/executor/single_threaded.hpp>
#include <helios/ecs/schedule/schedule.hpp>

using namespace helios;
using namespace helios::ecs;

namespace {

struct CounterSystem {
  void operator()() {}

  int* counter = nullptr;
};

}  // namespace

TEST_SUITE("helios::ecs::SingleThreadedExecutor") {
  TEST_CASE("ecs::SingleThreadedExecutor::Execute") {
    SUBCASE("Execute dispatches all systems in a built schedule") {
      async::Executor async_executor;
      int counter = 0;

      Schedule schedule;
      CounterSystem system{.counter = &counter};
      schedule.Add(std::move(system));
      const auto build_result = schedule.Build();
      CHECK(build_result.has_value());

      World world;
      SingleThreadedExecutor executor(async_executor);
      executor.Execute(schedule, world);
      executor.Wait();

      CHECK_EQ(counter, 0);
    }

    SUBCASE("Wait does nothing when no execution is pending") {
      async::Executor async_executor;
      SingleThreadedExecutor executor(async_executor);
      executor.Wait();
    }
  }
}
