#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/ecs/system.hpp>

#include <atomic>
#include <string_view>
#include <utility>

using namespace helios::app;
using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
};

struct Health {
  int points = 100;
};

struct GameTime {
  float delta_time = 0.016F;
  float total_time = 0.0F;
};

struct PhysicsSettings {
  float gravity = 9.8F;
};

struct RenderSettings {
  bool vsync = true;
};

struct TestEvent {
  int value = 0;
};

struct AnotherTestEvent {
  float data = 0.0F;
};

struct TestSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "TestSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Position&>().ReadResources<GameTime>(); }

  void Update(SystemContext& /*ctx*/) override {}
};

struct AnotherSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "AnotherSystem"; }
  static constexpr auto GetAccessPolicy() {
    return AccessPolicy().Query<Velocity&>().WriteResources<PhysicsSettings>();
  }

  void Update(SystemContext& /*ctx*/) override {}
};

struct ThirdSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "ThirdSystem"; }

  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Health&>(); }

  void Update(SystemContext& /*ctx*/) override {}
};

struct CounterSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "CounterSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().WriteResources<GameTime>(); }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    time.total_time += time.delta_time;
  }
};

struct BasicModule final : public Module {
  void Build(App& app) override {
    app.AddSystem<TestSystem>(kUpdate);
    app.InsertResource(GameTime{});
  }

  void Destroy(App& /*ctx*/) override {}
};

struct NamedModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "CustomModuleName"; }

  void Build(App& app) override {
    app.AddSystem<AnotherSystem>(kUpdate);
    app.InsertResource(PhysicsSettings{});
  }

  void Destroy(App& /*ctx*/) override {}
};

struct EmptyModule final : public Module {
  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct CountingModule final : public Module {
  void Build(App& /*ctx*/) override { ++build_count; }
  void Destroy(App& /*ctx*/) override { ++destroy_count; }

  inline static std::atomic<int> build_count{0};
  inline static std::atomic<int> destroy_count{0};
};

struct MainSubApp {};

struct RenderSubApp {
  static constexpr std::string_view GetName() noexcept { return "RenderSubApp"; }
};

struct PhysicsSubApp {
  static constexpr std::string_view GetName() noexcept { return "PhysicsSubApp"; }
};

struct AsyncRenderSubApp {
  static constexpr std::string_view GetName() noexcept { return "AsyncRenderSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return true; }
};

struct SyncSubApp {
  static constexpr std::string_view GetName() noexcept { return "SyncSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return false; }
};

struct MaxOverlappingSubApp {
  static constexpr std::string_view GetName() noexcept { return "MaxOverlappingSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return true; }
  static constexpr size_t GetMaxOverlappingUpdates() noexcept { return 3; }
};

struct RendererSubApp {
  static constexpr std::string_view GetName() noexcept { return "RendererSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return true; }
  static constexpr size_t GetMaxOverlappingUpdates() noexcept { return 2; }  // Max frames in flight
};

// System sets for testing
struct PhysicsSet {};
struct RenderSet {};
struct InputSet {};
struct GameplaySet {};

struct InputSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "InputSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy(); }

  void Update(SystemContext& /*ctx*/) override {}
};

struct MovementSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "MovementSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Position&, Velocity&>(); }

  void Update(SystemContext& /*ctx*/) override {}
};

struct CollisionSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "CollisionSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Position&>(); }

  void Update(SystemContext& /*ctx*/) override {}
};

struct PhysicsSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "PhysicsSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Velocity&>(); }
  void Update(SystemContext& /*ctx*/) override {}
};

struct RenderSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "RenderSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().Query<Position&>(); }
  void Update(SystemContext& /*ctx*/) override {}
};

}  // namespace

