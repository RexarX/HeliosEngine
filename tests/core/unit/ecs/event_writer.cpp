#include <helios/core/ecs/event_writer.hpp>
#include <helios/core/ecs/world.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace helios::ecs;

namespace {

struct TestEvent {
  int value = 0;

  constexpr bool operator==(const TestEvent& other) const noexcept = default;
};

struct ComplexEvent {
  int id = 0;
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const ComplexEvent& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::EventWriter") {
  TEST_CASE("EventWriter - Construction") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();

    // EventWriter should be movable
    auto writer2 = std::move(writer);

    CHECK(true);  // Successfully constructed and moved
  }

  TEST_CASE("EventWriter - Write Single Event (Copy)") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    TestEvent event{42};
    writer.Write(event);

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 1);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 42);
  }

  TEST_CASE("EventWriter - Write Single Event (Move)") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{123});

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 1);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 123);
  }

  TEST_CASE("EventWriter - Write Multiple Events") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    writer.Write(TestEvent{2});
    writer.Write(TestEvent{3});

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 3);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 1);
    CHECK_EQ(events[1].value, 2);
    CHECK_EQ(events[2].value, 3);
  }

  TEST_CASE("EventWriter - WriteBulk with Vector") {
    World world;
    world.AddEvent<TestEvent>();

    std::vector<TestEvent> events_to_write = {TestEvent{10}, TestEvent{20}, TestEvent{30}};

    auto writer = world.WriteEvents<TestEvent>();
    writer.WriteBulk(events_to_write);

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 3);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 10);
    CHECK_EQ(events[1].value, 20);
    CHECK_EQ(events[2].value, 30);
  }

  TEST_CASE("EventWriter - WriteBulk Empty Span") {
    World world;
    world.AddEvent<TestEvent>();

    std::vector<TestEvent> empty_events;

    auto writer = world.WriteEvents<TestEvent>();
    writer.WriteBulk(empty_events);

    auto reader = world.ReadEvents<TestEvent>();
    CHECK(reader.Empty());
  }

  TEST_CASE("EventWriter - Emplace Single Argument") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Emplace(999);

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 1);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 999);
  }

  TEST_CASE("EventWriter - Emplace Multiple Arguments") {
    World world;
    world.AddEvent<ComplexEvent>();

    auto writer = world.WriteEvents<ComplexEvent>();
    writer.Emplace(42, 1.5f, 2.5f);

    auto reader = world.ReadEvents<ComplexEvent>();
    REQUIRE_EQ(reader.Count(), 1);
    auto events = reader.Collect();
    CHECK_EQ(events[0].id, 42);
    CHECK_EQ(events[0].x, 1.5f);
    CHECK_EQ(events[0].y, 2.5f);
  }

  TEST_CASE("EventWriter - Emplace Multiple Events") {
    World world;
    world.AddEvent<ComplexEvent>();

    auto writer = world.WriteEvents<ComplexEvent>();
    writer.Emplace(1, 1.0f, 2.0f);
    writer.Emplace(2, 3.0f, 4.0f);
    writer.Emplace(3, 5.0f, 6.0f);

    auto reader = world.ReadEvents<ComplexEvent>();
    REQUIRE_EQ(reader.Count(), 3);
    auto events = reader.Collect();
    CHECK_EQ(events[0].id, 1);
    CHECK_EQ(events[1].id, 2);
    CHECK_EQ(events[2].id, 3);
  }

  TEST_CASE("EventWriter - Mixed Write and Emplace") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Emplace(20);
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 3);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 10);
    CHECK_EQ(events[1].value, 20);
    CHECK_EQ(events[2].value, 30);
  }

  TEST_CASE("EventWriter - Mixed Write and WriteBulk") {
    World world;
    world.AddEvent<TestEvent>();

    std::vector<TestEvent> bulk_events = {TestEvent{100}, TestEvent{200}};

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{50});
    writer.WriteBulk(bulk_events);
    writer.Write(TestEvent{300});

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 4);
    auto events = reader.Collect();
    CHECK_EQ(events[0].value, 50);
    CHECK_EQ(events[1].value, 100);
    CHECK_EQ(events[2].value, 200);
    CHECK_EQ(events[3].value, 300);
  }

  TEST_CASE("EventWriter - Works with Double Buffering") {
    World world;
    world.AddEvent<TestEvent>();

    // Frame 0: Write event
    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});

    auto reader_f0 = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader_f0.Count(), 1);

    // Frame 1: Event moves to previous, write new event
    world.Update();
    writer.Write(TestEvent{2});

    auto reader_f1 = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader_f1.Count(), 2);  // Both frame 0 and frame 1 events visible

    // Frame 2: Frame 0 event cleared, frame 1 event in previous, write new event
    world.Update();
    writer.Write(TestEvent{3});

    auto reader_f2 = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader_f2.Count(), 2);  // Only frame 1 and frame 2 events visible
  }

  TEST_CASE("EventWriter - Multiple Writers Same Type") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer1 = world.WriteEvents<TestEvent>();
    writer1.Write(TestEvent{10});

    auto writer2 = world.WriteEvents<TestEvent>();
    writer2.Write(TestEvent{20});

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 2);

    // Order doesn't matter, just check both are present
    bool has_10 = false;
    bool has_20 = false;
    for (const auto& event : reader) {
      if (event.value == 10)
        has_10 = true;
      if (event.value == 20)
        has_20 = true;
    }
    CHECK(has_10);
    CHECK(has_20);
  }

  TEST_CASE("EventWriter - Different Event Types") {
    World world;
    world.AddEvent<TestEvent>();
    world.AddEvent<ComplexEvent>();

    auto writer1 = world.WriteEvents<TestEvent>();
    writer1.Write(TestEvent{42});

    auto writer2 = world.WriteEvents<ComplexEvent>();
    writer2.Emplace(1, 2.0f, 3.0f);

    auto test_reader = world.ReadEvents<TestEvent>();
    auto complex_reader = world.ReadEvents<ComplexEvent>();

    CHECK_EQ(test_reader.Count(), 1);
    CHECK_EQ(complex_reader.Count(), 1);
    auto test_events = test_reader.Collect();
    auto complex_events = complex_reader.Collect();
    CHECK_EQ(test_events[0].value, 42);
    CHECK_EQ(complex_events[0].id, 1);
  }

  TEST_CASE("EventWriter - Large Bulk Write") {
    World world;
    world.AddEvent<TestEvent>();

    std::vector<TestEvent> large_batch;
    large_batch.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
      large_batch.push_back(TestEvent{i});
    }

    auto writer = world.WriteEvents<TestEvent>();
    writer.WriteBulk(large_batch);

    auto reader = world.ReadEvents<TestEvent>();
    REQUIRE_EQ(reader.Count(), 1000);
    auto events = reader.Collect();

    // Verify first, middle, and last
    CHECK_EQ(events[0].value, 0);
    CHECK_EQ(events[500].value, 500);
    CHECK_EQ(events[999].value, 999);
  }
}
