#include <doctest/doctest.h>

#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/manager.hpp>

#include <vector>

using namespace helios::ecs;

namespace {

struct TaskEvent {
  static constexpr bool kAsync = true;

  int id = 0;
};

struct OtherAsync {
  static constexpr bool kAsync = true;

  float value = 0.0F;
};

}  // namespace

TEST_SUITE("helios::ecs::AsyncMessageReader") {
  TEST_CASE("ecs::AsyncMessageReader::ctor") {
    SUBCASE("Construct from MessageManager") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager);

      CHECK(reader.Empty());
    }

    SUBCASE("Construct from AsyncMessageQueue directly") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager.AsyncQueue());

      CHECK(reader.Empty());
    }

    SUBCASE("Move ctor") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      manager.WriteAsync(TaskEvent{2});

      AsyncMessageReader<TaskEvent> original(manager);
      AsyncMessageReader<TaskEvent> moved(std::move(original));

      CHECK_FALSE(moved.Empty());
    }
  }

  TEST_CASE("ecs::AsyncMessageReader::operator=") {
    SUBCASE("Move assignment") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{99});

      AsyncMessageReader<TaskEvent> original(manager);
      AsyncMessageReader<TaskEvent> assigned(manager);

      assigned = std::move(original);

      CHECK_FALSE(assigned.Empty());
    }
  }

  TEST_CASE("ecs::AsyncMessageReader::Dequeue (return value)") {
    SUBCASE("Dequeue returns default-constructed T when queue is empty") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager);
      const TaskEvent result = reader.Dequeue();

      CHECK_EQ(result.id, 0);
    }

    SUBCASE("Dequeue returns enqueued message") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{42});
      const AsyncMessageReader<TaskEvent> reader(manager);
      const TaskEvent result = reader.Dequeue();

      CHECK_EQ(result.id, 42);
    }

    SUBCASE("Dequeue drains messages one by one") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      manager.WriteAsync(TaskEvent{2});
      const AsyncMessageReader<TaskEvent> reader(manager);

      [[maybe_unused]] const auto first = reader.Dequeue();
      [[maybe_unused]] const auto second = reader.Dequeue();

      CHECK(reader.Empty());
    }
  }

  TEST_CASE("ecs::AsyncMessageReader::Dequeue (bool overload)") {
    SUBCASE("Returns false and leaves dest unchanged when queue is empty") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager);

      TaskEvent dest{999};
      const bool result = reader.Dequeue(dest);

      CHECK_FALSE(result);
      CHECK_EQ(dest.id, 999);
    }

    SUBCASE("Returns true and stores message when queue has items") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{7});

      const AsyncMessageReader<TaskEvent> reader(manager);
      TaskEvent dest{};
      const bool result = reader.Dequeue(dest);

      CHECK(result);
      CHECK_EQ(dest.id, 7);
    }

    SUBCASE("Successive Dequeue calls drain the queue") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      manager.WriteAsync(TaskEvent{2});
      const AsyncMessageReader<TaskEvent> reader(manager);

      TaskEvent dest{};
      CHECK(reader.Dequeue(dest));
      CHECK(reader.Dequeue(dest));

      TaskEvent extra{};
      CHECK_FALSE(reader.Dequeue(extra));
    }
  }

  TEST_CASE("ecs::AsyncMessageReader::ForEach") {
    SUBCASE("ForEach visits every enqueued message") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      manager.WriteAsync(TaskEvent{2});
      manager.WriteAsync(TaskEvent{3});

      const AsyncMessageReader<TaskEvent> reader(manager);
      int sum = 0;
      reader.ForEach([&sum](TaskEvent& e) { sum += e.id; });

      CHECK_EQ(sum, 6);
    }

    SUBCASE("ForEach on empty queue does not invoke action") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager);

      int call_count = 0;
      reader.ForEach([&call_count](TaskEvent&) { ++call_count; });

      CHECK_EQ(call_count, 0);
    }

    SUBCASE("ForEach drains the queue") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{10});
      const AsyncMessageReader<TaskEvent> reader(manager);
      reader.ForEach([](TaskEvent&) {});

      CHECK(reader.Empty());
    }
  }

  TEST_CASE("ecs::AsyncMessageReader::Empty") {
    SUBCASE("Empty returns true when no messages are present") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager);

      CHECK(reader.Empty());
    }

    SUBCASE("Empty returns false after messages are written") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      const AsyncMessageReader<TaskEvent> reader(manager);

      CHECK_FALSE(reader.Empty());
    }

    SUBCASE("Empty returns true after all messages are drained") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});

      const AsyncMessageReader<TaskEvent> reader(manager);
      reader.ForEach([](TaskEvent&) {});

      CHECK(reader.Empty());
    }
  }

  TEST_CASE("ecs::AsyncMessageReader::CountApprox") {
    SUBCASE("CountApprox returns zero when queue is empty") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      const AsyncMessageReader<TaskEvent> reader(manager);

      CHECK_EQ(reader.CountApprox(), 0);
    }

    SUBCASE("CountApprox returns approximate count of messages") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      manager.WriteAsync(TaskEvent{2});
      manager.WriteAsync(TaskEvent{3});
      const AsyncMessageReader<TaskEvent> reader(manager);

      CHECK_GE(reader.CountApprox(), 1);
    }

    SUBCASE("CountApprox decreases as messages are dequeued") {
      MessageManager manager;

      manager.Register<TaskEvent>();
      manager.WriteAsync(TaskEvent{1});
      manager.WriteAsync(TaskEvent{2});
      const AsyncMessageReader<TaskEvent> reader(manager);

      const auto before = reader.CountApprox();
      reader.Dequeue();
      const auto after = reader.CountApprox();

      CHECK_LT(after, before);
    }
  }
}
