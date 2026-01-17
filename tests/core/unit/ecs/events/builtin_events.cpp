#include <doctest/doctest.h>

#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/events/builtin_events.hpp>
#include <helios/core/ecs/world.hpp>

#include <cstdint>
#include <string_view>
#include <utility>

using namespace helios::ecs;

TEST_SUITE("ecs::BuiltinEvents") {
  // ============================================================================
  // EntitySpawnedEvent Tests
  // ============================================================================

  TEST_CASE("EntitySpawnedEvent: Is valid event trait") {
    CHECK(EventTrait<EntitySpawnedEvent>);
  }

  TEST_CASE("EntitySpawnedEvent: Has correct name") {
    CHECK_EQ(EntitySpawnedEvent::GetName(), "EntitySpawnedEvent");
  }

  TEST_CASE("EntitySpawnedEvent: Uses automatic clear policy") {
    CHECK_EQ(EntitySpawnedEvent::GetClearPolicy(), EventClearPolicy::kAutomatic);
  }

  TEST_CASE("EntitySpawnedEvent: Can store entity") {
    Entity test_entity{42, 0};
    EntitySpawnedEvent event{.entity = test_entity};
    CHECK_EQ(event.entity.Index(), 42U);
  }

  TEST_CASE("EntitySpawnedEvent: Default construction") {
    EntitySpawnedEvent event{};
    CHECK_EQ(event.entity.Index(), Entity::kInvalidIndex);
    CHECK_FALSE(event.entity.Valid());
  }

  TEST_CASE("EntitySpawnedEvent: Is trivially copyable") {
    CHECK(std::is_trivially_copyable_v<EntitySpawnedEvent>);
  }

  TEST_CASE("EntitySpawnedEvent: Can be emitted and read") {
    World world;
    world.AddEvent<EntitySpawnedEvent>();

    // Emit event
    const Entity entity1{100, 0};
    world.WriteEvents<EntitySpawnedEvent>().Write(EntitySpawnedEvent{.entity = entity1});

    // Read event
    auto reader = world.ReadEvents<EntitySpawnedEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 1);
    CHECK_EQ(events[0].entity.Index(), 100U);
  }

  TEST_CASE("EntitySpawnedEvent: Multiple events can be stored") {
    World world;
    world.AddEvent<EntitySpawnedEvent>();

    // Emit multiple events
    for (int i = 0; i < 10; ++i) {
      world.WriteEvents<EntitySpawnedEvent>().Write(EntitySpawnedEvent{.entity = Entity{static_cast<uint32_t>(i), 0}});
    }

    // Read events
    auto reader = world.ReadEvents<EntitySpawnedEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 10);

    for (int i = 0; i < 10; ++i) {
      CHECK_EQ(events[i].entity.Index(), static_cast<uint32_t>(i));
    }
  }

  // ============================================================================
  // EntityDestroyedEvent Tests
  // ============================================================================

  TEST_CASE("EntityDestroyedEvent: Is valid event trait") {
    CHECK(EventTrait<EntityDestroyedEvent>);
  }

  TEST_CASE("EntityDestroyedEvent: Has correct name") {
    CHECK_EQ(EntityDestroyedEvent::GetName(), "EntityDestroyedEvent");
  }

  TEST_CASE("EntityDestroyedEvent: Uses automatic clear policy") {
    CHECK_EQ(EntityDestroyedEvent::GetClearPolicy(), EventClearPolicy::kAutomatic);
  }

  TEST_CASE("EntityDestroyedEvent: Can store entity") {
    Entity test_entity{99, 0};
    EntityDestroyedEvent event{.entity = test_entity};
    CHECK_EQ(event.entity.Index(), 99U);
  }

  TEST_CASE("EntityDestroyedEvent: Default construction") {
    EntityDestroyedEvent event{};
    CHECK_EQ(event.entity.Index(), Entity::kInvalidIndex);
    CHECK_FALSE(event.entity.Valid());
  }

  TEST_CASE("EntityDestroyedEvent: Is trivially copyable") {
    CHECK(std::is_trivially_copyable_v<EntityDestroyedEvent>);
  }

  TEST_CASE("EntityDestroyedEvent: Can be emitted and read") {
    World world;
    world.AddEvent<EntityDestroyedEvent>();

    // Emit event
    const Entity entity1{50, 0};
    world.WriteEvents<EntityDestroyedEvent>().Write(EntityDestroyedEvent{.entity = entity1});

    // Read event
    auto reader = world.ReadEvents<EntityDestroyedEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 1);
    CHECK_EQ(events[0].entity.Index(), 50U);
  }

  TEST_CASE("EntityDestroyedEvent: Multiple events can be stored") {
    World world;
    world.AddEvent<EntityDestroyedEvent>();

    // Emit multiple events
    for (int i = 0; i < 5; ++i) {
      world.WriteEvents<EntityDestroyedEvent>().Write(
          EntityDestroyedEvent{.entity = Entity{static_cast<uint32_t>(i), 0}});
    }

    // Read events
    auto reader = world.ReadEvents<EntityDestroyedEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 5);

    for (int i = 0; i < 5; ++i) {
      CHECK_EQ(events[i].entity.Index(), static_cast<uint32_t>(i));
    }
  }

  // ============================================================================
  // ShutdownExitCode Tests
  // ============================================================================

  TEST_CASE("ShutdownExitCode::k: Success value is 0") {
    CHECK_EQ(static_cast<uint8_t>(ShutdownExitCode::kSuccess), 0U);
  }

  TEST_CASE("ShutdownExitCode::k: Failure value is 1") {
    CHECK_EQ(static_cast<uint8_t>(ShutdownExitCode::kFailure), 1U);
  }

  TEST_CASE("ShutdownExitCode::k: Can be compared") {
    constexpr auto code1 = ShutdownExitCode::kSuccess;
    constexpr auto code2 = ShutdownExitCode::kSuccess;
    CHECK_EQ(code1, code2);
  }

  TEST_CASE("ShutdownExitCode::k: Different values are not equal") {
    constexpr auto code1 = ShutdownExitCode::kSuccess;
    constexpr auto code2 = ShutdownExitCode::kFailure;
    CHECK_NE(code1, code2);
  }

  // ============================================================================
  // ShutdownEvent Tests
  // ============================================================================

  TEST_CASE("ShutdownEvent: Is valid event trait") {
    CHECK(EventTrait<ShutdownEvent>);
  }

  TEST_CASE("ShutdownEvent: Has correct name") {
    CHECK_EQ(ShutdownEvent::GetName(), "ShutdownEvent");
  }

  TEST_CASE("ShutdownEvent: Uses manual clear policy") {
    CHECK_EQ(ShutdownEvent::GetClearPolicy(), EventClearPolicy::kManual);
  }

  TEST_CASE("ShutdownEvent: Default construction uses success exit code") {
    ShutdownEvent event{};
    CHECK_EQ(event.exit_code, ShutdownExitCode::kSuccess);
  }

  TEST_CASE("ShutdownEvent: Can set exit code") {
    ShutdownEvent event{.exit_code = ShutdownExitCode::kFailure};
    CHECK_EQ(event.exit_code, ShutdownExitCode::kFailure);
  }

  TEST_CASE("ShutdownEvent: Is trivially copyable") {
    CHECK(std::is_trivially_copyable_v<ShutdownEvent>);
  }

  TEST_CASE("ShutdownEvent: Can be emitted with success code") {
    World world;
    world.AddEvent<ShutdownEvent>();

    // Emit event with success
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kSuccess});

    // Read event
    auto reader = world.ReadEvents<ShutdownEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 1);
    CHECK_EQ(events[0].exit_code, ShutdownExitCode::kSuccess);
  }

  TEST_CASE("ShutdownEvent: Can be emitted with failure code") {
    World world;
    world.AddEvent<ShutdownEvent>();

    // Emit event with failure
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kFailure});

    // Read event
    auto reader = world.ReadEvents<ShutdownEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 1);
    CHECK_EQ(events[0].exit_code, ShutdownExitCode::kFailure);
  }

  TEST_CASE("ShutdownEvent: Multiple events can be stored") {
    World world;
    world.AddEvent<ShutdownEvent>();

    // Emit multiple events with different codes
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kSuccess});
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kFailure});
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kSuccess});

    // Read events
    auto reader = world.ReadEvents<ShutdownEvent>();
    const auto events = reader.Read();
    REQUIRE_EQ(events.size(), 3);

    CHECK_EQ(events[0].exit_code, ShutdownExitCode::kSuccess);
    CHECK_EQ(events[1].exit_code, ShutdownExitCode::kFailure);
    CHECK_EQ(events[2].exit_code, ShutdownExitCode::kSuccess);
  }

  // ============================================================================
  // Event Trait Concepts Tests
  // ============================================================================

  TEST_CASE("Event traits: EntitySpawnedEvent meets concept requirements") {
    // This test verifies that the event meets all EventTrait requirements
    CHECK(EventTrait<EntitySpawnedEvent>);
    CHECK_EQ(EntitySpawnedEvent::GetName().size(), 18);
    CHECK(EventClearPolicy::kAutomatic == EventClearPolicy::kAutomatic);
  }

  TEST_CASE("Event traits: EntityDestroyedEvent meets concept requirements") {
    // This test verifies that the event meets all EventTrait requirements
    CHECK(EventTrait<EntityDestroyedEvent>);
    CHECK_EQ(EntityDestroyedEvent::GetName().size(), 20);
    CHECK(EventClearPolicy::kAutomatic == EventClearPolicy::kAutomatic);
  }

  TEST_CASE("Event traits: ShutdownEvent meets concept requirements") {
    // This test verifies that the event meets all EventTrait requirements
    CHECK(EventTrait<ShutdownEvent>);
    CHECK_EQ(ShutdownEvent::GetName().size(), 13);
    CHECK(EventClearPolicy::kManual == EventClearPolicy::kManual);
  }

  // ============================================================================
  // Cross-Event Tests
  // ============================================================================

  TEST_CASE("Multiple event types can coexist in world") {
    World world;
    world.AddEvent<EntitySpawnedEvent>();
    world.AddEvent<EntityDestroyedEvent>();
    world.AddEvent<ShutdownEvent>();

    // Emit events of different types
    world.WriteEvents<EntitySpawnedEvent>().Write(EntitySpawnedEvent{.entity = Entity{1, 1}});
    world.WriteEvents<EntityDestroyedEvent>().Write(EntityDestroyedEvent{.entity = Entity{2, 1}});
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kSuccess});

    // Read events of different types
    auto spawned_reader = world.ReadEvents<EntitySpawnedEvent>();
    auto destroyed_reader = world.ReadEvents<EntityDestroyedEvent>();
    auto shutdown_reader = world.ReadEvents<ShutdownEvent>();
    const auto spawned = spawned_reader.Read();
    const auto destroyed = destroyed_reader.Read();
    const auto shutdown = shutdown_reader.Read();

    REQUIRE_EQ(spawned.size(), 1);
    REQUIRE_EQ(destroyed.size(), 1);
    REQUIRE_EQ(shutdown.size(), 1);

    CHECK_EQ(spawned[0].entity.Index(), 1U);
    CHECK_EQ(destroyed[0].entity.Index(), 2U);
    CHECK_EQ(shutdown[0].exit_code, ShutdownExitCode::kSuccess);
  }

  TEST_CASE("Clear policy affects event persistence") {
    World world;
    world.AddEvent<EntitySpawnedEvent>();  // kAutomatic
    world.AddEvent<ShutdownEvent>();       // kManual

    // Emit both event types
    world.WriteEvents<EntitySpawnedEvent>().Write(EntitySpawnedEvent{.entity = Entity{1, 1}});
    world.WriteEvents<ShutdownEvent>().Write(ShutdownEvent{.exit_code = ShutdownExitCode::kSuccess});

    // Read events
    {
      auto spawned_reader = world.ReadEvents<EntitySpawnedEvent>();
      auto shutdown_reader = world.ReadEvents<ShutdownEvent>();
      const auto spawned = spawned_reader.Read();
      const auto shutdown = shutdown_reader.Read();

      CHECK_EQ(spawned.size(), 1);
      CHECK_EQ(shutdown.size(), 1);
    }

    // After second read, automatic clear should remove EntitySpawnedEvent but not ShutdownEvent
    {
      auto spawned_reader = world.ReadEvents<EntitySpawnedEvent>();
      auto shutdown_reader = world.ReadEvents<ShutdownEvent>();
      const auto spawned = spawned_reader.Read();
      const auto shutdown = shutdown_reader.Read();

      CHECK_EQ(spawned.size(), 1);   // Automatic events persist in same frame
      CHECK_EQ(shutdown.size(), 1);  // Manual events persist
    }
  }

  TEST_CASE("Event size is reasonable") {
    // Verify that builtin events are small and efficient
    CHECK_LE(sizeof(EntitySpawnedEvent), 16);
    CHECK_LE(sizeof(EntityDestroyedEvent), 16);
    CHECK_LE(sizeof(ShutdownEvent), 8);
  }

  TEST_CASE("Events can be copy constructed") {
    EntitySpawnedEvent event1{.entity = Entity{100, 1}};
    EntitySpawnedEvent event2 = event1;
    CHECK_EQ(event2.entity.Index(), 100U);

    ShutdownEvent shutdown1{.exit_code = ShutdownExitCode::kFailure};
    ShutdownEvent shutdown2 = shutdown1;
    CHECK_EQ(shutdown2.exit_code, ShutdownExitCode::kFailure);
  }

  TEST_CASE("Events can be move constructed") {
    EntitySpawnedEvent event1{.entity = Entity{100, 1}};
    EntitySpawnedEvent event2 = std::move(event1);
    CHECK_EQ(event2.entity.Index(), 100U);

    ShutdownEvent shutdown1{.exit_code = ShutdownExitCode::kFailure};
    ShutdownEvent shutdown2 = std::move(shutdown1);
    CHECK_EQ(shutdown2.exit_code, ShutdownExitCode::kFailure);
  }
}
