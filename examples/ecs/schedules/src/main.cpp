#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/log/logger.hpp>

#include <cstddef>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct CustomSchedule {};

struct FrameCount {
  size_t count = 0;
};

struct OrderSlot {
  int first = -1;
  int custom = -1;
  int last = -1;
  int call_index = 0;
};

struct MarkOrder {
  int* slot = nullptr;
  int* call_index = nullptr;
  const char* label = nullptr;

  void operator()() const {
    *slot = (*call_index)++;
    hlog::Info("schedules: {} ran (order index {})", label, *slot);
  }
};

struct BumpFrame {
  void operator()(hecs::Res<FrameCount> frames) const { ++frames->count; }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 3) {
      exit_writer.Write({.code = happ::ExitCode::kSuccess});
    }
  }
};

}  // namespace

int main() {
  OrderSlot order{};

  happ::App app;
  app.InsertResources(FrameCount{});
  app.AddSystems(happ::kFirst, BumpFrame{},
                 MarkOrder{.slot = &order.first,
                           .call_index = &order.call_index,
                           .label = "kFirst"});
  app.AddSchedule(CustomSchedule{}, hecs::Schedule{})
      .InStage(happ::kUpdateStage)
      .After(happ::kFirst)
      .Before(happ::kLast);
  app.AddSystem(CustomSchedule{}, MarkOrder{.slot = &order.custom,
                                            .call_index = &order.call_index,
                                            .label = "CustomSchedule"});
  app.AddSystems(happ::kLast,
                 MarkOrder{.slot = &order.last,
                           .call_index = &order.call_index,
                           .label = "kLast"},
                 ExitAfterFrames{});

  const auto code = app.Run();
  hlog::Info("schedules: first={} custom={} last={}", order.first, order.custom,
             order.last);
  return std::to_underlying(code);
}
