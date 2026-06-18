#include <doctest/doctest.h>

#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/world.hpp>

#include <memory_resource>
#include <vector>

using namespace helios::ecs;

namespace {

int execute_count = 0;

struct SimpleCommand {
  int increment = 1;

  void Execute(World& /*world*/) { execute_count += increment; }
};

struct MultipleExecuteCommand {
  int value = 0;

  void Execute(World& /*world*/) { execute_count = value; }
};

}  // namespace

TEST_SUITE("helios::ecs::CmdQueue") {
  TEST_CASE("ecs::CmdQueue::ctor") {
    SUBCASE("Default ctor") {
      CmdQueue queue;

      CHECK(queue.Empty());
      CHECK_EQ(queue.Size(), 0);
    }

    SUBCASE("Move ctor") {
      CmdQueue queue1;
      queue1.Enqueue(SimpleCommand{42});

      CmdQueue queue2(std::move(queue1));

      CHECK_FALSE(queue2.Empty());
      CHECK_EQ(queue2.Size(), 1);
    }

    SUBCASE("Custom allocator ctor") {
      std::allocator<std::byte> alloc;
      CmdQueue<std::allocator<std::byte>> queue{alloc};

      CHECK(queue.Empty());
    }
  }

  TEST_CASE("ecs::CmdQueue::operator=") {
    SUBCASE("Move assignment") {
      CmdQueue queue1;
      queue1.Enqueue(SimpleCommand{99});

      CmdQueue queue2;
      queue2 = std::move(queue1);

      CHECK_FALSE(queue2.Empty());
      CHECK_EQ(queue2.Size(), 1);
    }
  }

  TEST_CASE("ecs::CmdQueue::Clear") {
    CmdQueue queue;

    queue.Enqueue(SimpleCommand{1});
    queue.Enqueue(SimpleCommand{2});
    queue.Enqueue(SimpleCommand{3});

    SUBCASE("Clear removes all commands") {
      queue.Clear();

      CHECK(queue.Empty());
      CHECK_EQ(queue.Size(), 0);
    }
  }

  TEST_CASE("ecs::CmdQueue::Reserve") {
    SUBCASE("Reserve doesn't add commands") {
      CmdQueue queue;

      queue.Reserve(10);

      CHECK(queue.Empty());
      CHECK_EQ(queue.Size(), 0);
    }

    SUBCASE("After reserve can enqueue") {
      CmdQueue queue;

      queue.Reserve(5);
      queue.Enqueue(SimpleCommand{1});
      queue.Enqueue(SimpleCommand{2});

      CHECK_EQ(queue.Size(), 2);
    }
  }

  TEST_CASE("ecs::CmdQueue::Enqueue") {
    SUBCASE("Enqueue single command") {
      CmdQueue queue;

      queue.Enqueue(SimpleCommand{10});

      CHECK_FALSE(queue.Empty());
      CHECK_EQ(queue.Size(), 1);
    }

    SUBCASE("Enqueue multiple commands") {
      CmdQueue queue;

      queue.Enqueue(SimpleCommand{5});
      queue.Enqueue(SimpleCommand{3});
      queue.Enqueue(SimpleCommand{7});

      CHECK_EQ(queue.Size(), 3);
    }

    SUBCASE("Enqueue with lvalue") {
      CmdQueue queue;
      SimpleCommand cmd{15};

      queue.Enqueue(cmd);

      CHECK_EQ(queue.Size(), 1);
    }

    SUBCASE("Enqueue with rvalue") {
      CmdQueue queue;
      queue.Enqueue(SimpleCommand{20});
      CHECK_EQ(queue.Size(), 1);
    }
  }

  TEST_CASE("ecs::CmdQueue::EnqueueBulk") {
    SUBCASE("EnqueueBulk with vector of commands") {
      CmdQueue queue;
      std::vector<SimpleCommand> commands = {{1}, {2}, {3}};

      queue.EnqueueBulk(commands);

      CHECK_EQ(queue.Size(), 3);
    }

    SUBCASE("EnqueueBulk with rvalue vector") {
      CmdQueue queue;
      queue.EnqueueBulk(std::vector<SimpleCommand>{{4}, {5}});
      CHECK_EQ(queue.Size(), 2);
    }
  }

  TEST_CASE("ecs::CmdQueue::ExecuteAll") {
    World world;

    SUBCASE("Execute single command") {
      CmdQueue queue;
      execute_count = 0;

      queue.Enqueue(SimpleCommand{5});
      queue.ExecuteAll(world);

      CHECK_EQ(execute_count, 5);
      CHECK(queue.Empty());
    }

    SUBCASE("Execute multiple commands in order") {
      CmdQueue queue;
      execute_count = 0;

      queue.Enqueue(SimpleCommand{1});
      queue.Enqueue(SimpleCommand{2});
      queue.Enqueue(SimpleCommand{3});
      queue.ExecuteAll(world);

      CHECK_EQ(execute_count, 6);
      CHECK(queue.Empty());
    }

    SUBCASE("Execute clears queue") {
      CmdQueue queue;
      execute_count = 0;

      queue.Enqueue(SimpleCommand{10});
      queue.ExecuteAll(world);

      CHECK(queue.Empty());
      CHECK_EQ(queue.Size(), 0);
    }

    SUBCASE("Execute commands with different values") {
      CmdQueue queue;
      execute_count = 0;

      queue.Enqueue(MultipleExecuteCommand{42});
      queue.ExecuteAll(world);

      CHECK_EQ(execute_count, 42);
    }
  }

  TEST_CASE("ecs::CmdQueue::Empty") {
    SUBCASE("Default queue is empty") {
      CmdQueue queue;
      CHECK(queue.Empty());
    }

    SUBCASE("Queue not empty after enqueue") {
      CmdQueue queue;
      queue.Enqueue(SimpleCommand{});
      CHECK_FALSE(queue.Empty());
    }

    SUBCASE("Queue empty after clear") {
      CmdQueue queue;
      queue.Enqueue(SimpleCommand{});
      queue.Clear();

      CHECK(queue.Empty());
    }
  }

  TEST_CASE("ecs::CmdQueue::Size") {
    SUBCASE("Size is zero initially") {
      CmdQueue queue;
      CHECK_EQ(queue.Size(), 0);
    }

    SUBCASE("Size increases after enqueue") {
      CmdQueue queue;

      queue.Enqueue(SimpleCommand{});
      CHECK_EQ(queue.Size(), 1);

      queue.Enqueue(SimpleCommand{});
      CHECK_EQ(queue.Size(), 2);
    }

    SUBCASE("Size is zero after clear") {
      CmdQueue queue;

      queue.Enqueue(SimpleCommand{});
      queue.Enqueue(SimpleCommand{});
      queue.Clear();

      CHECK_EQ(queue.Size(), 0);
    }
  }

  TEST_CASE("ecs::CmdQueue::GetAllocator") {
    std::allocator<std::byte> alloc;
    CmdQueue<std::allocator<std::byte>> queue{alloc};

    const auto retrieved_alloc = queue.GetAllocator();
    CHECK_EQ(retrieved_alloc, alloc);
  }

  TEST_CASE("ecs::PmrCmdQueue::works with memory_resource") {
    auto* resource = std::pmr::get_default_resource();

    PmrCmdQueue queue{resource};
    queue.Enqueue(SimpleCommand{5});

    World world;
    execute_count = 0;
    queue.ExecuteAll(world);

    CHECK_EQ(execute_count, 5);
    CHECK(queue.Empty());
  }

  TEST_CASE("ecs::CmdQueue::Merge: rvalue ref") {
    CmdQueue queue1;
    queue1.Enqueue(SimpleCommand{1});
    queue1.Enqueue(SimpleCommand{2});

    CmdQueue queue2;
    queue2.Enqueue(SimpleCommand{3});
    queue2.Enqueue(SimpleCommand{4});

    execute_count = 0;
    queue1.Merge(std::move(queue2));

    CHECK_EQ(queue1.Size(), 4);
    CHECK(queue2.Empty());

    World world;
    queue1.ExecuteAll(world);
    CHECK_EQ(execute_count, 10);  // 1+2+3+4
  }

  TEST_CASE("ecs::CmdQueue::Merge: empty other queue") {
    CmdQueue queue1;
    queue1.Enqueue(SimpleCommand{10});

    CmdQueue queue2;

    queue1.Merge(std::move(queue2));

    CHECK_EQ(queue1.Size(), 1);
    CHECK(queue2.Empty());

    execute_count = 0;
    World world;
    queue1.ExecuteAll(world);
    CHECK_EQ(execute_count, 10);
  }

  TEST_CASE("ecs::CmdQueue::Merge: self merge is no-op") {
    CmdQueue queue;
    queue.Enqueue(SimpleCommand{5});
    queue.Enqueue(SimpleCommand{10});

    queue.Merge(std::move(queue));

    CHECK_EQ(queue.Size(), 2);

    execute_count = 0;
    World world;
    queue.ExecuteAll(world);
    CHECK_EQ(execute_count, 15);
  }

  TEST_CASE("ecs::CmdQueue::Merge: preserves execution order") {
    CmdQueue queue1;
    queue1.Enqueue(MultipleExecuteCommand{1});
    queue1.Enqueue(MultipleExecuteCommand{2});

    CmdQueue queue2;
    queue2.Enqueue(MultipleExecuteCommand{3});
    queue2.Enqueue(MultipleExecuteCommand{4});

    queue1.Merge(std::move(queue2));

    CHECK_EQ(queue1.Size(), 4);

    execute_count = 0;
    World world;
    queue1.ExecuteAll(world);
    CHECK_EQ(execute_count, 4);  // Last command sets execute_count to 4
  }
}
