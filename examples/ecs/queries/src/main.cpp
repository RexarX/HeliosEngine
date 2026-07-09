#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// These components give the query examples a mix of required, optional, and
// tag-only data to filter over.
struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct Acceleration {
  float ddx = 0.1F;
  float ddy = 0.05F;
};

struct Player {};
struct Static {};

struct SpawnQueryTargets {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count != 0) {
      return;
    }

    // One entity has all movement components. The other is static, so optional
    // query parameters can demonstrate missing components.
    commands.Spawn().AddComponents(Player{}, Position{.x = 0.0F, .y = 0.0F},
                                   Velocity{.dx = 1.0F, .dy = 0.5F},
                                   Acceleration{});
    commands.Spawn().AddComponents(Static{}, Position{.x = 5.0F, .y = 5.0F});
  }
};

struct MoveWithOptionalComponents {
  void operator()(
      hecs::Query<Position&, Velocity*, const Acceleration*> entities) const {
    // Pointer query parameters are optional. Missing components arrive as null
    // instead of excluding the entity from the query.
    entities.ForEach(
        [](Position& pos, Velocity* vel, const Acceleration* accel) {
          if (vel == nullptr) {
            return;
          }
          if (accel != nullptr) {
            vel->dx += accel->ddx;
            vel->dy += accel->ddy;
          }
          pos.x += vel->dx;
          pos.y += vel->dy;
        });
  }
};

struct LogQueryResults {
  void operator()(
      hecs::Query<const Position&, hecs::With<Player>> players,
      hecs::Query<const Position&, hecs::Without<Velocity>> static_objects,
      hecs::Query<const Position&, const Velocity*, const Acceleration*>
          optional_components) const {
    // WithEntity adds the owning Entity to each result tuple.
    players.ForEachWithEntity([](hecs::Entity entity, const Position& pos) {
      hlog::Info("queries: player {} at ({}, {})", entity, pos.x, pos.y);
    });

    // Query adapters are lazy until a terminal operation such as Collect runs.
    const auto sums =
        static_objects.Map([](const Position& pos) { return pos.x + pos.y; })
            .Collect();
    hlog::Info("queries: static position sum={}",
               sums.empty() ? 0.0F : sums.front());

    // This pass prints which optional components were present on each entity.
    optional_components.ForEachWithEntity(
        [](hecs::Entity entity, const Position& pos, const Velocity* vel,
           const Acceleration* accel) {
          hlog::Info(
              "queries: entity {} at ({}, {}), has_velocity={}, "
              "has_acceleration={}",
              entity, pos.x, pos.y, vel != nullptr, accel != nullptr);
        });
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

  // The schedule order makes spawned entities visible before movement and logs
  // the final frame state afterward.
  app.AddSystem(happ::kPreUpdate, SpawnQueryTargets{});
  app.AddSystem(happ::kUpdate, MoveWithOptionalComponents{});
  app.AddSystem(happ::kPostUpdate, LogQueryResults{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
