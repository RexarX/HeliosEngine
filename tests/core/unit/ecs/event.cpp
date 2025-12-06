#include <doctest/doctest.h>

#include <helios/core/ecs/event.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

using namespace helios::ecs;

namespace {

// Test event types
struct SimpleEvent {
  int value = 0;
};

struct EventWithName {
  int data = 0;

  static constexpr std::string_view GetName() noexcept { return "CustomEventName"; }
};

struct ComplexEvent {
  std::array<char, 64> message = {};
  int code = 0;
  float timestamp = 0.0F;

  explicit ComplexEvent(std::string_view msg = "", int c = 0, float ts = 0.0F) : code(c), timestamp(ts) {
    const auto copy_size = std::min(msg.size(), message.size() - 1);
    std::copy_n(msg.begin(), copy_size, message.begin());
    message[copy_size] = '\0';
  }

  bool operator==(const ComplexEvent& other) const noexcept {
    return std::string_view(message.data()) == std::string_view(other.message.data()) && code == other.code &&
           timestamp == other.timestamp;
  }
};

struct EmptyEvent {};

// Non-movable type (should not satisfy EventTrait)
struct NonMovableEvent {
  int value = 0;

  NonMovableEvent() = default;
  NonMovableEvent(const NonMovableEvent&) = delete;
  NonMovableEvent(NonMovableEvent&&) = delete;
  NonMovableEvent& operator=(const NonMovableEvent&) = delete;
  NonMovableEvent& operator=(NonMovableEvent&&) = delete;
};

}  // namespace

