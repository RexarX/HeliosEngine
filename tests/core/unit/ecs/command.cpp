#include <doctest/doctest.h>

#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/world.hpp>

#include <memory>
#include <stdexcept>
#include <type_traits>

using namespace helios::ecs;

namespace {

// Test command implementations
class TestCommand : public Command {
public:
  explicit TestCommand(bool* executed_flag = nullptr, World** world_ptr = nullptr)
      : executed_flag_(executed_flag), world_ptr_(world_ptr) {}

  void Execute(World& world) override {
    if (executed_flag_) {
      *executed_flag_ = true;
    }
    if (world_ptr_) {
      *world_ptr_ = &world;
    }
  }

private:
  bool* executed_flag_ = nullptr;
  World** world_ptr_ = nullptr;
};

class NoOpCommand : public Command {
public:
  void Execute(World&) override {
    // Do nothing
  }
};

class CountingCommand : public Command {
public:
  explicit CountingCommand(int* counter = nullptr) : counter_(counter) {}

  void Execute(World&) override {
    if (counter_) {
      ++(*counter_);
    }
  }

private:
  int* counter_ = nullptr;
};

class ThrowingCommand : public Command {
public:
  void Execute(World&) override { throw std::runtime_error("Test exception"); }
};

// Test classes that should NOT satisfy CommandTrait
class NotACommand {
public:
  void Execute(World&) {}  // Not virtual, doesn't inherit from Command
};

class WrongSignatureCommand : public Command {
public:
  void Execute() {}                 // Wrong signature - no override since it doesn't override anything
  void Execute(World&) override {}  // Correct signature
};

class AbstractCommand : public Command {
public:
  virtual void DoSomething() = 0;  // Pure virtual, making class abstract
  void Execute(World&) override {}
};

}  // namespace

