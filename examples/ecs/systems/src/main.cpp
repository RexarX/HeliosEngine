#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// The systems in this example all mutate or read one shared resource so their
// ordering is easy to see in the log.
struct Counter {
  int value = 0;
};

// Set labels group systems so ordering and run conditions can be applied to the
// group instead of every system one by one.
struct MovementSet {};
constexpr MovementSet kMovementSet{};

struct DiagnosticsSet {};
constexpr DiagnosticsSet kDiagnosticsSet{};

// Function objects are systems when operator() has supported parameters.
struct FunctionSystemStruct {
  void operator()(hecs::Res<Counter> counter) const {
    counter->value += 2;
    hlog::Info("systems: FunctionSystemStruct value={}", counter->value);
  }
};

// Free functions can be registered through kSystem so the scheduler keeps the
// function type as the system identity.
void FreeFunctionSystem(hecs::Res<Counter> counter) {
  ++counter->value;
  hlog::Info("systems: FreeFunctionSystem value={}", counter->value);
}

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

  // AddSystems returns a handle to the group it just created. InSet attaches
  // both systems to MovementSet.
  auto movement =
      app.AddSystems(happ::kUpdate, hecs::kSystem<FreeFunctionSystem>,
                     FunctionSystemStruct{})
          .InSet(kMovementSet);

  // DiagnosticsSet is ordered after the movement group as a unit.
  app.ConfigureSet(happ::kUpdate, kDiagnosticsSet).After(movement);

  // Lambdas can be named explicitly, which makes logs and diagnostics clearer.
  const auto lambda_handle =
      app.AddSystem(happ::kUpdate, "LambdaSystem",
                    [](hecs::Res<Counter> counter) {
                      counter->value += 3;
                      hlog::Info("systems: LambdaSystem value={}",
                                 counter->value);
                    })
          .InSet(kDiagnosticsSet)
          // RunIf gates this system each frame without removing it from the
          // schedule graph.
          .RunIf("LambdaSystemRunIfEvenFrame",
                 [](hecs::Res<const happ::FrameCount> frames) {
                   return frames->count % 2 == 0;
                 });

  // System handles can order individual systems inside or across sets.
  app.AddSystem(happ::kUpdate, ConditionalSystem{})
      .InSet(kDiagnosticsSet)
      .After(lambda_handle);

  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
