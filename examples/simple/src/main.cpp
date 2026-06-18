#include <helios/app/app.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/log/logger.hpp>
#include <helios/profile/backends/tracy.hpp>
#include <helios/profile/profile.hpp>

#include <cstddef>
#include <thread>
#include <utility>
#include "helios/assert.hpp"

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hprofile = helios::profile;

namespace {

struct GameConfig {
  float speed_multiplier = 1.0F;
  bool paused = false;
};

struct FrameCount {
  size_t count = 0;
};

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 1.0F;
  float dy = 0.5F;
};

struct Player {};
struct Enemy {};

struct Lifetime {
  float remaining = 10.0F;
};

struct DamageEvent {
  hecs::Entity target;
  float amount = 0.0F;
};

happ::ExitCode GameRunner(happ::App& app) {
  while (!app.ShouldExit()) {
    app.Update();
  }
  return app.ShouldExit().value_or(happ::ExitCode::kSuccess);
}

struct MovementSystems {};
constexpr MovementSystems kMovementSystems{};

struct LifecycleSystems {};
constexpr LifecycleSystems kLifecycleSystems{};

struct DiagnosticsSystems {};
constexpr DiagnosticsSystems kDiagnosticsSystems{};

struct UpdateFrameCount {
  void operator()(hecs::Res<FrameCount> frame_count) { ++frame_count->count; }
};

struct SpawnEntities {
  void operator()(hecs::Res<const FrameCount> frame_count,
                  hecs::Commands commands) {
    // Spawn a player every 100 frames
    if (frame_count->count % 100 == 0) {
      commands.Spawn().AddComponents(Player{}, Position{100.0F, 100.0F},
                                     Velocity{0.0F, 0.0F});
    }
    // Spawn enemies every 50 frames
    if (frame_count->count % 50 == 0) {
      for (int i = 0; i < 3; ++i) {
        commands.Spawn().AddComponents(
            Enemy{}, Position{static_cast<float>(i * 50), 200.0F},
            Velocity{0.5F, -1.0F}, Lifetime{5.0F});
      }
    }
  }
};

struct MoveEntities {
  void operator()(hecs::Res<const GameConfig> config,
                  hecs::Query<Position&, const Velocity> movables) {
    if (config->paused) {
      return;
    }

    movables.ForEach([&config](Position& pos, const Velocity& vel) {
      pos.x += vel.dx * config->speed_multiplier;
      pos.y += vel.dy * config->speed_multiplier;
    });
  }
};

struct UpdateLifetimes {
  void operator()(hecs::Query<Lifetime&> lifetimes, hecs::Commands commands) {
    lifetimes.ForEachWithEntity(
        [&commands](hecs::Entity entity, Lifetime& lifetime) {
          lifetime.remaining -= 0.016F;  // ~60fps delta
          if (lifetime.remaining <= 0.0F) {
            commands.Entity(entity).Destroy();
          }
        });
  }
};

struct LogEntityCount {
  void operator()(hecs::Res<const FrameCount> frame_count,
                  hecs::Query<hecs::With<Player>> players,
                  hecs::Query<hecs::With<Enemy>> enemies,
                  hecs::Query<const Position&> positions) {
    helios::log::Info(
        "Frame {}: {} entities ({} players, {} enemies, {} with "
        "position)",
        frame_count->count, positions.Count(), players.Count(), enemies.Count(),
        positions.Count());
  }
};

struct DelayedShutdown {
  void operator()(hecs::Res<const FrameCount> frame_count,
                  hecs::MessageWriter<happ::AppExit> app_exit_writer) {
    helios::log::Info("Requesting shutdown after {} frames",
                      frame_count->count);
    app_exit_writer.Write(happ::AppExit{.code = happ::ExitCode::kSuccess});
  }
};

struct PhysicsLabel {};

struct PhysicsWorld {
  float gravity = -9.81F;
  float time = 0.0F;
};

