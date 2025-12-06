#include <doctest/doctest.h>

#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/command_buffer.hpp>
#include <helios/core/ecs/details/commands.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/world.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
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

struct Health {
  int value = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

// Test command for validation
class TestCommand : public Command {
public:
  explicit TestCommand(int value, bool* executed_flag, int* execution_value)
      : value_(value), executed_flag_(executed_flag), execution_value_(execution_value) {}

  void Execute([[maybe_unused]] World& world) override {
    if (executed_flag_)
      *executed_flag_ = true;
    if (execution_value_)
      *execution_value_ = value_;
  }

private:
  int value_ = 0;
  bool* executed_flag_ = nullptr;
  int* execution_value_ = nullptr;
};

// Counting command to test execution order
class CountingCommand : public Command {
public:
  explicit CountingCommand(int id, std::vector<int>* execution_order) : id_(id), execution_order_(execution_order) {}

  void Execute([[maybe_unused]] World& world) override {
    if (execution_order_) {
      execution_order_->push_back(id_);
    }
  }

private:
  int id_ = 0;
  std::vector<int>* execution_order_;
};

}  // namespace

TEST_SUITE("ecs::details::CommandBuffer") {
  TEST_CASE("CmdBuffer::ctor: basic construction") {
    SystemLocalStorage local_storage;

    SUBCASE("Construct with local storage reference") {
      CmdBuffer cmd_buffer(local_storage);
      // Should construct without issues
    }
  }

  TEST_CASE("CmdBuffer::Emplace: command enqueueing") {
    World world;
    SystemLocalStorage local_storage;

    SUBCASE("Emplace command with arguments") {
      bool executed = false;
      int execution_value = 0;
      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TestCommand>(42, &executed, &execution_value);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(executed);
      CHECK_EQ(execution_value, 42);
    }
  }

  TEST_CASE("CmdBuffer::Emplace: multiple commands execution order") {
    World world;
    SystemLocalStorage local_storage;
    std::vector<int> order;

    {
      CmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Emplace<CountingCommand>(1, &order);
      cmd_buffer.Emplace<CountingCommand>(2, &order);
      cmd_buffer.Emplace<CountingCommand>(3, &order);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK_EQ(order.size(), 3);
    CHECK_EQ(order[0], 1);
    CHECK_EQ(order[1], 2);
    CHECK_EQ(order[2], 3);
  }

  TEST_CASE("CmdBuffer: entity commands integration") {
    World world;
    SystemLocalStorage local_storage;

    Entity entity1 = world.CreateEntity();
    Entity entity2 = world.CreateEntity();

    SUBCASE("Add component commands") {
      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<AddComponentCmd<Position>>(entity1, Position{10.0F, 20.0F});
        cmd_buffer.Emplace<AddComponentCmd<Velocity>>(entity1, Velocity{1.0F, 2.0F});
        cmd_buffer.Emplace<AddComponentCmd<Position>>(entity2, Position{30.0F, 40.0F});
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity1));
      CHECK(world.HasComponent<Velocity>(entity1));
      CHECK(world.HasComponent<Position>(entity2));
      CHECK_FALSE(world.HasComponent<Velocity>(entity2));
    }

    SUBCASE("Remove component commands") {
      // First add components
      world.AddComponent(entity1, Position{10.0F, 20.0F});
      world.AddComponent(entity1, Velocity{1.0F, 2.0F});
      world.AddComponent(entity1, Health{100});

      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<RemoveComponentCmd<Velocity>>(entity1);
        cmd_buffer.Emplace<RemoveComponentCmd<Health>>(entity1);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity1));
      CHECK_FALSE(world.HasComponent<Velocity>(entity1));
      CHECK_FALSE(world.HasComponent<Health>(entity1));
    }

    SUBCASE("Destroy entity commands") {
      world.AddComponent(entity1, Position{10.0F, 20.0F});
      world.AddComponent(entity2, Velocity{1.0F, 2.0F});

      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<DestroyEntityCmd>(entity1);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_FALSE(world.Exists(entity1));
      CHECK(world.Exists(entity2));
      CHECK_EQ(world.EntityCount(), 1);
    }

    SUBCASE("Multiple entities destroy command") {
      world.AddComponent(entity1, Position{10.0F, 20.0F});
      world.AddComponent(entity2, Velocity{1.0F, 2.0F});

      {
        CmdBuffer cmd_buffer(local_storage);
        std::vector<Entity> entities_to_destroy = {entity1, entity2};
        cmd_buffer.Emplace<DestroyEntitiesCmd>(entities_to_destroy);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_FALSE(world.Exists(entity1));
      CHECK_FALSE(world.Exists(entity2));
      CHECK_EQ(world.EntityCount(), 0);
    }
  }

  TEST_CASE("CmdBuffer: function commands") {
    World world;
    SystemLocalStorage local_storage;
    std::atomic<int> execution_counter = 0;

    SUBCASE("Simple function command") {
      {
        CmdBuffer cmd_buffer(local_storage);
        auto lambda = [&](World&) { execution_counter++; };
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda)>>(lambda);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(execution_counter.load(), 1);
    }

    SUBCASE("Function command with world manipulation") {
      Entity entity = world.CreateEntity();

      {
        CmdBuffer cmd_buffer(local_storage);
        auto lambda = [=](World& w) {
          w.AddComponent(entity, Position{100.0F, 200.0F});
          w.AddComponent(entity, Health{50});
        };
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda)>>(lambda);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Health>(entity));
    }

    SUBCASE("Multiple function commands with dependencies") {
      std::vector<int> execution_order;

      {
        CmdBuffer cmd_buffer(local_storage);
        auto lambda1 = [&execution_order]([[maybe_unused]] World& w) { execution_order.push_back(1); };
        auto lambda2 = [&execution_order]([[maybe_unused]] World& w) { execution_order.push_back(2); };
        auto lambda3 = [&execution_order]([[maybe_unused]] World& w) { execution_order.push_back(3); };
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda1)>>(lambda1);
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda2)>>(lambda2);
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda3)>>(lambda3);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(execution_order.size(), 3);
      CHECK_EQ(execution_order[0], 1);
      CHECK_EQ(execution_order[1], 2);
      CHECK_EQ(execution_order[2], 3);
    }
  }

  TEST_CASE("CmdBuffer: complex command scenarios") {
    World world;
    SystemLocalStorage local_storage;

    SUBCASE("Mixed command types") {
      std::vector<int> execution_steps;

      Entity entity1 = world.CreateEntity();
      Entity entity2 = world.CreateEntity();

      {
        CmdBuffer cmd_buffer(local_storage);

        // Add components
        cmd_buffer.Emplace<AddComponentCmd<Position>>(entity1, Position{1.0F, 2.0F});

        // Function command
        auto lambda1 = [&]([[maybe_unused]] World& w) { execution_steps.push_back(1); };
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda1)>>(lambda1);

        // Add more components
        cmd_buffer.Emplace<AddComponentCmd<Velocity>>(entity1, Velocity{10.0F, 20.0F});
        cmd_buffer.Emplace<AddComponentCmd<Position>>(entity2, Position{3.0F, 4.0F});

        // Another function command
        auto lambda2 = [&]([[maybe_unused]] World& w) { execution_steps.push_back(2); };
        cmd_buffer.Emplace<FunctionCmd<decltype(lambda2)>>(lambda2);

        // Remove component
        cmd_buffer.Emplace<RemoveComponentCmd<Position>>(entity2);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      // Check execution order
      CHECK_EQ(execution_steps.size(), 2);
      CHECK_EQ(execution_steps[0], 1);
      CHECK_EQ(execution_steps[1], 2);

      // Check component states
      CHECK(world.HasComponent<Position>(entity1));
      CHECK(world.HasComponent<Velocity>(entity1));
      CHECK_FALSE(world.HasComponent<Position>(entity2));
    }

    SUBCASE("Command buffer with entity lifecycle") {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{5.0F, 10.0F});
      world.AddComponent(entity, Health{75});

      {
        CmdBuffer cmd_buffer(local_storage);

        // Modify component
        cmd_buffer.Emplace<AddComponentCmd<Velocity>>(entity, Velocity{5.0F, 15.0F});

        // Remove component
        cmd_buffer.Emplace<RemoveComponentCmd<Health>>(entity);

        // Clear all components
        cmd_buffer.Emplace<ClearComponentsCmd>(entity);

        // Add component after clear
        cmd_buffer.Emplace<AddComponentCmd<Position>>(entity, Position{100.0F, 200.0F});
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.Exists(entity));
      CHECK(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
      CHECK_FALSE(world.HasComponent<Health>(entity));
    }
  }

  TEST_CASE("CmdBuffer: empty buffer") {
    World world;
    SystemLocalStorage local_storage;

    SUBCASE("Empty buffer does not affect world") {
      Entity entity = world.CreateEntity();
      world.AddComponent(entity, Position{1.0F, 2.0F});

      size_t initial_count = world.EntityCount();

      {
        CmdBuffer cmd_buffer(local_storage);
        // Don't add any commands
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(world.EntityCount(), initial_count);
      CHECK(world.Exists(entity));
      CHECK(world.HasComponent<Position>(entity));
    }
  }

  TEST_CASE("CmdBuffer::Push: pre-constructed commands") {
    World world;
    SystemLocalStorage local_storage;

    SUBCASE("Push unique_ptr commands") {
      bool executed = false;
      int execution_value = 0;

      {
        CmdBuffer cmd_buffer(local_storage);
        auto command = std::make_unique<TestCommand>(99, &executed, &execution_value);
        cmd_buffer.Push(std::move(command));
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(executed);
      CHECK_EQ(execution_value, 99);
    }

    SUBCASE("Push multiple commands") {
      std::vector<int> order;

      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Push(std::make_unique<CountingCommand>(1, &order));
        cmd_buffer.Push(std::make_unique<CountingCommand>(2, &order));
        cmd_buffer.Push(std::make_unique<CountingCommand>(3, &order));
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(order.size(), 3);
      CHECK_EQ(order[0], 1);
      CHECK_EQ(order[1], 2);
      CHECK_EQ(order[2], 3);
    }
  }

  TEST_CASE("CmdBuffer: multiple buffer scopes") {
    World world;
    SystemLocalStorage local_storage;
    std::vector<int> order;

    SUBCASE("Sequential buffer scopes") {
      {
        CmdBuffer cmd_buffer1(local_storage);
        cmd_buffer1.Emplace<CountingCommand>(1, &order);
      }

      {
        CmdBuffer cmd_buffer2(local_storage);
        cmd_buffer2.Emplace<CountingCommand>(2, &order);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(order.size(), 2);
      CHECK(std::ranges::find(order, 1) != order.end());
      CHECK(std::ranges::find(order, 2) != order.end());
    }
  }

  TEST_CASE("CmdBuffer: try commands") {
    World world;
    SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();

    SUBCASE("TryAddComponent success") {
      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TryAddComponentCmd<Position>>(entity, Position{1.0F, 2.0F});
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));
    }

    SUBCASE("TryAddComponent failure (already exists)") {
      world.AddComponent(entity, Position{1.0F, 2.0F});

      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TryAddComponentCmd<Position>>(entity, Position{10.0F, 20.0F});
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK(world.HasComponent<Position>(entity));
    }

    SUBCASE("TryRemoveComponent success") {
      world.AddComponent(entity, Position{1.0F, 2.0F});

      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TryRemoveComponentCmd<Position>>(entity);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_FALSE(world.HasComponent<Position>(entity));
    }

    SUBCASE("TryRemoveComponent no-op (doesn't exist)") {
      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TryRemoveComponentCmd<Position>>(entity);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_FALSE(world.HasComponent<Position>(entity));
    }

    SUBCASE("TryDestroyEntity success") {
      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TryDestroyEntityCmd>(entity);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("TryDestroyEntity no-op (already destroyed)") {
      world.DestroyEntity(entity);

      {
        CmdBuffer cmd_buffer(local_storage);
        cmd_buffer.Emplace<TryDestroyEntityCmd>(entity);
      }

      world.MergeCommands(local_storage.GetCommands());
      world.Update();

      CHECK_EQ(world.EntityCount(), 0);
    }
  }

  TEST_CASE("CmdBuffer: clear components command") {
    World world;
    SystemLocalStorage local_storage;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F});
    world.AddComponent(entity, Velocity{3.0F, 4.0F});
    world.AddComponent(entity, Health{100});

    {
      CmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Emplace<ClearComponentsCmd>(entity);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    CHECK(world.Exists(entity));
    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));
  }

  TEST_CASE("CmdBuffer: multiple flush cycles") {
    World world;
    SystemLocalStorage local_storage;
    std::atomic<int> counter = 0;

    {
      CmdBuffer cmd_buffer(local_storage);
      cmd_buffer.Emplace<TestCommand>(1, nullptr, nullptr);
      auto lambda = [&counter]([[maybe_unused]] World& w) { counter++; };
      cmd_buffer.Emplace<FunctionCmd<decltype(lambda)>>(lambda);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK_EQ(counter.load(), 1);

    local_storage.Clear();

    {
      CmdBuffer cmd_buffer(local_storage);
      auto lambda = [&counter]([[maybe_unused]] World& w) { counter++; };
      cmd_buffer.Emplace<FunctionCmd<decltype(lambda)>>(lambda);
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();
    CHECK_EQ(counter.load(), 2);
  }

  TEST_CASE("CmdBuffer: large command batch") {
    World world;
    SystemLocalStorage local_storage;

    constexpr size_t command_count = 100;
    std::vector<Entity> entities;

    for (size_t i = 0; i < command_count; ++i) {
      entities.push_back(world.CreateEntity());
    }

    {
      CmdBuffer cmd_buffer(local_storage);
      for (size_t i = 0; i < command_count; ++i) {
        cmd_buffer.Emplace<AddComponentCmd<Position>>(entities[i], Position{static_cast<float>(i), 0.0F});
      }
    }

    world.MergeCommands(local_storage.GetCommands());
    world.Update();

    for (const auto& entity : entities) {
      CHECK(world.HasComponent<Position>(entity));
    }
  }

}  // TEST_SUITE
