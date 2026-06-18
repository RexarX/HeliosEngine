#include <doctest/doctest.h>

#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/manager.hpp>

#include <string_view>
#include <vector>

using namespace helios::ecs;

namespace {

struct WorkItem {
  static constexpr bool kAsync = true;

  int id = 0;
};

struct NamedAsync {
  static constexpr bool kAsync = true;
  static constexpr std::string_view kName = "NamedAsync";

  float value = 0.0F;
};

}  // namespace

TEST_SUITE("helios::ecs::AsyncMessageWriter") {
  TEST_CASE("ecs::AsyncMessageWriter::ctor") {
    SUBCASE("Construct from MessageManager") {
      MessageManager manager;

      manager.Register<WorkItem>();
      const AsyncMessageWriter<WorkItem> writer(manager);

      // Writer constructed without throwing; queue still empty before any Write
      CHECK_FALSE(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE("Construct from AsyncMessageQueue directly") {
      MessageManager manager;

      manager.Register<WorkItem>();
      const AsyncMessageWriter<WorkItem> writer(manager.AsyncQueue());

      CHECK_FALSE(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE("Move ctor") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> original(manager);
      AsyncMessageWriter<WorkItem> moved(std::move(original));
      moved.Write(WorkItem{});

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }
  }

  TEST_CASE("ecs::AsyncMessageWriter::operator=") {
    SUBCASE("Move assignment") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> original(manager);
      AsyncMessageWriter<WorkItem> assigned(manager);
      assigned = std::move(original);
      assigned.Write(WorkItem{});

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }
  }

  TEST_CASE("ecs::AsyncMessageWriter::Write (move)") {
    SUBCASE("Write single rvalue message is enqueued") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      writer.Write(WorkItem{});

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE("Write multiple rvalue messages accumulate") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      writer.Write(WorkItem{});
      writer.Write(WorkItem{});
      writer.Write(WorkItem{});

      CHECK_GE(manager.AsyncQueue().MessageCount<WorkItem>(), 3);
    }

    SUBCASE("Write to different type does not affect another type") {
      MessageManager manager;

      manager.Register<WorkItem>();
      manager.Register<NamedAsync>();

      AsyncMessageWriter<WorkItem> writer(manager);
      writer.Write(WorkItem{});

      CHECK(manager.HasAsyncMessages<WorkItem>());
      CHECK_FALSE(manager.HasAsyncMessages<NamedAsync>());
    }
  }

  TEST_CASE("ecs::AsyncMessageWriter::Write (copy)") {
    SUBCASE("Write const lvalue message is enqueued") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      const WorkItem msg{};
      writer.Write(msg);

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE("Original message is unchanged after copy-write") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      const WorkItem msg{77};
      writer.Write(msg);

      CHECK_EQ(msg.id, 77);
    }
  }

  TEST_CASE("ecs::AsyncMessageWriter::WriteBulk") {
    SUBCASE("WriteBulk enqueues all messages from range") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      const std::vector<WorkItem> msgs(5);
      writer.WriteBulk(msgs);

      CHECK_GE(manager.AsyncQueue().MessageCount<WorkItem>(), 5);
    }

    SUBCASE("WriteBulk with empty range does not enqueue any messages") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      const std::vector<WorkItem> empty;
      writer.WriteBulk(empty);

      CHECK_FALSE(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE("WriteBulk with single element range enqueues one message") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      const std::vector<WorkItem> single(1);
      writer.WriteBulk(single);

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }
  }

  TEST_CASE("ecs::AsyncMessageWriter::Emplace") {
    SUBCASE("Emplace constructs and enqueues a message in-place") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      writer.Emplace(7);

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE(
        "Emplace with default arguments enqueues a default-constructed "
        "message") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      writer.Emplace();

      CHECK(manager.HasAsyncMessages<WorkItem>());
    }

    SUBCASE("Emplace multiple times accumulates messages") {
      MessageManager manager;

      manager.Register<WorkItem>();

      AsyncMessageWriter<WorkItem> writer(manager);
      writer.Emplace();
      writer.Emplace();
      writer.Emplace();

      CHECK_GE(manager.AsyncQueue().MessageCount<WorkItem>(), 3);
    }

    SUBCASE("Emplace with float type") {
      MessageManager manager;

      manager.Register<NamedAsync>();

      AsyncMessageWriter<NamedAsync> writer(manager);
      writer.Emplace(3.14F);

      CHECK(manager.HasAsyncMessages<NamedAsync>());
    }
  }
}
