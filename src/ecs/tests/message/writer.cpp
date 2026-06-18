#include <doctest/doctest.h>

#include <helios/ecs/message/writer.hpp>

#include <vector>

using namespace helios::ecs;

namespace {

struct PositionMsg {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const PositionMsg&) const noexcept = default;
};

struct TagMsg {
  constexpr bool operator==(const TagMsg&) const noexcept = default;
};

// Helper that builds a queue with PositionMsg already registered.
MessageQueue<> MakeQueueWith() {
  MessageQueue queue;
  queue.Register<PositionMsg>();
  return queue;
}

}  // namespace

TEST_SUITE("helios::ecs::BasicMessageWriter") {
  TEST_CASE("ecs::BasicMessageWriter::ctor") {
    SUBCASE("Writer can be constructed from a registered queue") {
      auto queue = MakeQueueWith();
      const BasicMessageWriter<PositionMsg> writer(queue);
      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
    }

    SUBCASE("Move construction transfers the queue reference") {
      auto queue = MakeQueueWith();

      BasicMessageWriter<PositionMsg> src(queue);
      BasicMessageWriter<PositionMsg> dst(std::move(src));

      dst.Write(PositionMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::BasicMessageWriter::operator=") {
    SUBCASE("Move assignment transfers the queue reference") {
      auto queue = MakeQueueWith();
      auto queue2 = MakeQueueWith();

      BasicMessageWriter<PositionMsg> src(queue);
      BasicMessageWriter<PositionMsg> dst(queue2);
      dst = std::move(src);

      dst.Write(PositionMsg{});

      // Message must land in the queue that src originally referenced.
      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
      CHECK_EQ(queue2.MessageCount<PositionMsg>(), 0);
    }
  }

  TEST_CASE("ecs::BasicMessageWriter::Write") {
    SUBCASE("Write enqueues a single message into the queue") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Write(PositionMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }

    SUBCASE("Written message value is preserved in the queue") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      const PositionMsg pos{.x = 7.0F, .y = 8.0F};
      writer.Write(pos);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 1);
      CHECK_EQ(messages[0], pos);
    }

    SUBCASE("Multiple Write calls accumulate messages in insertion order") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Write(PositionMsg{.x = 1.0F});
      writer.Write(PositionMsg{.x = 2.0F});
      writer.Write(PositionMsg{.x = 3.0F});

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 3);
      CHECK_EQ(messages[0].x, 1.0F);
      CHECK_EQ(messages[1].x, 2.0F);
      CHECK_EQ(messages[2].x, 3.0F);
    }

    SUBCASE("Write on a const writer still enqueues the message") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Write(PositionMsg{});

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::BasicMessageWriter::WriteBulk") {
    SUBCASE("WriteBulk enqueues all messages in the range") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      const std::vector<PositionMsg> batch = {
          PositionMsg{.x = 1.0F},
          PositionMsg{.x = 2.0F},
          PositionMsg{.x = 3.0F},
      };
      writer.WriteBulk(batch);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 3);
    }

    SUBCASE("WriteBulk preserves message values in order") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      const std::vector<PositionMsg> batch = {PositionMsg{.x = 10.0F},
                                              PositionMsg{.x = 20.0F}};
      writer.WriteBulk(batch);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 2);
      CHECK_EQ(messages[0].x, 10.0F);
      CHECK_EQ(messages[1].x, 20.0F);
    }

    SUBCASE("WriteBulk of an empty range is a no-op") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      const std::vector<PositionMsg> empty;
      writer.WriteBulk(empty);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 0);
    }

    SUBCASE("WriteBulk on a const writer still enqueues all messages") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      const std::vector<PositionMsg> batch = {PositionMsg{}, PositionMsg{}};
      writer.WriteBulk(batch);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 2);
    }
  }

  TEST_CASE("ecs::BasicMessageWriter::Emplace") {
    SUBCASE("Emplace constructs and enqueues the message in-place") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Emplace(11.0F, 22.0F);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }

    SUBCASE("Emplace stores the correct field values") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Emplace(3.0F, 4.0F);

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 1);
      CHECK_EQ(messages[0], PositionMsg{.x = 3.0F, .y = 4.0F});
    }

    SUBCASE("Emplace with no arguments default-constructs the message") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Emplace();

      const auto messages = queue.Messages<PositionMsg>();
      REQUIRE_EQ(messages.size(), 1);
      CHECK_EQ(messages[0], PositionMsg{});
    }

    SUBCASE("Multiple Emplace calls accumulate messages") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Emplace(1.0F, 0.0F);
      writer.Emplace(2.0F, 0.0F);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 2);
    }

    SUBCASE("Emplace on a const writer still enqueues the message") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Emplace(7.0F, 8.0F);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::BasicMessageWriter - mixed operations") {
    SUBCASE("Write, WriteBulk and Emplace all land in the same queue") {
      auto queue = MakeQueueWith();

      const BasicMessageWriter<PositionMsg> writer(queue);
      writer.Write(PositionMsg{});
      writer.WriteBulk(std::vector<PositionMsg>{PositionMsg{}, PositionMsg{}});
      writer.Emplace(4.0F, 0.0F);

      CHECK_EQ(queue.MessageCount<PositionMsg>(), 4);
    }
  }
}
