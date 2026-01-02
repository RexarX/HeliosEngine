#include <doctest/doctest.h>

#include <helios/core/ecs/details/event_manager.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/events/builtin_events.hpp>

#include <array>
#include <string_view>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Test event types
struct TestEvent {
  int value = 0;

  constexpr bool operator==(const TestEvent& other) const noexcept = default;
};

struct AnotherEvent {
  float data = 0.0F;

  constexpr bool operator==(const AnotherEvent& other) const noexcept = default;
};

struct CustomNameEvent {
  int id = 0;

  static constexpr std::string_view GetName() noexcept { return "CustomNameEvent"; }

  constexpr bool operator==(const CustomNameEvent& other) const noexcept = default;
};

struct ManualClearEvent {
  int data = 0;

  static constexpr EventClearPolicy GetClearPolicy() noexcept { return EventClearPolicy::kManual; }

  constexpr bool operator==(const ManualClearEvent& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::details::EventManager") {
  TEST_CASE("EventManager::EventManager: construction and initialization") {
    EventManager manager;

    CHECK(manager.Empty());
    CHECK_EQ(manager.GetCurrentFrame(), 0);
    CHECK_EQ(manager.RegisteredEventCount(), 0);
  }

  TEST_CASE("EventManager::RegisterEvent") {
    EventManager manager;

    SUBCASE("Register single event") {
      manager.RegisterEvent<TestEvent>();

      CHECK(manager.IsRegistered<TestEvent>());
      CHECK_EQ(manager.RegisteredEventCount(), 1);
    }

    SUBCASE("Register multiple events") {
      manager.RegisterEvent<TestEvent>();
      manager.RegisterEvent<AnotherEvent>();

      CHECK(manager.IsRegistered<TestEvent>());
      CHECK(manager.IsRegistered<AnotherEvent>());
      CHECK_EQ(manager.RegisteredEventCount(), 2);
    }

    SUBCASE("Register events in batch") {
      manager.RegisterEvents<TestEvent, AnotherEvent, CustomNameEvent>();

      CHECK(manager.IsRegistered<TestEvent>());
      CHECK(manager.IsRegistered<AnotherEvent>());
      CHECK(manager.IsRegistered<CustomNameEvent>());
      CHECK_EQ(manager.RegisteredEventCount(), 3);
    }

    SUBCASE("Register with clear policy") {
      manager.RegisterEvent<TestEvent>();
      manager.RegisterEvent<ManualClearEvent>();

      const auto* metadata1 = manager.GetMetadata<TestEvent>();
      const auto* metadata2 = manager.GetMetadata<ManualClearEvent>();

      REQUIRE(metadata1 != nullptr);
      REQUIRE(metadata2 != nullptr);
      CHECK_EQ(metadata1->clear_policy, EventClearPolicy::kAutomatic);
      CHECK_EQ(metadata2->clear_policy, EventClearPolicy::kManual);
    }

    SUBCASE("Register builtin event") {
      manager.RegisterEvent<EntitySpawnedEvent>();

      const auto* metadata = manager.GetMetadata<EntitySpawnedEvent>();
      REQUIRE(metadata != nullptr);
      CHECK_EQ(metadata->clear_policy, EventClearPolicy::kAutomatic);
    }
  }

  TEST_CASE("EventManager::GetEventMetadata") {
    EventManager manager;

    SUBCASE("Get metadata for registered event") {
      manager.RegisterEvent<TestEvent>();

      const auto* metadata = manager.GetMetadata<TestEvent>();
      REQUIRE(metadata != nullptr);
      CHECK_EQ(metadata->type_id, EventTypeIdOf<TestEvent>());
      CHECK_EQ(metadata->clear_policy, EventClearPolicy::kAutomatic);
    }

    SUBCASE("Get metadata for unregistered event") {
      const auto* metadata = manager.GetMetadata<TestEvent>();
      CHECK_EQ(metadata, nullptr);
    }

    SUBCASE("Metadata tracks registration frame") {
      manager.RegisterEvent<TestEvent>();
      manager.Update();
      manager.RegisterEvent<AnotherEvent>();

      const auto* metadata1 = manager.GetMetadata<TestEvent>();
      const auto* metadata2 = manager.GetMetadata<AnotherEvent>();

      REQUIRE(metadata1 != nullptr);
      REQUIRE(metadata2 != nullptr);
      CHECK_EQ(metadata1->frame_registered, 0);
      CHECK_EQ(metadata2->frame_registered, 1);
    }
  }

  TEST_CASE("EventManager: write and read events") {
    EventManager manager;
    manager.RegisterEvent<TestEvent>();

    SUBCASE("Write and read single event") {
      manager.Write(TestEvent{42});

      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 1);
      CHECK_EQ(events[0].value, 42);
    }

    SUBCASE("Write multiple events") {
      manager.Write(TestEvent{10});
      manager.Write(TestEvent{20});
      manager.Write(TestEvent{30});

      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 3);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
    }

