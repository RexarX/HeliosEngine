#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 1.0F;
  float dy = 0.0F;
};

struct Health {
  int value = 100;
};

struct Player {};

// A component bundle groups values that are commonly inserted or removed
// together. It is a transfer object; queries still access its leaf components.
using MovementBundle = hecs::ComponentBundle<Position, Velocity>;

// Bundles can contain other bundles. Operations flatten nested bundles in
// declaration order, so this contributes Player, Position, Velocity, Health.
using PlayerBundle = hecs::ComponentBundle<Player, MovementBundle, Health>;

struct SpawnPlayer {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count != 0) {
      return;
    }

    // AddBundle owns the complete nested bundle until this deferred entity
    // command runs at the end of the schedule.
    commands.Spawn().AddBundle(
        PlayerBundle{Player{},
                     MovementBundle{Position{.x = 1.0F, .y = 2.0F},
                                    Velocity{.dx = 0.5F, .dy = 0.25F}},
                     Health{.value = 100}});
    hlog::Info("component bundles: queued player spawn");
  }
};

struct MovePlayer {
  void operator()(
      hecs::Res<const happ::FrameCount> frames,
      hecs::Query<Position&, const Velocity&, hecs::With<Player>> players,
      hecs::Commands commands) const {
    players.ForEachWithEntity([&frames, &commands](hecs::Entity entity,
                                                   Position& position,
                                                   const Velocity& velocity) {
      position.x += velocity.dx;
      position.y += velocity.dy;

      if (frames->count == 2) {
        // Removing a bundle removes every flattened leaf while preserving
        // unrelated components such as Player and Health.
        commands.Entity(entity).RemoveBundle<MovementBundle>();
      }
    });
  }
};

struct LogPlayer {
  void operator()(
      hecs::Query<const Position*, const Health&, hecs::With<Player>> players)
      const {
    players.ForEach([](const Position* position, const Health& health) {
      if (position != nullptr) {
        hlog::Info("component bundles: player pos=({},{}) health={}",
                   position->x, position->y, health.value);
      } else {
        hlog::Info("component bundles: movement bundle removed, health={}",
                   health.value);
      }
    });
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 4) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // The PreUpdate spawn is flushed before Update. Bundle removal queued during
  // Update is then visible to the PostUpdate logger in the same frame.
  app.AddSystem(happ::kPreUpdate, SpawnPlayer{});
  app.AddSystem(happ::kUpdate, MovePlayer{});
  app.AddSystems(happ::kPostUpdate, LogPlayer{}, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
