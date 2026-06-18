#include <doctest/doctest.h>

#include <helios/ecs/command/command.hpp>

#include <string>

using namespace helios::ecs;

namespace {

struct SimpleCommand {
  int value = 0;

  void Execute(World& /*world*/) {}
};

struct CommandWithData {
  std::string message;

  void Execute(World& /*world*/) {}
};

struct MoveOnlyCommand {
  MoveOnlyCommand() = default;
  MoveOnlyCommand(const MoveOnlyCommand&) = delete;
  MoveOnlyCommand(MoveOnlyCommand&&) = default;

  MoveOnlyCommand& operator=(const MoveOnlyCommand&) = delete;
  MoveOnlyCommand& operator=(MoveOnlyCommand&&) = default;

  int counter = 0;

  void Execute(World& /*world*/) {}
};

class PolymorphicCommand {
public:
  virtual ~PolymorphicCommand() = default;
  virtual void Execute(World& /*world*/) {}
};

struct NonCopyableNonMovableCommand {
  NonCopyableNonMovableCommand() = default;
  NonCopyableNonMovableCommand(const NonCopyableNonMovableCommand&) = delete;
  NonCopyableNonMovableCommand(NonCopyableNonMovableCommand&&) = delete;

  NonCopyableNonMovableCommand& operator=(const NonCopyableNonMovableCommand&) =
      delete;
  NonCopyableNonMovableCommand& operator=(NonCopyableNonMovableCommand&&) =
      delete;

  void Execute(World& /*world*/) {}
};

struct NoExecuteMethod {
  void DoSomething(World& /*world*/) {}
};

}  // namespace

TEST_SUITE("helios::ecs::CommandTrait") {
  TEST_CASE("ecs::CommandTrait::concept") {
    SUBCASE("Regular command types satisfy CommandTrait") {
      CHECK_UNARY(CommandTrait<SimpleCommand>);
      CHECK_UNARY(CommandTrait<CommandWithData>);
      CHECK_UNARY(CommandTrait<MoveOnlyCommand>);
    }

    SUBCASE("Polymorphic types do not satisfy CommandTrait") {
      CHECK_FALSE(CommandTrait<PolymorphicCommand>);
    }

    SUBCASE("Non-copyable and non-movable types do not satisfy CommandTrait") {
      CHECK_FALSE(CommandTrait<NonCopyableNonMovableCommand>);
    }

    SUBCASE("Types without Execute method do not satisfy CommandTrait") {
      CHECK_FALSE(CommandTrait<NoExecuteMethod>);
    }
  }
}
