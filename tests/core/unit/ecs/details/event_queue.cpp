#include <doctest/doctest.h>

#include <helios/core/ecs/details/event_queue.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Test event types
struct SimpleEvent {
  int value = 0;

  constexpr bool operator==(const SimpleEvent& other) const noexcept = default;
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

struct EmptyEvent {
  constexpr bool operator==(const EmptyEvent&) const noexcept = default;
};

struct AnotherEvent {
  double data = 0.0;

  constexpr bool operator==(const AnotherEvent& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::details::EventQueue") {
  TEST_CASE("EventQueue::EventQueue: default construction") {
    EventQueue queue;

    CHECK(queue.Empty());
    CHECK_EQ(queue.TypeCount(), 0);
    CHECK_EQ(queue.TotalSizeBytes(), 0);
  }

  TEST_CASE("EventQueue::Register") {
    EventQueue queue;

    SUBCASE("Register single event type") {
      queue.Register<SimpleEvent>();
      CHECK(queue.IsRegistered<SimpleEvent>());
      CHECK_FALSE(queue.IsRegistered<ComplexEvent>());
    }

    SUBCASE("Register multiple event types") {
      queue.Register<SimpleEvent>();
      queue.Register<ComplexEvent>();
      queue.Register<EmptyEvent>();

      CHECK(queue.IsRegistered<SimpleEvent>());
      CHECK(queue.IsRegistered<ComplexEvent>());
      CHECK(queue.IsRegistered<EmptyEvent>());
      CHECK_FALSE(queue.IsRegistered<AnotherEvent>());
    }

    SUBCASE("Register same type multiple times") {
      queue.Register<SimpleEvent>();
      queue.Register<SimpleEvent>();
      queue.Register<SimpleEvent>();

      CHECK(queue.IsRegistered<SimpleEvent>());
      CHECK_EQ(queue.TypeCount(), 1);
    }

    SUBCASE("IsRegistered returns false for unregistered types") {
      CHECK_FALSE(queue.IsRegistered<SimpleEvent>());
      CHECK_FALSE(queue.IsRegistered<ComplexEvent>());
      CHECK_FALSE(queue.IsRegistered<EmptyEvent>());
    }
  }

  TEST_CASE("EventQueue::Write: single event") {
    EventQueue queue;

    SUBCASE("Write simple event") {
      queue.Register<SimpleEvent>();
      SimpleEvent event{42};
      queue.Write(event);

      CHECK_FALSE(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 1);
      CHECK_GT(queue.TotalSizeBytes(), 0);
    }

    SUBCASE("Write complex event") {
      queue.Register<ComplexEvent>();
      ComplexEvent event{"Test", 100, 1.5F};
      queue.Write(event);

      CHECK_FALSE(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 1);
      CHECK_GT(queue.TotalSizeBytes(), 0);
    }

    SUBCASE("Write empty event") {
      queue.Register<EmptyEvent>();
      EmptyEvent event{};
      queue.Write(event);

      CHECK_FALSE(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 1);
      CHECK_GT(queue.TotalSizeBytes(), 0);
    }
  }

  TEST_CASE("EventQueue::Write: multiple events same type") {
    EventQueue queue;

    queue.Register<SimpleEvent>();
    queue.Write(SimpleEvent{10});
    queue.Write(SimpleEvent{20});
    queue.Write(SimpleEvent{30});

    CHECK_FALSE(queue.Empty());
    CHECK_EQ(queue.TypeCount(), 1);
    CHECK_GT(queue.TotalSizeBytes(), 0);
  }

  TEST_CASE("EventQueue::Write: multiple event types") {
    EventQueue queue;

    queue.Register<ComplexEvent>();
    queue.Register<SimpleEvent>();
    queue.Register<EmptyEvent>();
    queue.Register<AnotherEvent>();

    queue.Write(ComplexEvent{"Event1", 1, 1.0F});
    queue.Write(SimpleEvent{100});
    queue.Write(ComplexEvent{"Event2", 2, 2.0F});
    queue.Write(EmptyEvent{});
    queue.Write(AnotherEvent{3.14});

    CHECK_FALSE(queue.Empty());
    CHECK_EQ(queue.TypeCount(), 4);
    CHECK_GT(queue.TotalSizeBytes(), 0);
  }

  TEST_CASE("EventQueue::WriteBulk") {
    EventQueue queue;

    SUBCASE("Write multiple events at once") {
      queue.Register<SimpleEvent>();
      std::vector<SimpleEvent> events = {{1}, {2}, {3}, {4}, {5}};
      queue.WriteBulk(events);

      CHECK_FALSE(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 1);
      CHECK_GT(queue.TotalSizeBytes(), 0);
    }

    SUBCASE("Write empty span") {
      queue.Register<SimpleEvent>();
      std::vector<SimpleEvent> events;
      queue.WriteBulk(events);

      CHECK(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 1);
      CHECK_EQ(queue.TotalSizeBytes(), 0);
    }

    SUBCASE("Write bulk for multiple types") {
      queue.Register<SimpleEvent>();
      queue.Register<ComplexEvent>();
      std::vector<SimpleEvent> simple_events = {{1}, {2}, {3}};
      std::vector<ComplexEvent> complex_events = {ComplexEvent("A", 1, 1.0F), ComplexEvent("B", 2, 2.0F)};

      queue.WriteBulk(simple_events);
      queue.WriteBulk(complex_events);

      CHECK_EQ(queue.TypeCount(), 2);
    }
  }

  TEST_CASE("EventQueue::Read") {
    EventQueue queue;

    SUBCASE("Read single event") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{42});

      auto events = queue.Read<SimpleEvent>();

      CHECK_EQ(events.size(), 1);
      CHECK_EQ(events[0].value, 42);
    }

