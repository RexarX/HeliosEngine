#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 3) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

void DemonstrateEntities(hecs::World& world) {
  const hecs::Entity immediate = world.CreateEntity();
  hlog::Info("entities: CreateEntity -> index={} gen={}", immediate.Index(),
             immediate.Generation());

  const hecs::Entity reserved = world.ReserveEntity();
  hlog::Info("entities: ReserveEntity -> index={} (pending flush)",
             reserved.Index());

  world.Flush();
  hlog::Info("entities: after Flush reserved exists={}",
             world.Exists(reserved));

  world.DestroyEntity(immediate);
  hlog::Info("entities: destroyed immediate, stale handle exists={}",
             world.Exists(immediate));

  const hecs::Entity recycled = world.CreateEntity();
  hlog::Info("entities: recycled index={} gen={}", recycled.Index(),
             recycled.Generation());
}

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  DemonstrateEntities(app.GetWorld());

  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
