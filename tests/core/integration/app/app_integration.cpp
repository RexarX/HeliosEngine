#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/app/runners.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/app/time.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/timer.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <concepts>
#include <format>
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
    app.TickTime();
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
    app.TickTime();
    app.Update();
    ++frame;
  }
  return frame < max_frames ? AppExitCode::Success : AppExitCode::Failure;
}

// ============================================================================
// Basic Components
// ============================================================================

struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr bool operator==(const Position&) const = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity&) const = default;
};

struct Health {
  int max_health = 100;
  int current_health = 100;

  constexpr void TakeDamage(int damage) { current_health = std::max(0, current_health - damage); }
  constexpr void Heal(int amount) { current_health = std::min(max_health, current_health + amount); }

  constexpr bool IsDead() const noexcept { return current_health <= 0; }
  constexpr bool operator==(const Health&) const = default;
};

struct Name {
  std::string value;

  constexpr bool operator==(const Name&) const = default;
};

// Tag components
struct Player {};
struct Enemy {};

// ============================================================================
// Basic Resources
// ============================================================================

struct GameTime {
  float delta_time = 0.0F;
  float total_time = 0.0F;
  int frame_count = 0;

  static constexpr std::string_view GetName() noexcept { return "GameTime"; }
};

struct GameStats {
  std::atomic<int> entities_spawned{0};
  std::atomic<int> frames_rendered{0};

  GameStats() = default;
  GameStats(const GameStats& other) noexcept
      : entities_spawned(other.entities_spawned.load(std::memory_order_relaxed)),
        frames_rendered(other.frames_rendered.load(std::memory_order_relaxed)) {}

  GameStats(GameStats&& other) noexcept
      : entities_spawned(other.entities_spawned.load(std::memory_order_relaxed)),
        frames_rendered(other.frames_rendered.load(std::memory_order_relaxed)) {}

  GameStats& operator=(const GameStats&) = delete;
  GameStats& operator=(GameStats&&) = delete;

  static constexpr std::string_view GetName() noexcept { return "GameStats"; }
};

struct ThreadSafeCounter {
  std::atomic<int> value{0};

  static constexpr std::string_view GetName() noexcept { return "ThreadSafeCounter"; }
  static constexpr bool ThreadSafe() noexcept { return true; }

  constexpr ThreadSafeCounter() noexcept = default;
  ThreadSafeCounter(const ThreadSafeCounter& other) noexcept : value(other.value.load(std::memory_order_relaxed)) {}
  ThreadSafeCounter(ThreadSafeCounter&& other) noexcept : value(other.value.load(std::memory_order_relaxed)) {}
  ~ThreadSafeCounter() noexcept = default;

  ThreadSafeCounter& operator=(const ThreadSafeCounter& other) noexcept {
    if (this != &other) {
      value.store(other.value.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
    return *this;
  }

  ThreadSafeCounter& operator=(ThreadSafeCounter&& other) noexcept {
    if (this != &other) {
      value.store(other.value.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
    return *this;
  }
};

// ============================================================================
// Basic Events
// ============================================================================

struct CustomEntitySpawnedEvent {
  Entity entity;
  std::array<char, 32> entity_type = {};
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr CustomEntitySpawnedEvent() noexcept = default;
  constexpr CustomEntitySpawnedEvent(Entity entt, std::string_view type, float px, float py, float pz) noexcept
      : entity(entt), x(px), y(py), z(pz) {
    const size_t len = std::min(type.size(), entity_type.size() - 1);
    std::ranges::copy_n(type.begin(), len, entity_type.begin());
  }

  static constexpr std::string_view GetName() noexcept { return "CustomEntitySpawnedEvent"; }
};

// ============================================================================
// System Sets
// ============================================================================

struct InputSet {};
struct PhysicsSet {};
struct GameplaySet {};
struct RenderSet {};

// ============================================================================
// Basic Systems
// ============================================================================

struct IncrementSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "IncrementSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ThreadSafeCounter>(); }

  void Update(SystemContext& ctx) override {
    auto& counter = ctx.WriteResource<ThreadSafeCounter>();
    counter.value.fetch_add(1, std::memory_order_relaxed);
  }
};

struct TimeUpdateSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "TimeUpdateSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<GameTime>(); }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    time.delta_time = 0.016F;  // Simulated 60fps
    time.total_time += time.delta_time;
    ++time.frame_count;
  }
};

struct MovementSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "MovementSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Position&, const Velocity&>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    const float dt = time.delta_time;

