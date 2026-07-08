#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct Counter {
  int value = 0;
};

struct MovementSet {};
constexpr MovementSet kMovementSet{};

struct DiagnosticsSet {};
constexpr DiagnosticsSet kDiagnosticsSet{};

struct StructSystem {
  void operator()(hecs::Res<Counter> counter) const {
    ++counter->value;
    hlog::Info("systems: StructSystem value={}", counter->value);
  }
};

struct FunctionSystemStruct {
  void operator()(hecs::Res<Counter> counter) const {
    counter->value += 2;
    hlog::Info("systems: FunctionSystemStruct value={}", counter->value);
  }
};

struct LambdaSystem {
  void operator()(hecs::Res<Counter> counter) const {
    counter->value += 3;
    hlog::Info("systems: LambdaSystem value={}", counter->value);
  }
};

struct ConditionalSystem {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Res<Counter> counter) const {
    hlog::Info("systems: ConditionalSystem frame={} value={}", frames->count,
               counter->value);
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
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.InsertResources(Counter{});

  auto movement =
      app.AddSystems(happ::kUpdate, StructSystem{}, FunctionSystemStruct{})
          .InSet(kMovementSet);

  app.ConfigureSet(happ::kUpdate, kDiagnosticsSet).After(movement);
  const auto lambda_handle =
      app.AddSystem(happ::kUpdate, LambdaSystem{})
          .InSet(kDiagnosticsSet)
          .RunIf("LambdaSystemRunIfEvenFrame",
                 [](hecs::Res<const happ::FrameCount> frames) {
                   return frames->count % 2 == 0;
                 });
  app.AddSystem(happ::kUpdate, ConditionalSystem{})
      .InSet(kDiagnosticsSet)
      .After(lambda_handle);

  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
