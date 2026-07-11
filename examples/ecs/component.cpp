#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// Data components with fields use archetype storage by default. Entities with
// the same component set are packed together for query iteration.
struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 1.0F;
  float dy = 0.0F;
};

// Empty structs are tag components. They mark entities without storing payload.
struct Player {};

struct Enemy {
  // Sparse-set storage keeps this marker outside the archetype columns. This is
  // useful for tags or components that should not force archetype migration
  // (for example frequently added/removed components).
  static constexpr auto kStorageType = hecs::ComponentStorageType::kSparseSet;
};

struct SpawnComponents {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count != 0) {
      return;
    }

    // Component insertion is deferred through Commands, so spawning is safe
    // even while other systems may be reading the world.
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
    // With<Player> filters the query to tagged entities without reading a
    // Player value.
    players.ForEach([](const Position& pos, const Velocity& vel) {
      hlog::Info("components: player pos=({},{}) vel=({},{})", pos.x, pos.y,
                 vel.dx, vel.dy);
    });

    // A tag-only query can count matching entities without fetching component
    // payload.
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

  // Spawning happens first, then the update query observes the flushed
  // component layout.
  app.AddSystem(happ::kPreUpdate, SpawnComponents{});
  app.AddSystem(happ::kUpdate, LogComponents{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
