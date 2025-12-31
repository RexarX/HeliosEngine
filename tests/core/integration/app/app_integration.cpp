#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/timer.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <concepts>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace helios::app;
using namespace helios::ecs;

namespace {

// ============================================================================
// Runner Helpers
// ============================================================================

AppExitCode FixedFrameRunner(App& app, int max_frames) {
  int frame = 0;
  while (frame < max_frames) {
    app.Update();
    ++frame;
  }
  return AppExitCode::Success;
}

template <typename Condition>
  requires std::predicate<Condition, App&>
AppExitCode ConditionalRunner(App& app, Condition condition, int max_frames = 1000) {
  int frame = 0;
  while (!condition(app) && frame < max_frames) {
    app.Update();
    ++frame;
  }
  return frame < max_frames ? AppExitCode::Success : AppExitCode::Failure;
}

// ============================================================================
// Game Simulation Components
// ============================================================================

struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr bool operator==(const Position&) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity&) const noexcept = default;
};

struct Acceleration {
  float ddx = 0.0F;
  float ddy = 0.0F;
  float ddz = 0.0F;
};

struct Health {
  int max_health = 100;
  int current_health = 100;

  constexpr void TakeDamage(int damage) noexcept { current_health = std::max(0, current_health - damage); }
  constexpr void Heal(int amount) noexcept { current_health = std::min(max_health, current_health + amount); }

  constexpr bool IsDead() const noexcept { return current_health <= 0; }

  constexpr bool operator==(const Health&) const noexcept = default;
};

struct Damage {
  int amount = 10;
};

struct Name {
  std::string value;

  constexpr bool operator==(const Name&) const noexcept = default;
};

struct Lifetime {
  float remaining = 5.0F;
};

// Tag components
struct Player {};
struct Enemy {};
struct Projectile {};
struct Dead {};
struct NeedsCleanup {};

// ============================================================================
// Game Resources
// ============================================================================

struct GameTime {
  float delta_time = 0.016F;
  float total_time = 0.0F;
  int frame_count = 0;

  static constexpr std::string_view GetName() noexcept { return "GameTime"; }
};

struct PhysicsSettings {
  float gravity = -9.81F;
  float friction = 0.98F;
  float max_velocity = 100.0F;

  static constexpr std::string_view GetName() noexcept { return "PhysicsSettings"; }
};

struct GameConfig {
  int max_enemies = 10;
  int max_projectiles = 50;
  float spawn_interval = 2.0F;
  float time_since_spawn = 0.0F;

  static constexpr std::string_view GetName() noexcept { return "GameConfig"; }
};

struct GameStats {
  std::atomic<int> entities_spawned{0};
  std::atomic<int> entities_destroyed{0};
  std::atomic<int> projectiles_fired{0};
  std::atomic<int> combat_events{0};
  std::atomic<int> frames_rendered{0};

  GameStats() noexcept = default;
  GameStats(const GameStats& other) noexcept
      : entities_spawned(other.entities_spawned.load(std::memory_order_relaxed)),
        entities_destroyed(other.entities_destroyed.load(std::memory_order_relaxed)),
        projectiles_fired(other.projectiles_fired.load(std::memory_order_relaxed)),
        combat_events(other.combat_events.load(std::memory_order_relaxed)),
        frames_rendered(other.frames_rendered.load(std::memory_order_relaxed)) {}

  GameStats(GameStats&& other) noexcept
      : entities_spawned(other.entities_spawned.load(std::memory_order_relaxed)),
        entities_destroyed(other.entities_destroyed.load(std::memory_order_relaxed)),
        projectiles_fired(other.projectiles_fired.load(std::memory_order_relaxed)),
        combat_events(other.combat_events.load(std::memory_order_relaxed)),
        frames_rendered(other.frames_rendered.load(std::memory_order_relaxed)) {}

  GameStats& operator=(const GameStats&) = delete;
  GameStats& operator=(GameStats&&) = delete;

  static constexpr std::string_view GetName() noexcept { return "GameStats"; }
};

struct RenderData {
  struct RenderableEntity {
    Entity entity;
    Position position;
    std::string name;
  };

  std::vector<RenderableEntity> entities;
  int frame_number = 0;

  static constexpr std::string_view GetName() noexcept { return "RenderData"; }
};

struct AudioSettings {
  float master_volume = 1.0F;
  float music_volume = 0.7F;
  float sfx_volume = 0.8F;

  static constexpr std::string_view GetName() noexcept { return "AudioSettings"; }
};

// Thread-safe counter for testing (needs to be at file scope for AccessPolicy type matching)
struct ThreadSafeCounter {
  std::atomic<int> value{0};

  static constexpr std::string_view GetName() noexcept { return "ThreadSafeCounter"; }
  static constexpr bool ThreadSafe() noexcept { return true; }

  constexpr ThreadSafeCounter() noexcept = default;
  ThreadSafeCounter(const ThreadSafeCounter& other) noexcept : value(other.value.load(std::memory_order_acquire)) {}
  ThreadSafeCounter(ThreadSafeCounter&& other) noexcept : value(other.value.load(std::memory_order_acquire)) {}
  ~ThreadSafeCounter() noexcept = default;

  ThreadSafeCounter& operator=(const ThreadSafeCounter& other) noexcept {
    if (this != &other) {
      value.store(other.value.load(std::memory_order_acquire), std::memory_order_release);
    }
    return *this;
  }

  ThreadSafeCounter& operator=(ThreadSafeCounter&& other) noexcept {
    if (this != &other) {
      value.store(other.value.load(std::memory_order_acquire), std::memory_order_release);
    }
    return *this;
  }
};

// Increment system for thread-safe counter testing (at file scope)
struct IncrementSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "IncrementSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ThreadSafeCounter>(); }

  void Update(SystemContext& ctx) override {
    auto& counter = ctx.WriteResource<ThreadSafeCounter>();
    counter.value.fetch_add(1, std::memory_order_relaxed);
  }
};

// ============================================================================
// Game Events
// ============================================================================

struct EntitySpawnedEvent {
  Entity entity;
  std::array<char, 32> entity_type = {};
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr EntitySpawnedEvent() noexcept = default;
  constexpr EntitySpawnedEvent(Entity entt, std::string_view type, float px, float py, float pz) noexcept
      : entity(entt), x(px), y(py), z(pz) {
    const size_t len = std::min(type.size(), entity_type.size() - 1);
    std::ranges::copy_n(type.begin(), len, entity_type.begin());
  }

  static constexpr std::string_view GetName() noexcept { return "EntitySpawnedEvent"; }
};

struct EntityDestroyedEvent {
  Entity entity;
  std::array<char, 32> reason = {};

