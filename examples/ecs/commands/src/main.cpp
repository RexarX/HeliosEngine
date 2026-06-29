#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/log/logger.hpp>

#include <cstddef>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct FrameCount {
  size_t count = 0;
};

struct Health {
  int hp = 3;
};

struct Spawned {};

struct ReinforcementsCalled {
  int count = 0;
};

struct SpawnReinforcementCommand {
  void Execute(hecs::World& world) const {
    const hecs::Entity entity = world.CreateEntity();
    world.AddComponents(entity, Health{.hp = 5}, Spawned{});
    world.WriteResource<ReinforcementsCalled>().count += 1;
    hlog::Info("commands: custom command spawned reinforcement {}",
               entity.Index());
  }
};

struct BumpFrame {
  void operator()(hecs::Res<FrameCount> frames) const { ++frames->count; }
};

struct SpawnUnit {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count == 1) {
      hlog::Info("commands: spawning entity (deferred)");
      commands.Spawn().AddComponents(Health{.hp = 3}, Spawned{});
    }
  }
};

struct EnqueueReinforcement {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count == 2) {
      hlog::Info("commands: enqueuing custom reinforcement command");
      commands.Enqueue(SpawnReinforcementCommand{});
    }
  }
};

struct DamageUnits {
  void operator()(hecs::Query<Health&> units, hecs::Commands commands) const {
    units.ForEachWithEntity([&commands](hecs::Entity entity, Health& health) {
      --health.hp;
      hlog::Info("commands: entity {} hp={}", entity.Index(), health.hp);
      if (health.hp <= 0) {
        hlog::Info("commands: queue destroy for {}", entity.Index());
        commands.Entity(entity).Destroy();
      }
    });
  }
};

struct LogRemaining {
  void operator()(hecs::Query<const Health&> units,
                  hecs::Res<const ReinforcementsCalled> reinforcements) const {
    hlog::Info("commands: {} entities with Health", units.Count());
    hlog::Info("commands: reinforcements called {}", reinforcements->count);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      exit_writer.Write({.code = happ::ExitCode::kSuccess});
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddSystem(happ::kFirst, BumpFrame{});
  app.AddSystems(happ::kPreUpdate, SpawnUnit{}, EnqueueReinforcement{});
  app.AddSystem(happ::kUpdate, DamageUnits{});
  app.AddSystem(happ::kPostUpdate, LogRemaining{});
  app.AddSystem(happ::kLast, ExitAfterFrames{});
  app.InsertResources(FrameCount{}, ReinforcementsCalled{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
