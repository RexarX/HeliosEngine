#include <helios/app/application.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/stage.hpp>
#include <helios/log/logger.hpp>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

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

  auto& scheduler = app.GetMainSubApp().GetScheduler();
  scheduler.AddStage(FirstStage{});
  scheduler.AddStage(SecondStage{});
  scheduler.OrderStage(SecondStage{}).After(FirstStage{});

  scheduler.Add(FirstSchedule{}, hecs::Schedule{}).InStage(FirstStage{});
  scheduler.Add(SecondSchedule{}, hecs::Schedule{}).InStage(SecondStage{});

  const auto increment = scheduler.In(FirstSchedule{}).Add(Increment{});
  scheduler.In(FirstSchedule{})
      .Add(LogStage{.name = "FirstStage"})
      .After(increment);
  const auto increment_second = scheduler.In(SecondSchedule{}).Add(Increment{});
  scheduler.In(SecondSchedule{})
      .Add(LogStage{.name = "SecondStage"})
      .After(increment_second);

  app.Initialize();

  auto& world = app.GetWorld();
  hlog::Info("stages: running FirstStage");
  scheduler.RunStage(FirstStage{}, world);
  hlog::Info("stages: running SecondStage");
  scheduler.RunStage(SecondStage{}, world);

  hlog::Info("stages: final counter={}", world.ReadResource<Counter>().value);
  return 0;
}
