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
  // CreateEntity mutates the world immediately and returns a live handle.
  const hecs::Entity immediate = world.CreateEntity();
  hlog::Info("entities: CreateEntity -> entity={} gen={}", immediate,
             immediate.Generation());

  // ReserveEntity only allocates an id. The entity becomes real on Flush.
  const hecs::Entity reserved = world.ReserveEntity();
  hlog::Info("entities: ReserveEntity -> entity={} (pending flush)", reserved);

  // Flush publishes reserved entities and runs queued world commands.
  world.Flush();
  hlog::Info("entities: after Flush reserved exists={}",
             world.Exists(reserved));

  // Destroying bumps the generation, so old handles stop validating.
  world.DestroyEntity(immediate);
  hlog::Info("entities: destroyed immediate, stale handle exists={}",
             world.Exists(immediate));

  // The index can be recycled, but the generation distinguishes the new entity
  // from stale handles that used the same slot.
  const hecs::Entity recycled = world.CreateEntity();
  hlog::Info("entities: recycled entity={} gen={}", recycled,
             recycled.Generation());
}

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // This example uses the immediate World API directly before the app runner
  // starts, where no systems are executing concurrently.
  DemonstrateEntities(app.GetWorld());

  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
