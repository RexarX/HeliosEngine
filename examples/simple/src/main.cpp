#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// `FrameCountPlugin` inserts FrameCount as a resource. Systems can read it with
// `Res<const FrameCount>` just like any other ECS resource.
struct LogFrame {
  void operator()(hecs::Res<const happ::FrameCount> frames) const {
    hlog::Info("simple: frame {}", frames->count);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    // App shutdown is requested by sending AppExit. The app runner observes the
    // message after this stage and returns the embedded exit code.
    if (frames->count >= 5) {
      hlog::Info("simple: exiting after {} frames", frames->count);
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;

  // Plugins are the usual way to install shared resources and systems.
  app.AddPlugins(happ::FrameCountPlugin{});

  // Update does the frame work; PostUpdate is a good place for decisions that
  // should happen after the frame's main systems have run.
  app.AddSystem(happ::kUpdate, LogFrame{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
