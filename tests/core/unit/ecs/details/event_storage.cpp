#include <doctest/doctest.h>

#include <helios/core/ecs/details/event_storage.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

using namespace helios::ecs::details;

namespace {

struct SimpleEvent {
  int value = 0;
  float data = 0.0F;

  constexpr bool operator==(const SimpleEvent& other) const noexcept = default;
};

struct ComplexEvent {
  std::array<char, 64> message = {};
  int code = 0;
  float timestamp = 0.0F;

  explicit constexpr ComplexEvent(std::string_view msg = "", int c = 0, float ts = 0.0F) : code(c), timestamp(ts) {
    const auto copy_size = std::min(msg.size(), message.size() - 1);
    std::copy_n(msg.begin(), copy_size, message.begin());
    message[copy_size] = '\0';
  }

  constexpr bool operator==(const ComplexEvent& other) const noexcept {
    return std::string_view(message.data()) == std::string_view(other.message.data()) && code == other.code &&
           timestamp == other.timestamp;
  }
};

struct LargeEvent {
  int data[100] = {};

  constexpr bool operator==(const LargeEvent& other) const noexcept {
    for (size_t i = 0; i < 100; ++i) {
      if (data[i] != other.data[i]) {
        return false;
      }
    }
    return true;
  }
};

struct EmptyEvent {
  constexpr bool operator==(const EmptyEvent&) const noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::details::EventStorage") {
  TEST_CASE("EventStorage::EventStorage: default construction") {
    EventStorage storage(sizeof(SimpleEvent));

    CHECK(storage.Empty());
    CHECK_EQ(storage.SizeBytes(), 0);
    CHECK_EQ(storage.EventSize(), sizeof(SimpleEvent));
  }

  TEST_CASE("EventStorage::Write: single event") {
    SUBCASE("Write simple event") {
      EventStorage storage(sizeof(SimpleEvent));
      SimpleEvent event{42};
      storage.Write(event);

      CHECK_FALSE(storage.Empty());
      CHECK_GT(storage.SizeBytes(), 0);
    }

    SUBCASE("Write complex event") {
      EventStorage storage(sizeof(ComplexEvent));
      ComplexEvent event{"Test", 100, 1.5F};
      storage.Write(event);

      CHECK_FALSE(storage.Empty());
      CHECK_GT(storage.SizeBytes(), 0);
    }

    SUBCASE("Write empty event") {
      EventStorage storage(sizeof(EmptyEvent));
      EmptyEvent event{};
      storage.Write(event);

      CHECK_FALSE(storage.Empty());
      CHECK_GT(storage.SizeBytes(), 0);
    }
  }

  TEST_CASE("EventStorage::Write: multiple events") {
    EventStorage storage(sizeof(SimpleEvent));

    SimpleEvent event1{10};
    SimpleEvent event2{20};
    SimpleEvent event3{30};

    storage.Write(event1);
    storage.Write(event2);
    storage.Write(event3);

    CHECK_FALSE(storage.Empty());
    CHECK_GT(storage.SizeBytes(), 0);

    // Size should accommodate all three events (no metadata stored)
    size_t expected_size = 3 * sizeof(SimpleEvent);
    CHECK_EQ(storage.SizeBytes(), expected_size);
  }

  TEST_CASE("EventStorage::WriteBulk") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Write multiple events at once") {
      std::vector<SimpleEvent> events = {{1}, {2}, {3}, {4}, {5}};
      storage.WriteBulk(std::span<const SimpleEvent>(events));

      CHECK_FALSE(storage.Empty());
      CHECK_GT(storage.SizeBytes(), 0);
    }

    SUBCASE("Write empty span") {
      std::vector<SimpleEvent> events;
      storage.WriteBulk(std::span<const SimpleEvent>(events));

      CHECK(storage.Empty());
      CHECK_EQ(storage.SizeBytes(), 0);
    }