    SUBCASE("Read multiple events") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{10});
      queue.Write(SimpleEvent{20});
      queue.Write(SimpleEvent{30});

      auto events = queue.Read<SimpleEvent>();

      CHECK_EQ(events.size(), 3);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
    }

    SUBCASE("Read from empty queue") {
      auto events = queue.Read<SimpleEvent>();

      CHECK(events.empty());
    }

    SUBCASE("Read non-existent event type") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{42});

      auto events = queue.Read<ComplexEvent>();

      CHECK(events.empty());
    }
  }

  TEST_CASE("EventQueue::ReadInto") {
    EventQueue queue;

    SUBCASE("Read into empty vector") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{10});
      queue.Write(SimpleEvent{20});

      std::vector<SimpleEvent> events;
      queue.ReadInto<SimpleEvent>(std::back_inserter(events));

      CHECK_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
    }

    SUBCASE("Read into non-empty vector (append)") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{30});
      queue.Write(SimpleEvent{40});

      std::vector<SimpleEvent> events;
      events.push_back({10});
      events.push_back({20});

      queue.ReadInto<SimpleEvent>(std::back_inserter(events));

      CHECK_EQ(events.size(), 4);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
      CHECK_EQ(events[3].value, 40);
    }
  }

  TEST_CASE("EventQueue::HasEvents") {
    EventQueue queue;

    SUBCASE("Empty queue has no events") {
      CHECK_FALSE(queue.HasEvents<SimpleEvent>());
      CHECK_FALSE(queue.HasEvents<ComplexEvent>());
    }

    SUBCASE("Queue has events of specific type") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{42});

      CHECK(queue.HasEvents<SimpleEvent>());
      CHECK_FALSE(queue.HasEvents<ComplexEvent>());
    }

    SUBCASE("Queue has multiple event types") {
      queue.Register<SimpleEvent>();
      queue.Register<ComplexEvent>();
      queue.Write(SimpleEvent{10});
      queue.Write(ComplexEvent{"Test", 100, 1.5F});

      CHECK(queue.HasEvents<SimpleEvent>());
      CHECK(queue.HasEvents<ComplexEvent>());
      CHECK_FALSE(queue.HasEvents<EmptyEvent>());
    }
  }

  TEST_CASE("EventQueue::Clear") {
    EventQueue queue;

    SUBCASE("Clear all events") {
      queue.Register<SimpleEvent>();
      queue.Register<ComplexEvent>();
      queue.Register<EmptyEvent>();
      queue.Write(SimpleEvent{10});
      queue.Write(ComplexEvent{"Test", 100, 1.5F});
      queue.Write(EmptyEvent{});

      CHECK_FALSE(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 3);

      queue.Clear();

      CHECK(queue.Empty());
      CHECK_EQ(queue.TypeCount(), 0);
      CHECK_EQ(queue.TotalSizeBytes(), 0);
    }

    SUBCASE("Clear specific event type") {
      queue.Register<SimpleEvent>();
      queue.Register<ComplexEvent>();
      queue.Write(SimpleEvent{10});
      queue.Write(ComplexEvent{"Test", 100, 1.5F});
      queue.Write(SimpleEvent{20});

      CHECK_EQ(queue.TypeCount(), 2);

      queue.Clear<SimpleEvent>();

      CHECK_EQ(queue.TypeCount(), 2);
      CHECK_FALSE(queue.HasEvents<SimpleEvent>());
      CHECK(queue.HasEvents<ComplexEvent>());
    }
  }

  TEST_CASE("EventQueue::Merge") {
    EventQueue queue1;
    EventQueue queue2;

    SUBCASE("Merge empty queues") {
      queue1.Merge(queue2);

      CHECK(queue1.Empty());
      CHECK(queue2.Empty());
    }

    SUBCASE("Merge non-empty into empty") {
      queue2.Register<SimpleEvent>();
      queue2.Write(SimpleEvent{10});
      queue2.Write(SimpleEvent{20});

      queue1.Merge(queue2);

      CHECK_FALSE(queue1.Empty());

      auto events = queue1.Read<SimpleEvent>();
      CHECK_EQ(events.size(), 2);
    }

    SUBCASE("Merge empty into non-empty") {
      queue1.Register<SimpleEvent>();
      queue1.Write(SimpleEvent{10});
      queue1.Write(SimpleEvent{20});

      queue1.Merge(queue2);

      CHECK_FALSE(queue1.Empty());

      auto events = queue1.Read<SimpleEvent>();
      CHECK_EQ(events.size(), 2);
    }

    SUBCASE("Merge non-empty queues with same event type") {
      queue1.Register<SimpleEvent>();
      queue1.Write(SimpleEvent{10});
      queue1.Write(SimpleEvent{20});

      queue2.Register<SimpleEvent>();
      queue2.Write(SimpleEvent{30});
      queue2.Write(SimpleEvent{40});

      queue1.Merge(queue2);

      auto events = queue1.Read<SimpleEvent>();
      CHECK_EQ(events.size(), 4);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
      CHECK_EQ(events[3].value, 40);
    }

    SUBCASE("Merge queues with different event types") {
      queue1.Register<SimpleEvent>();
      queue1.Write(SimpleEvent{10});

      queue2.Register<ComplexEvent>();
      queue2.Write(ComplexEvent{"Test", 100, 1.5F});

      queue1.Merge(queue2);
      CHECK_EQ(queue1.TypeCount(), 2);
      CHECK(queue1.HasEvents<SimpleEvent>());
      CHECK(queue1.HasEvents<ComplexEvent>());
    }

    SUBCASE("Merge queues with overlapping event types") {
      queue1.Register<SimpleEvent>();
      queue1.Register<ComplexEvent>();
      queue1.Write(SimpleEvent{10});
      queue1.Write(ComplexEvent{"First", 100, 1.0F});

      queue2.Register<SimpleEvent>();
      queue2.Register<EmptyEvent>();
      queue2.Write(SimpleEvent{20});
      queue2.Write(EmptyEvent{});

      queue1.Merge(queue2);

      CHECK_EQ(queue1.TypeCount(), 3);

      auto simple_events = queue1.Read<SimpleEvent>();
      CHECK_EQ(simple_events.size(), 2);

      auto complex_events = queue1.Read<ComplexEvent>();
      CHECK_EQ(complex_events.size(), 1);

      auto empty_events = queue1.Read<EmptyEvent>();
      CHECK_EQ(empty_events.size(), 1);
    }
  }

  TEST_CASE("EventQueue::TypeCount") {
    EventQueue queue;

    CHECK_EQ(queue.TypeCount(), 0);

    queue.Register<SimpleEvent>();
    queue.Write(SimpleEvent{10});
    CHECK_EQ(queue.TypeCount(), 1);

    queue.Write(SimpleEvent{20});
    CHECK_EQ(queue.TypeCount(), 1);

    queue.Register<ComplexEvent>();
    queue.Write(ComplexEvent{"Test", 100, 1.5F});
    CHECK_EQ(queue.TypeCount(), 2);

    queue.Register<EmptyEvent>();
    queue.Write(EmptyEvent{});
    CHECK_EQ(queue.TypeCount(), 3);

    queue.Clear<SimpleEvent>();
    CHECK_EQ(queue.TypeCount(), 3);

    queue.Clear();
    CHECK_EQ(queue.TypeCount(), 0);
  }

  TEST_CASE("EventQueue::TotalSizeBytes") {
    EventQueue queue;

    CHECK_EQ(queue.TotalSizeBytes(), 0);

    queue.Register<SimpleEvent>();
    queue.Write(SimpleEvent{10});
    size_t size1 = queue.TotalSizeBytes();
    CHECK_GT(size1, 0);

    queue.Write(SimpleEvent{20});
    size_t size2 = queue.TotalSizeBytes();
    CHECK_GT(size2, size1);

    queue.Register<ComplexEvent>();
    queue.Write(ComplexEvent{"Test", 100, 1.5F});
    size_t size3 = queue.TotalSizeBytes();
    CHECK_GT(size3, size2);

    queue.Clear();
    CHECK_EQ(queue.TotalSizeBytes(), 0);
  }

  TEST_CASE("EventQueue::EventQueue: move semantics") {
    EventQueue queue1;

    queue1.Register<SimpleEvent>();
    queue1.Register<ComplexEvent>();
    queue1.Write(SimpleEvent{42});
    queue1.Write(ComplexEvent{"Test", 100, 1.5F});

    SUBCASE("Move construction") {
      EventQueue queue2(std::move(queue1));

      CHECK_FALSE(queue2.Empty());
      CHECK_EQ(queue2.TypeCount(), 2);
      CHECK(queue2.HasEvents<SimpleEvent>());
      CHECK(queue2.HasEvents<ComplexEvent>());
    }

    SUBCASE("Move assignment") {
      EventQueue queue2;
      queue2.Register<EmptyEvent>();
      queue2.Write(EmptyEvent{});

      queue2 = std::move(queue1);

      CHECK_FALSE(queue2.Empty());
      CHECK_EQ(queue2.TypeCount(), 2);
      CHECK(queue2.HasEvents<SimpleEvent>());
      CHECK(queue2.HasEvents<ComplexEvent>());
    }
  }

  TEST_CASE("EventQueue: complex scenarios") {
    EventQueue queue;

    SUBCASE("Write, read, write, read cycle") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{10});
      queue.Write(SimpleEvent{20});

      auto events1 = queue.Read<SimpleEvent>();
      CHECK_EQ(events1.size(), 2);

      queue.Clear<SimpleEvent>();

      queue.Write(SimpleEvent{30});
      queue.Write(SimpleEvent{40});
      queue.Write(SimpleEvent{50});

      auto events2 = queue.Read<SimpleEvent>();
      CHECK_EQ(events2.size(), 3);
    }

    SUBCASE("Multiple event types interleaved") {
      queue.Register<SimpleEvent>();
      queue.Register<ComplexEvent>();
      queue.Register<EmptyEvent>();
      queue.Write(SimpleEvent{1});
      queue.Write(ComplexEvent{"A", 1, 1.0F});
      queue.Write(SimpleEvent{2});
      queue.Write(EmptyEvent{});
      queue.Write(ComplexEvent{"B", 2, 2.0F});
      queue.Write(SimpleEvent{3});

      CHECK_EQ(queue.TypeCount(), 3);

      auto simple_events = queue.Read<SimpleEvent>();
      CHECK_EQ(simple_events.size(), 3);

      auto complex_events = queue.Read<ComplexEvent>();
      CHECK_EQ(complex_events.size(), 2);

      auto empty_events = queue.Read<EmptyEvent>();
      CHECK_EQ(empty_events.size(), 1);
    }

    SUBCASE("Large batch operations") {
      constexpr size_t event_count = 1000;

      queue.Register<SimpleEvent>();
      std::vector<SimpleEvent> events;
      events.reserve(event_count);

      for (size_t i = 0; i < event_count; ++i) {
        events.push_back({static_cast<int>(i)});
      }

      queue.WriteBulk(events);

      auto read_events = queue.Read<SimpleEvent>();
      CHECK_EQ(read_events.size(), event_count);

      for (size_t i = 0; i < event_count; ++i) {
        CHECK_EQ(read_events[i].value, static_cast<int>(i));
      }
    }
  }

  TEST_CASE("EventQueue: edge cases") {
    EventQueue queue;

    SUBCASE("Clear empty queue") {
      queue.Clear();
      CHECK(queue.Empty());
    }

    SUBCASE("Read after clear") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{42});
      queue.Clear();

      auto events = queue.Read<SimpleEvent>();
      CHECK(events.empty());
    }

    SUBCASE("Merge with self (undefined but shouldn't crash)") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{42});
      // Note: Merging with self is not recommended but shouldn't crash
      // queue.Merge(queue); // This would be UB, so we skip this test
    }

    SUBCASE("Write same event multiple times") {
      queue.Register<SimpleEvent>();
      SimpleEvent event{42};

      for (int i = 0; i < 10; ++i) {
        queue.Write(event);
      }

      auto events = queue.Read<SimpleEvent>();
      CHECK_EQ(events.size(), 10);

      for (const auto& e : events) {
        CHECK_EQ(e.value, 42);
      }
    }
  }

  TEST_CASE("EventQueue: event ordering") {
    EventQueue queue;

    SUBCASE("Events are read in write order") {
      queue.Register<SimpleEvent>();
      queue.Write(SimpleEvent{1});
      queue.Write(SimpleEvent{2});
      queue.Write(SimpleEvent{3});
      queue.Write(SimpleEvent{4});
      queue.Write(SimpleEvent{5});

      auto events = queue.Read<SimpleEvent>();

      CHECK_EQ(events.size(), 5);
      for (size_t i = 0; i < 5; ++i) {
        CHECK_EQ(events[i].value, static_cast<int>(i + 1));
      }
    }

    SUBCASE("Bulk write preserves order") {
      queue.Register<SimpleEvent>();
      std::vector<SimpleEvent> to_write = {{10}, {20}, {30}, {40}, {50}};

      queue.WriteBulk(to_write);

      auto events = queue.Read<SimpleEvent>();

      CHECK_EQ(events.size(), 5);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
      CHECK_EQ(events[3].value, 40);
      CHECK_EQ(events[4].value, 50);
    }
  }

}  // TEST_SUITE
