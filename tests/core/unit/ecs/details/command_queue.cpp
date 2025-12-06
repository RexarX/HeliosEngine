#include <doctest/doctest.h>

#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/details/command_queue.hpp>
#include <helios/core/ecs/world.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Test command implementations
class TestCommand : public Command {
public:
  mutable bool executed = false;
  mutable World* world_ptr = nullptr;

  void Execute(World& world) override {
    executed = true;
    world_ptr = &world;
  }
};

class CountingCommand : public Command {
public:
  explicit CountingCommand(std::atomic<int>* counter) : counter_(counter) {}

  void Execute(World&) override {
    if (counter_)
      counter_->fetch_add(1);
  }

private:
  std::atomic<int>* counter_ = nullptr;
};

class NoOpCommand : public Command {
public:
  void Execute(World&) override {
    // Do nothing
  }
};

class ParameterizedCommand : public Command {
public:
  int value = 0;

  explicit ParameterizedCommand(int val) : value(val) {}

  void Execute(World&) override {
    // Store value for verification
  }
};

}  // namespace

TEST_SUITE("ecs::details::CommandQueue") {
  TEST_CASE("CmdQueue::CmdQueue: default construction") {
    CmdQueue queue;

    CHECK_EQ(queue.Size(), 0);
    CHECK(queue.Empty());
  }

  TEST_CASE("CmdQueue::Enqueue: single command") {
    CmdQueue queue;

    auto command = std::make_unique<TestCommand>();
    TestCommand* raw_ptr = command.get();

    queue.Enqueue(std::move(command));
    CHECK_EQ(queue.Size(), 1);
    CHECK_FALSE(queue.Empty());

    // Dequeue all and verify
    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 1);
    REQUIRE(dequeued[0] != nullptr);
    CHECK_EQ(dequeued[0].get(), raw_ptr);
    CHECK(queue.Empty());
  }

  TEST_CASE("CmdQueue::Emplace") {
    CmdQueue queue;

    queue.Emplace<ParameterizedCommand>(42);
    CHECK_EQ(queue.Size(), 1);

    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 1);
    REQUIRE(dequeued[0] != nullptr);

    auto* param_cmd = dynamic_cast<ParameterizedCommand*>(dequeued[0].get());
    REQUIRE(param_cmd != nullptr);
    CHECK_EQ(param_cmd->value, 42);
  }

  TEST_CASE("CmdQueue: multiple commands FIFO order") {
    CmdQueue queue;

    std::vector<ParameterizedCommand*> raw_ptrs;

    for (int i = 0; i < 5; ++i) {
      auto command = std::make_unique<ParameterizedCommand>(i);
      raw_ptrs.push_back(command.get());
      queue.Enqueue(std::move(command));
    }

    CHECK_EQ(queue.Size(), 5);

    // Dequeue all and verify order
    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 5);
    CHECK(queue.Empty());

    for (int i = 0; i < 5; ++i) {
      REQUIRE(dequeued[i] != nullptr);
      auto* param_cmd = dynamic_cast<ParameterizedCommand*>(dequeued[i].get());
      REQUIRE(param_cmd != nullptr);
      CHECK_EQ(param_cmd->value, i);
    }
  }

  TEST_CASE("CmdQueue::DequeueAll: empty queue") {
    CmdQueue queue;

    auto dequeued = queue.DequeueAll();
    CHECK(dequeued.empty());
    CHECK(queue.Empty());
  }

  TEST_CASE("CmdQueue::EnqueueBulk") {
    CmdQueue queue;

    std::vector<std::unique_ptr<Command>> commands;
    for (int i = 0; i < 10; ++i) {
      commands.push_back(std::make_unique<ParameterizedCommand>(i));
    }

    queue.EnqueueBulk(commands);
    CHECK_EQ(queue.Size(), 10);

    // Verify all commands were enqueued in order
    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 10);

    for (int i = 0; i < 10; ++i) {
      REQUIRE(dequeued[i] != nullptr);
      auto* param_cmd = dynamic_cast<ParameterizedCommand*>(dequeued[i].get());
      REQUIRE(param_cmd != nullptr);
      CHECK_EQ(param_cmd->value, i);
    }
  }

  TEST_CASE("CmdQueue::Clear") {
    CmdQueue queue;

    // Enqueue many commands
    for (int i = 0; i < 100; ++i) {
      queue.Emplace<NoOpCommand>();
    }

    CHECK_EQ(queue.Size(), 100);

    queue.Clear();
    CHECK_EQ(queue.Size(), 0);
    CHECK(queue.Empty());

    // Should be able to enqueue after clear
    queue.Emplace<TestCommand>();
    CHECK_EQ(queue.Size(), 1);
  }

  TEST_CASE("CmdQueue::Size: accuracy") {
    CmdQueue queue;

    CHECK_EQ(queue.Size(), 0);

    // Add commands and check size
    for (int i = 1; i <= 10; ++i) {
      queue.Emplace<NoOpCommand>();
      CHECK_EQ(queue.Size(), i);
    }

    // DequeueAll removes all
    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 10);
    CHECK_EQ(queue.Size(), 0);
  }

  TEST_CASE("CmdQueue::Reserve: capacity") {
    CmdQueue queue;

    queue.Reserve(100);

    // Enqueue commands without reallocation
    for (int i = 0; i < 50; ++i) {
      queue.Emplace<ParameterizedCommand>(i);
    }

    CHECK_EQ(queue.Size(), 50);

    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 50);

    // Verify order is maintained
    for (int i = 0; i < 50; ++i) {
      auto* param_cmd = dynamic_cast<ParameterizedCommand*>(dequeued[i].get());
      REQUIRE(param_cmd != nullptr);
      CHECK_EQ(param_cmd->value, i);
    }
  }

  TEST_CASE("CmdQueue: empty bulk operations") {
    CmdQueue queue;

    // Test empty bulk enqueue
    std::vector<std::unique_ptr<Command>> empty_commands;
    queue.EnqueueBulk(empty_commands);
    CHECK_EQ(queue.Size(), 0);

    // Test DequeueAll from empty queue
    auto dequeued = queue.DequeueAll();
    CHECK(dequeued.empty());
  }

  TEST_CASE("CmdQueue: large scale operations") {
    CmdQueue queue;
    constexpr size_t large_count = 10000;

    // Reserve space for efficiency
    queue.Reserve(large_count);

    // Enqueue many commands
    std::vector<std::unique_ptr<Command>> commands;
    commands.reserve(large_count);

    for (size_t i = 0; i < large_count; ++i) {
      commands.push_back(std::make_unique<ParameterizedCommand>(static_cast<int>(i)));
    }

    queue.EnqueueBulk(commands);
    CHECK_EQ(queue.Size(), large_count);

    // Dequeue all at once
    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), large_count);
    CHECK(queue.Empty());

    // Verify order is maintained
    for (size_t i = 0; i < large_count; ++i) {
      REQUIRE(dequeued[i] != nullptr);
      auto* param_cmd = dynamic_cast<ParameterizedCommand*>(dequeued[i].get());
      REQUIRE(param_cmd != nullptr);
      CHECK_EQ(param_cmd->value, static_cast<int>(i));
    }
  }

  TEST_CASE("CmdQueue: mixed enqueue operations") {
    CmdQueue queue;

    // Mix single enqueues and bulk enqueues
    queue.Emplace<ParameterizedCommand>(0);
    queue.Emplace<ParameterizedCommand>(1);

    std::vector<std::unique_ptr<Command>> bulk1;
    bulk1.push_back(std::make_unique<ParameterizedCommand>(2));
    bulk1.push_back(std::make_unique<ParameterizedCommand>(3));
    queue.EnqueueBulk(bulk1);

    queue.Emplace<ParameterizedCommand>(4);

    std::vector<std::unique_ptr<Command>> bulk2;
    bulk2.push_back(std::make_unique<ParameterizedCommand>(5));
    queue.EnqueueBulk(bulk2);

    CHECK_EQ(queue.Size(), 6);

    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 6);

    // Verify all commands are in order
    for (int i = 0; i < 6; ++i) {
      REQUIRE(dequeued[i] != nullptr);
      auto* param_cmd = dynamic_cast<ParameterizedCommand*>(dequeued[i].get());
      REQUIRE(param_cmd != nullptr);
      CHECK_EQ(param_cmd->value, i);
    }
  }

  TEST_CASE("CmdQueue: command execution") {
    CmdQueue queue;
    World world;

    std::atomic<int> execution_count{0};

    // Add counting commands
    for (int i = 0; i < 50; ++i) {
      queue.Emplace<CountingCommand>(&execution_count);
    }

    CHECK_EQ(queue.Size(), 50);

    // Execute all commands
    auto commands = queue.DequeueAll();
    CHECK(queue.Empty());

    for (auto& command : commands) {
      if (command) {
        command->Execute(world);
      }
    }

    CHECK_EQ(execution_count.load(), 50);
  }

  TEST_CASE("CmdQueue: reuse after DequeueAll") {
    CmdQueue queue;

    // First batch
    for (int i = 0; i < 10; ++i) {
      queue.Emplace<ParameterizedCommand>(i);
    }

    auto first_batch = queue.DequeueAll();
    CHECK_EQ(first_batch.size(), 10);
    CHECK(queue.Empty());

    // Second batch - queue should be reusable
    for (int i = 10; i < 20; ++i) {
      queue.Emplace<ParameterizedCommand>(i);
    }

    auto second_batch = queue.DequeueAll();
    CHECK_EQ(second_batch.size(), 10);
    CHECK(queue.Empty());

    // Verify both batches are correct
    for (int i = 0; i < 10; ++i) {
      auto* cmd1 = dynamic_cast<ParameterizedCommand*>(first_batch[i].get());
      REQUIRE(cmd1 != nullptr);
      CHECK_EQ(cmd1->value, i);

      auto* cmd2 = dynamic_cast<ParameterizedCommand*>(second_batch[i].get());
      REQUIRE(cmd2 != nullptr);
      CHECK_EQ(cmd2->value, i + 10);
    }
  }

  TEST_CASE("CmdQueue::EnqueueBulk: preserves order") {
    CmdQueue queue;

    // Create two batches
    std::vector<std::unique_ptr<Command>> batch1;
    for (int i = 0; i < 5; ++i) {
      batch1.push_back(std::make_unique<ParameterizedCommand>(i));
    }

    std::vector<std::unique_ptr<Command>> batch2;
    for (int i = 5; i < 10; ++i) {
      batch2.push_back(std::make_unique<ParameterizedCommand>(i));
    }

    // Enqueue both batches
    queue.EnqueueBulk(batch1);
    queue.EnqueueBulk(batch2);

    CHECK_EQ(queue.Size(), 10);

    // Verify order is maintained across batches
    auto dequeued = queue.DequeueAll();
    CHECK_EQ(dequeued.size(), 10);

    for (int i = 0; i < 10; ++i) {
      auto* cmd = dynamic_cast<ParameterizedCommand*>(dequeued[i].get());
      REQUIRE(cmd != nullptr);
      CHECK_EQ(cmd->value, i);
    }
  }

  TEST_CASE("CmdQueue::Clear: after operations") {
    CmdQueue queue;

    // Enqueue, dequeue, then clear
    for (int i = 0; i < 5; ++i) {
      queue.Emplace<NoOpCommand>();
    }

    auto dequeued = queue.DequeueAll();
    CHECK(queue.Empty());

    // Add more commands
    for (int i = 0; i < 3; ++i) {
      queue.Emplace<NoOpCommand>();
    }
    CHECK_EQ(queue.Size(), 3);

    // Clear should work
    queue.Clear();
    CHECK(queue.Empty());
    CHECK_EQ(queue.Size(), 0);
  }

  TEST_CASE("CmdQueue::Empty: check consistency") {
    CmdQueue queue;

    CHECK(queue.Empty());
    CHECK_EQ(queue.Size(), 0);

    queue.Emplace<NoOpCommand>();
    CHECK_FALSE(queue.Empty());
    CHECK_EQ(queue.Size(), 1);

    queue.Clear();
    CHECK(queue.Empty());
    CHECK_EQ(queue.Size(), 0);

    queue.Emplace<NoOpCommand>();
    CHECK_FALSE(queue.Empty());

    [[maybe_unused]] const auto commands = queue.DequeueAll();
    CHECK(queue.Empty());
    CHECK_EQ(queue.Size(), 0);
  }

}  // TEST_SUITE