  constexpr EntityDestroyedEvent() noexcept = default;
  constexpr EntityDestroyedEvent(Entity entt, std::string_view str) noexcept : entity(entt) {
    const size_t len = std::min(str.size(), reason.size() - 1);
    std::ranges::copy_n(str.begin(), len, reason.begin());
  }

  static constexpr std::string_view GetName() noexcept { return "EntityDestroyedEvent"; }
};

struct CombatEvent {
  Entity attacker;
  Entity target;
  int damage = 0;

  static constexpr std::string_view GetName() noexcept { return "CombatEvent"; }
};

struct ProjectileFiredEvent {
  Entity projectile;
  Entity source;
  Position position;

  static constexpr std::string_view GetName() noexcept { return "ProjectileFiredEvent"; }
};

// ============================================================================
// System Sets
// ============================================================================

struct InputSet {};
struct PhysicsSet {};
struct GameplaySet {};
struct CombatSet {};
struct RenderSet {};
struct AudioSet {};
struct CleanupSet {};

// ============================================================================
// Core Systems
// ============================================================================

struct TimeUpdateSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "TimeUpdateSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<GameTime>(); }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    time.delta_time = 0.016F;  // Simulate 60 FPS
    time.total_time += time.delta_time;
    ++time.frame_count;
  }
};

struct EventLoggerSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "EventLoggerSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<GameStats>(); }

  void Update(SystemContext& ctx) override {
    auto& stats = ctx.WriteResource<GameStats>();

    auto spawned_reader = ctx.ReadEvents<EntitySpawnedEvent>();
    for (const auto& event : spawned_reader) {
      stats.entities_spawned.fetch_add(1, std::memory_order_relaxed);
    }

    auto destroyed_reader = ctx.ReadEvents<EntityDestroyedEvent>();
    for (const auto& event : destroyed_reader) {
      stats.entities_destroyed.fetch_add(1, std::memory_order_relaxed);
    }

    auto combat_reader = ctx.ReadEvents<CombatEvent>();
    for (const auto& event : combat_reader) {
      stats.combat_events.fetch_add(1, std::memory_order_relaxed);
    }

    auto projectile_reader = ctx.ReadEvents<ProjectileFiredEvent>();
    for (const auto& event : projectile_reader) {
      stats.projectiles_fired.fetch_add(1, std::memory_order_relaxed);
    }
  }
};

// ============================================================================
// Physics Systems
// ============================================================================

struct AccelerationSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "AccelerationSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<const Acceleration&, Velocity&>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    ctx.Query().Get<const Acceleration&, Velocity&>().ForEach([&time](const Acceleration& acc, Velocity& vel) {
      vel.dx += acc.ddx * time.delta_time;
      vel.dy += acc.ddy * time.delta_time;
      vel.dz += acc.ddz * time.delta_time;
    });
  }
};

struct MovementSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "MovementSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<const Velocity&, Position&>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    ctx.Query().Get<const Velocity&, Position&>().ForEach([&time](const Velocity& vel, Position& pos) {
      pos.x += vel.dx * time.delta_time;
      pos.y += vel.dy * time.delta_time;
      pos.z += vel.dz * time.delta_time;
    });
  }
};

struct GravitySystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "GravitySystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Velocity&>().ReadResources<PhysicsSettings, GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& physics = ctx.ReadResource<PhysicsSettings>();
    const auto& time = ctx.ReadResource<GameTime>();
    ctx.Query().Without<Dead>().Get<Velocity&>().ForEach(
        [&physics, &time](Velocity& vel) { vel.dy += physics.gravity * time.delta_time; });
  }
};

struct FrictionSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "FrictionSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Velocity&>().ReadResources<PhysicsSettings>();
  }

  void Update(SystemContext& ctx) override {
    const auto& physics = ctx.ReadResource<PhysicsSettings>();
    ctx.Query().Get<Velocity&>().ForEach([&physics](Velocity& vel) {
      vel.dx *= physics.friction;
      vel.dy *= physics.friction;
      vel.dz *= physics.friction;
    });
  }
};

struct VelocityClampSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "VelocityClampSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Velocity&>().ReadResources<PhysicsSettings>();
  }

  void Update(SystemContext& ctx) override {
    const auto& physics = ctx.ReadResource<PhysicsSettings>();
    auto query = ctx.Query().Get<Velocity&>();
    const float max_vel = physics.max_velocity;

    query.ForEach([&physics, max_vel](Velocity& vel) {
      const float speed = std::sqrt(vel.dx * vel.dx + vel.dy * vel.dy + vel.dz * vel.dz);
      if (speed > max_vel) {
        const float scale = max_vel / speed;
        vel.dx *= scale;
        vel.dy *= scale;
        vel.dz *= scale;
      }
    });
  }
};

// ============================================================================
// Gameplay Systems
// ============================================================================

struct EnemySpawnerSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "EnemySpawnerSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().WriteResources<GameConfig>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    auto& config = ctx.WriteResource<GameConfig>();
    const auto& time = ctx.ReadResource<GameTime>();

    config.time_since_spawn += time.delta_time;
    if (config.time_since_spawn >= config.spawn_interval) {
      const auto enemy_query = ctx.Query().With<Enemy>().Get();
      const auto current_enemies = static_cast<int>(enemy_query.Count());

      if (current_enemies < config.max_enemies) {
        const float spawn_x = static_cast<float>((current_enemies % 5) * 10);
        const float spawn_z = static_cast<float>((current_enemies / 5) * 10);

        auto cmd = ctx.EntityCommands(ctx.ReserveEntity());
        cmd.AddComponents(Enemy{}, Position{spawn_x, 0.0F, spawn_z}, Velocity{0.0F, 0.0F, 0.0F}, Health{50, 50},
                          Name{std::format("Enemy_{}", current_enemies)});

        ctx.EmitEvent(EntitySpawnedEvent{cmd.GetEntity(), "Enemy", spawn_x, 0.0F, spawn_z});
      }

      config.time_since_spawn = 0.0F;
    }
  }
};

struct ProjectileSpawnerSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "ProjectileSpawnerSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<const Position&>().ReadResources<GameConfig, GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& config = ctx.ReadResource<GameConfig>();
    const auto& time = ctx.ReadResource<GameTime>();

    // Fire projectile every 0.5 seconds if player exists
    static float time_since_fire = 0.0F;
    time_since_fire += time.delta_time;

    if (time_since_fire >= 0.5F) {
      auto player_query = ctx.Query().With<Player>().Get<const Position&>();
      const auto projectile_query = ctx.Query().With<Projectile>().Get();
      if (player_query.Count() > 0 && projectile_query.Count() < static_cast<size_t>(config.max_projectiles)) {
        player_query.ForEachWithEntity([&ctx](Entity player, const Position& pos) {
          auto cmd = ctx.EntityCommands(ctx.ReserveEntity());
          cmd.AddComponents(Projectile{}, Position{pos.x, pos.y + 1.0F, pos.z}, Velocity{10.0F, 0.0F, 0.0F}, Damage{25},
                            Lifetime{3.0F});

          ctx.EmitEvent(ProjectileFiredEvent{cmd.GetEntity(), player, pos});
        });
      }

      time_since_fire = 0.0F;
    }
  }
};