struct PhysicsBody {
  float mass = 1.0F;
  float velocity_x = 0.0F;
  float velocity_y = 0.0F;
  float force_x = 0.0F;
  float force_y = 0.0F;
};

void ExtractPhysics(const hecs::World& main_world, hecs::World& physics_world) {
  auto positions = main_world.ReadOnlyQuery<const Velocity>();

  // Clear and re-insert resources in physics world
  physics_world.InsertResources(PhysicsWorld{.gravity = -9.81F, .time = 0.0F});

  // Copy entities
  positions.ForEach([&physics_world](const Velocity& vel) {
    auto phys_entity = physics_world.CreateEntity();
    physics_world.AddComponents(
        phys_entity, PhysicsBody{.velocity_x = vel.dx, .velocity_y = vel.dy});
  });
}

struct SimulatePhysics {
  void operator()(hecs::Res<PhysicsWorld> world,
                  hecs::Query<PhysicsBody&> bodies) {
    constexpr float dt = 0.016F;
    world->time += dt;

    // Pre-calculate constants
    const float gravity_accel = world->gravity * dt;

    bodies.ForEach([&world, gravity_accel](PhysicsBody& body) {
      // Avoid division if mass is always 1.0
      if (body.mass != 1.0F) {
        const float inv_mass = 1.0F / body.mass;
        body.velocity_x += body.force_x * inv_mass * dt;
        body.velocity_y += (body.force_y * inv_mass + world->gravity) * dt;
      } else {
        body.velocity_x += body.force_x * dt;
        body.velocity_y += body.force_y * dt + gravity_accel;
      }

      // Reset forces
      body.force_x = 0.0F;
      body.force_y = 0.0F;
    });
  }
};

}  // namespace

int main() {
  // Setup profiler
  auto& profiler = hprofile::Profiler::Instance();
  profiler.AddBackend<hprofile::TracyBackend>();
  profiler.Finalize();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  happ::App app;

  app.SetRunner(GameRunner)
      .AddMessages<DamageEvent>()
      .InsertResources(GameConfig{.speed_multiplier = 1.5F}, FrameCount{});

  // kFirst: advance frame counter. Spawn commands in kPreUpdate are flushed
  // before kUpdate, so movement and lifecycle see new entities same frame.
  app.AddSystem(happ::kFirst, UpdateFrameCount{});
  app.AddSystem(happ::kPreUpdate, SpawnEntities{});

  auto lifecycle_set = app.ConfigureSet(happ::kUpdate, kLifecycleSystems);
  app.AddSystem(happ::kUpdate, UpdateLifetimes{}).InSet(kLifecycleSystems);

  app.ConfigureSet(happ::kUpdate, kMovementSystems).Before(lifecycle_set);
  app.AddSystem(happ::kUpdate, MoveEntities{}).InSet(kMovementSystems);

  app.ConfigureSet(happ::kUpdate, kDiagnosticsSystems);
  app.AddSystem(happ::kUpdate, LogEntityCount{})
      .InSet(kDiagnosticsSystems)
      .After(lifecycle_set)
      .RunIf("LogEntityCountCheckFrame",
             [](hecs::Res<const FrameCount> frame_count) {
               return frame_count->count % 60 == 0;
             });

  app.AddSystem(happ::kLast, DelayedShutdown{})
      .RunIf("FrameLimitReached", [](hecs::Res<const FrameCount> frame_count) {
        return frame_count->count >= 1000;
      });

  auto physics_sub_app = happ::SubApp::From(PhysicsLabel{});
  physics_sub_app.SetExtractFunction(ExtractPhysics);
  physics_sub_app.InsertResources(PhysicsWorld{.gravity = -9.81F, .time = 0.0F})
      .AddSystem(happ::kUpdate, SimulatePhysics{});

  app.InsertSubApp(PhysicsLabel{}, std::move(physics_sub_app));

  const auto code = app.Run();
  return std::to_underlying(code);
}