    SUBCASE("Write bulk events") {
      std::vector<TestEvent> to_write = {{1}, {2}, {3}, {4}, {5}};
      manager.WriteBulk(to_write);

      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 5);
      for (size_t i = 0; i < events.size(); ++i) {
        CHECK_EQ(events[i].value, static_cast<int>(i + 1));
      }
    }

    SUBCASE("ReadInto with back_inserter") {
      manager.Write(TestEvent{100});
      manager.Write(TestEvent{200});

      std::vector<TestEvent> events;
      manager.ReadInto<TestEvent>(std::back_inserter(events));

      REQUIRE_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 100);
      CHECK_EQ(events[1].value, 200);
    }
  }

  TEST_CASE("EventManager: double buffering") {
    EventManager manager;
    manager.RegisterEvent<TestEvent>();

    SUBCASE("Events written in frame 0, read in frame 1") {
      // Frame 0: Write events
      manager.Write(TestEvent{10});
      CHECK_EQ(manager.GetCurrentFrame(), 0);

      // Update to frame 1 (swap buffers)
      manager.Update();
      CHECK_EQ(manager.GetCurrentFrame(), 1);

      // Frame 1: Read events from previous frame
      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 1);
      CHECK_EQ(events[0].value, 10);
    }

    SUBCASE("Events persist for one full update cycle") {
      // Frame 0: Write
      manager.Write(TestEvent{1});
      manager.Update();

      // Frame 1: Read (should have frame 0 events in previous queue)
      CHECK_EQ(manager.Read<TestEvent>().size(), 1);
      manager.Write(TestEvent{2});
      manager.Update();

      // Frame 2: Read (should have only frame 1 events, frame 0 cleared)
      auto events = manager.Read<TestEvent>();
      CHECK_EQ(events.size(), 1);
      CHECK_EQ(events[0].value, 2);
    }

    SUBCASE("Reading from both current and previous queues") {
      // Frame 0
      manager.Write(TestEvent{100});
      manager.Update();

      // Frame 1
      manager.Write(TestEvent{200});

      // Should read from both previous (100) and current (200)
      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 100);  // From previous
      CHECK_EQ(events[1].value, 200);  // From current
    }
  }

  TEST_CASE("EventManager::ClearEvents: manual") {
    EventManager manager;

    SUBCASE("Manual clear for manually-managed events") {
      manager.RegisterEvent<ManualClearEvent>();

      manager.Write(ManualClearEvent{1});
      manager.Write(ManualClearEvent{2});

      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 2);

      manager.ManualClear<ManualClearEvent>();

      CHECK(manager.Read<ManualClearEvent>().empty());
    }

    SUBCASE("Manual clear removes from both queues") {
      manager.RegisterEvent<ManualClearEvent>();

      manager.Write(ManualClearEvent{1});
      manager.Update();
      manager.Write(ManualClearEvent{2});

      // Events in both current and previous queues
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 2);

      manager.ManualClear<ManualClearEvent>();

      CHECK(manager.Read<ManualClearEvent>().empty());
    }
  }

  TEST_CASE("EventManager::MergeLocalEvents") {
    EventManager manager;
    manager.RegisterEvent<TestEvent>();

    EventQueue queue;
    queue.Register<TestEvent>();
    queue.Write(TestEvent{10});
    queue.Write(TestEvent{20});

    SUBCASE("Merge events from external queue") {
      manager.Merge(queue);

      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
    }

    SUBCASE("Merge with existing events") {
      manager.Write(TestEvent{5});

      manager.Merge(queue);

      auto events = manager.Read<TestEvent>();
      REQUIRE_EQ(events.size(), 3);
      CHECK_EQ(events[0].value, 5);
      CHECK_EQ(events[1].value, 10);
      CHECK_EQ(events[2].value, 20);
    }
  }

  TEST_CASE("EventManager::HasEvents") {
    EventManager manager;
    manager.RegisterEvent<TestEvent>();
    manager.RegisterEvent<AnotherEvent>();

    SUBCASE("HasEvents returns false when empty") {
      CHECK_FALSE(manager.HasEvents<TestEvent>());
    }

    SUBCASE("HasEvents returns true after write") {
      manager.Write(TestEvent{1});

      CHECK(manager.HasEvents<TestEvent>());
      CHECK_FALSE(manager.HasEvents<AnotherEvent>());
    }

    SUBCASE("HasEvents checks both queues") {
      manager.Write(TestEvent{1});
      manager.Update();

      // Event now in previous queue
      CHECK(manager.HasEvents<TestEvent>());

      manager.Write(TestEvent{2});

      // Events in both queues
      CHECK(manager.HasEvents<TestEvent>());
    }
  }

  TEST_CASE("EventManager::Empty") {
    EventManager manager;
    manager.RegisterEvent<TestEvent>();

    SUBCASE("Empty on construction") {
      CHECK(manager.Empty());
    }

    SUBCASE("Not empty after write") {
      manager.Write(TestEvent{1});
      CHECK_FALSE(manager.Empty());
    }

    SUBCASE("Empty after clear") {
      manager.Write(TestEvent{1});
      manager.Clear();
      CHECK(manager.Empty());
    }
  }

  TEST_CASE("EventManager: multiple event types") {
    EventManager manager;
    manager.RegisterEvent<ManualClearEvent>();  // Manual clear policy for lifecycle test
    manager.RegisterEvent<TestEvent>();
    manager.RegisterEvent<CustomNameEvent>();

    SUBCASE("Write and read different event types independently") {
      manager.Write(ManualClearEvent{10});
      manager.Write(TestEvent{20});
      manager.Write(CustomNameEvent{100});

      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);
      CHECK_EQ(manager.Read<TestEvent>().size(), 1);
      CHECK_EQ(manager.Read<CustomNameEvent>().size(), 1);

      CHECK_EQ(manager.Read<ManualClearEvent>()[0].data, 10);
      CHECK_EQ(manager.Read<TestEvent>()[0].value, 20);
      CHECK_EQ(manager.Read<CustomNameEvent>()[0].id, 100);
    }

    SUBCASE("Different event types have independent lifecycles") {
      manager.Write(ManualClearEvent{1});
      manager.Update();

      // ManualClearEvent is now in previous queue and readable
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      manager.Write(TestEvent{20});
      manager.Update();

      // With kManual policy, ManualClearEvent persists even after 2 updates
      // TestEvent is readable after 1 update
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);
      CHECK_EQ(manager.Read<TestEvent>().size(), 1);

      // Another update - ManualClearEvent persists (kManual), TestEvent cleared (kAutomatic)
      manager.Update();
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);
      CHECK_EQ(manager.Read<TestEvent>().size(), 0);

      // Manual clear should remove ManualClearEvent
      manager.ManualClear<ManualClearEvent>();
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 0);
    }
  }

  TEST_CASE("EventManager::AdvanceFrame") {
    EventManager manager;

    SUBCASE("Frame counter increments on update") {
      CHECK_EQ(manager.GetCurrentFrame(), 0);

      manager.Update();
      CHECK_EQ(manager.GetCurrentFrame(), 1);

      manager.Update();
      CHECK_EQ(manager.GetCurrentFrame(), 2);
    }

    SUBCASE("Frame counter resets on clear") {
      manager.Update();
      manager.Update();
      CHECK_EQ(manager.GetCurrentFrame(), 2);

      manager.Clear();
      CHECK_EQ(manager.GetCurrentFrame(), 0);
    }
  }

  TEST_CASE("EventManager: clear policy behavior") {
    EventManager manager;

    SUBCASE("Events with kAutomatic policy are cleared after double buffer cycle") {
      manager.RegisterEvent<TestEvent>();  // kAutomatic policy

      manager.Write(TestEvent{42});
      CHECK_EQ(manager.Read<TestEvent>().size(), 1);

      manager.Update();  // Frame 1: Event in previous queue, still readable
      CHECK_EQ(manager.Read<TestEvent>().size(), 1);
      CHECK_EQ(manager.Read<TestEvent>()[0].value, 42);

      manager.Update();  // Frame 2: Event cleared from previous queue
      CHECK_EQ(manager.Read<TestEvent>().size(), 0);
    }

    SUBCASE("Events with kManual policy persist indefinitely") {
      manager.RegisterEvent<ManualClearEvent>();  // kManual policy

      manager.Write(ManualClearEvent{100});
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      manager.Update();  // Frame 1
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      manager.Update();  // Frame 2
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      manager.Update();  // Frame 3
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      // Still present after many frames
      CHECK_EQ(manager.Read<ManualClearEvent>()[0].data, 100);
    }

    SUBCASE("Mixed clear policy behavior with multiple event types") {
      manager.RegisterEvent<TestEvent>();         // kAutomatic policy
      manager.RegisterEvent<ManualClearEvent>();  // kManual policy

      manager.Write(TestEvent{1});
      manager.Write(ManualClearEvent{2});

      manager.Update();  // Frame 1: Both readable
      CHECK_EQ(manager.Read<TestEvent>().size(), 1);
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      manager.Update();  // Frame 2: TestEvent cleared, ManualClearEvent persists
      CHECK_EQ(manager.Read<TestEvent>().size(), 0);
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);
      CHECK_EQ(manager.Read<ManualClearEvent>()[0].data, 2);

      manager.Update();  // Frame 3: TestEvent still gone, ManualClearEvent still persists
      CHECK_EQ(manager.Read<TestEvent>().size(), 0);
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);
    }

    SUBCASE("Manual clear works for kManual policy events") {
      manager.RegisterEvent<ManualClearEvent>();

      manager.Write(ManualClearEvent{77});
      manager.Update();
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 1);

      manager.ManualClear<ManualClearEvent>();
      CHECK_EQ(manager.Read<ManualClearEvent>().size(), 0);
    }

    SUBCASE("Multiple events with kManual policy accumulate") {
      manager.RegisterEvent<ManualClearEvent>();

      manager.Write(ManualClearEvent{1});
      manager.Update();
      manager.Write(ManualClearEvent{2});
      manager.Update();
      manager.Write(ManualClearEvent{3});

      auto events = manager.Read<ManualClearEvent>();
      CHECK_EQ(events.size(), 3);

      // Verify all three events are present
      bool has_1 = false, has_2 = false, has_3 = false;
      for (const auto& e : events) {
        if (e.data == 1)
          has_1 = true;
        if (e.data == 2)
          has_2 = true;
        if (e.data == 3)
          has_3 = true;
      }
      CHECK(has_1);
      CHECK(has_2);
      CHECK(has_3);
    }
  }

  TEST_CASE("EventManager: clear policy game state events") {
    EventManager manager;

    // Simulate a game scenario where some events should persist (game state changes)
    // while others should be cleared each frame (input events, collisions)

    struct PlayerLevelUpEvent {
      int new_level = 0;
      int exp_gained = 0;

      static constexpr EventClearPolicy GetClearPolicy() noexcept { return EventClearPolicy::kManual; }
    };

    struct CollisionEvent {
      int entity_a = 0;
      int entity_b = 0;
    };

    struct InputEvent {
      int key_code = 0;
    };

    // Game state events persist until explicitly acknowledged (kManual policy)
    manager.RegisterEvent<PlayerLevelUpEvent>();

    // Transient events cleared automatically each frame (kAutomatic policy)
    manager.RegisterEvent<CollisionEvent>();
    manager.RegisterEvent<InputEvent>();

    SUBCASE("Game state events persist across frames for UI systems") {
      // Frame 1: Player levels up
      manager.Write(PlayerLevelUpEvent{5, 1000});
      manager.Write(CollisionEvent{1, 2});
      manager.Write(InputEvent{32});  // Space key

      CHECK_EQ(manager.Read<PlayerLevelUpEvent>().size(), 1);
      CHECK_EQ(manager.Read<CollisionEvent>().size(), 1);
      CHECK_EQ(manager.Read<InputEvent>().size(), 1);

      // Frame 2: After update, auto_clear events stay for one frame, then cleared
      manager.Update();

      // Old collision and input are in previous_queue (readable for one more frame)
      CHECK_EQ(manager.Read<PlayerLevelUpEvent>().size(), 1);  // Still present!
      CHECK_EQ(manager.Read<CollisionEvent>().size(), 1);      // Old collision still readable
      CHECK_EQ(manager.Read<InputEvent>().size(), 1);          // Old input still readable

      // Write new events
      manager.Write(CollisionEvent{3, 4});                 // New collision
      CHECK_EQ(manager.Read<CollisionEvent>().size(), 2);  // Old + new collision

      // Frame 3: Old auto_clear events cleared, level up still persists
      manager.Update();

      CHECK_EQ(manager.Read<PlayerLevelUpEvent>().size(), 1);  // Still present!
      CHECK_EQ(manager.Read<CollisionEvent>().size(), 1);      // Only frame 2 collision
      CHECK_EQ(manager.Read<InputEvent>().size(), 0);          // Frame 1 input cleared

      // Frame 4: UI system processes level up and clears it
      auto level_ups = manager.Read<PlayerLevelUpEvent>();
      REQUIRE_EQ(level_ups.size(), 1);
      CHECK_EQ(level_ups[0].new_level, 5);
      CHECK_EQ(level_ups[0].exp_gained, 1000);

      // UI acknowledges the event
      manager.ManualClear<PlayerLevelUpEvent>();

      manager.Update();
      CHECK_EQ(manager.Read<PlayerLevelUpEvent>().size(), 0);
      CHECK_EQ(manager.Read<CollisionEvent>().size(), 0);  // Frame 2 collision now cleared
    }

    SUBCASE("Multiple game state events accumulate until processed") {
      // Simulate multiple level ups before UI system processes them
      manager.Write(PlayerLevelUpEvent{2, 500});
      manager.Update();

      manager.Write(PlayerLevelUpEvent{3, 700});
      manager.Update();

      manager.Write(PlayerLevelUpEvent{4, 900});

      auto level_ups = manager.Read<PlayerLevelUpEvent>();
      CHECK_EQ(level_ups.size(), 3);

      // Process all level ups at once
      int total_exp = 0;
      for (const auto& event : level_ups) {
        total_exp += event.exp_gained;
      }
      CHECK_EQ(total_exp, 2100);

      // Clear after processing
      manager.ManualClear<PlayerLevelUpEvent>();
      CHECK_EQ(manager.Read<PlayerLevelUpEvent>().size(), 0);
    }
  }

  TEST_CASE("EventManager::ClearAllEvents") {
    EventManager manager;
    manager.RegisterEvent<TestEvent>();
    manager.RegisterEvent<AnotherEvent>();

    manager.Write(TestEvent{1});
    manager.Write(AnotherEvent{2.0F});
    manager.Update();

    SUBCASE("Clear removes all events and registrations") {
      CHECK_EQ(manager.RegisteredEventCount(), 2);
      CHECK_FALSE(manager.Empty());

      manager.Clear();

      CHECK_EQ(manager.RegisteredEventCount(), 0);
      CHECK(manager.Empty());
      CHECK_FALSE(manager.IsRegistered<TestEvent>());
      CHECK_FALSE(manager.IsRegistered<AnotherEvent>());
    }
  }

  TEST_CASE("EventManager: builtin events") {
    EventManager manager;

    SUBCASE("Register built-in events") {
      manager.RegisterEvent<EntitySpawnedEvent>();
      manager.RegisterEvent<EntityDestroyedEvent>();

      CHECK(manager.IsRegistered<EntitySpawnedEvent>());
      CHECK(manager.IsRegistered<EntityDestroyedEvent>());

      const auto* metadata1 = manager.GetMetadata<EntitySpawnedEvent>();
      const auto* metadata2 = manager.GetMetadata<EntityDestroyedEvent>();

      REQUIRE(metadata1 != nullptr);
      REQUIRE(metadata2 != nullptr);
      CHECK_EQ(metadata1->clear_policy, EventClearPolicy::kAutomatic);
      CHECK_EQ(metadata2->clear_policy, EventClearPolicy::kAutomatic);
    }
  }

}  // TEST_SUITE