struct LifetimeSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "LifetimeSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Lifetime&>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    ctx.Query().Get<Lifetime&>().ForEachWithEntity([&ctx, &time](Entity entity, Lifetime& lifetime) {
      lifetime.remaining -= time.delta_time;
      if (lifetime.remaining <= 0.0F) {
        ctx.EntityCommands(entity).AddComponent(NeedsCleanup{});
      }
    });
  }
};

// ============================================================================
// Combat Systems
// ============================================================================

struct CollisionDetectionSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "CollisionDetectionSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().Query<const Position&, const Damage&>(); }

  void Update(SystemContext& ctx) override {
    constexpr float collision_radius = 2.0F;

    ctx.Query().With<Projectile>().Get<const Position&, const Damage&>().ForEachWithEntity(
        [&ctx](Entity proj_entity, const Position& proj_pos, const Damage& damage) {
          ctx.Query().With<Enemy>().Get<const Position&>().ForEachWithEntity(
              [&ctx, proj_entity, &proj_pos, &damage](Entity enemy_entity, const Position& enemy_pos) {
                const float dx = proj_pos.x - enemy_pos.x;
                const float dy = proj_pos.y - enemy_pos.y;
                const float dz = proj_pos.z - enemy_pos.z;
                const float dist_sq = dx * dx + dy * dy + dz * dz;

                if (dist_sq < collision_radius * collision_radius) {
                  ctx.EmitEvent(CombatEvent{proj_entity, enemy_entity, damage.amount});
                  ctx.EntityCommands(proj_entity).AddComponent(NeedsCleanup{});
                }
              });
        });
  }
};

class DamageApplicationSystem final : public System {
public:
  static constexpr std::string_view GetName() noexcept { return "DamageApplicationSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().Query<Health&>(); }

  void Update(SystemContext& ctx) override {
    damage_map_.clear();

    // Collect events into the reusable map for batch processing
    ctx.ReadEvents<CombatEvent>()
        .Filter([&ctx](const CombatEvent& event) { return ctx.EntityExists(event.target); })
        .ForEach([this](const CombatEvent& event) { damage_map_[event.target] += event.damage; });

    ctx.Query().Get<Health&>().ForEachWithEntity([this](Entity entity, Health& health) {
      if (const auto it = damage_map_.find(entity); it != damage_map_.end()) {
        health.TakeDamage(it->second);
      }
    });
  }

private:
  std::unordered_map<Entity, float> damage_map_;
};

struct DeathDetectionSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "DeathDetectionSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().Query<const Health&>(); }

  void Update(SystemContext& ctx) override {
    auto query = ctx.Query().Without<Dead>().Get<const Health&>();
    query.WithEntity()
        .Filter([](Entity /*entity*/, const Health& health) { return health.IsDead(); })
        .ForEach([&ctx](Entity entity, const Health& /*health*/) {
          ctx.EntityCommands(entity).AddComponents(Dead{}, NeedsCleanup{});
          ctx.EmitEvent(EntityDestroyedEvent{entity, "Killed in combat"});
        });
  }
};

// ============================================================================
// Cleanup Systems
// ============================================================================

struct EntityCleanupSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "EntityCleanupSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& ctx) override {
    ctx.Query().With<NeedsCleanup>().Get().ForEachWithEntity([&ctx](Entity entity) { ctx.Commands().Destroy(entity); });
  }
};

struct ManualEventClearSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "ManualEventClearSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& ctx) override {
    // Manually clear old events
    auto cmd = ctx.Commands();
    cmd.ClearEvents<EntitySpawnedEvent>();
    cmd.ClearEvents<CombatEvent>();
  }
};

// ============================================================================
// Render Systems (in SubApp)
// ============================================================================

struct RenderDataExtractionSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "RenderDataExtractionSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<const Position&, const Name&>().ReadResources<GameTime>().WriteResources<RenderData>();
  }

  void Update(SystemContext& ctx) override {
    auto& render_data = ctx.WriteResource<RenderData>();
    const auto& time = ctx.ReadResource<GameTime>();

    render_data.entities.clear();
    render_data.frame_number = time.frame_count;

    ctx.Query().With<Name>().Get<const Position&, const Name&>().ForEachWithEntity(
        [&render_data](Entity entity, const Position& pos, const Name& name) {
          render_data.entities.emplace_back(entity, pos, name.value);
        });
  }
};

struct RenderSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "RenderSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<RenderData, GameStats>(); }

  void Update(SystemContext& ctx) override {
    auto& render_data = ctx.WriteResource<RenderData>();
    auto& stats = ctx.WriteResource<GameStats>();

    // Simulate rendering
    stats.frames_rendered.fetch_add(1, std::memory_order_relaxed);
  }
};

// ============================================================================
// Audio Systems (in SubApp)
// ============================================================================

struct AudioSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "AudioSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().ReadResources<AudioSettings>(); }

  void Update(SystemContext& ctx) override {
    const auto& audio = ctx.ReadResource<AudioSettings>();

    // Process audio for projectile fires
    const auto fire_reader = ctx.ReadEvents<ProjectileFiredEvent>();
    for (const auto& event : fire_reader) {
      // Play sound effect at volume
      [[maybe_unused]] const float volume = audio.sfx_volume * audio.master_volume;
    }

    // Process audio for combat
    const auto combat_reader = ctx.ReadEvents<CombatEvent>();
    for (const auto& event : combat_reader) {
      // Play impact sound
      [[maybe_unused]] const float volume = audio.sfx_volume * audio.master_volume;
    }
  }
};

// ============================================================================
// SubApps
// ============================================================================