TEST_SUITE("ecs::Event") {
  TEST_CASE("EventTrait: Valid Event Types") {
    SUBCASE("Simple POD event") {
      CHECK(EventTrait<SimpleEvent>);
    }

    SUBCASE("Event with name trait") {
      CHECK(EventTrait<EventWithName>);
    }

    SUBCASE("Complex event with strings") {
      CHECK(EventTrait<ComplexEvent>);
    }

    SUBCASE("Empty event") {
      CHECK(EventTrait<EmptyEvent>);
    }
  }

  TEST_CASE("EventTrait: Invalid Event Types") {
    SUBCASE("Non-movable type") {
      CHECK_FALSE(EventTrait<NonMovableEvent>);
    }

    SUBCASE("Const type") {
      CHECK_FALSE(EventTrait<const SimpleEvent>);
    }

    SUBCASE("Reference type") {
      CHECK_FALSE(EventTrait<SimpleEvent&>);
      CHECK_FALSE(EventTrait<SimpleEvent&&>);
    }

    SUBCASE("Pointer type") {
      // Pointers are trivially copyable but satisfy all EventTrait requirements
      CHECK(EventTrait<SimpleEvent*>);
    }

    SUBCASE("Void type") {
      CHECK_FALSE(EventTrait<void>);
    }

    SUBCASE("Function type") {
      CHECK_FALSE(EventTrait<void()>);
    }
  }

  TEST_CASE("EventWithNameTrait: Valid Types") {
    SUBCASE("Event with GetName method") {
      CHECK(EventWithNameTrait<EventWithName>);
    }
  }

  TEST_CASE("EventWithNameTrait: Invalid Types") {
    SUBCASE("Event without GetName method") {
      CHECK_FALSE(EventWithNameTrait<SimpleEvent>);
      CHECK_FALSE(EventWithNameTrait<ComplexEvent>);
    }

    SUBCASE("Non-event types") {
      CHECK_FALSE(EventWithNameTrait<int>);
      CHECK_FALSE(EventWithNameTrait<std::string>);
    }
  }

  TEST_CASE("EventTypeIdOf: Unique Type IDs") {
    SUBCASE("Different events have different type IDs") {
      constexpr EventTypeId simple_id = EventTypeIdOf<SimpleEvent>();
      constexpr EventTypeId complex_id = EventTypeIdOf<ComplexEvent>();
      constexpr EventTypeId empty_id = EventTypeIdOf<EmptyEvent>();
      constexpr EventTypeId named_id = EventTypeIdOf<EventWithName>();

      CHECK_NE(simple_id, complex_id);
      CHECK_NE(simple_id, empty_id);
      CHECK_NE(simple_id, named_id);
      CHECK_NE(complex_id, empty_id);
      CHECK_NE(complex_id, named_id);
      CHECK_NE(empty_id, named_id);
    }

    SUBCASE("Same event type has same type ID") {
      constexpr EventTypeId id1 = EventTypeIdOf<SimpleEvent>();
      constexpr EventTypeId id2 = EventTypeIdOf<SimpleEvent>();

      CHECK_EQ(id1, id2);
    }

    SUBCASE("Type ID is compile-time constant") {
      constexpr EventTypeId id = EventTypeIdOf<SimpleEvent>();
      static_assert(id == EventTypeIdOf<SimpleEvent>(), "Type ID should be constexpr");
    }
  }

  TEST_CASE("EventNameOf: Event Name Resolution") {
    SUBCASE("Event with custom name") {
      constexpr std::string_view name = EventNameOf<EventWithName>();
      CHECK_EQ(name, "CustomEventName");
    }

    SUBCASE("Event without custom name uses type name") {
      constexpr std::string_view name = EventNameOf<SimpleEvent>();
      // Should contain type information (exact format depends on CTTI)
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Different events have different names") {
      constexpr std::string_view name1 = EventNameOf<SimpleEvent>();
      constexpr std::string_view name2 = EventNameOf<ComplexEvent>();

      CHECK_NE(name1, name2);
    }

    SUBCASE("Name is compile-time constant") {
      constexpr std::string_view name = EventNameOf<EventWithName>();
      static_assert(!name.empty(), "Name should be constexpr");
    }
  }

  TEST_CASE("EventNameOf: Name Consistency") {
    SUBCASE("Multiple calls return same name") {
      constexpr std::string_view name1 = EventNameOf<EventWithName>();
      constexpr std::string_view name2 = EventNameOf<EventWithName>();

      CHECK_EQ(name1, name2);
    }

    SUBCASE("Custom name is preferred over type name") {
      constexpr std::string_view name = EventNameOf<EventWithName>();

      // Should use custom name, not type-generated name
      CHECK_EQ(name, "CustomEventName");
    }
  }

  TEST_CASE("Event: Type Properties") {
    SUBCASE("SimpleEvent properties") {
      CHECK(std::is_move_constructible_v<SimpleEvent>);
      CHECK(std::is_move_assignable_v<SimpleEvent>);
      CHECK(std::is_copy_constructible_v<SimpleEvent>);
      CHECK(std::is_copy_assignable_v<SimpleEvent>);
    }

    SUBCASE("ComplexEvent properties") {
      CHECK(std::is_move_constructible_v<ComplexEvent>);
      CHECK(std::is_move_assignable_v<ComplexEvent>);
    }

    SUBCASE("EmptyEvent properties") {
      CHECK(std::is_move_constructible_v<EmptyEvent>);
      CHECK(std::is_move_assignable_v<EmptyEvent>);
      CHECK(std::is_empty_v<EmptyEvent>);
    }
  }

  TEST_CASE("Event: Practical Usage") {
    SUBCASE("Create and move simple event") {
      SimpleEvent event1{42};
      SimpleEvent event2 = std::move(event1);

      CHECK_EQ(event2.value, 42);
    }

    SUBCASE("Create complex event") {
      ComplexEvent event("Test message", 200, 1.5F);

      CHECK_EQ(std::string_view(event.message.data()), "Test message");
      CHECK_EQ(event.code, 200);
      CHECK_EQ(event.timestamp, 1.5F);
    }

    SUBCASE("Move complex event") {
      ComplexEvent event1("Original", 100, 2.5F);
      ComplexEvent event2 = std::move(event1);

      CHECK_EQ(std::string_view(event2.message.data()), "Original");
      CHECK_EQ(event2.code, 100);
      CHECK_EQ(event2.timestamp, 2.5F);
    }

    SUBCASE("Empty event is valid") {
      EmptyEvent event1;
      EmptyEvent event2 = std::move(event1);

      // No data to check, but should compile and work
      static_cast<void>(event2);
    }
  }

  TEST_CASE("Event: Type ID Stability") {
    SUBCASE("Type ID remains constant across multiple queries") {
      std::vector<EventTypeId> ids;

      for (int i = 0; i < 10; ++i) {
        ids.push_back(EventTypeIdOf<SimpleEvent>());
      }

      for (size_t i = 1; i < ids.size(); ++i) {
        CHECK_EQ(ids[i], ids[0]);
      }
    }

    SUBCASE("Type ID is usable as map key") {
      std::unordered_map<EventTypeId, std::string> event_names;

      event_names[EventTypeIdOf<SimpleEvent>()] = "Simple";
      event_names[EventTypeIdOf<ComplexEvent>()] = "Complex";
      event_names[EventTypeIdOf<EmptyEvent>()] = "Empty";

      CHECK_EQ(event_names.size(), 3);
      CHECK_EQ(event_names[EventTypeIdOf<SimpleEvent>()], "Simple");
      CHECK_EQ(event_names[EventTypeIdOf<ComplexEvent>()], "Complex");
      CHECK_EQ(event_names[EventTypeIdOf<EmptyEvent>()], "Empty");
    }
  }

}  // TEST_SUITE
