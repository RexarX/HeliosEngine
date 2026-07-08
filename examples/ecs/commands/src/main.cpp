#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

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

struct SpawnUnit {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count == 0) {
      hlog::Info("commands: spawning entity (deferred)");
      commands.Spawn().AddComponents(Health{.hp = 3}, Spawned{});
    }
  }
};

struct EnqueueReinforcement {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count == 1) {
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
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddSystems(happ::kPreUpdate, SpawnUnit{}, EnqueueReinforcement{});
  app.AddSystem(happ::kUpdate, DamageUnits{});
  app.AddSystem(happ::kPostUpdate, LogRemaining{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});
  app.InsertResources(ReinforcementsCalled{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
