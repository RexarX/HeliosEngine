#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct LogFrame {
  void operator()(hecs::Res<const happ::FrameCount> frames) const {
    hlog::Info("simple: frame {}", frames->count);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      hlog::Info("simple: exiting after {} frames", frames->count);
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

  const auto code = app.Run();
  return std::to_underlying(code);
}