TEST_SUITE("app::App") {
  TEST_CASE("App::ctor: Default construction") {
    App app;

    CHECK_EQ(app.ModuleCount(), 0);
    CHECK_EQ(app.SystemCount(), 0);
  }

  TEST_CASE("App::AddModule: Single Module") {
    App app;
    app.AddModule<BasicModule>();

    CHECK_EQ(app.ModuleCount(), 1);
    CHECK(app.ContainsModule<BasicModule>());
  }

  TEST_CASE("App::AddModule: Multiple Modules") {
    App app;
    app.AddModule<BasicModule>();
    app.AddModule<NamedModule>();
    app.AddModule<EmptyModule>();

    CHECK_EQ(app.ModuleCount(), 3);
    CHECK(app.ContainsModule<BasicModule>());
    CHECK(app.ContainsModule<NamedModule>());
    CHECK(app.ContainsModule<EmptyModule>());
  }

  TEST_CASE("App::AddModule:s Multiple At Once") {
    App app;
    app.AddModules<BasicModule, NamedModule, EmptyModule>();

    CHECK_EQ(app.ModuleCount(), 3);
    CHECK(app.ContainsModule<BasicModule>());
    CHECK(app.ContainsModule<NamedModule>());
    CHECK(app.ContainsModule<EmptyModule>());
  }

  TEST_CASE("App::ContainsModule: Returns False For Non-Existent Module") {
    App app;
    app.AddModule<BasicModule>();

    CHECK(app.ContainsModule<BasicModule>());
    CHECK_FALSE(app.ContainsModule<NamedModule>());
    CHECK_FALSE(app.ContainsModule<EmptyModule>());
  }

  TEST_CASE("App::AddSystem: Single System") {
    App app;
    app.AddSystem<TestSystem>(kUpdate);

    CHECK_EQ(app.SystemCount(), 1);
    CHECK_EQ(app.SystemCount(kUpdate), 1);
    CHECK(app.ContainsSystem<TestSystem>());
    CHECK(app.ContainsSystem<TestSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem: Multiple Systems") {
    App app;
    app.AddSystem<TestSystem>(kUpdate);
    app.AddSystem<AnotherSystem>(kUpdate);
    app.AddSystem<ThirdSystem>(kPostUpdate);

    CHECK_EQ(app.SystemCount(), 3);
    CHECK_EQ(app.SystemCount(kUpdate), 2);
    CHECK_EQ(app.SystemCount(kPostUpdate), 1);
    CHECK(app.ContainsSystem<TestSystem>());
    CHECK(app.ContainsSystem<AnotherSystem>());
    CHECK(app.ContainsSystem<ThirdSystem>());
  }

  TEST_CASE("App::AddSystem:s Multiple At Once") {
    App app;
    app.AddSystems<TestSystem, AnotherSystem, ThirdSystem>(kUpdate);

    CHECK_EQ(app.SystemCount(), 3);
    CHECK_EQ(app.SystemCount(kUpdate), 3);
    CHECK(app.ContainsSystem<TestSystem>());
    CHECK(app.ContainsSystem<AnotherSystem>());
    CHECK(app.ContainsSystem<ThirdSystem>());
  }

  TEST_CASE("App::ContainsSystem: In Different Schedules") {
    App app;
    app.AddSystem<TestSystem>(kUpdate);
    app.AddSystem<AnotherSystem>(kPostUpdate);

    CHECK(app.ContainsSystem<TestSystem>(kUpdate));
    CHECK_FALSE(app.ContainsSystem<TestSystem>(kPostUpdate));
    CHECK(app.ContainsSystem<AnotherSystem>(kPostUpdate));
    CHECK_FALSE(app.ContainsSystem<AnotherSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem: Same System In Multiple Schedules") {
    App app;

    // Add the same system type to different schedules
    app.AddSystem<TestSystem>(kPreUpdate);
    app.AddSystem<TestSystem>(kUpdate);
    app.AddSystem<TestSystem>(kPostUpdate);

    // Total systems should be 3 (one per schedule)
    CHECK_EQ(app.SystemCount(), 3);

    // Each schedule should have exactly one system
    CHECK_EQ(app.SystemCount(kPreUpdate), 1);
    CHECK_EQ(app.SystemCount(kUpdate), 1);
    CHECK_EQ(app.SystemCount(kPostUpdate), 1);

    // ContainsSystem should return true for each schedule
    CHECK(app.ContainsSystem<TestSystem>(kPreUpdate));
    CHECK(app.ContainsSystem<TestSystem>(kUpdate));
    CHECK(app.ContainsSystem<TestSystem>(kPostUpdate));

    // Global ContainsSystem should also return true
    CHECK(app.ContainsSystem<TestSystem>());
  }

  TEST_CASE("App::AddSystem: Same System In Multiple Schedules With Other Systems") {
    App app;

    // Add TestSystem to multiple schedules
    app.AddSystem<TestSystem>(kPreUpdate);
    app.AddSystem<TestSystem>(kPostUpdate);

    // Add other systems to Update
    app.AddSystem<AnotherSystem>(kUpdate);
    app.AddSystem<ThirdSystem>(kUpdate);

    CHECK_EQ(app.SystemCount(), 4);
    CHECK_EQ(app.SystemCount(kPreUpdate), 1);
    CHECK_EQ(app.SystemCount(kUpdate), 2);
    CHECK_EQ(app.SystemCount(kPostUpdate), 1);

    CHECK(app.ContainsSystem<TestSystem>(kPreUpdate));
    CHECK_FALSE(app.ContainsSystem<TestSystem>(kUpdate));
    CHECK(app.ContainsSystem<TestSystem>(kPostUpdate));

    CHECK_FALSE(app.ContainsSystem<AnotherSystem>(kPreUpdate));
    CHECK(app.ContainsSystem<AnotherSystem>(kUpdate));
    CHECK_FALSE(app.ContainsSystem<AnotherSystem>(kPostUpdate));
  }

  TEST_CASE("App::AddSystem: Cleanup System Pattern - Same System In PreUpdate And PostUpdate") {
    // This test simulates a real-world use case: a cleanup system that runs
    // both at the beginning and end of an update cycle
    App app;

    app.AddSystem<InputSystem>(kPreUpdate);  // Cleanup at start
    app.AddSystem<MovementSystem>(kUpdate);
    app.AddSystem<CollisionSystem>(kUpdate);
    app.AddSystem<InputSystem>(kPostUpdate);  // Cleanup at end

    CHECK_EQ(app.SystemCount(), 4);
    CHECK(app.ContainsSystem<InputSystem>(kPreUpdate));
    CHECK_FALSE(app.ContainsSystem<InputSystem>(kUpdate));
    CHECK(app.ContainsSystem<InputSystem>(kPostUpdate));
  }

  TEST_CASE("App::InsertResource:") {
    App app;
    app.InsertResource(GameTime{0.016F, 0.0F});

    const auto& world = std::as_const(app.GetMainSubApp()).GetWorld();
    CHECK(world.HasResource<GameTime>());

    const auto& time = world.ReadResource<GameTime>();
    CHECK_EQ(time.delta_time, doctest::Approx(0.016F));
    CHECK_EQ(time.total_time, doctest::Approx(0.0F));
  }

  TEST_CASE("App::EmplaceResource:") {
    App app;
    app.EmplaceResource<GameTime>(0.016F, 0.0F);

    const auto& world = std::as_const(app.GetMainSubApp()).GetWorld();
    CHECK(world.HasResource<GameTime>());

    const auto& time = world.ReadResource<GameTime>();
    CHECK_EQ(time.delta_time, doctest::Approx(0.016F));
    CHECK_EQ(time.total_time, doctest::Approx(0.0F));
  }

  TEST_CASE("App::InsertResource: Multiple Resources") {
    App app;
    app.InsertResource(GameTime{});
    app.InsertResource(PhysicsSettings{});
    app.InsertResource(RenderSettings{});

    const auto& world = std::as_const(app.GetMainSubApp()).GetWorld();
    CHECK(world.HasResource<GameTime>());
    CHECK(world.HasResource<PhysicsSettings>());
    CHECK(world.HasResource<RenderSettings>());
  }

  TEST_CASE("App::Clear: Removes All Data") {
    App app;
    app.AddModule<BasicModule>();
    app.AddModule<NamedModule>();
    app.AddSystem<ThirdSystem>(kUpdate);
    app.InsertResource(GameTime{});

    app.Clear();

    CHECK_EQ(app.SystemCount(), 0);
    CHECK_FALSE(app.ContainsSystem<ThirdSystem>());
    CHECK_FALSE(std::as_const(app.GetMainSubApp()).GetWorld().HasResource<GameTime>());
  }

  TEST_CASE("App::AddSystem:Builder with Before") {
    App app;
    app.AddSystem<TestSystem>(kUpdate);
    app.AddSystemBuilder<AnotherSystem>(kUpdate).Before<TestSystem>();

    CHECK_EQ(app.SystemCount(kUpdate), 2);
    CHECK(app.ContainsSystem<TestSystem>());
    CHECK(app.ContainsSystem<AnotherSystem>());
  }

  TEST_CASE("App::AddSystem:Builder with InSet") {
    App app;
    app.AddSystemBuilder<PhysicsSystem>(kUpdate).InSet<PhysicsSet>();

    CHECK_EQ(app.SystemCount(kUpdate), 1);
    CHECK(app.ContainsSystem<PhysicsSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem:sBuilder with Sequence") {
    App app;
    app.AddSystemsBuilder<MovementSystem, CollisionSystem, PhysicsSystem>(kUpdate).Sequence();

    CHECK_EQ(app.SystemCount(kUpdate), 3);
    CHECK(app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(app.ContainsSystem<CollisionSystem>(kUpdate));
    CHECK(app.ContainsSystem<PhysicsSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem:sBuilder with After and Before") {
    App app;
    app.AddSystem<InputSystem>(kUpdate);
    app.AddSystem<RenderSystem>(kUpdate);

    app.AddSystemsBuilder<MovementSystem, CollisionSystem>(kUpdate).After<InputSystem>().Before<RenderSystem>();

    CHECK_EQ(app.SystemCount(kUpdate), 4);
    CHECK(app.ContainsSystem<InputSystem>(kUpdate));
    CHECK(app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(app.ContainsSystem<CollisionSystem>(kUpdate));
    CHECK(app.ContainsSystem<RenderSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem:sBuilder with InSet and Sequence") {
    App app;
    app.AddSystemsBuilder<MovementSystem, CollisionSystem, PhysicsSystem>(kUpdate).InSet<PhysicsSet>().Sequence();

    CHECK_EQ(app.SystemCount(kUpdate), 3);
    CHECK(app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(app.ContainsSystem<CollisionSystem>(kUpdate));
    CHECK(app.ContainsSystem<PhysicsSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem:sBuilder with Multiple InSet") {
    App app;
    app.AddSystemsBuilder<MovementSystem, CollisionSystem>(kUpdate).InSet<PhysicsSet>().InSet<GameplaySet>();

    CHECK_EQ(app.SystemCount(kUpdate), 2);
    CHECK(app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(app.ContainsSystem<CollisionSystem>(kUpdate));
  }

  TEST_CASE("App::AddSystem:sBuilder Complex Configuration") {
    App app;
    app.AddSystem<InputSystem>(kUpdate);
    app.AddSystem<RenderSystem>(kUpdate);

    app.AddSystemsBuilder<MovementSystem, CollisionSystem, PhysicsSystem>(kUpdate)
        .After<InputSystem>()
        .Before<RenderSystem>()
        .InSet<PhysicsSet>()
        .Sequence();

    CHECK_EQ(app.SystemCount(kUpdate), 5);
    CHECK(app.ContainsSystem<InputSystem>(kUpdate));
    CHECK(app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(app.ContainsSystem<CollisionSystem>(kUpdate));
    CHECK(app.ContainsSystem<PhysicsSystem>(kUpdate));
    CHECK(app.ContainsSystem<RenderSystem>(kUpdate));
  }

  TEST_CASE("App::ConfigureSet: with After") {
    App app;
    app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>();

    CHECK(true);
  }

  TEST_CASE("App::ConfigureSet: with Before") {
    App app;
    app.ConfigureSet<PhysicsSet>(kUpdate).Before<RenderSet>();

    CHECK(true);
  }

  TEST_CASE("App::ConfigureSet: with Multiple Constraints") {
    App app;
    app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>().Before<RenderSet>();

    CHECK(true);
  }

  TEST_CASE("App::AddSystem:Builder with After") {
    App app;
    app.AddSystem<TestSystem>(kUpdate);
    app.AddSystemBuilder<AnotherSystem>(kUpdate).After<TestSystem>();

    CHECK_EQ(app.SystemCount(kUpdate), 2);
    CHECK(app.ContainsSystem<TestSystem>());
    CHECK(app.ContainsSystem<AnotherSystem>());
  }

  TEST_CASE("App::GetMainSubApp: Returns Reference") {
    App app;
    const auto& sub_app = app.GetMainSubApp();

    CHECK_EQ(sub_app.SystemCount(), 0);
  }

  TEST_CASE("App::GetMainSubApp: Const Version") {
    const App app;
    const auto& sub_app = app.GetMainSubApp();

    CHECK_EQ(sub_app.SystemCount(), 0);
  }

  TEST_CASE("App::GetExecutor: Returns Reference") {
    App app;
    auto& executor = app.GetExecutor();

    CHECK_GE(executor.WorkerCount(), 0);
  }

  TEST_CASE("App::GetExecutor: Const Version") {
    const App app;
    const auto& executor = app.GetExecutor();

    CHECK_GE(executor.WorkerCount(), 0);
  }

  TEST_CASE("App::AddSubApp: Single SubApp") {
    App app;
    app.AddSubApp<RenderSubApp>();

    CHECK(app.ContainsSubApp<RenderSubApp>());
    auto& sub_app = app.GetSubApp<RenderSubApp>();
    CHECK_EQ(sub_app.SystemCount(), 0);
  }

  TEST_CASE("App::AddSubApp: Multiple SubApps") {
    App app;
    app.AddSubApp<RenderSubApp>();
    app.AddSubApp<PhysicsSubApp>();

    CHECK(app.ContainsSubApp<RenderSubApp>());
    CHECK(app.ContainsSubApp<PhysicsSubApp>());

    auto& render_sub_app = app.GetSubApp<RenderSubApp>();
    auto& physics_sub_app = app.GetSubApp<PhysicsSubApp>();

    CHECK_EQ(render_sub_app.SystemCount(), 0);
    CHECK_EQ(physics_sub_app.SystemCount(), 0);
  }

  TEST_CASE("App::AddSubApp: With Instance") {
    App app;
    SubApp render_sub_app;
    render_sub_app.AddSystem<TestSystem>(kUpdate);

    app.AddSubApp<RenderSubApp>(std::move(render_sub_app));
    CHECK(app.ContainsSubApp<RenderSubApp>());

    auto& sub_app = app.GetSubApp<RenderSubApp>();
    CHECK_EQ(sub_app.SystemCount(), 1);
    CHECK(sub_app.ContainsSystem<TestSystem>());
  }

  TEST_CASE("App::GetSubApp: Returns Reference") {
    App app;
    app.AddSubApp<RenderSubApp>();

    auto& sub_app = app.GetSubApp<RenderSubApp>();
    sub_app.AddSystem<TestSystem>(kUpdate);

    CHECK_EQ(sub_app.SystemCount(), 1);
  }

  TEST_CASE("App::GetSubApp: Const Version") {
    App app;
    app.AddSubApp<RenderSubApp>();

    CHECK(app.ContainsSubApp<RenderSubApp>());

    const App& const_app = app;
    const auto& sub_app = const_app.GetSubApp<RenderSubApp>();

    CHECK_EQ(sub_app.SystemCount(), 0);
  }

  TEST_CASE("App::SetSubAppExtraction:") {
    App app;
    app.AddSubApp<RenderSubApp>();

    bool extraction_called = false;
    app.SetSubAppExtraction<RenderSubApp>(
        [&extraction_called](const World& /*main_world*/, World& /*sub_world*/) { extraction_called = true; });

    CHECK_FALSE(extraction_called);
  }

  TEST_CASE("App: Method chaining AddModule") {
    App app;
    auto& result = app.AddModule<BasicModule>();

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining AddModules") {
    App app;
    auto& result = app.AddModules<BasicModule, NamedModule>();

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining AddSystem") {
    App app;
    auto& result = app.AddSystem<TestSystem>(kUpdate);

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining AddSystems") {
    App app;
    auto& result = app.AddSystems<TestSystem, AnotherSystem>(kUpdate);

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining InsertResource") {
    App app;
    auto& result = app.InsertResource(GameTime{});

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining EmplaceResource") {
    App app;
    auto& result = app.EmplaceResource<GameTime>();

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining AddSubApp") {
    App app;
    auto& result = app.AddSubApp<RenderSubApp>();

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining SetRunner") {
    App app;
    auto& result = app.SetRunner([](App& /*app*/) { return AppExitCode::Success; });

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Method chaining SetSubAppExtraction") {
    App app;
    app.AddSubApp<RenderSubApp>();
    auto& result = app.SetSubAppExtraction<RenderSubApp>([](const World& /*src*/, World& /*dst*/) {});

    CHECK_EQ(&result, &app);
  }

  TEST_CASE("App: Fluent API Chain") {
    App app;
    app.AddModule<BasicModule>()
        .AddModule<NamedModule>()
        .AddSystem<ThirdSystem>(kUpdate)
        .InsertResource(RenderSettings{})
        .AddSubApp<RenderSubApp>();

    CHECK_EQ(app.ModuleCount(), 2);
    CHECK(app.ContainsModule<BasicModule>());
    CHECK(app.ContainsModule<NamedModule>());
    CHECK(app.ContainsSystem<ThirdSystem>());
    CHECK(std::as_const(app.GetMainSubApp()).GetWorld().HasResource<RenderSettings>());
  }

  TEST_CASE("App::SystemCount: Across Schedules") {
    App app;
    app.AddSystem<TestSystem>(kPreUpdate);
    app.AddSystem<AnotherSystem>(kUpdate);
    app.AddSystem<ThirdSystem>(kPostUpdate);

    CHECK_EQ(app.SystemCount(), 3);
    CHECK_EQ(app.SystemCount(kPreUpdate), 1);
    CHECK_EQ(app.SystemCount(kUpdate), 1);
    CHECK_EQ(app.SystemCount(kPostUpdate), 1);
    CHECK_EQ(app.SystemCount(Main{}), 0);
  }

  TEST_CASE("AppExitCode: Success Value") {
    CHECK_EQ(static_cast<int>(AppExitCode::Success), 0);
  }

  TEST_CASE("AppExitCode: Failure Value") {
    CHECK_EQ(static_cast<int>(AppExitCode::Failure), 1);
  }

  TEST_CASE("App::AddSubApp: Sets Overlapping Flag For Async SubApp") {
    App app;
    app.AddSubApp<AsyncRenderSubApp>();

    auto& sub_app = app.GetSubApp<AsyncRenderSubApp>();
    CHECK(sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("App::AddSubApp: Sets Overlapping Flag For Sync SubApp") {
    App app;
    app.AddSubApp<SyncSubApp>();

    auto& sub_app = app.GetSubApp<SyncSubApp>();
    CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("App::AddSubApp: Default SubApp Has No Overlapping") {
    App app;
    app.AddSubApp<RenderSubApp>();

    auto& sub_app = app.GetSubApp<RenderSubApp>();
    CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("App::AddSubApp: With Instance Sets Overlapping Flag") {
    App app;
    SubApp render_sub_app;
    app.AddSubApp<AsyncRenderSubApp>(std::move(render_sub_app));

    auto& sub_app = app.GetSubApp<AsyncRenderSubApp>();
    CHECK(sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("App::AddSubApp: Multiple with different async traits With Different Async Traits") {
    App app;
    app.AddSubApp<AsyncRenderSubApp>();
    app.AddSubApp<SyncSubApp>();
    app.AddSubApp<RenderSubApp>();

    auto& async_sub_app = app.GetSubApp<AsyncRenderSubApp>();
    auto& sync_sub_app = app.GetSubApp<SyncSubApp>();
    auto& default_sub_app = app.GetSubApp<RenderSubApp>();

    CHECK(async_sub_app.AllowsOverlappingUpdates());
    CHECK_FALSE(sync_sub_app.AllowsOverlappingUpdates());
    CHECK_FALSE(default_sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("App::HasEvent") {
    App app;

    SUBCASE("HasEvent Returns False Before Event Registration") {
      CHECK_FALSE(app.HasEvent<TestEvent>());
      CHECK_FALSE(app.HasEvent<AnotherTestEvent>());
    }

    SUBCASE("HasEvent Returns True After Event Registration") {
      app.AddEvent<TestEvent>();
      CHECK(app.HasEvent<TestEvent>());
      CHECK_FALSE(app.HasEvent<AnotherTestEvent>());

      app.AddEvent<AnotherTestEvent>();
      CHECK(app.HasEvent<TestEvent>());
      CHECK(app.HasEvent<AnotherTestEvent>());
    }

    SUBCASE("HasEvent After Clear") {
      app.AddEvent<TestEvent>();
      app.AddEvent<AnotherTestEvent>();
      CHECK(app.HasEvent<TestEvent>());
      CHECK(app.HasEvent<AnotherTestEvent>());

      app.Clear();
      CHECK_FALSE(app.HasEvent<TestEvent>());
      CHECK_FALSE(app.HasEvent<AnotherTestEvent>());
    }

    SUBCASE("HasEvent Delegates To Main SubApp") {
      app.AddEvent<TestEvent>();
      CHECK(app.HasEvent<TestEvent>());
      CHECK(app.GetMainSubApp().HasEvent<TestEvent>());
    }
  }

  TEST_CASE("App::Initialize") {
    App app;

    SUBCASE("Initialize Sets Initialized Flag") {
      CHECK_FALSE(app.IsInitialized());
      app.Initialize();
      CHECK(app.IsInitialized());
    }

    SUBCASE("Initialize Builds Scheduler") {
      app.AddSystem<CounterSystem>(kUpdate);
      app.InsertResource(GameTime{});
      CHECK_FALSE(app.IsInitialized());

      app.Initialize();
      CHECK(app.IsInitialized());
    }

    SUBCASE("Initialize With Sub Apps") {
      app.AddSubApp<RenderSubApp>();
      app.AddSystem<CounterSystem>(kUpdate);
      app.InsertResource(GameTime{});

      CHECK_FALSE(app.IsInitialized());
      app.Initialize();
      CHECK(app.IsInitialized());
    }

    SUBCASE("Initialize With Modules") {
      app.AddModule<BasicModule>();
      CHECK_FALSE(app.IsInitialized());

      // BuildModules is called internally by Run(), so we test Initialize without it
      app.Initialize();
      CHECK(app.IsInitialized());
    }
  }

  TEST_CASE("App::WaitForOverlappingUpdates: Specific SubApp") {
    App app;

    SUBCASE("WaitForOverlappingUpdates Template Version") {
      app.AddSubApp<AsyncRenderSubApp>();

      // Should not crash even if no overlapping updates exist
      app.WaitForOverlappingUpdates<AsyncRenderSubApp>();
    }

    SUBCASE("WaitForOverlappingUpdates With Non-Existent SubApp") {
      // Should not crash even if sub-app doesn't exist
      app.WaitForOverlappingUpdates<RenderSubApp>();
    }

    SUBCASE("WaitForOverlappingUpdates Instance Version") {
      app.AddSubApp<AsyncRenderSubApp>();
      auto& sub_app = app.GetSubApp<AsyncRenderSubApp>();

      // Should not crash even if no overlapping updates exist
      app.WaitForOverlappingUpdates(sub_app);
    }
  }

  TEST_CASE("SubAppWithMaxOverlappingUpdatesTrait") {
    SUBCASE("SubApp With Max Overlapping Updates") {
      CHECK(SubAppWithMaxOverlappingUpdatesTrait<MaxOverlappingSubApp>);
      CHECK(SubAppWithMaxOverlappingUpdatesTrait<RendererSubApp>);
    }

    SUBCASE("SubApp Without Max Overlapping Updates") {
      CHECK_FALSE(SubAppWithMaxOverlappingUpdatesTrait<AsyncRenderSubApp>);
      CHECK_FALSE(SubAppWithMaxOverlappingUpdatesTrait<SyncSubApp>);
      CHECK_FALSE(SubAppWithMaxOverlappingUpdatesTrait<MainSubApp>);
    }
  }

  TEST_CASE("SubAppMaxOverlappingUpdates") {
    SUBCASE("Returns Correct Value For SubApp With Trait") {
      constexpr size_t max1 = SubAppMaxOverlappingUpdates<MaxOverlappingSubApp>();
      constexpr size_t max2 = SubAppMaxOverlappingUpdates<RendererSubApp>();

      CHECK_EQ(max1, 3);
      CHECK_EQ(max2, 2);
    }

    SUBCASE("Returns Zero For SubApp Without Trait") {
      constexpr size_t max1 = SubAppMaxOverlappingUpdates<AsyncRenderSubApp>();
      constexpr size_t max2 = SubAppMaxOverlappingUpdates<SyncSubApp>();
      constexpr size_t max3 = SubAppMaxOverlappingUpdates<MainSubApp>();

      CHECK_EQ(max1, 0);
      CHECK_EQ(max2, 0);
      CHECK_EQ(max3, 0);
    }
  }
}
