#include <doctest/doctest.h>

#include <helios/ecs/message/queue.hpp>

#include <memory_resource>
#include <utility>
#include <vector>

using namespace helios::ecs;

namespace {

struct PositionMsg {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const PositionMsg&) const noexcept = default;
};

struct VelocityMsg {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const VelocityMsg&) const noexcept = default;
};

struct ManualMsg {
  static constexpr MessageClearPolicy kClearPolicy =
      MessageClearPolicy::kManual;

  int value = 0;

  constexpr bool operator==(const ManualMsg&) const noexcept = default;
};

// Move-only message
struct MoveOnlyMsg {
  int value = 0;

  MoveOnlyMsg() = default;
  explicit MoveOnlyMsg(int val) : value(val) {}
  MoveOnlyMsg(const MoveOnlyMsg&) = delete;
  MoveOnlyMsg(MoveOnlyMsg&&) noexcept = default;

  MoveOnlyMsg& operator=(const MoveOnlyMsg&) = delete;
  MoveOnlyMsg& operator=(MoveOnlyMsg&&) noexcept = default;
};

}  // namespace

TEST_SUITE("helios::ecs::MessageQueue") {
  TEST_CASE("ecs::MessageQueue::ctor") {
    SUBCASE("Default construction produces an empty queue") {
      const MessageQueue queue;

      CHECK_EQ(queue.TypeCount(), 0);
      CHECK_EQ(queue.MessageCount(), 0);
      CHECK_FALSE(queue.HasMessages());
    }

    SUBCASE("Construction with allocator produces an empty queue") {
      const std::allocator<std::byte> alloc;
      const MessageQueue queue(alloc);

      CHECK_EQ(queue.TypeCount(), 0);
      CHECK_FALSE(queue.HasMessages());
    }

    SUBCASE("Copy construction preserves messages") {
      MessageQueue src;

      src.Register<PositionMsg>();
      src.Enqueue(PositionMsg{.x = 1.0F, .y = 2.0F});

      const MessageQueue copy(src);

      CHECK_EQ(copy.MessageCount<PositionMsg>(), 1);
      CHECK_EQ(copy.Messages<PositionMsg>()[0],
               PositionMsg{.x = 1.0F, .y = 2.0F});
    }

    SUBCASE("Move construction transfers messages and leaves source empty") {
      MessageQueue src;
      src.Register<PositionMsg>();
      src.Enqueue(PositionMsg{});

      MessageQueue dst(std::move(src));
      CHECK_EQ(dst.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::operator=") {
    SUBCASE("Copy assignment replicates messages") {
      MessageQueue src;
      MessageQueue dst;

      src.Register<PositionMsg>();
      src.Enqueue(PositionMsg{.x = 5.0F, .y = 6.0F});
      dst = src;

      CHECK_EQ(dst.MessageCount<PositionMsg>(), 1);
      CHECK_EQ(dst.Messages<PositionMsg>()[0],
               PositionMsg{.x = 5.0F, .y = 6.0F});
    }

    SUBCASE("Move assignment transfers messages and leaves source empty-ish") {
      MessageQueue src;
      MessageQueue dst;

      src.Register<PositionMsg>();
      src.Enqueue(PositionMsg{});
      dst = std::move(src);

      CHECK_EQ(dst.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::Register") {
    SUBCASE("Type is registered after Register<T>()") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK(queue.IsRegistered<PositionMsg>());
    }

    SUBCASE("Unregistered type returns false from IsRegistered") {
      const MessageQueue queue;
      CHECK_FALSE(queue.IsRegistered<PositionMsg>());
    }

    SUBCASE("Re-registering the same type is idempotent") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<PositionMsg>();

      CHECK(queue.IsRegistered<PositionMsg>());
      CHECK_EQ(queue.TypeCount(), 1);
    }

    SUBCASE("Multiple distinct types can all be registered") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();

      CHECK(queue.IsRegistered<PositionMsg>());
      CHECK(queue.IsRegistered<VelocityMsg>());
      CHECK_EQ(queue.TypeCount(), 2);
    }

    SUBCASE("Runtime Register(type_index) registers the type") {
      MessageQueue queue;
      queue.Register(MessageTypeIndex::From<PositionMsg>());
      CHECK(queue.IsRegistered(MessageTypeIndex::From<PositionMsg>()));
    }

    SUBCASE("Runtime Register(type_index) is idempotent") {
      MessageQueue queue;

      queue.Register(MessageTypeIndex::From<PositionMsg>());
      queue.Register(MessageTypeIndex::From<PositionMsg>());

      CHECK_EQ(queue.TypeCount(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::ClearAll") {
    SUBCASE(
        "All messages are gone after ClearAll but types remain registered") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});
      queue.ClearAll();

      CHECK_EQ(queue.MessageCount(), 0);
      CHECK(queue.IsRegistered<PositionMsg>());
      CHECK(queue.IsRegistered<VelocityMsg>());
    }

    SUBCASE("HasMessages returns false after ClearAll") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.ClearAll();

      CHECK_FALSE(queue.HasMessages());
    }
  }

  TEST_CASE("ecs::MessageQueue::Clear (typed)") {
    SUBCASE("Clears only the specified type") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});
      queue.Clear<PositionMsg>();

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
      CHECK_EQ(queue.MessageCount<VelocityMsg>(), 1);
    }

    SUBCASE("Clear by runtime type index clears only that type") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});
      queue.Clear(MessageTypeIndex::From<PositionMsg>());

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
      CHECK_EQ(queue.MessageCount<VelocityMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::ResetAll") {
    SUBCASE("ResetAll removes all types and all messages") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.ResetAll();

      CHECK_EQ(queue.TypeCount(), 0);
      CHECK_EQ(queue.MessageCount(), 0);
      CHECK_FALSE(queue.IsRegistered<PositionMsg>());
    }
  }

  TEST_CASE("ecs::MessageQueue::Reset (typed)") {
    SUBCASE("Reset<T> unregisters the type and removes its messages") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});
      queue.Reset<PositionMsg>();

      CHECK_FALSE(queue.IsRegistered<PositionMsg>());
      CHECK(queue.IsRegistered<VelocityMsg>());
      CHECK_EQ(queue.MessageCount<VelocityMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::Merge") {
    SUBCASE("Const lvalue source appends messages without modifying source") {
      MessageQueue dst;
      MessageQueue src_mut;
      const MessageQueue<>& src = src_mut;

      dst.Register<PositionMsg>();
      dst.Enqueue(PositionMsg{});
      src_mut.Register<PositionMsg>();
      src_mut.Enqueue(PositionMsg{});
      src_mut.Enqueue(PositionMsg{});

      dst.Merge(src);

      CHECK_EQ(dst.MessageCount<PositionMsg>(), 3);
      CHECK_EQ(src.MessageCount<PositionMsg>(), 2);
      CHECK(src.IsRegistered<PositionMsg>());
    }

    SUBCASE("Rvalue source appends messages and clears source") {
      MessageQueue dst;
      MessageQueue src;

      dst.Register<PositionMsg>();
      dst.Enqueue(PositionMsg{});
      src.Register<PositionMsg>();
      src.Enqueue(PositionMsg{});
      src.Enqueue(PositionMsg{});

      dst.Merge(std::move(src));

      CHECK_EQ(dst.MessageCount<PositionMsg>(), 3);
      CHECK_EQ(src.TypeCount(), 0);
      CHECK_EQ(src.MessageCount(), 0);
    }

    SUBCASE("Merging an empty queue is a no-op") {
      MessageQueue dst;
      MessageQueue empty;

      dst.Register<PositionMsg>();
      dst.Enqueue(PositionMsg{});
      empty.Register<PositionMsg>();
      dst.Merge(std::move(empty));

      CHECK_EQ(dst.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::Enqueue") {
    SUBCASE("Enqueuing a message increments the count") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }

    SUBCASE("Enqueued message values are preserved") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      const PositionMsg pos{.x = 4.0F, .y = 5.0F};
      queue.Enqueue(pos);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 1);
      CHECK_EQ(messages[0], pos);
    }

    SUBCASE("Multiple messages of the same type are all stored") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 3);
    }

    SUBCASE("Move-only messages can be enqueued") {
      MessageQueue queue;

      queue.Register<MoveOnlyMsg>();
      queue.Enqueue(MoveOnlyMsg{42});

      CHECK_EQ(queue.MessageCount<MoveOnlyMsg>(), 1);
      CHECK_EQ(queue.Messages<MoveOnlyMsg>()[0].value, 42);
    }

    SUBCASE("Messages of different types are stored independently") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
      CHECK_EQ(queue.MessageCount<VelocityMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::EnqueueBulk") {
    SUBCASE("All messages in a bulk range are enqueued") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      const std::vector<PositionMsg> batch = {
          PositionMsg{},
          PositionMsg{},
          PositionMsg{},
      };
      queue.EnqueueBulk(batch);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 3);
    }

    SUBCASE("Bulk enqueue preserves message values in order") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      const std::vector<PositionMsg> batch = {PositionMsg{.x = 10.0F},
                                              PositionMsg{.x = 20.0F}};
      queue.EnqueueBulk(batch);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 2);
      CHECK_EQ(messages[0].x, 10.0F);
      CHECK_EQ(messages[1].x, 20.0F);
    }

    SUBCASE("Bulk enqueue of empty range is a no-op") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      const std::vector<PositionMsg> empty;
      queue.EnqueueBulk(empty);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
    }
  }

  TEST_CASE("ecs::MessageQueue::RemoveIndices") {
    SUBCASE("Single index is removed correctly") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{.x = 1.0F});
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{.x = 3.0F});

      const std::vector<MessageQueue<>::size_type> indices = {1};
      queue.RemoveIndices<PositionMsg>(indices);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 2);
      CHECK_EQ(messages[0].x, 1.0F);
      CHECK_EQ(messages[1].x, 3.0F);
    }

    SUBCASE("Multiple non-contiguous indices are removed correctly") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{.x = 2.0F});
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{.x = 4.0F});

      const std::vector<MessageQueue<>::size_type> indices = {0, 2};
      queue.RemoveIndices<PositionMsg>(indices);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 2);
      CHECK_EQ(messages[0].x, 2.0F);
      CHECK_EQ(messages[1].x, 4.0F);
    }

    SUBCASE("Contiguous indices are coalesced and removed correctly") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{.x = 3.0F});

      const std::vector<MessageQueue<>::size_type> indices = {0, 1};
      queue.RemoveIndices<PositionMsg>(indices);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 1);
      CHECK_EQ(messages[0].x, 3.0F);
    }

    SUBCASE("Passing an empty indices span is a no-op") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});

      const std::vector<MessageQueue<>::size_type> indices;
      queue.RemoveIndices<PositionMsg>(indices);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }

    SUBCASE("RemoveIndices by runtime type index works correctly") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{.x = 2.0F});

      const std::vector<MessageQueue<>::size_type> indices = {0};
      queue.RemoveIndices(MessageTypeIndex::From<PositionMsg>(), indices);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 1);
      CHECK_EQ(messages[0].x, 2.0F);
    }
  }

  TEST_CASE("ecs::MessageQueue::Swap") {
    SUBCASE("Swap exchanges messages between two queues") {
      MessageQueue q1;
      MessageQueue q2;

      q1.Register<PositionMsg>();
      q1.Enqueue(PositionMsg{});
      q2.Register<VelocityMsg>();
      q2.Enqueue(VelocityMsg{});

      q1.Swap(q2);

      CHECK(q1.IsRegistered<VelocityMsg>());
      CHECK_FALSE(q1.IsRegistered<PositionMsg>());
      CHECK(q2.IsRegistered<PositionMsg>());
      CHECK_FALSE(q2.IsRegistered<VelocityMsg>());
    }

    SUBCASE("Friend swap() overload exchanges messages between two queues") {
      MessageQueue q1;
      q1.Register<PositionMsg>();
      q1.Enqueue(PositionMsg{});

      MessageQueue q2;
      q2.Register<PositionMsg>();
      q2.Enqueue(PositionMsg{});
      q2.Enqueue(PositionMsg{});

      std::swap(q1, q2);

      CHECK_EQ(q1.MessageCount<PositionMsg>(), 2);
      CHECK_EQ(q2.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::MessageQueue::IsRegistered") {
    SUBCASE("Returns true for a registered type") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK(queue.IsRegistered<PositionMsg>());
    }

    SUBCASE("Returns false for an unregistered type") {
      const MessageQueue queue;
      CHECK_FALSE(queue.IsRegistered<PositionMsg>());
    }

    SUBCASE(
        "Runtime IsRegistered(type_index) returns true for registered type") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK(queue.IsRegistered(MessageTypeIndex::From<PositionMsg>()));
    }

    SUBCASE(
        "Runtime IsRegistered(type_index) returns false for unregistered "
        "type") {
      const MessageQueue queue;
      CHECK_FALSE(queue.IsRegistered(MessageTypeIndex::From<PositionMsg>()));
    }
  }

  TEST_CASE("ecs::MessageQueue::HasMessages") {
    SUBCASE("Returns false when queue is empty") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK_FALSE(queue.HasMessages());
    }

    SUBCASE("Returns true after at least one message is enqueued") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      CHECK(queue.HasMessages());
    }

    SUBCASE("Typed HasMessages<T> returns false when type has no messages") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK_FALSE(queue.HasMessages<PositionMsg>());
    }

    SUBCASE("Typed HasMessages<T> returns true after enqueue for that type") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});

      CHECK(queue.HasMessages<PositionMsg>());
    }

    SUBCASE("Typed HasMessages<T> returns false for unregistered type") {
      const MessageQueue queue;
      CHECK_FALSE(queue.HasMessages<PositionMsg>());
    }
  }

  TEST_CASE("ecs::MessageQueue::Messages") {
    const PositionMsg pos{.x = 1.0F, .y = 2.0F};

    SUBCASE("Returns empty span for unregistered type") {
      const MessageQueue queue;
      const auto span = queue.Messages<PositionMsg>();
      CHECK(span.empty());
    }

    SUBCASE("Returns empty span for registered but empty type") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK(queue.Messages<PositionMsg>().empty());
    }

    SUBCASE("Const Messages<T> returns correct read-only view") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(pos);

      const auto& const_queue = queue;
      const auto span = const_queue.Messages<PositionMsg>();
      REQUIRE_EQ(span.size(), 1);
      CHECK_EQ(span[0], pos);
    }

    SUBCASE("Mutable Messages<T> allows in-place modification") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(pos);
      queue.Messages<PositionMsg>()[0].x = 99.0F;

      CHECK_EQ(queue.Messages<PositionMsg>()[0].x, 99.0F);
    }
  }

  TEST_CASE("ecs::MessageQueue::TypeCount") {
    SUBCASE("Returns zero for empty queue") {
      const MessageQueue queue;
      CHECK_EQ(queue.TypeCount(), 0);
    }

    SUBCASE("Increments per distinct registered type") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      CHECK_EQ(queue.TypeCount(), 1);
      queue.Register<VelocityMsg>();
      CHECK_EQ(queue.TypeCount(), 2);
    }
  }

  TEST_CASE("ecs::MessageQueue::MessageCount") {
    SUBCASE("Total MessageCount returns zero for empty queue") {
      const MessageQueue queue;
      CHECK_EQ(queue.MessageCount(), 0);
    }

    SUBCASE("Total MessageCount accumulates across all types") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});

      CHECK_EQ(queue.MessageCount(), 3);
    }

    SUBCASE(
        "Typed MessageCount<T> returns zero for registered but empty type") {
      MessageQueue queue;
      queue.Register<PositionMsg>();
      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
    }

    SUBCASE("Typed MessageCount<T> returns zero for unregistered type") {
      const MessageQueue queue;
      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
    }

    SUBCASE("Typed MessageCount<T> counts only messages of that type") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Register<VelocityMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(VelocityMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 2);
      CHECK_EQ(queue.MessageCount<VelocityMsg>(), 1);
    }

    SUBCASE("Runtime MessageCount(type_index) returns correct count") {
      MessageQueue queue;

      queue.Register<PositionMsg>();
      queue.Enqueue(PositionMsg{});
      queue.Enqueue(PositionMsg{});

      CHECK_EQ(queue.MessageCount(MessageTypeIndex::From<PositionMsg>()), 2);
    }

    SUBCASE(
        "Runtime MessageCount(type_index) returns zero for unregistered type") {
      const MessageQueue queue;
      CHECK_EQ(queue.MessageCount(MessageTypeIndex::From<PositionMsg>()), 0);
    }
  }

  TEST_CASE("ecs::PmrMessageQueue::works with memory_resource") {
    std::byte buffer[2048];
    std::pmr::monotonic_buffer_resource resource(buffer, sizeof(buffer));

    PmrMessageQueue queue{&resource};
    queue.Register<PositionMsg>();
    queue.Enqueue(PositionMsg{.x = 3.0F, .y = 4.0F});

    CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    CHECK_EQ(queue.Messages<PositionMsg>()[0],
             PositionMsg{.x = 3.0F, .y = 4.0F});
  }
}
