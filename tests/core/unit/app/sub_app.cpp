#include <doctest/doctest.h>

#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>

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

struct UpdateCounterSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "UpdateCounterSystem"; }
  static constexpr auto GetAccessPolicy() { return AccessPolicy().WriteResources<GameTime>(); }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    time.total_time += time.delta_time;
  }
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

struct SyncPhysicsSubApp {
  static constexpr std::string_view GetName() noexcept { return "SyncPhysicsSubApp"; }
  static constexpr bool AllowOverlappingUpdates() noexcept { return false; }
};

}  // namespace

TEST_SUITE("app::SubApp") {
  TEST_CASE("SubApp::ctor: Default Construction") {
    const SubApp sub_app;

    CHECK_EQ(sub_app.SystemCount(), 0);
    CHECK_EQ(sub_app.GetWorld().EntityCount(), 0);
  }

  TEST_CASE("SubApp::ctor: AddSystem Single System") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(Update{});

    CHECK_EQ(sub_app.SystemCount(), 1);
    CHECK_EQ(sub_app.SystemCount(Update{}), 1);
    CHECK(sub_app.ContainsSystem<TestSystem>());
    CHECK(sub_app.ContainsSystem<TestSystem>(Update{}));
  }

  TEST_CASE("SubApp::ctor: AddSystem Multiple Systems") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(Update{});
    sub_app.AddSystem<AnotherSystem>(Update{});
    sub_app.AddSystem<ThirdSystem>(PostUpdate{});

    CHECK_EQ(sub_app.SystemCount(), 3);
    CHECK_EQ(sub_app.SystemCount(Update{}), 2);
    CHECK_EQ(sub_app.SystemCount(PostUpdate{}), 1);
    CHECK(sub_app.ContainsSystem<TestSystem>());
    CHECK(sub_app.ContainsSystem<AnotherSystem>());
    CHECK(sub_app.ContainsSystem<ThirdSystem>());
  }

  TEST_CASE("SubApp::ctor: AddSystems Multiple At Once") {
    SubApp sub_app;
    sub_app.AddSystems<TestSystem, AnotherSystem, ThirdSystem>(Update{});

    CHECK_EQ(sub_app.SystemCount(), 3);
    CHECK_EQ(sub_app.SystemCount(Update{}), 3);
    CHECK(sub_app.ContainsSystem<TestSystem>());
    CHECK(sub_app.ContainsSystem<AnotherSystem>());
    CHECK(sub_app.ContainsSystem<ThirdSystem>());
  }

  TEST_CASE("SubApp::ctor: ContainsSystem In Different Schedules") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(Update{});
    sub_app.AddSystem<AnotherSystem>(PostUpdate{});

    CHECK(sub_app.ContainsSystem<TestSystem>(Update{}));
    CHECK_FALSE(sub_app.ContainsSystem<TestSystem>(PostUpdate{}));
    CHECK(sub_app.ContainsSystem<AnotherSystem>(PostUpdate{}));
    CHECK_FALSE(sub_app.ContainsSystem<AnotherSystem>(Update{}));
  }

  TEST_CASE("SubApp::ctor: InsertResource") {
    SubApp sub_app;
    sub_app.InsertResource(GameTime{0.016F, 0.0F});

    const auto& world = std::as_const(sub_app).GetWorld();
    CHECK(world.HasResource<GameTime>());

    const auto& time = world.ReadResource<GameTime>();
    CHECK_EQ(time.delta_time, doctest::Approx(0.016F));
    CHECK_EQ(time.total_time, doctest::Approx(0.0F));
  }

  TEST_CASE("SubApp::ctor: EmplaceResource") {
    SubApp sub_app;
    sub_app.EmplaceResource<GameTime>(0.016F, 0.0F);

    const auto& world = std::as_const(sub_app).GetWorld();
    CHECK(world.HasResource<GameTime>());

    const auto& time = world.ReadResource<GameTime>();
    CHECK_EQ(time.delta_time, doctest::Approx(0.016F));
    CHECK_EQ(time.total_time, doctest::Approx(0.0F));
  }

  TEST_CASE("SubApp::ctor: InsertResource Multiple Resources") {
    SubApp sub_app;
    sub_app.InsertResource(GameTime{});
    sub_app.InsertResource(PhysicsSettings{});

    const auto& world = std::as_const(sub_app).GetWorld();
    CHECK(world.HasResource<GameTime>());
    CHECK(world.HasResource<PhysicsSettings>());
  }

  TEST_CASE("SubApp::ctor: Clear Removes All Data") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(Update{});
    sub_app.AddSystem<AnotherSystem>(PostUpdate{});
    sub_app.InsertResource(GameTime{});

    sub_app.Clear();

    CHECK_EQ(sub_app.SystemCount(), 0);
    CHECK_FALSE(sub_app.ContainsSystem<TestSystem>());
    CHECK_FALSE(sub_app.ContainsSystem<AnotherSystem>());
    CHECK_FALSE(std::as_const(sub_app).GetWorld().HasResource<GameTime>());
  }

  TEST_CASE("SubApp::ctor: SystemCount Across Schedules") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(PreUpdate{});
    sub_app.AddSystem<UpdateCounterSystem>(Update{});
    sub_app.AddSystem<ThirdSystem>(PostUpdate{});

    CHECK_EQ(sub_app.SystemCount(), 3);
    CHECK_EQ(sub_app.SystemCount(PreUpdate{}), 1);
    CHECK_EQ(sub_app.SystemCount(Update{}), 1);
    CHECK_EQ(sub_app.SystemCount(PostUpdate{}), 1);
    CHECK_EQ(sub_app.SystemCount(Main{}), 0);
  }

  TEST_CASE("SubApp::ctor: GetWorld Returns World Reference") {
    const SubApp sub_app;
    const auto& world = sub_app.GetWorld();

    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("SubApp::ctor: AddSystemBuilder with Before") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(Update{});
    sub_app.AddSystemBuilder<AnotherSystem>(Update{}).Before<TestSystem>();

    CHECK_EQ(sub_app.SystemCount(Update{}), 2);
    CHECK(sub_app.ContainsSystem<TestSystem>());
    CHECK(sub_app.ContainsSystem<AnotherSystem>());
  }

  TEST_CASE("SubApp::ctor: AddSystemBuilder with After") {
    SubApp sub_app;
    sub_app.AddSystem<TestSystem>(Update{});
    sub_app.AddSystemBuilder<AnotherSystem>(Update{}).After<TestSystem>();

    CHECK_EQ(sub_app.SystemCount(Update{}), 2);
    CHECK(sub_app.ContainsSystem<TestSystem>());
    CHECK(sub_app.ContainsSystem<AnotherSystem>());
  }

  TEST_CASE("SubApp::ctor: Move Construction") {
    SubApp sub_app1;
    sub_app1.AddSystem<TestSystem>(Update{});
    sub_app1.InsertResource(GameTime{});

    SubApp sub_app2 = std::move(sub_app1);

    CHECK_EQ(sub_app2.SystemCount(), 1);
    CHECK(sub_app2.ContainsSystem<TestSystem>());
    CHECK(std::as_const(sub_app2).GetWorld().HasResource<GameTime>());
  }

  TEST_CASE("SubApp::ctor: Move Assignment") {
    SubApp sub_app1;
    sub_app1.AddSystem<TestSystem>(Update{});
    sub_app1.InsertResource(GameTime{});

    SubApp sub_app2;
    sub_app2 = std::move(sub_app1);

    CHECK_EQ(sub_app2.SystemCount(), 1);
    CHECK(sub_app2.ContainsSystem<TestSystem>());
    CHECK(std::as_const(sub_app2).GetWorld().HasResource<GameTime>());
  }

  TEST_CASE("SubAppTrait: Valid Empty Structs") {
    CHECK(SubAppTrait<MainSubApp>);
    CHECK(SubAppTrait<RenderSubApp>);
    CHECK(SubAppTrait<PhysicsSubApp>);
  }

  TEST_CASE("SubAppTrait: Invalid Non-Empty Types") {
    CHECK_FALSE(SubAppTrait<int>);
    CHECK_FALSE(SubAppTrait<Position>);
    CHECK_FALSE(SubAppTrait<GameTime>);
  }

  TEST_CASE("SubAppWithNameTrait: Valid Named SubApp") {
    CHECK(SubAppWithNameTrait<RenderSubApp>);
    CHECK(SubAppWithNameTrait<PhysicsSubApp>);
  }

  TEST_CASE("SubAppWithNameTrait: Unnamed SubApp") {
    CHECK_FALSE(SubAppWithNameTrait<MainSubApp>);
  }

  TEST_CASE("SubAppTypeIdOf: Returns Unique IDs") {
    const SubAppTypeId id1 = SubAppTypeIdOf<MainSubApp>();
    const SubAppTypeId id2 = SubAppTypeIdOf<RenderSubApp>();
    const SubAppTypeId id3 = SubAppTypeIdOf<PhysicsSubApp>();

    CHECK_NE(id1, id2);
    CHECK_NE(id2, id3);
    CHECK_NE(id1, id3);
  }

  TEST_CASE("SubAppTypeIdOf: Returns Consistent IDs") {
    const SubAppTypeId id1 = SubAppTypeIdOf<MainSubApp>();
    const SubAppTypeId id2 = SubAppTypeIdOf<MainSubApp>();

    CHECK_EQ(id1, id2);
  }

  TEST_CASE("SubAppNameOf: Returns Custom Name For Named SubApp") {
    const std::string_view name = SubAppNameOf<RenderSubApp>();
    CHECK_EQ(name, "RenderSubApp");
  }

  TEST_CASE("SubAppNameOf: Returns Type Name For Unnamed SubApp") {
    const std::string_view name = SubAppNameOf<MainSubApp>();
    CHECK_FALSE(name.empty());
  }

  TEST_CASE("SubAppNameOf: Different SubApps Have Different Names") {
    const std::string_view name1 = SubAppNameOf<MainSubApp>();
    const std::string_view name2 = SubAppNameOf<RenderSubApp>();
    const std::string_view name3 = SubAppNameOf<PhysicsSubApp>();

    CHECK_NE(name1, name2);
    CHECK_NE(name2, name3);
    CHECK_NE(name1, name3);
  }

  TEST_CASE("SubApp::ctor: Default Overlapping Updates Is False") {
    SubApp sub_app;
    CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("SubApp::ctor: SetAllowOverlappingUpdates") {
    SubApp sub_app;
    CHECK_FALSE(sub_app.AllowsOverlappingUpdates());

    sub_app.SetAllowOverlappingUpdates(true);
    CHECK(sub_app.AllowsOverlappingUpdates());

    sub_app.SetAllowOverlappingUpdates(false);
    CHECK_FALSE(sub_app.AllowsOverlappingUpdates());
  }

  TEST_CASE("SubApp::ctor: IsUpdating Initially False") {
    SubApp sub_app;
    CHECK_FALSE(sub_app.IsUpdating());
  }

  TEST_CASE("SubAppWithAsyncTrait: Valid Async SubApp") {
    CHECK(SubAppWithAsyncTrait<AsyncRenderSubApp>);
    CHECK(SubAppWithAsyncTrait<SyncPhysicsSubApp>);
  }

  TEST_CASE("SubAppWithAsyncTrait: Non-Async SubApp") {
    CHECK_FALSE(SubAppWithAsyncTrait<MainSubApp>);
    CHECK_FALSE(SubAppWithAsyncTrait<RenderSubApp>);
    CHECK_FALSE(SubAppWithAsyncTrait<PhysicsSubApp>);
  }

  TEST_CASE("SubAppAllowsOverlappingUpdates: Returns True For Async SubApp") {
    constexpr bool allows = SubAppAllowsOverlappingUpdates<AsyncRenderSubApp>();
    CHECK(allows);
  }

  TEST_CASE("SubAppAllowsOverlappingUpdates: Returns False For Sync SubApp") {
    constexpr bool allows = SubAppAllowsOverlappingUpdates<SyncPhysicsSubApp>();
    CHECK_FALSE(allows);
  }

  TEST_CASE("SubAppAllowsOverlappingUpdates: Returns False For Default SubApp") {
    constexpr bool allows1 = SubAppAllowsOverlappingUpdates<MainSubApp>();
    constexpr bool allows2 = SubAppAllowsOverlappingUpdates<RenderSubApp>();
    constexpr bool allows3 = SubAppAllowsOverlappingUpdates<PhysicsSubApp>();

    CHECK_FALSE(allows1);
    CHECK_FALSE(allows2);
    CHECK_FALSE(allows3);
  }

  TEST_CASE("SubApp::ctor: HasEvent") {
    SubApp sub_app;

    SUBCASE("HasEvent Returns False Before Event Registration") {
      CHECK_FALSE(sub_app.HasEvent<TestEvent>());
      CHECK_FALSE(sub_app.HasEvent<AnotherTestEvent>());
    }

    SUBCASE("HasEvent Returns True After Event Registration") {
      sub_app.AddEvent<TestEvent>();
      CHECK(sub_app.HasEvent<TestEvent>());
      CHECK_FALSE(sub_app.HasEvent<AnotherTestEvent>());

      sub_app.AddEvent<AnotherTestEvent>();
      CHECK(sub_app.HasEvent<TestEvent>());
      CHECK(sub_app.HasEvent<AnotherTestEvent>());
    }

    SUBCASE("HasEvent After Clear") {
      sub_app.AddEvent<TestEvent>();
      sub_app.AddEvent<AnotherTestEvent>();
      CHECK(sub_app.HasEvent<TestEvent>());
      CHECK(sub_app.HasEvent<AnotherTestEvent>());

      sub_app.Clear();
      CHECK_FALSE(sub_app.HasEvent<TestEvent>());
      CHECK_FALSE(sub_app.HasEvent<AnotherTestEvent>());
    }

    SUBCASE("HasEvent Independent Of World State") {
      sub_app.AddEvent<TestEvent>();
      CHECK(sub_app.HasEvent<TestEvent>());

      // Event registration persists independently of world state
      CHECK(sub_app.HasEvent<TestEvent>());
    }
  }
}