    ctx.Query().Get<Position&, const Velocity&>().ForEach([dt](Position& pos, const Velocity& vel) {
      pos.x += vel.dx * dt;
      pos.y += vel.dy * dt;
      pos.z += vel.dz * dt;
    });
  }
};

struct SpawnSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "SpawnSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& ctx) override {
    auto player_cmd = ctx.EntityCommands(ctx.ReserveEntity());
    player_cmd.AddComponents(Player{}, Position{0.0F, 0.0F, 0.0F}, Velocity{1.0F, 0.0F, 0.0F}, Health{100, 100},
                             Name{"Player"});
    ctx.EmitEvent(CustomEntitySpawnedEvent{player_cmd.GetEntity(), "Player", 0.0F, 0.0F, 0.0F});
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
// Basic Modules
// ============================================================================

struct CoreModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "CoreModule"; }

  void Build(App& app) override {
    app.InsertResource(GameTime{}).InsertResource(GameStats{}).AddEvent<CustomEntitySpawnedEvent>();

    app.AddSystem<TimeUpdateSystem>(kMain);
  }

  void Destroy(App& /*app*/) override {}
};

struct GameplayModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "GameplayModule"; }

  void Build(App& app) override { app.AddSystem<MovementSystem>(kUpdate); }

  void Destroy(App& /*app*/) override {}
};

}  // namespace

// ============================================================================
// Integration Tests
// ============================================================================

