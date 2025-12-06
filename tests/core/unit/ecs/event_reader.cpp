#include <helios/core/ecs/event_reader.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <iterator>
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
  bool is_critical = false;

  constexpr bool operator==(const ComplexEvent& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::EventReader") {
  TEST_CASE("EventReader - Construction") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();

    // EventReader should be movable
    auto reader2 = std::move(reader);

    CHECK(true);  // Successfully constructed and moved
  }

  TEST_CASE("EventReader - Empty Check") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();
    CHECK(reader.Empty());
    CHECK_EQ(reader.Count(), 0);
  }

  TEST_CASE("EventReader - Empty After Writing") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{42});

    auto reader = world.ReadEvents<TestEvent>();
    CHECK_FALSE(reader.Empty());
    CHECK_EQ(reader.Count(), 1);
  }

  TEST_CASE("EventReader - Basic Iteration") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    writer.Write(TestEvent{2});
    writer.Write(TestEvent{3});

    auto reader = world.ReadEvents<TestEvent>();

    std::vector<int> values;
    for (const auto& event : reader) {
      values.push_back(event.value);
    }

    REQUIRE_EQ(values.size(), 3);
    CHECK_NE(std::ranges::find(values, 1), values.end());
    CHECK_NE(std::ranges::find(values, 2), values.end());
    CHECK_NE(std::ranges::find(values, 3), values.end());
  }

  TEST_CASE("EventReader - Iteration Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();

    int count = 0;
    for ([[maybe_unused]] const auto& event : reader) {
      ++count;
    }

    CHECK_EQ(count, 0);
  }

  TEST_CASE("EventReader - Count") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    writer.Write(TestEvent{2});
    writer.Write(TestEvent{3});

    auto reader = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader.Count(), 3);
  }

  TEST_CASE("EventReader - Read") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});

    auto reader = world.ReadEvents<TestEvent>();
    auto events = reader.Read();

    REQUIRE_EQ(events.size(), 2);
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 10; }), events.end());
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 20; }), events.end());
  }

  TEST_CASE("EventReader - Collect") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});

    auto reader = world.ReadEvents<TestEvent>();
    auto events = reader.Collect();

    REQUIRE_EQ(events.size(), 2);
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 10; }), events.end());
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 20; }), events.end());
  }

  TEST_CASE("EventReader - Read Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();
    auto events = reader.Read();

    CHECK(events.empty());
  }

  TEST_CASE("EventReader - Collect Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();
    auto events = reader.Collect();

    CHECK(events.empty());
  }

  TEST_CASE("EventReader - ReadInto") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{5});
    writer.Write(TestEvent{15});
    writer.Write(TestEvent{25});

    auto reader = world.ReadEvents<TestEvent>();

    std::vector<TestEvent> events;
    reader.ReadInto(std::back_inserter(events));

    REQUIRE_EQ(events.size(), 3);
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 5; }), events.end());
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 15; }), events.end());
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 25; }), events.end());
  }

  TEST_CASE("EventReader - ReadInto Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();

    std::vector<TestEvent> events;
    reader.ReadInto(std::back_inserter(events));

    CHECK(events.empty());
  }

  TEST_CASE("EventReader - Into") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{5});
    writer.Write(TestEvent{15});
    writer.Write(TestEvent{25});

    auto reader = world.ReadEvents<TestEvent>();

    std::vector<TestEvent> events;
    reader.Into(std::back_inserter(events));

    REQUIRE_EQ(events.size(), 3);
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 5; }), events.end());
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 15; }), events.end());
    CHECK_NE(std::ranges::find_if(events, [](auto& e) { return e.value == 25; }), events.end());
  }

  TEST_CASE("EventReader - Into Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();

    std::vector<TestEvent> events;
    reader.Into(std::back_inserter(events));

    CHECK(events.empty());
  }

  TEST_CASE("EventReader - Into with Filter") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{5});
    writer.Write(TestEvent{15});
    writer.Write(TestEvent{25});
    writer.Write(TestEvent{35});

    auto reader = world.ReadEvents<TestEvent>();

    std::vector<std::tuple<TestEvent>> filtered_events;
    reader.Filter([](const TestEvent& e) { return e.value > 15; }).Into(std::back_inserter(filtered_events));

    REQUIRE_EQ(filtered_events.size(), 2);
    CHECK_NE(std::ranges::find_if(filtered_events, [](auto& e) { return std::get<0>(e).value == 25; }),
             filtered_events.end());
    CHECK_NE(std::ranges::find_if(filtered_events, [](auto& e) { return std::get<0>(e).value == 35; }),
             filtered_events.end());
  }

  TEST_CASE("EventReader - Into with Map") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    writer.Write(TestEvent{2});
    writer.Write(TestEvent{3});

    auto reader = world.ReadEvents<TestEvent>();

    // Map returns the transformed values directly (not in tuples)
    std::vector<int> values;
    reader.Map([](const TestEvent& e) { return e.value * 10; }).Into(std::back_inserter(values));

    REQUIRE_EQ(values.size(), 3);
    CHECK_NE(std::ranges::find(values, 10), values.end());
    CHECK_NE(std::ranges::find(values, 20), values.end());
    CHECK_NE(std::ranges::find(values, 30), values.end());
  }

  TEST_CASE("EventReader - Into with complex chain") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    for (int i = 1; i <= 20; ++i) {
      writer.Write(TestEvent{i});
    }

    auto reader = world.ReadEvents<TestEvent>();

    // Map returns ints directly after filtering
    std::vector<int> results;
    reader.Filter([](const TestEvent& e) { return e.value % 2 == 0; })
        .Map([](const TestEvent& e) { return e.value * 2; })
        .Take(5)
        .Into(std::back_inserter(results));

    REQUIRE_EQ(results.size(), 5);
    // First 5 even values doubled: 2*2=4, 4*2=8, 6*2=12, 8*2=16, 10*2=20
    CHECK_NE(std::ranges::find(results, 4), results.end());
    CHECK_NE(std::ranges::find(results, 8), results.end());
    CHECK_NE(std::ranges::find(results, 12), results.end());
    CHECK_NE(std::ranges::find(results, 16), results.end());
    CHECK_NE(std::ranges::find(results, 20), results.end());
  }

  TEST_CASE("EventReader - FindFirst Found") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();

    const auto* found = reader.FindFirst([](const auto& e) { return e.value == 20; });
    REQUIRE_NE(found, nullptr);
    CHECK_EQ(found->value, 20);
  }

  TEST_CASE("EventReader - FindFirst Not Found") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});

    auto reader = world.ReadEvents<TestEvent>();

    const auto* not_found = reader.FindFirst([](const auto& e) { return e.value == 99; });
    CHECK_EQ(not_found, nullptr);
  }

  TEST_CASE("EventReader - FindFirst Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();

    const auto* not_found = reader.FindFirst([](const auto& e) { return e.value == 1; });
    CHECK_EQ(not_found, nullptr);
  }

  TEST_CASE("EventReader - FindFirst Complex Predicate") {
    World world;
    world.AddEvent<ComplexEvent>();

    auto writer = world.WriteEvents<ComplexEvent>();
    writer.Write(ComplexEvent{1, 1.0f, 2.0f, false});
    writer.Write(ComplexEvent{2, 3.0f, 4.0f, true});
    writer.Write(ComplexEvent{3, 5.0f, 6.0f, false});

    auto reader = world.ReadEvents<ComplexEvent>();

    const auto* found = reader.FindFirst([](const auto& e) { return e.is_critical && e.x > 2.0f; });
    REQUIRE_NE(found, nullptr);
    CHECK_EQ(found->id, 2);
  }

  TEST_CASE("EventReader - CountIf Zero Matches") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});

    auto reader = world.ReadEvents<TestEvent>();

    size_t count = reader.CountIf([](const auto& e) { return e.value > 100; });
    CHECK_EQ(count, 0);
  }

  TEST_CASE("EventReader - CountIf All Match") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();

    size_t count = reader.CountIf([](const auto& e) { return e.value > 0; });
    CHECK_EQ(count, 3);
  }

  TEST_CASE("EventReader - CountIf Partial Match") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});
    writer.Write(TestEvent{40});

    auto reader = world.ReadEvents<TestEvent>();

    size_t count = reader.CountIf([](const auto& e) { return e.value >= 25; });
    CHECK_EQ(count, 2);
  }

  TEST_CASE("EventReader - CountIf Empty") {
    World world;
    world.AddEvent<TestEvent>();

    auto reader = world.ReadEvents<TestEvent>();

    size_t count = reader.CountIf([](const auto& e) { return e.value > 0; });
    CHECK_EQ(count, 0);
  }

  TEST_CASE("EventReader - Double Buffering Current Frame") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});

    auto reader = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader.Count(), 1);
  }

  TEST_CASE("EventReader - Double Buffering Previous Frame") {
    World world;
    world.AddEvent<TestEvent>();

    // Frame 0: Write event
    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    world.Update();

    // Frame 1: Event now in previous queue
    auto reader = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader.Count(), 1);
  }

  TEST_CASE("EventReader - Double Buffering Both Queues") {
    World world;
    world.AddEvent<TestEvent>();

    // Frame 0: Write event
    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    world.Update();

    // Frame 1: Write new event, previous still visible
    writer.Write(TestEvent{2});

    auto reader = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader.Count(), 2);

    bool has_1 = false;
    bool has_2 = false;
    for (const auto& event : reader) {
      if (event.value == 1)
        has_1 = true;
      if (event.value == 2)
        has_2 = true;
    }
    CHECK(has_1);
    CHECK(has_2);
  }

  TEST_CASE("EventReader - Double Buffering Event Cleared") {
    World world;
    world.AddEvent<TestEvent>();

    // Frame 0: Write event
    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    world.Update();

    // Frame 1: Event in previous queue
    world.Update();

    // Frame 2: Event cleared
    auto reader = world.ReadEvents<TestEvent>();
    CHECK(reader.Empty());
  }

  TEST_CASE("EventReader - Multiple Readers Same Type") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{42});

    auto reader1 = world.ReadEvents<TestEvent>();
    auto reader2 = world.ReadEvents<TestEvent>();

    CHECK_EQ(reader1.Count(), 1);
    CHECK_EQ(reader2.Count(), 1);

    // Both readers should see the same events
    auto events1 = reader1.Read();
    auto events2 = reader2.Read();

    CHECK_EQ(events1.size(), events2.size());
    CHECK_EQ(events1[0].value, events2[0].value);
  }

  TEST_CASE("EventReader - Different Event Types") {
    World world;
    world.AddEvent<TestEvent>();
    world.AddEvent<ComplexEvent>();

    auto test_writer = world.WriteEvents<TestEvent>();
    test_writer.Write(TestEvent{42});
    auto complex_writer = world.WriteEvents<ComplexEvent>();
    complex_writer.Write(ComplexEvent{1, 2.0f, 3.0f, false});

    auto test_reader = world.ReadEvents<TestEvent>();
    auto complex_reader = world.ReadEvents<ComplexEvent>();

    CHECK_EQ(test_reader.Count(), 1);
    CHECK_EQ(complex_reader.Count(), 1);

    auto test_events = test_reader.Read();
    auto complex_events = complex_reader.Read();

    CHECK_EQ(test_events[0].value, 42);
    CHECK_EQ(complex_events[0].id, 1);
  }

  TEST_CASE("EventReader - Cache Consistency") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    writer.Write(TestEvent{2});

    auto reader = world.ReadEvents<TestEvent>();

    // First access caches events
    auto count1 = reader.Count();

    // Subsequent accesses should use cache
    auto count2 = reader.Count();
    auto events = reader.Read();

    CHECK_EQ(count1, count2);
    CHECK_EQ(events.size(), count1);
  }

  TEST_CASE("EventReader - Large Event Count") {
    World world;
    world.AddEvent<TestEvent>();

    // Write 1000 events
    auto writer = world.WriteEvents<TestEvent>();
    for (int i = 0; i < 1000; ++i) {
      writer.Write(TestEvent{i});
    }

    auto reader = world.ReadEvents<TestEvent>();
    CHECK_EQ(reader.Count(), 1000);

    // Verify some events
    auto found_0 = reader.FindFirst([](const auto& e) { return e.value == 0; });
    auto found_500 = reader.FindFirst([](const auto& e) { return e.value == 500; });
    auto found_999 = reader.FindFirst([](const auto& e) { return e.value == 999; });

    CHECK_NE(found_0, nullptr);
    CHECK_NE(found_500, nullptr);
    CHECK_NE(found_999, nullptr);

    // Count even numbers
    size_t even_count = reader.CountIf([](const auto& e) { return e.value % 2 == 0; });
    CHECK_EQ(even_count, 500);
  }

  TEST_CASE("EventReader - Complex Queries") {
    World world;
    world.AddEvent<ComplexEvent>();

    auto writer = world.WriteEvents<ComplexEvent>();
    writer.Write(ComplexEvent{1, 10.0f, 20.0f, false});
    writer.Write(ComplexEvent{2, 30.0f, 40.0f, true});
    writer.Write(ComplexEvent{3, 50.0f, 60.0f, false});
    writer.Write(ComplexEvent{4, 70.0f, 80.0f, true});
    writer.Write(ComplexEvent{5, 90.0f, 100.0f, true});

    auto reader = world.ReadEvents<ComplexEvent>();

    // Find first critical event
    auto first_critical = reader.FindFirst([](const auto& e) { return e.is_critical; });
    REQUIRE_NE(first_critical, nullptr);
    CHECK_EQ(first_critical->id, 2);

    // Count critical events
    size_t critical_count = reader.CountIf([](const auto& e) { return e.is_critical; });
    CHECK_EQ(critical_count, 3);

    // Count high X values
    size_t high_x = reader.CountIf([](const auto& e) { return e.x >= 50.0f; });
    CHECK_EQ(high_x, 3);

    // Find specific combination
    auto specific = reader.FindFirst([](const auto& e) { return e.is_critical && e.x > 60.0f && e.y > 70.0f; });
    REQUIRE_NE(specific, nullptr);
    CHECK_EQ(specific->id, 4);
  }

  TEST_CASE("EventReader - Iterator Operations") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{1});
    writer.Write(TestEvent{2});
    writer.Write(TestEvent{3});

    auto reader = world.ReadEvents<TestEvent>();

    // Test begin/end
    auto it = reader.begin();
    auto end_it = reader.end();

    CHECK_NE(it, end_it);

    // Count using iterator
    size_t count = 0;
    for (auto iter = reader.begin(); iter != reader.end(); ++iter) {
      ++count;
    }
    CHECK_EQ(count, 3);
  }

  TEST_CASE("EventReader - Const Correctness") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{42});

    const auto reader = world.ReadEvents<TestEvent>();

    // All these operations should work with const reader
    CHECK_FALSE(reader.Empty());
    CHECK_EQ(reader.Count(), 1);

    auto events = reader.Read();
    CHECK_EQ(events.size(), 1);

    for (const auto& event : reader) {
      CHECK_EQ(event.value, 42);
    }

    const auto* found = reader.FindFirst([](const auto& e) { return e.value == 42; });
    CHECK_NE(found, nullptr);

    size_t count = reader.CountIf([](const auto& e) { return e.value > 0; });
    CHECK_EQ(count, 1);
  }

  TEST_CASE("EventReader - Filter") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});
    writer.Write(TestEvent{40});

    auto reader = world.ReadEvents<TestEvent>();
    auto filtered = reader.Filter([](const TestEvent& e) { return e.value >= 25; });

    size_t count = 0;
    for (const auto& event : filtered) {
      CHECK(std::get<0>(event).value >= 25);
      ++count;
    }
    CHECK_EQ(count, 2);
  }

  TEST_CASE("EventReader - Map") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();
    auto mapped = reader.Map([](const TestEvent& e) { return e.value * 2; });

    std::vector<int> values;
    for (auto value : mapped) {
      values.push_back(value);
    }

    REQUIRE_EQ(values.size(), 3);
    CHECK_NE(std::ranges::find(values, 20), values.end());
    CHECK_NE(std::ranges::find(values, 40), values.end());
    CHECK_NE(std::ranges::find(values, 60), values.end());
  }

  TEST_CASE("EventReader - Enumerate") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();
    auto enumerated = reader.Enumerate();

    size_t expected_idx = 0;
    for (auto [idx, event] : enumerated) {
      CHECK_EQ(idx, expected_idx);
      ++expected_idx;
    }
    CHECK_EQ(expected_idx, 3);
  }

  TEST_CASE("EventReader - Any") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();

    CHECK(reader.Any([](const auto& e) { return e.value == 20; }));
    CHECK_FALSE(reader.Any([](const auto& e) { return e.value == 99; }));
  }

  TEST_CASE("EventReader - All") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();

    CHECK(reader.All([](const auto& e) { return e.value > 0; }));
    CHECK_FALSE(reader.All([](const auto& e) { return e.value > 15; }));
  }

  TEST_CASE("EventReader - Find") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    auto reader = world.ReadEvents<TestEvent>();

    auto found = reader.Find([](const auto& e) { return e.value == 20; });
    REQUIRE(found.has_value());
    CHECK_EQ(found->value, 20);

    auto not_found = reader.Find([](const auto& e) { return e.value == 99; });
    CHECK_FALSE(not_found.has_value());
  }

  TEST_CASE("EventReader - Take") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});
    writer.Write(TestEvent{40});

    auto reader = world.ReadEvents<TestEvent>();
    auto taken = reader.Take(2);

    size_t count = 0;
    for ([[maybe_unused]] const auto& event : taken) {
      ++count;
    }
    CHECK_EQ(count, 2);
  }

  TEST_CASE("EventReader - Skip") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});
    writer.Write(TestEvent{40});

    auto reader = world.ReadEvents<TestEvent>();
    auto skipped = reader.Skip(2);

    size_t count = 0;
    for ([[maybe_unused]] const auto& event : skipped) {
      ++count;
    }
    CHECK_EQ(count, 2);
  }

  TEST_CASE("EventReader - Chained Operations") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    for (int i = 1; i <= 10; ++i) {
      writer.Write(TestEvent{i * 10});
    }

    auto reader = world.ReadEvents<TestEvent>();
    auto result = reader.Filter([](const TestEvent& e) { return e.value >= 30; }).Take(3).Map([](const TestEvent& e) {
      return e.value / 10;
    });

    std::vector<int> values;
    for (auto value : result) {
      values.push_back(value);
    }

    CHECK_EQ(values.size(), 3);
    // Values should be 3, 4, 5 (from 30, 40, 50)
    CHECK_NE(std::ranges::find(values, 3), values.end());
    CHECK_NE(std::ranges::find(values, 4), values.end());
    CHECK_NE(std::ranges::find(values, 5), values.end());
  }

  TEST_CASE("EventReader - CollectWith Custom Allocator") {
    World world;
    world.AddEvent<TestEvent>();

    auto writer = world.WriteEvents<TestEvent>();
    writer.Write(TestEvent{10});
    writer.Write(TestEvent{20});
    writer.Write(TestEvent{30});

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using Alloc = helios::memory::STLGrowableAllocator<TestEvent, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    auto reader = world.ReadEvents<TestEvent>();
    auto collected = reader.CollectWith(alloc);

    CHECK_EQ(collected.size(), 3);
    CHECK_EQ(collected[0].value, 10);
    CHECK_EQ(collected[1].value, 20);
    CHECK_EQ(collected[2].value, 30);

    // Verify allocator was used
    CHECK_GT(frame_alloc.Stats().total_allocated, 0);
  }

  TEST_CASE("EventReader - CollectWith Empty Events") {
    World world;
    world.AddEvent<TestEvent>();

    // Create a growable frame allocator
    helios::memory::GrowableAllocator<helios::memory::FrameAllocator> frame_alloc(4096);
    using Alloc = helios::memory::STLGrowableAllocator<TestEvent, helios::memory::FrameAllocator>;
    Alloc alloc(frame_alloc);

    auto reader = world.ReadEvents<TestEvent>();
    auto collected = reader.CollectWith(alloc);

    CHECK(collected.empty());
  }
}
