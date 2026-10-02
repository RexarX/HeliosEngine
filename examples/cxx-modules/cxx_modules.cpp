#include <utility>

import helios.app;
import helios.ecs;
import helios.log;

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct LogFrame {
  void operator()(hecs::Res<const happ::FrameCount> frames) const {
    hlog::Info("Frame {}", frames->count);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      hlog::Info("Exiting after {} frames", frames->count);
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;

  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddSystem(happ::kUpdate, LogFrame{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  return std::to_underlying(app.Run());
}