struct RenderSubApp {
  static constexpr std::string_view GetName() noexcept { return "RenderSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return true; }
  static constexpr size_t GetMaxOverlappingUpdates() noexcept { return 2; }
};

struct PhysicsSubApp {
  static constexpr std::string_view GetName() noexcept { return "PhysicsSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return false; }
};

struct AudioSubApp {
  static constexpr std::string_view GetName() noexcept { return "AudioSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return true; }
};

// ============================================================================
// Custom Schedules
// ============================================================================

struct LateUpdate {
  static constexpr std::string_view GetName() noexcept { return "LateUpdate"; }
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto After() noexcept { return std::array{ScheduleIdOf<Update>()}; }
  static constexpr auto Before() noexcept { return std::array<ScheduleId, 0>{}; }
};

struct EarlyUpdate {
  static constexpr std::string_view GetName() noexcept { return "EarlyUpdate"; }
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto After() noexcept { return std::array<ScheduleId, 0>{}; }
  static constexpr auto Before() noexcept { return std::array{ScheduleIdOf<Update>()}; }
};

inline constexpr LateUpdate kLateUpdate{};
inline constexpr EarlyUpdate kEarlyUpdate{};

// ============================================================================
// Modules
// ============================================================================

struct CoreModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "CoreModule"; }

  void Build(App& app) override {
    app.InsertResource(GameTime{})
        .InsertResource(GameStats{})
        .InsertResource(GameConfig{})
        .AddEvent<EntitySpawnedEvent>()
        .AddEvent<EntityDestroyedEvent>()
        .AddEvent<CombatEvent>()
        .AddEvent<ProjectileFiredEvent>()
        .AddSystem<TimeUpdateSystem>(kMain)
        .AddSystem<EventLoggerSystem>(kPostUpdate);
  }

  void Destroy(App& /*app*/) override {}
};

struct PhysicsModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "PhysicsModule"; }

  void Build(App& app) override {
    app.InsertResource(PhysicsSettings{}).ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>().Before<GameplaySet>();

    app.AddSystemsBuilder<AccelerationSystem, GravitySystem>(kUpdate).InSet<PhysicsSet>().Sequence();
    app.AddSystemsBuilder<MovementSystem, FrictionSystem, VelocityClampSystem>(kUpdate)
        .InSet<PhysicsSet>()
        .After<AccelerationSystem, GravitySystem>();
  }

  void Destroy(App& /*app*/) override {}
};

struct GameplayModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "GameplayModule"; }

  void Build(App& app) override {
    app.ConfigureSet<GameplaySet>(kUpdate).After<PhysicsSet>().Before<CombatSet>();

    app.AddSystemsBuilder<EnemySpawnerSystem, ProjectileSpawnerSystem, LifetimeSystem>(kUpdate)
        .InSet<GameplaySet>()
        .Sequence();
  }

  void Destroy(App& /*app*/) override {}
};

struct CombatModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "CombatModule"; }

  void Build(App& app) override {
    app.ConfigureSet<CombatSet>(kUpdate).After<GameplaySet>().Before<CleanupSet>();

    app.AddSystemsBuilder<CollisionDetectionSystem, DamageApplicationSystem, DeathDetectionSystem>(kUpdate)
        .InSet<CombatSet>()
        .Sequence();
  }

  void Destroy(App& /*app*/) override {}
};

struct CleanupModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "CleanupModule"; }

  void Build(App& app) override {
    app.ConfigureSet<CleanupSet>(kPostUpdate);

    app.AddSystemBuilder<EntityCleanupSystem>(kPostUpdate).InSet<CleanupSet>();
    app.AddSystemBuilder<ManualEventClearSystem>(kPostUpdate).After<EntityCleanupSystem>();
  }

  void Destroy(App& /*app*/) override {}
};

struct RenderModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "RenderModule"; }

  void Build(App& app) override {
    app.InsertResource(RenderData{})
        .AddSubApp<RenderSubApp>()
        .InsertResource(RenderData{})
        .AddSystemsBuilder<RenderDataExtractionSystem, RenderSystem>(kUpdate)
        .Sequence();
  }

  void Destroy(App& /*app*/) override {}
};

struct AudioModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "AudioModule"; }

  void Build(App& app) override {
    app.InsertResource(AudioSettings{})
        .AddSubApp<AudioSubApp>()
        .InsertResource(AudioSettings{})
        .AddSystem<AudioSystem>(kUpdate);
  }

  void Destroy(App& /*app*/) override {}
};

// ============================================================================
// Setup System for Initial Entities
// ============================================================================

struct InitialSetupSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "InitialSetupSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& ctx) override {
    // Create player
    auto player_cmd = ctx.EntityCommands(ctx.ReserveEntity());
    player_cmd.AddComponents(Player{}, Position{0.0F, 0.0F, 0.0F}, Velocity{0.0F, 0.0F, 0.0F}, Health{100, 100},
                             Name{"Player"});
    ctx.EmitEvent(EntitySpawnedEvent{player_cmd.GetEntity(), "Player", 0.0F, 0.0F, 0.0F});

    // Create initial enemies
    for (int i = 0; i < 3; ++i) {
      const float x = static_cast<float>(i * 15);
      auto enemy_cmd = ctx.EntityCommands(ctx.ReserveEntity());
      enemy_cmd.AddComponents(Enemy{}, Position{x, 0.0F, 0.0F}, Velocity{0.0F, 0.0F, 0.0F}, Health{50, 50},
                              Name{std::format("Enemy_{}", i)});
      ctx.EmitEvent(EntitySpawnedEvent{enemy_cmd.GetEntity(), "Enemy", x, 0.0F, 0.0F});
    }
  }
};

}  // namespace

// ============================================================================
// Integration Tests
// ============================================================================

