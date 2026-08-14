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

using MovementBundle = hecs::ComponentBundleTypes<Position, Velocity>;

struct PlayerBundle {
  using ComponentTypes =
      hecs::ComponentBundleTypes<Player, MovementBundle, Health>;

  Position position;
  Velocity velocity;
  Health health;

  [[nodiscard]] ComponentTypes Build() {
    return {Player{}, MovementBundle{position, velocity}, health};
  }
};

struct SpawnPlayer {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count != 0) {
      return;
    }

    commands.Spawn().AddBundle(
        PlayerBundle{.position = {.x = 1.0F, .y = 2.0F},
                     .velocity = {.dx = 0.5F, .dy = 0.25F},
                     .health = {.value = 100}});
    hlog::Info("Queued player spawn");
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
        hlog::Info("Player pos=({},{}) health={}", position->x, position->y,
                   health.value);
      } else {
        hlog::Info("Movement bundle removed, health={}", health.value);
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

  app.AddSystem(happ::kPreUpdate, SpawnPlayer{});
  app.AddSystem(happ::kUpdate, MovePlayer{});
  app.AddSystems(happ::kPostUpdate, LogPlayer{}, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
