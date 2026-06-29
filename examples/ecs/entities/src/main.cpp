#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/world.hpp>
#include <helios/log/logger.hpp>

#include <cstddef>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct FrameCount {
  size_t count = 0;
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
  DemonstrateEntities(app.GetWorld());

  app.InsertResources(FrameCount{});
  app.AddSystem(happ::kFirst, BumpFrame{});
  app.AddSystem(happ::kLast, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