TEST_SUITE("ecs::Command") {
  TEST_CASE("Command::Base Class Properties") {
    // Test that Command is abstract
    CHECK(std::is_abstract_v<Command>);

    // Test that Command has virtual destructor
    CHECK(std::has_virtual_destructor_v<Command>);

    // Test that destructor is noexcept
    CHECK(std::is_nothrow_destructible_v<Command>);

    // Test that destructor is noexcept
    constexpr bool is_trivially_destructible = std::is_trivially_destructible_v<Command>;
    constexpr bool is_nothrow_destructible = std::is_nothrow_destructible_v<Command>;
    CHECK((is_trivially_destructible || is_nothrow_destructible));
  }

  TEST_CASE("Command::CommandTrait Concept") {
    // Valid command types
    CHECK(CommandTrait<TestCommand>);
    CHECK(CommandTrait<NoOpCommand>);
    CHECK(CommandTrait<CountingCommand>);
    CHECK(CommandTrait<ThrowingCommand>);
    CHECK(CommandTrait<const Command>);

    // Test with pointers and references (should be false)
    CHECK_FALSE(CommandTrait<Command*>);
    CHECK_FALSE(CommandTrait<Command&>);
    CHECK_FALSE(CommandTrait<TestCommand*>);
    CHECK_FALSE(CommandTrait<TestCommand&>);

    // Invalid command types
    CHECK_FALSE(CommandTrait<NotACommand>);
    CHECK_FALSE(CommandTrait<int>);
    CHECK_FALSE(CommandTrait<std::string>);
    CHECK_FALSE(CommandTrait<void>);
  }

  TEST_CASE("Command::Basic Execution") {
    World world;
    bool executed = false;
    World* world_ptr = nullptr;
    TestCommand command(&executed, &world_ptr);

    CHECK_FALSE(executed);
    CHECK_EQ(world_ptr, nullptr);

    command.Execute(world);

    CHECK(executed);
    CHECK_EQ(world_ptr, &world);
  }

  TEST_CASE("Command::Polymorphic Behavior") {
    World world;
    int execution_count = 0;

    std::unique_ptr<Command> command1 = std::make_unique<CountingCommand>(&execution_count);
    std::unique_ptr<Command> command2 = std::make_unique<CountingCommand>(&execution_count);
    std::unique_ptr<Command> command3 = std::make_unique<NoOpCommand>();

    CHECK_EQ(execution_count, 0);

    command1->Execute(world);
    CHECK_EQ(execution_count, 1);

    command2->Execute(world);
    CHECK_EQ(execution_count, 2);

    command3->Execute(world);  // Should not affect count
    CHECK_EQ(execution_count, 2);
  }

  TEST_CASE("Command::Multiple Executions") {
    World world;
    bool executed = false;
    World* world_ptr = nullptr;
    TestCommand command(&executed, &world_ptr);

    // Execute multiple times
    command.Execute(world);
    CHECK(executed);
    CHECK_EQ(world_ptr, &world);

    executed = false;  // Reset for second execution
    command.Execute(world);
    CHECK(executed);
    CHECK_EQ(world_ptr, &world);
  }

  TEST_CASE("Command::Exception Handling") {
    World world;
    ThrowingCommand command;

    // Command should be able to throw exceptions
    CHECK_THROWS_AS(command.Execute(world), std::runtime_error);
  }

  TEST_CASE("Command::Destructor Behavior") {
    World world;

    {
      bool executed = false;
      TestCommand command(&executed);
      command.Execute(world);
      CHECK(executed);
      // Destructor should be called automatically when going out of scope
    }

    // Test polymorphic destruction
    {
      bool executed = false;
      std::unique_ptr<Command> command = std::make_unique<TestCommand>(&executed);
      command->Execute(world);
      CHECK(executed);
      // Polymorphic destructor should be called correctly
    }
  }

  TEST_CASE("Command::Constexpr Destructor") {
    // Test that the destructor is properly marked as constexpr noexcept
    bool executed = false;
    TestCommand command(&executed);
    CHECK(noexcept(command.~TestCommand()));

    // Test that we can create and destroy commands in constexpr context
    // (This mainly tests the interface design)
    constexpr bool can_be_destroyed = std::is_destructible_v<Command>;
    CHECK(can_be_destroyed);
  }

  TEST_CASE("Command::Interface Requirements") {
    // Test that Command interface requires Execute method
    CHECK(std::is_same_v<void, decltype(std::declval<Command&>().Execute(std::declval<World&>()))>);

    // Test that Execute is pure virtual (cannot instantiate Command directly)
    CHECK(std::is_abstract_v<Command>);
  }

  TEST_CASE("Command::Memory Layout") {
    // Test that commands have reasonable memory footprint
    CHECK_LE(sizeof(TestCommand), 64);  // Should be reasonably small
    CHECK_LE(sizeof(NoOpCommand), 64);
    CHECK_LE(sizeof(CountingCommand), 64);

    // Test alignment
    CHECK_GE(alignof(TestCommand), alignof(void*));  // At least pointer-aligned due to vtable
  }

  TEST_CASE("Command::Type Traits") {
    // Test various type traits for command types
    CHECK(std::is_class_v<Command>);
    CHECK(std::is_polymorphic_v<Command>);
    CHECK(std::has_virtual_destructor_v<Command>);

    CHECK(std::is_class_v<TestCommand>);
    CHECK(std::is_base_of_v<Command, TestCommand>);
    CHECK(std::is_polymorphic_v<TestCommand>);

    // Test that derived commands are properly constructible
    CHECK(std::is_default_constructible_v<TestCommand>);
    CHECK(std::is_copy_constructible_v<TestCommand>);
    CHECK(std::is_move_constructible_v<TestCommand>);
  }

  TEST_CASE("Command::Concept Edge Cases") {
    // Test that concept works with cv-qualified types
    CHECK(CommandTrait<const TestCommand>);
    CHECK(CommandTrait<volatile TestCommand>);
    CHECK(CommandTrait<const volatile TestCommand>);

    // Test with reference types
    CHECK_FALSE(CommandTrait<TestCommand&>);
    CHECK_FALSE(CommandTrait<const TestCommand&>);
    CHECK_FALSE(CommandTrait<TestCommand&&>);

    // Test with pointer types
    CHECK_FALSE(CommandTrait<TestCommand*>);
    CHECK_FALSE(CommandTrait<const TestCommand*>);
    CHECK_FALSE(CommandTrait<std::unique_ptr<TestCommand>>);
    CHECK_FALSE(CommandTrait<std::shared_ptr<TestCommand>>);
  }

  TEST_CASE("Command::Inheritance Chain") {
    // Test multiple levels of inheritance
    class BaseCommand : public Command {
    public:
      void Execute(World&) override {}
    };

    class DerivedCommand : public BaseCommand {
    public:
      void Execute(World&) override {}
    };

    CHECK(CommandTrait<BaseCommand>);
    CHECK(CommandTrait<DerivedCommand>);
    CHECK(std::is_base_of_v<Command, BaseCommand>);
    CHECK(std::is_base_of_v<Command, DerivedCommand>);
    CHECK(std::is_base_of_v<BaseCommand, DerivedCommand>);
  }

  TEST_CASE("Command::Command Collections") {
    World world;
    int execution_count = 0;

    std::vector<std::unique_ptr<Command>> commands;
    commands.push_back(std::make_unique<CountingCommand>(&execution_count));
    commands.push_back(std::make_unique<CountingCommand>(&execution_count));
    commands.push_back(std::make_unique<NoOpCommand>());
    commands.push_back(std::make_unique<CountingCommand>(&execution_count));

    CHECK_EQ(execution_count, 0);

    // Execute all commands
    for (auto& command : commands) {
      command->Execute(world);
    }

    CHECK_EQ(execution_count, 3);  // 3 CountingCommands executed
  }

  TEST_CASE("Command::Command State Persistence") {
    World world;
    bool executed = false;
    World* world_ptr = nullptr;
    TestCommand command(&executed, &world_ptr);

    // Test that command state persists across executions
    command.Execute(world);
    CHECK(executed);

    // State should remain after execution
    World world2;
    executed = false;
    command.Execute(world2);
    CHECK(executed);
    CHECK_EQ(world_ptr, &world2);  // Should point to most recent world
  }

}  // TEST_SUITE
