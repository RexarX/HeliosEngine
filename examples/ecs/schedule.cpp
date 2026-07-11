#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// Schedule labels are lightweight types. They identify where systems are stored
// in a scheduler stage.
struct CustomSchedule {};

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
    // The shared counter records the order in which schedules actually execute.
    *slot = (*call_index)++;
    hlog::Info("schedules: {} ran (order index {})", label, *slot);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 3) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  // This state lives outside the World because the example is about schedule
  // ordering, not resource access.
  OrderSlot order{};

  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddSystem(happ::kFirst, MarkOrder{.slot = &order.first,
                                        .call_index = &order.call_index,
                                        .label = "kFirst"});

  // AddSchedule inserts a whole schedule into an existing stage. Ordering it
  // between kFirst and kLast controls when its systems are run.
  app.AddSchedule(CustomSchedule{}, hecs::Schedule{})
      .InStage(happ::kUpdateStage)
      .After(happ::kFirst)
      .Before(happ::kLast);

  app.AddSystem(CustomSchedule{}, MarkOrder{.slot = &order.custom,
                                            .call_index = &order.call_index,
                                            .label = "CustomSchedule"});

  // kPostUpdate is still part of the same update stage ordering chain.
  app.AddSystems(happ::kPostUpdate,
                 MarkOrder{.slot = &order.last,
                           .call_index = &order.call_index,
                           .label = "kLast"},
                 ExitAfterFrames{});

  const auto code = app.Run();
  hlog::Info("schedules: first={} custom={} last={}", order.first, order.custom,
             order.last);
  return std::to_underlying(code);
}
