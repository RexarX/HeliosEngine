#include <doctest/doctest.h>

#include <helios/core/ecs/details/commands.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/world.hpp>

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Test components
struct Position {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

// Test events
struct SimpleEvent {
  int value = 0;

  constexpr bool operator==(const SimpleEvent& other) const noexcept = default;
};

struct ComplexEvent {
  std::array<char, 64> message = {};
  int code = 0;

  explicit ComplexEvent(std::string_view msg = "", int c = 0) : code(c) {
    const auto copy_size = std::min(msg.size(), message.size() - 1);
    std::copy_n(msg.begin(), copy_size, message.begin());
    message[copy_size] = '\0';
  }

  bool operator==(const ComplexEvent& other) const noexcept {
    return std::string_view(message.data()) == std::string_view(other.message.data()) && code == other.code;
  }
};

// Test command
class TestCommand : public Command {
public:
  explicit TestCommand(bool* executed_flag) : executed_flag_(executed_flag) {}

  void Execute([[maybe_unused]] World& world) override {
    if (executed_flag_) {
      *executed_flag_ = true;
    }
  }

private:
  bool* executed_flag_ = nullptr;
};

}  // namespace

TEST_SUITE("ecs::details::SystemLocalStorage") {
  TEST_CASE("SystemLocalStorage::SystemLocalStorage: default construction") {
    SystemLocalStorage storage;

    CHECK(storage.Empty());
    CHECK_EQ(storage.CommandCount(), 0);
  }

  TEST_CASE("SystemLocalStorage::EmplaceCommand") {
    World world;
    SystemLocalStorage storage;

    SUBCASE("Emplace single command") {
      bool executed = false;
      storage.EmplaceCommand<TestCommand>(&executed);

      CHECK_FALSE(storage.Empty());
      CHECK_EQ(storage.CommandCount(), 1);

      world.MergeCommands(storage.GetCommands());
      world.Update();

      CHECK(executed);
    }

    SUBCASE("Emplace multiple commands") {
      Entity entity = world.CreateEntity();

      storage.EmplaceCommand<AddComponentCmd<Position>>(entity, Position{1.0F, 2.0F});
      storage.EmplaceCommand<AddComponentCmd<Velocity>>(entity, Velocity{3.0F, 4.0F});

      CHECK_EQ(storage.CommandCount(), 2);

      world.MergeCommands(storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
    }
  }

  TEST_CASE("SystemLocalStorage::AddCommand") {
    World world;
    SystemLocalStorage storage;

    bool executed = false;
    auto command = std::make_unique<TestCommand>(&executed);

    storage.AddCommand(std::move(command));

    CHECK_EQ(storage.CommandCount(), 1);

    world.MergeCommands(storage.GetCommands());
    world.Update();

    CHECK(executed);
  }

  TEST_CASE("SystemLocalStorage::WriteEvent") {
    SystemLocalStorage storage;

    SUBCASE("Write single event") {
      SimpleEvent event{42};
      storage.WriteEvent(event);

      CHECK_FALSE(storage.Empty());

      auto& event_queue = storage.GetEventQueue();
      CHECK(event_queue.HasEvents<SimpleEvent>());
    }

    SUBCASE("Write multiple events same type") {
      storage.WriteEvent(SimpleEvent{10});
      storage.WriteEvent(SimpleEvent{20});
      storage.WriteEvent(SimpleEvent{30});

      auto& event_queue = storage.GetEventQueue();
      auto events = event_queue.Read<SimpleEvent>();

      CHECK_EQ(events.size(), 3);
      CHECK_EQ(events[0].value, 10);
      CHECK_EQ(events[1].value, 20);
      CHECK_EQ(events[2].value, 30);
    }

    SUBCASE("Write different event types") {
      storage.WriteEvent(SimpleEvent{42});
      storage.WriteEvent(ComplexEvent("Test", 100));

      auto& event_queue = storage.GetEventQueue();
      CHECK(event_queue.HasEvents<SimpleEvent>());
      CHECK(event_queue.HasEvents<ComplexEvent>());
      CHECK_EQ(event_queue.TypeCount(), 2);
    }
  }

  TEST_CASE("SystemLocalStorage::WriteEventBulk") {
    SystemLocalStorage storage;

    SUBCASE("Write multiple events at once") {
      std::vector<SimpleEvent> events = {{1}, {2}, {3}, {4}, {5}};
      storage.WriteEventBulk(std::span<const SimpleEvent>(events));

      auto& event_queue = storage.GetEventQueue();
      auto read_events = event_queue.Read<SimpleEvent>();

      CHECK_EQ(read_events.size(), 5);
      for (size_t i = 0; i < 5; ++i) {
        CHECK_EQ(read_events[i].value, static_cast<int>(i + 1));
      }
    }

    SUBCASE("Write empty span") {
      std::vector<SimpleEvent> events;
      storage.WriteEventBulk(std::span<const SimpleEvent>(events));

      auto& event_queue = storage.GetEventQueue();
      CHECK_FALSE(event_queue.HasEvents<SimpleEvent>());
    }
  }

  TEST_CASE("SystemLocalStorage::Clear") {
    SystemLocalStorage storage;

    SUBCASE("Clear commands") {
      storage.EmplaceCommand<TestCommand>(nullptr);
      storage.EmplaceCommand<TestCommand>(nullptr);

      CHECK_EQ(storage.CommandCount(), 2);

      storage.Clear();

      CHECK(storage.Empty());
      CHECK_EQ(storage.CommandCount(), 0);
    }

    SUBCASE("Clear events") {
      storage.WriteEvent(SimpleEvent{42});
      storage.WriteEvent(ComplexEvent{"Test", 100});

      auto& event_queue = storage.GetEventQueue();
      CHECK_EQ(event_queue.TypeCount(), 2);

      storage.Clear();

      CHECK(storage.Empty());
      CHECK_EQ(event_queue.TypeCount(), 0);
    }

    SUBCASE("Clear commands and events") {
      storage.EmplaceCommand<TestCommand>(nullptr);
      storage.WriteEvent(SimpleEvent{42});

      CHECK_FALSE(storage.Empty());

      storage.Clear();

      CHECK(storage.Empty());
      CHECK_EQ(storage.CommandCount(), 0);
      CHECK_EQ(storage.GetEventQueue().TypeCount(), 0);
    }
  }

  TEST_CASE("SystemLocalStorage::Empty") {
    SystemLocalStorage storage;

    SUBCASE("Empty after construction") {
      CHECK(storage.Empty());
    }

    SUBCASE("Not empty with commands") {
      storage.EmplaceCommand<TestCommand>(nullptr);
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Not empty with events") {
      storage.WriteEvent(SimpleEvent{42});
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Not empty with both") {
      storage.EmplaceCommand<TestCommand>(nullptr);
      storage.WriteEvent(SimpleEvent{42});
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Empty after clear") {
      storage.EmplaceCommand<TestCommand>(nullptr);
      storage.WriteEvent(SimpleEvent{42});

      storage.Clear();

      CHECK(storage.Empty());
    }
  }

  TEST_CASE("SystemLocalStorage::CommandCount") {
    SystemLocalStorage storage;

    CHECK_EQ(storage.CommandCount(), 0);

    storage.EmplaceCommand<TestCommand>(nullptr);
    CHECK_EQ(storage.CommandCount(), 1);

    storage.EmplaceCommand<TestCommand>(nullptr);
    CHECK_EQ(storage.CommandCount(), 2);

    storage.EmplaceCommand<TestCommand>(nullptr);
    CHECK_EQ(storage.CommandCount(), 3);

    storage.Clear();
    CHECK_EQ(storage.CommandCount(), 0);
  }

  TEST_CASE("SystemLocalStorage::ReserveCommands") {
    SystemLocalStorage storage;

    SUBCASE("Reserve space") {
      storage.ReserveCommands(100);

      // Should still be empty after reserve
      CHECK(storage.Empty());
      CHECK_EQ(storage.CommandCount(), 0);
    }

    SUBCASE("Reserve and add") {
      storage.ReserveCommands(10);

      for (int i = 0; i < 5; ++i) {
        storage.EmplaceCommand<TestCommand>(nullptr);
      }

      CHECK_EQ(storage.CommandCount(), 5);
    }
  }

  TEST_CASE("SystemLocalStorage::GetCommands") {
    World world;
    SystemLocalStorage storage;

    SUBCASE("Get mutable reference") {
      Entity entity = world.CreateEntity();
      storage.EmplaceCommand<AddComponentCmd<Position>>(entity, Position{1.0F, 2.0F});

      auto& commands = storage.GetCommands();
      CHECK_EQ(commands.size(), 1);
    }

    SUBCASE("Get const reference") {
      Entity entity = world.CreateEntity();
      storage.EmplaceCommand<AddComponentCmd<Position>>(entity, Position{1.0F, 2.0F});

      const auto& const_storage = storage;
      const auto& commands = const_storage.GetCommands();
      CHECK_EQ(commands.size(), 1);
    }
  }

  TEST_CASE("SystemLocalStorage::GetEventQueue") {
    SystemLocalStorage storage;

    SUBCASE("Get mutable reference") {
      storage.WriteEvent(SimpleEvent{42});

      auto& event_queue = storage.GetEventQueue();
      CHECK(event_queue.HasEvents<SimpleEvent>());
    }

    SUBCASE("Get const reference") {
      storage.WriteEvent(SimpleEvent{42});

      const auto& const_storage = storage;
      const auto& event_queue = const_storage.GetEventQueue();
      CHECK(event_queue.HasEvents<SimpleEvent>());
    }
  }

  TEST_CASE("SystemLocalStorage::SystemLocalStorage: move semantics") {
    SystemLocalStorage storage1;

    storage1.EmplaceCommand<TestCommand>(nullptr);
    storage1.WriteEvent(SimpleEvent{42});

    SUBCASE("Move construction") {
      SystemLocalStorage storage2(std::move(storage1));

      CHECK_FALSE(storage2.Empty());
      CHECK_EQ(storage2.CommandCount(), 1);
      CHECK(storage2.GetEventQueue().HasEvents<SimpleEvent>());
    }

    SUBCASE("Move assignment") {
      SystemLocalStorage storage2;
      storage2 = std::move(storage1);

      CHECK_FALSE(storage2.Empty());
      CHECK_EQ(storage2.CommandCount(), 1);
      CHECK(storage2.GetEventQueue().HasEvents<SimpleEvent>());
    }
  }

  TEST_CASE("SystemLocalStorage: integration with World") {
    World world;
    SystemLocalStorage storage;

    SUBCASE("Commands execute correctly") {
      Entity entity = world.CreateEntity();

      storage.EmplaceCommand<AddComponentCmd<Position>>(entity, Position{1.0F, 2.0F});
      storage.EmplaceCommand<AddComponentCmd<Velocity>>(entity, Velocity{3.0F, 4.0F});

      world.MergeCommands(storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("Events merge correctly") {
      world.AddEvent<SimpleEvent>();
      world.AddEvent<ComplexEvent>();

      storage.WriteEvent(SimpleEvent{10});
      storage.WriteEvent(SimpleEvent{20});
      storage.WriteEvent(ComplexEvent{"Test", 100});

      world.MergeEventQueue(storage.GetEventQueue());

      auto simple_reader = world.ReadEvents<SimpleEvent>();
      CHECK_EQ(simple_reader.Count(), 2);

      auto complex_reader = world.ReadEvents<ComplexEvent>();
      CHECK_EQ(complex_reader.Count(), 1);
    }
  }

  TEST_CASE("SystemLocalStorage: multiple flush cycles") {
    World world;
    SystemLocalStorage storage;

    Entity entity = world.CreateEntity();

    SUBCASE("First cycle") {
      world.AddEvent<SimpleEvent>();

      storage.EmplaceCommand<AddComponentCmd<Position>>(entity, Position{1.0F, 2.0F});
      storage.WriteEvent(SimpleEvent{10});

      world.MergeCommands(storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));

      storage.Clear();
    }

    SUBCASE("Second cycle") {
      world.AddEvent<SimpleEvent>();

      storage.EmplaceCommand<AddComponentCmd<Velocity>>(entity, Velocity{3.0F, 4.0F});
      storage.WriteEvent(SimpleEvent{20});

      world.MergeCommands(storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Velocity>(entity));

      storage.Clear();
    }
  }

  TEST_CASE("SystemLocalStorage: large scale operations") {
    World world;
    SystemLocalStorage storage;

    SUBCASE("Many commands") {
      constexpr size_t command_count = 100;

      for (size_t i = 0; i < command_count; ++i) {
        Entity entity = world.CreateEntity();
        storage.EmplaceCommand<AddComponentCmd<Position>>(entity, Position{static_cast<float>(i), 0.0F});
      }

      CHECK_EQ(storage.CommandCount(), command_count);

      world.MergeCommands(storage.GetCommands());
      world.Update();

      CHECK_EQ(world.EntityCount(), command_count);
    }

    SUBCASE("Many events") {
      constexpr size_t event_count = 1000;

      std::vector<SimpleEvent> events;
      events.reserve(event_count);

      for (size_t i = 0; i < event_count; ++i) {
        events.push_back({static_cast<int>(i)});
      }

      storage.WriteEventBulk(std::span<const SimpleEvent>(events));

      auto& event_queue = storage.GetEventQueue();
      auto read_events = event_queue.Read<SimpleEvent>();

      CHECK_EQ(read_events.size(), event_count);
    }
  }

  TEST_CASE("SystemLocalStorage: mixed operations") {
    World world;
    SystemLocalStorage storage;

    // Register events before using them
    world.AddEvent<SimpleEvent>();
    world.AddEvent<ComplexEvent>();

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    // Add commands
    storage.EmplaceCommand<AddComponentCmd<Position>>(entity1, Position{1.0F, 2.0F});
    storage.EmplaceCommand<AddComponentCmd<Velocity>>(entity1, Velocity{3.0F, 4.0F});
    storage.EmplaceCommand<AddComponentCmd<Position>>(entity2, Position{5.0F, 6.0F});

    // Add events
    storage.WriteEvent(SimpleEvent{100});
    storage.WriteEvent(ComplexEvent{"Message", 200});
    storage.WriteEvent(SimpleEvent{200});

    CHECK_EQ(storage.CommandCount(), 3);
    CHECK_FALSE(storage.Empty());

    // Merge and execute
    world.MergeCommands(storage.GetCommands());
    world.MergeEventQueue(storage.GetEventQueue());
    world.Update();

    // Verify commands
    CHECK(world.HasComponent<Position>(entity1));
    CHECK(world.HasComponent<Velocity>(entity1));
    CHECK(world.HasComponent<Position>(entity2));
    CHECK_FALSE(world.HasComponent<Velocity>(entity2));

    // Verify events
    auto simple_reader = world.ReadEvents<SimpleEvent>();
    CHECK_EQ(simple_reader.Count(), 2);

    auto complex_reader = world.ReadEvents<ComplexEvent>();
    CHECK_EQ(complex_reader.Count(), 1);
  }

  TEST_CASE("SystemLocalStorage: reuse after clear") {
    World world;
    SystemLocalStorage storage;

    // Register event before using it
    world.AddEvent<SimpleEvent>();

    // First use
    storage.EmplaceCommand<TestCommand>(nullptr);
    storage.WriteEvent(SimpleEvent{10});

    CHECK_FALSE(storage.Empty());

    storage.Clear();
    CHECK(storage.Empty());

    // Second use
    storage.EmplaceCommand<TestCommand>(nullptr);
    storage.WriteEvent(SimpleEvent{20});

    CHECK_FALSE(storage.Empty());
    CHECK_EQ(storage.CommandCount(), 1);

    auto& event_queue = storage.GetEventQueue();
    auto events = event_queue.Read<SimpleEvent>();
    CHECK_EQ(events.size(), 1);
    CHECK_EQ(events[0].value, 20);
  }

}  // TEST_SUITE
