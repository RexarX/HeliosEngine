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

struct Player {};

struct Enemy {
  static constexpr auto kStorageType = hecs::ComponentStorageType::kSparseSet;
};

struct SpawnComponents {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count != 0) {
      return;
    }

    commands.Spawn().AddComponents(Player{}, Position{.x = 1.0F, .y = 2.0F},
                                   Velocity{.dx = 0.5F, .dy = 0.25F});
    commands.Spawn().AddComponents(Enemy{}, Position{.x = 10.0F, .y = 20.0F});
    hlog::Info("components: queued player and enemy spawns");
  }
};

struct LogComponents {
  void operator()(
      hecs::Query<const Position&, const Velocity&, hecs::With<Player>> players,
      hecs::Query<hecs::With<Enemy>> enemies) const {
    players.ForEach([](const Position& pos, const Velocity& vel) {
      hlog::Info("components: player pos=({},{}) vel=({},{})", pos.x, pos.y,
                 vel.dx, vel.dy);
    });
    hlog::Info("components: {} enemies (tag/sparse)", enemies.Count());
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 3) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddSystem(happ::kPreUpdate, SpawnComponents{});
  app.AddSystem(happ::kUpdate, LogComponents{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