TEST_SUITE("app::AppIntegration") {
  TEST_CASE("App: Basic Initialization and Run") {
    HELIOS_INFO("Testing basic app initialization and run");
    helios::Timer timer;

    App app;

    app.InsertResource(ThreadSafeCounter{});
    app.AddSystem<IncrementSystem>(kUpdate);

    int captured_value = 0;
    const int frames = 10;

    app.SetRunner([frames, &captured_value](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, frames);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<ThreadSafeCounter>();
      captured_value = counter.value.load(std::memory_order_relaxed);
      return result;
    });

    AppExitCode result = app.Run();

    CHECK_EQ(result, AppExitCode::Success);
    CHECK_EQ(captured_value, frames);

    const double elapsed = timer.ElapsedMilliSec();
    HELIOS_INFO("Basic test completed in {:.3f}ms", elapsed);
  }

  TEST_CASE("App: Module System") {
    HELIOS_INFO("Testing module system");
    helios::Timer timer;

    App app;

    app.AddModules<CoreModule, GameplayModule>();

    CHECK_EQ(app.ModuleCount(), 2);
    CHECK(app.ContainsModule<CoreModule>());
    CHECK(app.ContainsModule<GameplayModule>());

    bool has_game_time = false;
    bool has_game_stats = false;
    int frame_count = 0;

    const int frames = 50;
    app.SetRunner([frames, &has_game_time, &has_game_stats, &frame_count](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, frames);

      has_game_time = running_app.HasResource<GameTime>();
      has_game_stats = running_app.HasResource<GameStats>();

      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& time = world.ReadResource<GameTime>();
      frame_count = time.frame_count;

      return result;
    });

    app.Run();

    CHECK(has_game_time);
    CHECK(has_game_stats);
    CHECK_EQ(frame_count, frames);

    const double elapsed = timer.ElapsedMilliSec();
    HELIOS_INFO("Module test completed in {:.3f}ms", elapsed);
  }

  TEST_CASE("App: Resource Management") {
    HELIOS_INFO("Testing resource management");

    App app;

    app.InsertResource(GameTime{});
    app.InsertResource(ThreadSafeCounter{});

    CHECK(app.HasResource<GameTime>());
    CHECK(app.HasResource<ThreadSafeCounter>());
    CHECK_FALSE(app.HasResource<GameStats>());

    app.EmplaceResource<GameStats>();
    CHECK(app.HasResource<GameStats>());
  }

  TEST_CASE("App: SubApp Registration") {
    HELIOS_INFO("Testing sub-app registration");

    App app;

    app.AddSubApp<RenderSubApp>();
    app.AddSubApp<AudioSubApp>();

    CHECK(app.ContainsSubApp<RenderSubApp>());
    CHECK(app.ContainsSubApp<AudioSubApp>());

    auto& render_subapp = app.GetSubApp<RenderSubApp>();
    CHECK(render_subapp.AllowsOverlappingUpdates());
  }

  TEST_CASE("App: Event System") {
    HELIOS_INFO("Testing event system");

    App app;

    app.AddEvent<CustomEntitySpawnedEvent>();
    CHECK(app.HasEvent<CustomEntitySpawnedEvent>());

    struct EventEmitter final : public System {
      static constexpr std::string_view GetName() noexcept { return "EventEmitter"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& ctx) override {
        ctx.EmitEvent(CustomEntitySpawnedEvent{Entity{}, "Test", 0.0F, 0.0F, 0.0F});
      }
    };

    struct EventCounter {
      int count = 0;
      static constexpr std::string_view GetName() noexcept { return "EventCounter"; }
    };

    struct EventReader final : public System {
      static constexpr std::string_view GetName() noexcept { return "EventReader"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<EventCounter>(); }

      void Update(SystemContext& ctx) override {
        auto events = ctx.ReadEvents<CustomEntitySpawnedEvent>();
        auto& counter = ctx.WriteResource<EventCounter>();
        for ([[maybe_unused]] const auto& event : events) {
          ++counter.count;
        }
      }
    };

    app.InsertResource(EventCounter{});
    app.AddSystem<EventEmitter>(kUpdate);
    app.AddSystem<EventReader>(kPostUpdate);

    int captured_count = 0;
    app.SetRunner([&captured_count](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 5);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_count = world.ReadResource<EventCounter>().count;
      return result;
    });

    app.Run();

    CHECK_GT(captured_count, 0);
  }

  TEST_CASE("App: System Set Ordering") {
    HELIOS_INFO("Testing system set ordering");

    App app;

    struct ExecutionOrder {
      std::vector<std::string> order;
      static constexpr std::string_view GetName() noexcept { return "ExecutionOrder"; }
    };

    app.InsertResource(ExecutionOrder{});

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

    app.ConfigureSet<InputSet>(kUpdate);
    app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>();
    app.ConfigureSet<GameplaySet>(kUpdate).After<PhysicsSet>();
    app.ConfigureSet<RenderSet>(kUpdate).After<GameplaySet>();

    app.AddSystemBuilder<InputSystem>(kUpdate).InSet<InputSet>();
    app.AddSystemBuilder<PhysicsSystem>(kUpdate).InSet<PhysicsSet>();
    app.AddSystemBuilder<GameplaySystem>(kUpdate).InSet<GameplaySet>();
    app.AddSystemBuilder<RenderSystem>(kUpdate).InSet<RenderSet>();

    std::vector<std::string> captured_order;
    app.SetRunner([&captured_order](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_order = world.ReadResource<ExecutionOrder>().order;
      return result;
    });

    app.Run();

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
      static constexpr std::string_view GetName() noexcept { return "ScheduleTracker"; }
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

    std::vector<std::string> captured_schedules;
    app.SetRunner([&captured_schedules](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_schedules = world.ReadResource<ScheduleTracker>().schedules;
      return result;
    });

    app.Run();

    REQUIRE_EQ(captured_schedules.size(), 3);
    CHECK_EQ(captured_schedules[0], "Early");
    CHECK_EQ(captured_schedules[1], "Update");
    CHECK_EQ(captured_schedules[2], "Late");
  }

  TEST_CASE("App: First and Last Schedules") {
    HELIOS_INFO("Testing First and Last schedules");

    App app;

    struct ScheduleTracker {
      std::vector<std::string> schedules;
      static constexpr std::string_view GetName() noexcept { return "ScheduleTracker"; }
    };

    app.InsertResource(ScheduleTracker{});

    struct FirstSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "FirstSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("First"); }
    };

    struct PreUpdateSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "PreUpdateSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override {
        ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("PreUpdate");
      }
    };

    struct UpdateSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "UpdateSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override {
        ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("Update");
      }
    };

    struct PostUpdateSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "PostUpdateSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override {
        ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("PostUpdate");
      }
    };

    struct LastSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "LastSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<ScheduleTracker>(); }

      void Update(SystemContext& ctx) override { ctx.WriteResource<ScheduleTracker>().schedules.emplace_back("Last"); }
    };

    app.AddSystem<FirstSystem>(kFirst);
    app.AddSystem<PreUpdateSystem>(kPreUpdate);
    app.AddSystem<UpdateSystem>(kUpdate);
    app.AddSystem<PostUpdateSystem>(kPostUpdate);
    app.AddSystem<LastSystem>(kLast);

    std::vector<std::string> captured_schedules;
    app.SetRunner([&captured_schedules](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_schedules = world.ReadResource<ScheduleTracker>().schedules;
      return result;
    });

    app.Run();

    REQUIRE_EQ(captured_schedules.size(), 5);
    CHECK_EQ(captured_schedules[0], "First");
    CHECK_EQ(captured_schedules[1], "PreUpdate");
    CHECK_EQ(captured_schedules[2], "Update");
    CHECK_EQ(captured_schedules[3], "PostUpdate");
    CHECK_EQ(captured_schedules[4], "Last");
  }

  TEST_CASE("App: Time Resource") {
    HELIOS_INFO("Testing Time resource");

    App app;

    struct FrameCounter {
      uint64_t frame_count = 0;
      float total_delta = 0.0F;
      static constexpr std::string_view GetName() noexcept { return "FrameCounter"; }
    };

    app.InsertResource(FrameCounter{});

    struct TimeCheckSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "TimeCheckSystem"; }
      static constexpr auto GetAccessPolicy() noexcept {
        return AccessPolicy().ReadResources<Time>().WriteResources<FrameCounter>();
      }

      void Update(SystemContext& ctx) override {
        const auto& time = ctx.ReadResource<Time>();
        auto& counter = ctx.WriteResource<FrameCounter>();

        counter.frame_count = time.FrameCount();
        counter.total_delta += time.DeltaSeconds();
      }
    };

    app.AddSystem<TimeCheckSystem>(kUpdate);

    uint64_t captured_frames = 0;
    float captured_total = 0.0F;
    const int frames = 100;

    app.SetRunner([frames, &captured_frames, &captured_total](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, frames);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<FrameCounter>();
      captured_frames = counter.frame_count;
      captured_total = counter.total_delta;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_frames, frames);
    CHECK_GT(captured_total, 0.0F);
  }

  TEST_CASE("App: Entity Creation and Queries") {
    HELIOS_INFO("Testing entity creation and queries");

    App app;

    struct EntityCounter {
      int player_count = 0;
      int enemy_count = 0;
      static constexpr std::string_view GetName() noexcept { return "EntityCounter"; }
    };

    app.InsertResource(EntityCounter{});

    struct SpawnEntitiesSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "SpawnEntitiesSystem"; }
      static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

      void Update(SystemContext& ctx) override {
        // Spawn player
        auto player_cmd = ctx.EntityCommands(ctx.ReserveEntity());
        player_cmd.AddComponents(Player{}, Position{0.0F, 0.0F, 0.0F}, Health{100, 100});

        // Spawn enemies
        for (int i = 0; i < 3; ++i) {
          auto enemy_cmd = ctx.EntityCommands(ctx.ReserveEntity());
          enemy_cmd.AddComponents(Enemy{}, Position{static_cast<float>(i) * 10.0F, 0.0F, 0.0F}, Health{50, 50});
        }
      }
    };

    struct CountEntitiesSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "CountEntitiesSystem"; }
      static constexpr auto GetAccessPolicy() noexcept {
        return AccessPolicy().Query<const Position&, const Health&>().WriteResources<EntityCounter>();
      }

      void Update(SystemContext& ctx) override {
        auto& counter = ctx.WriteResource<EntityCounter>();

        // Count entities by checking Health values - players have max_health 100, enemies have 50
        counter.player_count = 0;
        counter.enemy_count = 0;

        ctx.Query().Get<const Health&>().ForEach([&counter](const Health& health) {
          if (health.max_health == 100) {
            ++counter.player_count;
          } else if (health.max_health == 50) {
            ++counter.enemy_count;
          }
        });
      }
    };

    app.AddSystem<SpawnEntitiesSystem>(kStartup);
    app.AddSystem<CountEntitiesSystem>(kUpdate);

    int captured_players = 0;
    int captured_enemies = 0;

    app.SetRunner([&captured_players, &captured_enemies](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, 1);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<EntityCounter>();
      captured_players = counter.player_count;
      captured_enemies = counter.enemy_count;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_players, 1);
    CHECK_EQ(captured_enemies, 3);
  }

  TEST_CASE("App: Same System In Multiple Schedules") {
    HELIOS_INFO("Testing same system in multiple schedules");

    App app;

    struct CleanupCounter {
      std::atomic<int> total_count{0};

      CleanupCounter() = default;
      CleanupCounter(const CleanupCounter& other) noexcept
          : total_count(other.total_count.load(std::memory_order_relaxed)) {}

      CleanupCounter(CleanupCounter&& other) noexcept
          : total_count(other.total_count.load(std::memory_order_relaxed)) {}

      CleanupCounter& operator=(const CleanupCounter&) = delete;
      CleanupCounter& operator=(CleanupCounter&&) = delete;

      static constexpr std::string_view GetName() noexcept { return "CleanupCounter"; }
    };

    app.InsertResource(CleanupCounter{});

    struct CleanupSystem final : public System {
      static constexpr std::string_view GetName() noexcept { return "CleanupSystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<CleanupCounter>(); }

      void Update(SystemContext& ctx) override {
        auto& counter = ctx.WriteResource<CleanupCounter>();
        counter.total_count.fetch_add(1, std::memory_order_relaxed);
      }
    };

    app.AddSystem<CleanupSystem>(kPreUpdate);
    app.AddSystem<CleanupSystem>(kPostUpdate);

    CHECK_EQ(app.SystemCount(kPreUpdate), 1);
    CHECK_EQ(app.SystemCount(kPostUpdate), 1);
    CHECK(app.ContainsSystem<CleanupSystem>(kPreUpdate));
    CHECK(app.ContainsSystem<CleanupSystem>(kPostUpdate));

    int captured_total = 0;
    const int frames = 10;

    app.SetRunner([frames, &captured_total](App& running_app) {
      AppExitCode result = FixedFrameRunner(running_app, frames);
      const auto& world = std::as_const(running_app).GetMainWorld();
      const auto& counter = world.ReadResource<CleanupCounter>();
      captured_total = counter.total_count.load(std::memory_order_relaxed);
      return result;
    });

    app.Run();

    // CleanupSystem runs twice per frame (PreUpdate + PostUpdate)
    CHECK_EQ(captured_total, frames * 2);
  }

  TEST_CASE("App: Module Lifecycle") {
    HELIOS_INFO("Testing module lifecycle");

    struct LifecycleTracker {
      int build_count = 0;
      int destroy_count = 0;
      static constexpr std::string_view GetName() noexcept { return "LifecycleTracker"; }
    };

    static LifecycleTracker* tracker_ptr = nullptr;

    struct LifecycleModule final : public Module {
      static constexpr std::string_view GetName() noexcept { return "LifecycleModule"; }

      void Build(App& app) override {
        if (tracker_ptr) {
          ++tracker_ptr->build_count;
        }
      }

      void Destroy(App& /*app*/) override {
        if (tracker_ptr) {
          ++tracker_ptr->destroy_count;
        }
      }
    };

    LifecycleTracker tracker;
    tracker_ptr = &tracker;

    {
      App app;
      app.AddModule<LifecycleModule>();

      app.SetRunner([](App& running_app) { return FixedFrameRunner(running_app, 1); });

      app.Run();
    }

    CHECK_EQ(tracker.build_count, 1);
    CHECK_EQ(tracker.destroy_count, 1);

    tracker_ptr = nullptr;
  }
}
