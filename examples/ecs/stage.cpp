#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// Stages are coarser than schedules: a stage owns one or more schedules and can
// be run explicitly.
struct Counter {
  int value = 0;
};

struct FirstStage {};
struct SecondStage {};

struct FirstSchedule {};
struct SecondSchedule {};

struct Increment {
  void operator()(hecs::Res<Counter> counter) const { ++counter->value; }
};

struct LogStage {
  const char* name = nullptr;

  void operator()(hecs::Res<const Counter> counter) const {
    hlog::Info("stages: {} counter={}", name, counter->value);
  }
};

}  // namespace

int main() {
  happ::App app;
  app.InsertResources(Counter{});

  // This example uses the scheduler directly to show manual stage construction.
  auto& scheduler = app.GetMainSubApp().GetScheduler();

  // Stage ordering is separate from schedule ordering inside a stage.
  scheduler.AddStage(FirstStage{});
  scheduler.AddStage(SecondStage{});
  scheduler.OrderStage(SecondStage{}).After(FirstStage{});

  // Each schedule is placed into the stage it should run inside.
  scheduler.Add(FirstSchedule{}, hecs::Schedule{}).InStage(FirstStage{});
  scheduler.Add(SecondSchedule{}, hecs::Schedule{}).InStage(SecondStage{});

  // Within a schedule, system handles can express before/after dependencies.
  const auto increment = scheduler.In(FirstSchedule{}).Add(Increment{});
  scheduler.In(FirstSchedule{})
      .Add(LogStage{.name = "FirstStage"})
      .After(increment);
  const auto increment_second = scheduler.In(SecondSchedule{}).Add(Increment{});
  scheduler.In(SecondSchedule{})
      .Add(LogStage{.name = "SecondStage"})
      .After(increment_second);

  // Initialize prepares app/plugin state without entering the default runner.
  app.Initialize();

  auto& world = app.GetWorld();
  // Running stages manually is useful for tests or custom engine loops.
  hlog::Info("stages: running FirstStage");
  scheduler.RunStage(FirstStage{}, world);
  hlog::Info("stages: running SecondStage");
  scheduler.RunStage(SecondStage{}, world);

  hlog::Info("stages: final counter={}", world.ReadResource<Counter>().value);
  return 0;
}