TEST_SUITE("app::AppIntegration") {
  TEST_CASE("App: Game Simulation Complete Feature Showcase") {
    HELIOS_INFO("=== Starting Complete Game Simulation Test ===");
    helios::Timer timer;

    App app;

    // Add all modules
    app.AddModules<CoreModule, PhysicsModule, GameplayModule, CombatModule, CleanupModule, RenderModule, AudioModule>();

    // Add initial setup system
    app.AddSystem<InitialSetupSystem>(kStartup);

    // Verify modules are registered (before Build is called during Run)
    CHECK_EQ(app.ModuleCount(), 7);
    CHECK(app.ContainsModule<CoreModule>());
    CHECK(app.ContainsModule<PhysicsModule>());
    CHECK(app.ContainsModule<GameplayModule>());
    CHECK(app.ContainsModule<CombatModule>());
    CHECK(app.ContainsModule<CleanupModule>());
    CHECK(app.ContainsModule<RenderModule>());
    CHECK(app.ContainsModule<AudioModule>());

    // Capture state during run (before cleanup clears everything)
    struct CapturedState {
      bool has_game_time = false;
      bool has_game_stats = false;
      bool has_game_config = false;
      bool has_physics_settings = false;
      bool has_audio_settings = false;
      bool has_time_update_system = false;
      bool has_acceleration_system = false;
      bool has_movement_system = false;
      bool has_enemy_spawner_system = false;
      bool has_cleanup_system = false;
      bool has_render_subapp = false;
      bool has_audio_subapp = false;
      size_t entity_count = 0;
      int entities_spawned = 0;
      int frames_rendered = 0;
      int frame_count = 0;
    };
    CapturedState captured;

    const int frames = 100;
    app.SetRunner([frames, &captured](App& running_app) {
      // Run the simulation
      AppExitCode result = FixedFrameRunner(running_app, frames);

      // Capture state before cleanup
      captured.has_game_time = running_app.HasResource<GameTime>();
      captured.has_game_stats = running_app.HasResource<GameStats>();
      captured.has_game_config = running_app.HasResource<GameConfig>();
      captured.has_physics_settings = running_app.HasResource<PhysicsSettings>();
      captured.has_audio_settings = running_app.HasResource<AudioSettings>();
      captured.has_time_update_system = running_app.ContainsSystem<TimeUpdateSystem>(kMain);
      captured.has_acceleration_system = running_app.ContainsSystem<AccelerationSystem>(kUpdate);
      captured.has_movement_system = running_app.ContainsSystem<MovementSystem>(kUpdate);
      captured.has_enemy_spawner_system = running_app.ContainsSystem<EnemySpawnerSystem>(kUpdate);
      captured.has_cleanup_system = running_app.ContainsSystem<EntityCleanupSystem>(kPostUpdate);
      captured.has_render_subapp = running_app.ContainsSubApp<RenderSubApp>();
      captured.has_audio_subapp = running_app.ContainsSubApp<AudioSubApp>();

      const auto& world = std::as_const(running_app).GetMainWorld();
      captured.entity_count = world.EntityCount();

      const auto& stats = world.ReadResource<GameStats>();
      captured.entities_spawned = stats.entities_spawned.load(std::memory_order_relaxed);
      captured.frames_rendered = stats.frames_rendered.load(std::memory_order_relaxed);

      const auto& time = world.ReadResource<GameTime>();
      captured.frame_count = time.frame_count;

      return result;
    });
    app.Run();

    // Verify resources existed during run
    CHECK(captured.has_game_time);
    CHECK(captured.has_game_stats);
    CHECK(captured.has_game_config);
    CHECK(captured.has_physics_settings);
    CHECK(captured.has_audio_settings);

    // Verify systems existed during run
    CHECK(captured.has_time_update_system);
    CHECK(captured.has_acceleration_system);
    CHECK(captured.has_movement_system);
    CHECK(captured.has_enemy_spawner_system);
    CHECK(captured.has_cleanup_system);

    // Verify sub-apps existed during run
    CHECK(captured.has_render_subapp);
    CHECK(captured.has_audio_subapp);

    // Verify game state during run
    CHECK_GT(captured.entity_count, 0);
    CHECK_GT(captured.entities_spawned, 0);
    CHECK_GT(captured.frames_rendered, 0);
    CHECK_EQ(captured.frame_count, frames);

    const double test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("=== Complete simulation test finished in {:.3f}ms ===", test_time);
  }

  TEST_CASE("App: System Sets Ordering") {
    HELIOS_INFO("Testing system sets ordering");

    App app;

    struct ExecutionOrder {
      std::vector<std::string> order;
    };

    app.InsertResource(ExecutionOrder{});

    // Create systems that track execution order
    struct InputSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "InputSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ExecutionOrder>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ExecutionOrder>().order.emplace_back("Input"); }
    };

    struct PhysicsSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "PhysicsSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ExecutionOrder>(); }
      void Update(SystemContext& ctx) override { ctx.WriteResource<ExecutionOrder>().order.emplace_back("Physics"); }
    };

    struct GameplaySystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "GameplaySystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ExecutionOrder>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ExecutionOrder>().order.emplace_back("Gameplay"); }
    };

    struct RenderSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "RenderSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ExecutionOrder>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ExecutionOrder>().order.emplace_back("Render"); }
    };

    // Configure system sets with ordering
    app.ConfigureSet<InputSet>(kUpdate);
    app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>();
    app.ConfigureSet<GameplaySet>(kUpdate).After<PhysicsSet>();
    app.ConfigureSet<RenderSet>(kUpdate).After<GameplaySet>();

    // Add systems to sets
    app.AddSystemBuilder<InputSystem>(kUpdate).InSet<InputSet>();
    app.AddSystemBuilder<PhysicsSystem>(kUpdate).InSet<PhysicsSet>();
    app.AddSystemBuilder<GameplaySystem>(kUpdate).InSet<GameplaySet>();
    app.AddSystemBuilder<RenderSystem>(kUpdate).InSet<RenderSet>();

    // Capture execution order during run
    std::vector<std::string> captured_order;
    app.SetRunner([&captured_order](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& order = world.ReadResource<ExecutionOrder>();
      captured_order = order.order;
      return result;
    });
    app.Run();

    // Verify execution order
    REQUIRE_EQ(captured_order.size(), 4);
    CHECK_EQ(captured_order[0], "Input");
    CHECK_EQ(captured_order[1], "Physics");
    CHECK_EQ(captured_order[2], "Gameplay");
    CHECK_EQ(captured_order[3], "Render");
  }

  TEST_CASE("App: Custom Schedules") {
    HELIOS_INFO("Testing custom schedules");

    App app;

    struct ScheduleTracker {
      std::vector<std::string> schedules;
    };

    app.InsertResource(ScheduleTracker{});

    struct EarlySystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "EarlySystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("Early"); }
    };

    struct UpdateSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "UpdateSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override {
        ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("Update");
      }
    };

    struct LateSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "LateSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("Late"); }
    };

    app.AddSystem<EarlySystem>(kEarlyUpdate);
    app.AddSystem<UpdateSystem>(kUpdate);
    app.AddSystem<LateSystem>(kLateUpdate);

    // Capture schedule order during run
    std::vector<std::string> captured_schedules;
    app.SetRunner([&captured_schedules](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& tracker = world.ReadResource<ScheduleTracker>();
      captured_schedules = tracker.schedules;
      return result;
    });
    app.Run();

    // Verify schedule order
    REQUIRE_EQ(captured_schedules.size(), 3);
    CHECK_EQ(captured_schedules[0], "Early");
    CHECK_EQ(captured_schedules[1], "Update");
    CHECK_EQ(captured_schedules[2], "Late");
  }

  TEST_CASE("App: SubApp Overlapping Updates") {
    HELIOS_INFO("Testing sub-app overlapping updates");

    App app;

    struct SubAppCounter {
      std::atomic<int> render_count{0};
      std::atomic<int> audio_count{0};

      SubAppCounter() = default;
      SubAppCounter(const SubAppCounter& other) noexcept
          : render_count(other.render_count.load(std::memory_order_relaxed)),
            audio_count(other.audio_count.load(std::memory_order_relaxed)) {}
      SubAppCounter(SubAppCounter&& other) noexcept
          : render_count(other.render_count.load(std::memory_order_relaxed)),
            audio_count(other.audio_count.load(std::memory_order_relaxed)) {}
      SubAppCounter& operator=(const SubAppCounter&) = delete;
      SubAppCounter& operator=(SubAppCounter&&) = delete;
    };

    // Insert resource in main sub-app
    app.InsertResource(SubAppCounter{});

    struct RenderCountSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "RenderCountSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<SubAppCounter>(); }

      void Update(SystemContext& ctx) override {
        ctx.WriteResource<SubAppCounter>().render_count.fetch_add(1, std::memory_order_relaxed);
      }
    };

    struct AudioCountSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "AudioCountSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<SubAppCounter>(); }

      void Update(SystemContext& ctx) override {
        ctx.WriteResource<SubAppCounter>().audio_count.fetch_add(1, std::memory_order_relaxed);
      }
    };

    // Add sub-apps (for overlapping update support)
    app.AddSubApp<RenderSubApp>();
    app.AddSubApp<AudioSubApp>();

    // Add systems to main sub-app (they share the SubAppCounter resource)
    app.AddSystem<RenderCountSystem>(kUpdate);
    app.AddSystem<AudioCountSystem>(kUpdate);

    // Capture counter values during run
    int captured_render_count = 0;
    int captured_audio_count = 0;
    app.SetRunner([&captured_render_count, &captured_audio_count](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 10);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<SubAppCounter>();
      captured_render_count = counter.render_count.load(std::memory_order_relaxed);
      captured_audio_count = counter.audio_count.load(std::memory_order_relaxed);
      return result;
    });
    app.Run();

    // Both counters should be incremented 10 times (once per frame)
    CHECK_EQ(captured_render_count, 10);
    CHECK_EQ(captured_audio_count, 10);
  }

  TEST_CASE("App: Event Lifecycle") {
    HELIOS_INFO("Testing event lifecycle and manual clearing");

    App app;

    struct EventCounter {
      int spawn_events = 0;
      int total_seen = 0;
    };

    app.InsertResource(EventCounter{}).AddEvent<EntitySpawnedEvent>();

    struct EventEmitterSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "EventEmitterSystem"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& ctx) override {
        ctx.EmitEvent(EntitySpawnedEvent{Entity{}, "Test", 0.0F, 0.0F, 0.0F});
      }
    };

    struct EventCounterSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "EventCounterSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<EventCounter>(); }

      void Update(SystemContext& ctx) override {
        auto& counter = ctx.WriteResource<EventCounter>();
        auto reader = ctx.ReadEvents<EntitySpawnedEvent>();

        const auto count = static_cast<int>(reader.Count());
        counter.spawn_events += count;
        counter.total_seen += count;
      }
    };

    struct EventClearSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "EventClearSystem"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& ctx) override { ctx.Commands().ClearEvents<EntitySpawnedEvent>(); }
    };

    // Use kMain stage for immediate event visibility between ordered systems
    // In async stages (like kUpdate), events are only merged after all systems complete,
    // so EventCounterSystem wouldn't see events from EventEmitterSystem in the same frame.
    app.AddSystem<EventEmitterSystem>(kMain);
    app.AddSystemBuilder<EventCounterSystem>(kMain).After<EventEmitterSystem>();
    app.AddSystem<EventClearSystem>(kPostUpdate);

    // Capture counter value during run
    int captured_total_seen = 0;
    app.SetRunner([&captured_total_seen](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 5);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<EventCounter>();
      captured_total_seen = counter.total_seen;
      return result;
    });
    app.Run();

    // In kMain stage, events are merged after each system, so EventCounterSystem
    // sees events from EventEmitterSystem immediately in the same frame.
    // After 5 frames: each frame emits 1 event that is immediately visible (5 events seen)
    CHECK_EQ(captured_total_seen, 5);
  }

  TEST_CASE("App: Component Operations") {
    HELIOS_INFO("Testing comprehensive component operations");

    App app;

    struct ComponentTestSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "ComponentTestSystem"; }
      static constexpr auto GetAccessPolicy() noexcept {
        return AccessPolicy().Query<Position&, const Velocity&, Health&>();
      }

      void Update(SystemContext& ctx) override {
        // Create entity with components
        auto entity = ctx.ReserveEntity();
        auto cmd = ctx.EntityCommands(entity);
        cmd.AddComponents(Position{1.0F, 2.0F, 3.0F}, Velocity{0.5F, 0.5F, 0.5F}, Health{100, 100});

        // Query components
        ctx.Query().Get<Position&, const Velocity&>().ForEach(
            [](Position& position, const Velocity& velocity) { position.x += velocity.dx; });

        // Remove component
        cmd.RemoveComponent<Velocity>();

        // Add tag component
        cmd.AddComponent(Player{});
      }
    };

    app.AddSystem<ComponentTestSystem>(kStartup);

    // Capture entity count during run
    size_t captured_entity_count = 0;
    app.SetRunner([&captured_entity_count](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_entity_count = world.EntityCount();
      return result;
    });
    app.Run();

    CHECK_GT(captured_entity_count, 0);
  }

  TEST_CASE("App: Thread-Safe Resources") {
    HELIOS_INFO("Testing thread-safe resource access");

    App app;

    // Use the file-scope ThreadSafeCounter and IncrementSystem
    app.InsertResource(ThreadSafeCounter{});

    // Add the increment system (defined at file scope)
    app.AddSystem<IncrementSystem>(kUpdate);

    // Capture counter value during run
    int captured_value = 0;
    app.SetRunner([&captured_value](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 10);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<ThreadSafeCounter>();
      captured_value = counter.value.load(std::memory_order_relaxed);
      return result;
    });
    app.Run();

    CHECK_EQ(captured_value, 10);
  }

  TEST_CASE("App: Fluent API Chaining") {
    HELIOS_INFO("Testing fluent API builder pattern");

    App app;

    // Chain multiple resource insertions
    app.InsertResource(GameTime{})
        .InsertResource(GameStats{})
        .InsertResource(PhysicsSettings{})
        .InsertResource(AudioSettings{});

    // Chain multiple events
    app.AddEvent<EntitySpawnedEvent>().AddEvent<CombatEvent>().AddEvent<ProjectileFiredEvent>();

    // Chain system additions with configuration
    struct System1 final : public System {
      static constexpr std::string_view GetName() noexcept { return "System1"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& /*ctx*/) override {}
    };

    struct System2 final : public System {
      static constexpr std::string_view GetName() noexcept { return "System2"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& /*ctx*/) override {}
    };

    struct System3 final : public System {
      static constexpr std::string_view GetName() noexcept { return "System3"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& /*ctx*/) override {}
    };

    app.AddSystemsBuilder<System1, System2, System3>(kUpdate).Sequence();

    // Verify everything was added
    CHECK(app.HasResource<GameTime>());
    CHECK(app.HasResource<GameStats>());
    CHECK(app.HasResource<AudioSettings>());
    CHECK(app.ContainsSystem<System1>(kUpdate));
    CHECK(app.ContainsSystem<System2>(kUpdate));
    CHECK(app.ContainsSystem<System3>(kUpdate));
  }

  TEST_CASE("App: Module Lifecycle") {
    HELIOS_INFO("Testing module build and destroy lifecycle");

    struct LifecycleTracker {
      int build_count = 0;
      int destroy_count = 0;
    };

    static LifecycleTracker tracker;
    tracker = {};

    struct LifecycleModule final : public Module {
      static constexpr std::string_view GetName() noexcept { return "LifecycleModule"; }

      void Build(App& app) override {
        ++tracker.build_count;
        app.InsertResource(GameTime{});
      }

      void Destroy(App& /*app*/) override { ++tracker.destroy_count; }
    };

    App app;

    app.AddModule<LifecycleModule>();
    CHECK(app.ContainsModule<LifecycleModule>());

    CHECK_EQ(tracker.build_count, 0);
    app.SetRunner([](App& running_app) {
      CHECK_EQ(tracker.build_count, 1);
      return FixedFrameRunner(running_app, 1);
    });
    app.Run();

    CHECK_EQ(tracker.destroy_count, 1);
  }

  TEST_CASE("App: Complex Entity Relationships") {
    HELIOS_INFO("Testing complex entity relationships and hierarchy");

    App app;

    struct Parent {
      Entity child;
    };

    struct Child {
      Entity parent;
    };

    struct HierarchySystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "HierarchySystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().Query<const Parent&, const Child&>(); }

      void Update(SystemContext& ctx) override {
        // Create parent
        auto parent_entity = ctx.ReserveEntity();
        auto parent_cmd = ctx.EntityCommands(parent_entity);
        parent_cmd.AddComponents(Position{0.0F, 0.0F, 0.0F}, Parent{});

        // Create child
        auto child_entity = ctx.ReserveEntity();
        auto child_cmd = ctx.EntityCommands(child_entity);
        child_cmd.AddComponents(Position{1.0F, 1.0F, 1.0F}, Child{parent_entity});

        auto parent_query = ctx.Query().Get<Parent&>();
        const auto parent_tuple =
            parent_query.WithEntity()
                .FindFirst([parent_entity](Entity entity, const Parent& /*parent*/) { return entity == parent_entity; })
                .value();
        std::get<1>(parent_tuple).child = child_entity;

        auto child_query = ctx.Query().Get<const Child&>();
        const auto child_tuple =
            child_query.WithEntity()
                .FindFirst([child_entity](Entity entity, const Child& /*child*/) { return entity == child_entity; })
                .value();

        // Verify relationship
        CHECK_EQ(std::get<1>(child_tuple).parent, parent_entity);
      }
    };

    app.AddSystem<HierarchySystem>(kStartup);

    // Capture entity count during run
    size_t captured_entity_count = 0;
    app.SetRunner([&captured_entity_count](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      captured_entity_count = std::as_const(running_app).GetMainWorld().EntityCount();
      return result;
    });
    app.Run();

    CHECK_GE(captured_entity_count, 2);
  }

  TEST_CASE("App: Performance With Many Entities") {
    HELIOS_INFO("Testing performance with many entities");
    helios::Timer timer;

    App app;
    app.InsertResource(GameTime{});

    struct SpawnManySystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "SpawnManySystem"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& ctx) override {
        // Spawn 1000 entities
        for (int i = 0; i < 1000; ++i) {
          auto entity = ctx.ReserveEntity();
          auto cmd = ctx.EntityCommands(entity);
          cmd.AddComponents(Position{static_cast<float>(i), 0.0F, 0.0F},
                            Velocity{static_cast<float>(i % 10), 0.0F, 0.0F});
        }
      }
    };

    struct ProcessManySystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "ProcessManySystem"; }
      static constexpr auto GetAccessPolicy() noexcept {
        return AccessPolicy().Query<const Velocity&, Position&>().ReadResources<GameTime>();
      }

      void Update(SystemContext& ctx) override {
        const auto& time = ctx.ReadResource<GameTime>();
        ctx.Query().Get<const Velocity&, Position&>().ForEach([&time](const Velocity& vel, Position& pos) {
          pos.x += vel.dx * time.delta_time;
          pos.y += vel.dy * time.delta_time;
          pos.z += vel.dz * time.delta_time;
        });
      }
    };

    app.AddSystem<SpawnManySystem>(kStartup);
    app.AddSystem<ProcessManySystem>(kUpdate);

    // Capture entity count during run
    size_t captured_entity_count = 0;
    app.SetRunner([&captured_entity_count](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 10);
      captured_entity_count = std::as_const(running_app).GetMainWorld().EntityCount();
      return result;
    });
    app.Run();

    CHECK_EQ(captured_entity_count, 1000);

    const double elapsed = timer.ElapsedMilliSec();
    HELIOS_INFO("Processed 1000 entities for 10 frames in {:.3f}ms", elapsed);
  }

  TEST_CASE("App: Frame Allocator Integration") {
    HELIOS_INFO("=== Starting Frame Allocator Integration Test ===");
    helios::Timer timer;

    App app;

    // System that uses frame allocator for temporary collections
    struct FrameAllocatorSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "FrameAllocatorSystem"; }
      static constexpr auto GetAccessPolicy() noexcept {
        return AccessPolicy().Query<const Position&, const Velocity&>();
      }

      void Update(SystemContext& ctx) override {
        // Use frame allocator for temporary query results
        auto query = ctx.Query().Get<const Position&, const Velocity&>();

        // CollectWith using frame allocator - memory is automatically reclaimed at frame end
        using ValueType = std::tuple<const Position&, const Velocity&>;
        auto alloc = ctx.MakeFrameAllocator<ValueType>();
        auto results = query.CollectWith(alloc);

        // Process the collected results
        float total_speed = 0.0F;
        for (const auto& [pos, vel] : results) {
          total_speed += std::abs(vel.dx) + std::abs(vel.dy) + std::abs(vel.dz);
        }

        // Also demonstrate using frame allocator directly for custom containers
        auto int_alloc = ctx.MakeFrameAllocator<int>();
        std::vector<int, decltype(int_alloc)> temp_indices{int_alloc};
        temp_indices.reserve(results.size());

        for (size_t i = 0; i < results.size(); ++i) {
          const auto& [pos, vel] = results[i];
          if (pos.x > 0.0F) {
            temp_indices.push_back(static_cast<int>(i));
          }
        }

        // Verify we processed something meaningful
        HELIOS_ASSERT(results.size() > 0 || ctx.EntityCount() == 0, "Expected results if entities exist");
      }
    };

    // Setup system to create entities
    struct SetupSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "SetupSystem"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& ctx) override {
        for (int i = 0; i < 100; ++i) {
          auto entity = ctx.ReserveEntity();
          auto cmd = ctx.EntityCommands(entity);
          cmd.AddComponents(Position{static_cast<float>(i), static_cast<float>(i * 2), 0.0F},
                            Velocity{1.0F, 2.0F, 0.0F});
        }
      }
    };

    app.AddSystem<SetupSystem>(kStartup);
    app.AddSystem<FrameAllocatorSystem>(kUpdate);

    // Run for multiple frames to ensure frame allocator is being reset properly
    size_t final_entity_count = 0;
    app.SetRunner([&final_entity_count](App& running_app) {
      // Run for 5 frames - frame allocator should be reset between frames
      AppExitCode result = FixedFrameRunner(running_app, 5);
      final_entity_count = std::as_const(running_app).GetMainWorld().EntityCount();
      return result;
    });

    AppExitCode result = app.Run();

    CHECK_EQ(result, AppExitCode::Success);
    CHECK_EQ(final_entity_count, 100);

    const double elapsed = timer.ElapsedMilliSec();
    HELIOS_INFO("Frame allocator test completed in {:.3f}ms", elapsed);
  }

  TEST_CASE("App: Same System In Multiple Schedules") {
    HELIOS_INFO("Testing same system in multiple schedules");
    helios::Timer timer;

    App app;

    // Resource to track execution counts per schedule
    struct CleanupCounter {
      std::atomic<int> pre_update_count{0};
      std::atomic<int> post_update_count{0};
      std::atomic<int> total_count{0};

      CleanupCounter() = default;
      CleanupCounter(const CleanupCounter& other) noexcept
          : pre_update_count(other.pre_update_count.load(std::memory_order_relaxed)),
            post_update_count(other.post_update_count.load(std::memory_order_relaxed)),
            total_count(other.total_count.load(std::memory_order_relaxed)) {}

      CleanupCounter(CleanupCounter&& other) noexcept
          : pre_update_count(other.pre_update_count.load(std::memory_order_relaxed)),
            post_update_count(other.post_update_count.load(std::memory_order_relaxed)),
            total_count(other.total_count.load(std::memory_order_relaxed)) {}

      CleanupCounter& operator=(const CleanupCounter&) = delete;
      CleanupCounter& operator=(CleanupCounter&&) = delete;
    };

    app.InsertResource(CleanupCounter{});

    // A cleanup system that can run in multiple schedules
    // This simulates a real-world pattern: cleanup at both start and end of update
    struct CleanupSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "CleanupSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<CleanupCounter>(); }

      void Update(SystemContext& ctx) override {
        auto& counter = ctx.WriteResource<CleanupCounter>();
        counter.total_count.fetch_add(1, std::memory_order_relaxed);
      }
    };

    // A system that runs only in Update schedule
    struct GameplaySystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "GameplaySystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<CleanupCounter>(); }

      void Update(SystemContext& /*ctx*/) override {
        // Just runs in Update schedule
      }
    };

    // Add the same CleanupSystem to PreUpdate and PostUpdate schedules
    app.AddSystem<CleanupSystem>(kPreUpdate);
    app.AddSystem<GameplaySystem>(kUpdate);
    app.AddSystem<CleanupSystem>(kPostUpdate);

    // Verify the systems are correctly registered
    CHECK_EQ(app.SystemCount(), 3);
    CHECK_EQ(app.SystemCount(kPreUpdate), 1);
    CHECK_EQ(app.SystemCount(kUpdate), 1);
    CHECK_EQ(app.SystemCount(kPostUpdate), 1);

    CHECK(app.ContainsSystem<CleanupSystem>(kPreUpdate));
    CHECK_FALSE(app.ContainsSystem<CleanupSystem>(kUpdate));
    CHECK(app.ContainsSystem<CleanupSystem>(kPostUpdate));

    CHECK_FALSE(app.ContainsSystem<GameplaySystem>(kPreUpdate));
    CHECK(app.ContainsSystem<GameplaySystem>(kUpdate));
    CHECK_FALSE(app.ContainsSystem<GameplaySystem>(kPostUpdate));

    // Run and verify execution counts
    int captured_total = 0;
    const int frames = 10;

    app.SetRunner([frames, &captured_total](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, frames);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<CleanupCounter>();
      captured_total = counter.total_count.load(std::memory_order_relaxed);
      return result;
    });

    AppExitCode result = app.Run();

    CHECK_EQ(result, AppExitCode::Success);
    // CleanupSystem runs twice per frame (PreUpdate + PostUpdate)
    CHECK_EQ(captured_total, frames * 2);

    const double elapsed = timer.ElapsedMilliSec();
    HELIOS_INFO("Same system in multiple schedules test completed in {:.3f}ms", elapsed);
  }

  TEST_CASE("App: Multiple Instances Of Same System Execute Independently") {
    HELIOS_INFO("Testing independent execution of same system in multiple schedules");

    App app;

    // Track which schedules executed
    struct ScheduleTracker {
      std::vector<std::string> execution_order;
    };

    app.InsertResource(ScheduleTracker{});

    struct TrackerSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "TrackerSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override {
        // This system runs multiple times per frame (in different schedules)
        // We can't know which schedule we're in from inside the system,
        // but we can track that we executed
        ctx.WriteResource<ScheduleTracker>().execution_order.emplace_back("TrackerExecuted");
      }
    };

    // Add TrackerSystem to three different schedules
    app.AddSystem<TrackerSystem>(kPreUpdate);
    app.AddSystem<TrackerSystem>(kUpdate);
    app.AddSystem<TrackerSystem>(kPostUpdate);

    CHECK_EQ(app.SystemCount(), 3);
    CHECK(app.ContainsSystem<TrackerSystem>(kPreUpdate));
    CHECK(app.ContainsSystem<TrackerSystem>(kUpdate));
    CHECK(app.ContainsSystem<TrackerSystem>(kPostUpdate));

    // Global check should also pass
    CHECK(app.ContainsSystem<TrackerSystem>());

    std::vector<std::string> captured_order;
    app.SetRunner([&captured_order](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& tracker = world.ReadResource<ScheduleTracker>();
      captured_order = tracker.execution_order;
      return result;
    });

    app.Run();

    // TrackerSystem should have executed 3 times (once per schedule)
    CHECK_EQ(captured_order.size(), 3);
  }
}  // TEST_SUITE
