#include <doctest/doctest.h>

#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_config.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/ecs/system.hpp>

using namespace helios::app;
using namespace helios::ecs;

namespace {

// Test system sets
struct PhysicsSet {};
struct RenderSet {};
struct GameplaySet {};
struct InputSet {};

// Test systems
struct InputSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "InputSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

struct MovementSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "MovementSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

struct CollisionSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "CollisionSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

struct PhysicsSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "PhysicsSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

struct RenderSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "RenderSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

struct AISystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "AISystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

struct CombatSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "CombatSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

}  // namespace

TEST_SUITE("app::SystemConfig") {
  TEST_CASE("SystemConfig::SystemConfig: basic construction") {
    SubApp sub_app;

    // Should compile and create builder
    auto config = sub_app.AddSystemBuilder<InputSystem>(kUpdate);

    // Verify it exists (implicit through compilation)
    CHECK(true);
  }

  TEST_CASE("SystemConfig::SystemConfig: multiple systems builder") {
    SubApp sub_app;

    // Should compile with multiple systems
    auto config = sub_app.AddSystemsBuilder<MovementSystem, CollisionSystem>(kUpdate);

    CHECK(true);
  }

  TEST_CASE("SystemConfig::After: constraint") {
    SubApp sub_app;

    sub_app.AddSystem<InputSystem>(kUpdate);

    // Should compile with After constraint
    sub_app.AddSystemBuilder<MovementSystem>(kUpdate).After<InputSystem>();

    CHECK(sub_app.ContainsSystem<InputSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<MovementSystem>(kUpdate));
  }

  TEST_CASE("SystemConfig::Before: constraint") {
    SubApp sub_app;

    sub_app.AddSystem<RenderSystem>(kUpdate);

    // Should compile with Before constraint
    sub_app.AddSystemBuilder<PhysicsSystem>(kUpdate).Before<RenderSystem>();

    CHECK(sub_app.ContainsSystem<PhysicsSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<RenderSystem>(kUpdate));
  }

  TEST_CASE("SystemConfig::InSet: adds to set") {
    SubApp sub_app;

    // Should compile with InSet
    sub_app.AddSystemBuilder<PhysicsSystem>(kUpdate).InSet<PhysicsSet>();

    CHECK(sub_app.ContainsSystem<PhysicsSystem>(kUpdate));
  }

  TEST_CASE("SystemConfig::Sequence: creates sequential dependencies") {
    SubApp sub_app;

    // Add systems in sequence
    sub_app.AddSystemsBuilder<MovementSystem, CollisionSystem, PhysicsSystem>(kUpdate).Sequence();

    // All systems should be added
    CHECK(sub_app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<CollisionSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<PhysicsSystem>(kUpdate));
  }

  TEST_CASE("SystemConfig::Sequence: complex chaining") {
    SubApp sub_app;

    sub_app.AddSystem<InputSystem>(kUpdate);
    sub_app.AddSystem<RenderSystem>(kUpdate);

    // Complex configuration with chaining
    sub_app.AddSystemsBuilder<MovementSystem, CollisionSystem>(kUpdate)
        .After<InputSystem>()
        .Before<RenderSystem>()
        .InSet<PhysicsSet>()
        .Sequence();

    CHECK(sub_app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<CollisionSystem>(kUpdate));
  }

  TEST_CASE("SystemConfig::InSet: multiple sets") {
    SubApp sub_app;

    // System can belong to multiple sets
    sub_app.AddSystemBuilder<PhysicsSystem>(kUpdate).InSet<PhysicsSet>().InSet<GameplaySet>();

    CHECK(sub_app.ContainsSystem<PhysicsSystem>(kUpdate));
  }

  TEST_CASE("SystemConfig::InSet: multiple systems with multiple sets") {
    SubApp sub_app;

    // Multiple systems in multiple sets
    sub_app.AddSystemsBuilder<AISystem, CombatSystem>(kUpdate).InSet<GameplaySet>().InSet<PhysicsSet>().Sequence();

    CHECK(sub_app.ContainsSystem<AISystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<CombatSystem>(kUpdate));
  }
}

TEST_SUITE("app::SystemConfig Integration") {
  TEST_CASE("SystemConfig: complete physics pipeline") {
    SubApp sub_app;

    // Configure sets
    sub_app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>().Before<RenderSet>();

    // Add input system
    sub_app.AddSystemBuilder<InputSystem>(kUpdate).InSet<InputSet>();

    // Add physics systems with sequence
    sub_app.AddSystemsBuilder<MovementSystem, CollisionSystem, PhysicsSystem>(kUpdate).InSet<PhysicsSet>().Sequence();

    // Add render system
    sub_app.AddSystemBuilder<RenderSystem>(kUpdate).InSet<RenderSet>();

    // Verify all systems added
    CHECK(sub_app.ContainsSystem<InputSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<MovementSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<CollisionSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<PhysicsSystem>(kUpdate));
    CHECK(sub_app.ContainsSystem<RenderSystem>(kUpdate));

    CHECK_EQ(sub_app.SystemCount(kUpdate), 5);
  }

  TEST_CASE("SystemConfig: mixed old and new API") {
    SubApp sub_app;

    // Old API
    sub_app.AddSystem<InputSystem>(kUpdate);

    // New API
    sub_app.AddSystemBuilder<MovementSystem>(kUpdate).After<InputSystem>();

    // Old API
    sub_app.AddSystem<RenderSystem>(kUpdate);

    // New API with chaining
    sub_app.AddSystemsBuilder<CollisionSystem, PhysicsSystem>(kUpdate)
        .After<MovementSystem>()
        .Before<RenderSystem>()
        .Sequence();

    CHECK_EQ(sub_app.SystemCount(kUpdate), 5);
  }
}