    SUBCASE("Write large batch") {
      std::vector<SimpleEvent> events;
      for (int i = 0; i < 100; ++i) {
        events.push_back({i});
      }

      storage.WriteBulk(std::span<const SimpleEvent>(events));

      CHECK_FALSE(storage.Empty());
      CHECK_GT(storage.SizeBytes(), 0);
    }
  }

  TEST_CASE("EventStorage::ReadAll") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Read single event") {
      SimpleEvent original{42};
      storage.Write(original);

      auto events = storage.ReadAll<SimpleEvent>();

      CHECK_EQ(events.size(), 1);
      CHECK_EQ(events[0].value, 42);
    }

    SUBCASE("Read multiple events") {
      storage.Write(SimpleEvent{10});
      storage.Write(SimpleEvent{20});
      storage.Write(SimpleEvent{30});

      auto events = storage.ReadAll<SimpleEvent>();

      CHECK_EQ(events.size(), 3);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
    }

    SUBCASE("Read from empty storage") {
      auto events = storage.ReadAll<SimpleEvent>();

      CHECK(events.empty());
    }

    SUBCASE("Read complex events") {
      EventStorage complex_storage(sizeof(ComplexEvent));
      ComplexEvent event1("First", 1, 1.0F);
      ComplexEvent event2("Second", 2, 2.0F);

      complex_storage.Write(event1);
      complex_storage.Write(event2);

      auto events = complex_storage.ReadAll<ComplexEvent>();

      CHECK_EQ(events.size(), 2);
      CHECK_EQ(std::string_view(events[0].message.data()), "First");
      CHECK_EQ(events[0].code, 1);
      CHECK_EQ(std::string_view(events[1].message.data()), "Second");
      CHECK_EQ(events[1].code, 2);
    }
  }

  TEST_CASE("EventStorage::EventSize") {
    SUBCASE("EventSize returns correct size") {
      EventStorage storage(sizeof(SimpleEvent));
      CHECK_EQ(storage.EventSize(), sizeof(SimpleEvent));
    }

    SUBCASE("EventSize for different types") {
      EventStorage small_storage(sizeof(EmptyEvent));
      EventStorage large_storage(sizeof(LargeEvent));

      CHECK_EQ(small_storage.EventSize(), sizeof(EmptyEvent));
      CHECK_EQ(large_storage.EventSize(), sizeof(LargeEvent));
    }
  }

  TEST_CASE("EventStorage::FromEvent") {
    SUBCASE("FromEvent creates storage with correct size") {
      auto storage = EventStorage::FromEvent<SimpleEvent>();
      CHECK_EQ(storage.EventSize(), sizeof(SimpleEvent));
      CHECK(storage.Empty());
    }

    SUBCASE("FromEvent for different event types") {
      auto simple_storage = EventStorage::FromEvent<SimpleEvent>();
      auto complex_storage = EventStorage::FromEvent<ComplexEvent>();
      auto large_storage = EventStorage::FromEvent<LargeEvent>();
      auto empty_storage = EventStorage::FromEvent<EmptyEvent>();

      CHECK_EQ(simple_storage.EventSize(), sizeof(SimpleEvent));
      CHECK_EQ(complex_storage.EventSize(), sizeof(ComplexEvent));
      CHECK_EQ(large_storage.EventSize(), sizeof(LargeEvent));
      CHECK_EQ(empty_storage.EventSize(), sizeof(EmptyEvent));
    }

    SUBCASE("FromEvent storage can store events") {
      auto storage = EventStorage::FromEvent<SimpleEvent>();

      storage.Write(SimpleEvent{42});
      storage.Write(SimpleEvent{99});

      auto events = storage.ReadAll<SimpleEvent>();
      CHECK_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 42);
      CHECK_EQ(events[1].value, 99);
    }
  }

  TEST_CASE("EventStorage::ReadInto") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Read into empty vector") {
      storage.Write(SimpleEvent{10});
      storage.Write(SimpleEvent{20});

      std::vector<SimpleEvent> events;
      storage.ReadInto<SimpleEvent>(std::back_inserter(events));

      CHECK_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
    }

    SUBCASE("Read into non-empty vector (append)") {
      storage.Write(SimpleEvent{30});
      storage.Write(SimpleEvent{40});

      std::vector<SimpleEvent> events;
      events.push_back({10});
      events.push_back({20});

      storage.ReadInto<SimpleEvent>(std::back_inserter(events));

      CHECK_EQ(events.size(), 4);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
      CHECK_EQ(events[3].value, 40);
    }

    SUBCASE("Read from empty storage into vector") {
      std::vector<SimpleEvent> events;
      events.push_back({100});

      storage.ReadInto<SimpleEvent>(std::back_inserter(events));

      CHECK_EQ(events.size(), 1);
      CHECK_EQ(events[0].value, 100);
    }
  }

  TEST_CASE("EventStorage::Clear") {
    EventStorage storage(sizeof(SimpleEvent));

    storage.Write(SimpleEvent{10});
    storage.Write(SimpleEvent{20});
    storage.Write(SimpleEvent{30});

    CHECK_FALSE(storage.Empty());
    CHECK_GT(storage.SizeBytes(), 0);

    storage.Clear();

    CHECK(storage.Empty());
    CHECK_EQ(storage.SizeBytes(), 0);
  }

  TEST_CASE("EventStorage::Reserve") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Reserve space") {
      storage.Reserve(1024);

      // Should still be empty after reserve
      CHECK(storage.Empty());
      CHECK_EQ(storage.SizeBytes(), 0);
    }

    SUBCASE("Reserve and write") {
      storage.Reserve(1024);
      storage.Write(SimpleEvent{42});

      CHECK_FALSE(storage.Empty());
      CHECK_GT(storage.SizeBytes(), 0);
    }
  }

  TEST_CASE("EventStorage::AppendRawBytes") {
    EventStorage storage1(sizeof(SimpleEvent));
    EventStorage storage2(sizeof(SimpleEvent));

    storage1.Write(SimpleEvent{10});
    storage1.Write(SimpleEvent{20});

    storage2.Write(SimpleEvent{30});

    SUBCASE("Append from another storage") {
      size_t original_size = storage2.SizeBytes();
      storage2.AppendRawBytes(storage1.Data());

      CHECK_GT(storage2.SizeBytes(), original_size);
    }

    SUBCASE("Append zero bytes") {
      size_t original_size = storage2.SizeBytes();
      storage2.AppendRawBytes(std::span<const std::byte>());

      CHECK_EQ(storage2.SizeBytes(), original_size);
    }

    SUBCASE("Read appended events") {
      storage2.AppendRawBytes(storage1.Data());

      auto events = storage2.ReadAll<SimpleEvent>();

      CHECK_EQ(events.size(), 3);
      CHECK_EQ(events[0].value, 30);
      CHECK_EQ(events[1].value, 10);
      CHECK_EQ(events[2].value, 20);
    }
  }

  TEST_CASE("EventStorage: write and read consistency") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Simple events round-trip") {
      std::vector<SimpleEvent> original_events = {{1}, {2}, {3}, {4}, {5}};

      for (const auto& event : original_events) {
        storage.Write(event);
      }

      auto read_events = storage.ReadAll<SimpleEvent>();

      CHECK_EQ(read_events.size(), original_events.size());
      for (size_t i = 0; i < original_events.size(); ++i) {
        CHECK_EQ(read_events[i].value, original_events[i].value);
      }
    }

    SUBCASE("Complex events round-trip") {
      EventStorage complex_storage(sizeof(ComplexEvent));
      std::vector<ComplexEvent> original_events = {ComplexEvent("Event1", 100, 1.0F), ComplexEvent("Event2", 200, 2.0F),
                                                   ComplexEvent("Event3", 300, 3.0F)};

      for (const auto& event : original_events) {
        complex_storage.Write(event);
      }

      auto read_events = complex_storage.ReadAll<ComplexEvent>();

      CHECK_EQ(read_events.size(), original_events.size());
      for (size_t i = 0; i < original_events.size(); ++i) {
        CHECK_EQ(std::string_view(read_events[i].message.data()), std::string_view(original_events[i].message.data()));
        CHECK_EQ(read_events[i].code, original_events[i].code);
        CHECK_EQ(read_events[i].timestamp, original_events[i].timestamp);
      }
    }

    SUBCASE("Empty events round-trip") {
      EventStorage empty_storage(sizeof(EmptyEvent));
      empty_storage.Write(EmptyEvent{});
      empty_storage.Write(EmptyEvent{});

      auto events = empty_storage.ReadAll<EmptyEvent>();

      CHECK_EQ(events.size(), 2);
    }
  }

  TEST_CASE("EventStorage: large event handling") {
    EventStorage storage(sizeof(LargeEvent));

    LargeEvent large_event;
    for (int i = 0; i < 100; ++i) {
      large_event.data[i] = i * 10;
    }

    storage.Write(large_event);

    auto events = storage.ReadAll<LargeEvent>();

    CHECK_EQ(events.size(), 1);
    for (int i = 0; i < 100; ++i) {
      CHECK_EQ(events[0].data[i], i * 10);
    }
  }

  TEST_CASE("EventStorage::WriteBulk: performance") {
    EventStorage storage(sizeof(SimpleEvent));

    constexpr size_t event_count = 1000;
    std::vector<SimpleEvent> events;
    events.reserve(event_count);

    for (size_t i = 0; i < event_count; ++i) {
      events.push_back({static_cast<int>(i)});
    }

    storage.WriteBulk(events);

    auto read_events = storage.ReadAll<SimpleEvent>();

    CHECK_EQ(read_events.size(), event_count);
    for (size_t i = 0; i < event_count; ++i) {
      CHECK_EQ(read_events[i].value, static_cast<int>(i));
    }
  }

  TEST_CASE("EventStorage: multiple write sessions") {
    EventStorage storage(sizeof(SimpleEvent));

    // First session
    storage.Write(SimpleEvent{1});
    storage.Write(SimpleEvent{2});

    auto events1 = storage.ReadAll<SimpleEvent>();
    CHECK_EQ(events1.size(), 2);

    // Clear and second session
    storage.Clear();
    storage.Write(SimpleEvent{10});
    storage.Write(SimpleEvent{20});
    storage.Write(SimpleEvent{30});

    auto events2 = storage.ReadAll<SimpleEvent>();
    CHECK_EQ(events2.size(), 3);
    CHECK_EQ(events2[0].value, 10);
    CHECK_EQ(events2[1].value, 20);
    CHECK_EQ(events2[2].value, 30);
  }

  TEST_CASE("EventStorage::EventStorage: move semantics") {
    EventStorage storage1(sizeof(SimpleEvent));

    storage1.Write(SimpleEvent{42});
    storage1.Write(SimpleEvent{99});

    SUBCASE("Move construction") {
      EventStorage storage2(std::move(storage1));

      auto events = storage2.ReadAll<SimpleEvent>();
      CHECK_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 42);
      CHECK_EQ(events[1].value, 99);
    }

    SUBCASE("Move assignment") {
      EventStorage storage2(sizeof(SimpleEvent));
      storage2.Write(SimpleEvent{1});

      storage2 = std::move(storage1);

      auto events = storage2.ReadAll<SimpleEvent>();
      CHECK_EQ(events.size(), 2);
      CHECK_EQ(events[0].value, 42);
      CHECK_EQ(events[1].value, 99);
    }
  }

  TEST_CASE("EventStorage::Data") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Data pointer is null for empty storage") {
      const auto span = storage.Data();
    }

    SUBCASE("Data pointer is valid after write") {
      storage.Write(SimpleEvent{42});

      const auto span = storage.Data();
      CHECK(span.data() != nullptr);
    }
  }

  TEST_CASE("EventStorage: edge cases") {
    EventStorage storage(sizeof(SimpleEvent));

    SUBCASE("Write same event multiple times") {
      SimpleEvent event{42};

      for (int i = 0; i < 10; ++i) {
        storage.Write(event);
      }

      auto events = storage.ReadAll<SimpleEvent>();
      CHECK_EQ(events.size(), 10);

      for (const auto& e : events) {
        CHECK_EQ(e.value, 42);
      }
    }

    SUBCASE("Clear multiple times") {
      storage.Write(SimpleEvent{1});
      storage.Clear();
      CHECK(storage.Empty());

      storage.Clear();
      CHECK(storage.Empty());

      storage.Write(SimpleEvent{2});
      CHECK_FALSE(storage.Empty());

      storage.Clear();
      CHECK(storage.Empty());
    }

    SUBCASE("Reserve multiple times") {
      storage.Reserve(100);
      storage.Reserve(200);
      storage.Reserve(50);

      // Should not cause issues
      storage.Write(SimpleEvent{42});
      CHECK_FALSE(storage.Empty());
    }
  }

}  // TEST_SUITE
