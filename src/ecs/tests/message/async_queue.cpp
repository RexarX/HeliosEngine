#include <doctest/doctest.h>

#include <helios/ecs/message/async_queue.hpp>

#include <algorithm>
#include <vector>

using namespace helios::ecs;

namespace {

struct EventMsg {
  static constexpr bool kAsync = true;

  int id = 0;

  constexpr bool operator==(const EventMsg&) const noexcept = default;
};

struct CommandMsg {
  static constexpr bool kAsync = true;

  int code = 0;

  constexpr bool operator==(const CommandMsg&) const noexcept = default;
};

}  // namespace

TEST_SUITE("helios::ecs::TypedAsyncMessageStorage") {
  TEST_CASE("ecs::TypedAsyncMessageStorage::ctor") {
    SUBCASE("Default construction produces an empty storage") {
      TypedAsyncMessageStorage<EventMsg> storage;
      CHECK(storage.Empty());
      CHECK_EQ(storage.SizeApprox(), 0);
    }

    SUBCASE("Move construction transfers messages") {
      TypedAsyncMessageStorage<EventMsg> src;
      src.Enqueue(EventMsg{1});

      TypedAsyncMessageStorage<EventMsg> dst(std::move(src));
      CHECK_FALSE(dst.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::operator=") {
    SUBCASE("Move assignment transfers messages") {
      TypedAsyncMessageStorage<EventMsg> src;
      TypedAsyncMessageStorage<EventMsg> dst;

      src.Enqueue(EventMsg{});
      dst = std::move(src);

      CHECK_FALSE(dst.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Clear") {
    SUBCASE("All messages are removed after Clear") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});
      storage.Clear();

      CHECK(storage.Empty());
      CHECK_EQ(storage.SizeApprox(), 0);
    }

    SUBCASE("Clear on already empty storage is a no-op") {
      TypedAsyncMessageStorage<EventMsg> storage;
      storage.Clear();
      CHECK(storage.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Enqueue (copy)") {
    const EventMsg msg{};

    SUBCASE("Copy enqueue stores the message") {
      TypedAsyncMessageStorage<EventMsg> storage;
      storage.Enqueue(msg);
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Copy enqueue with producer token stores the message") {
      TypedAsyncMessageStorage<EventMsg> storage;

      auto token = storage.MakeProducerToken();
      storage.Enqueue(token, msg);

      CHECK_FALSE(storage.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Enqueue (move)") {
    SUBCASE("Move enqueue stores the message") {
      TypedAsyncMessageStorage<EventMsg> storage;
      storage.Enqueue(EventMsg{});
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Move enqueue with producer token stores the message") {
      TypedAsyncMessageStorage<EventMsg> storage;

      auto token = storage.MakeProducerToken();
      storage.Enqueue(token, EventMsg{});

      CHECK_FALSE(storage.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::EnqueueBulk") {
    SUBCASE("All messages in a bulk vector are enqueued") {
      TypedAsyncMessageStorage<EventMsg> storage;

      std::vector<EventMsg> batch = {EventMsg{}, EventMsg{}, EventMsg{}};
      storage.EnqueueBulk(batch);

      CHECK_GE(storage.SizeApprox(), 3);
    }

    SUBCASE("Bulk enqueue with producer token stores all messages") {
      TypedAsyncMessageStorage<EventMsg> storage;

      auto token = storage.MakeProducerToken();
      std::vector<EventMsg> batch = {EventMsg{}, EventMsg{}};
      storage.EnqueueBulk(token, batch);

      CHECK_GE(storage.SizeApprox(), 2);
    }

    SUBCASE("Bulk enqueue of empty range is a no-op") {
      TypedAsyncMessageStorage<EventMsg> storage;

      std::vector<EventMsg> empty;
      storage.EnqueueBulk(empty);

      CHECK(storage.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Dequeue (return value)") {
    SUBCASE("Dequeued message has the value that was enqueued") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{55});
      const EventMsg result = storage.Dequeue();

      CHECK_EQ(result.id, 55);
    }

    SUBCASE("Dequeue on empty storage returns default-constructed value") {
      TypedAsyncMessageStorage<EventMsg> storage;
      const EventMsg result = storage.Dequeue();
      CHECK_EQ(result, EventMsg{});
    }

    SUBCASE("Dequeue with consumer token returns the enqueued value") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{77});
      auto token = storage.MakeConsumerToken();
      const EventMsg result = storage.Dequeue(token);

      CHECK_EQ(result.id, 77);
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Dequeue (bool, dest ref)") {
    SUBCASE("Returns true and populates dest when queue is non-empty") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{33});
      EventMsg dest;
      const bool ok = storage.Dequeue(dest);

      CHECK(ok);
      CHECK_EQ(dest.id, 33);
    }

    SUBCASE("Returns false and leaves dest unchanged when queue is empty") {
      TypedAsyncMessageStorage<EventMsg> storage;

      EventMsg dest{99};
      const bool ok = storage.Dequeue(dest);

      CHECK_FALSE(ok);
      CHECK_EQ(dest.id, 99);
    }

    SUBCASE("Dequeue with consumer token returns true and populates dest") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{44});
      auto token = storage.MakeConsumerToken();
      EventMsg dest;
      const bool ok = storage.Dequeue(token, dest);

      CHECK(ok);
      CHECK_EQ(dest.id, 44);
    }

    SUBCASE("Dequeue with consumer token returns false on empty queue") {
      TypedAsyncMessageStorage<EventMsg> storage;

      auto token = storage.MakeConsumerToken();
      EventMsg dest{11};
      const bool ok = storage.Dequeue(token, dest);

      CHECK_FALSE(ok);
      CHECK_EQ(dest.id, 11);
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Into") {
    SUBCASE("Moves all messages into the output iterator") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});

      std::vector<EventMsg> out;
      const size_t count = storage.Into(std::back_inserter(out));

      CHECK_EQ(count, 3);
      CHECK_EQ(out.size(), 3);
      CHECK(storage.Empty());
    }

    SUBCASE("Respects max_count limit") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});

      std::vector<EventMsg> out;
      const size_t count = storage.Into(std::back_inserter(out), 2);

      CHECK_EQ(count, 2);
      CHECK_EQ(out.size(), 2);
      // One message should remain
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Returns zero for empty storage") {
      TypedAsyncMessageStorage<EventMsg> storage;

      std::vector<EventMsg> out;
      const size_t count = storage.Into(std::back_inserter(out));

      CHECK_EQ(count, 0);
      CHECK(out.empty());
    }

    SUBCASE("Into with consumer token drains all messages") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});
      auto token = storage.MakeConsumerToken();

      std::vector<EventMsg> out;
      const size_t count = storage.Into(token, std::back_inserter(out));

      CHECK_EQ(count, 2);
      CHECK_EQ(out.size(), 2);
    }

    SUBCASE("Into with consumer token respects max_count") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});
      auto token = storage.MakeConsumerToken();

      std::vector<EventMsg> out;
      const size_t count = storage.Into(token, std::back_inserter(out), 1);

      CHECK_EQ(count, 1);
      CHECK_EQ(out.size(), 1);
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::MakeProducerToken") {
    SUBCASE("Producer token can be used to enqueue a message") {
      TypedAsyncMessageStorage<EventMsg> storage;

      auto token = storage.MakeProducerToken();
      storage.Enqueue(token, EventMsg{});

      CHECK_FALSE(storage.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::MakeConsumerToken") {
    SUBCASE("Consumer token can be used to dequeue a message") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{88});
      auto token = storage.MakeConsumerToken();

      EventMsg dest;
      CHECK(storage.Dequeue(token, dest));
      CHECK_EQ(dest.id, 88);
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::Empty") {
    SUBCASE("Returns true for a freshly constructed storage") {
      const TypedAsyncMessageStorage<EventMsg> storage;
      CHECK(storage.Empty());
    }

    SUBCASE("Returns false after an enqueue") {
      TypedAsyncMessageStorage<EventMsg> storage;
      storage.Enqueue(EventMsg{});
      CHECK_FALSE(storage.Empty());
    }

    SUBCASE("Returns true again after draining via Dequeue") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      EventMsg dest;
      storage.Dequeue(dest);

      CHECK(storage.Empty());
    }
  }

  TEST_CASE("ecs::TypedAsyncMessageStorage::SizeApprox") {
    SUBCASE("Returns zero for empty storage") {
      const TypedAsyncMessageStorage<EventMsg> storage;
      CHECK_EQ(storage.SizeApprox(), 0);
    }

    SUBCASE("Returns at least the number of messages enqueued") {
      TypedAsyncMessageStorage<EventMsg> storage;

      storage.Enqueue(EventMsg{});
      storage.Enqueue(EventMsg{});

      CHECK_GE(storage.SizeApprox(), 2);
    }
  }
}

TEST_SUITE("helios::ecs::AsyncMessageQueue") {
  TEST_CASE("ecs::AsyncMessageQueue::ctor") {
    SUBCASE("Default construction produces an empty queue") {
      const AsyncMessageQueue queue;

      CHECK_EQ(queue.TypeCount(), 0);
      CHECK_EQ(queue.MessageCount(), 0);
      CHECK_FALSE(queue.HasMessages());
    }

    SUBCASE("Move construction transfers registrations and messages") {
      AsyncMessageQueue src;

      src.Register<EventMsg>();
      src.Enqueue(EventMsg{});

      AsyncMessageQueue dst(std::move(src));

      CHECK(dst.IsRegistered<EventMsg>());
      CHECK_GE(dst.MessageCount<EventMsg>(), 1);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::operator=") {
    SUBCASE("Move assignment transfers registrations and messages") {
      AsyncMessageQueue src;
      AsyncMessageQueue dst;

      src.Register<EventMsg>();
      src.Enqueue(EventMsg{});
      dst = std::move(src);

      CHECK(dst.IsRegistered<EventMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Register") {
    SUBCASE("Type is registered after Register<T>()") {
      AsyncMessageQueue queue;
      queue.Register<EventMsg>();
      CHECK(queue.IsRegistered<EventMsg>());
    }

    SUBCASE("Unregistered type returns false from IsRegistered") {
      const AsyncMessageQueue queue;
      CHECK_FALSE(queue.IsRegistered<EventMsg>());
    }

    SUBCASE("Re-registering the same type is idempotent") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<EventMsg>();

      CHECK_EQ(queue.TypeCount(), 1);
    }

    SUBCASE("Multiple distinct types can all be registered") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<CommandMsg>();

      CHECK(queue.IsRegistered<EventMsg>());
      CHECK(queue.IsRegistered<CommandMsg>());
      CHECK_EQ(queue.TypeCount(), 2);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Clear (all)") {
    SUBCASE("All messages are removed but types remain registered") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<CommandMsg>();
      queue.Enqueue(EventMsg{});
      queue.Enqueue(CommandMsg{});
      queue.Clear();

      CHECK(queue.IsRegistered<EventMsg>());
      CHECK(queue.IsRegistered<CommandMsg>());
      CHECK_FALSE(queue.HasMessages());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Clear (typed)") {
    SUBCASE("Clears only the specified type") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<CommandMsg>();
      queue.Enqueue(EventMsg{});
      queue.Enqueue(CommandMsg{});
      queue.Clear<EventMsg>();

      CHECK_EQ(queue.MessageCount<EventMsg>(), 0);
      CHECK_GE(queue.MessageCount<CommandMsg>(), 1);
    }

    SUBCASE("Clear on unregistered type is a no-op") {
      AsyncMessageQueue queue;
      queue.Clear<EventMsg>();  // Must not assert or throw
      CHECK_EQ(queue.TypeCount(), 0);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Reset (all)") {
    SUBCASE("Reset removes all types and all messages") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{});
      queue.Reset();

      CHECK_EQ(queue.TypeCount(), 0);
      CHECK_FALSE(queue.IsRegistered<EventMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Reset (typed)") {
    SUBCASE("Reset<T> unregisters the type and removes its messages") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<CommandMsg>();
      queue.Enqueue(EventMsg{});
      queue.Enqueue(CommandMsg{});
      queue.Reset<EventMsg>();

      CHECK_FALSE(queue.IsRegistered<EventMsg>());
      CHECK(queue.IsRegistered<CommandMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Enqueue (move)") {
    SUBCASE("Enqueuing a message makes HasMessages return true") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{});

      CHECK(queue.HasMessages<EventMsg>());
    }

    SUBCASE("SizeApprox reflects enqueued count") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{});
      queue.Enqueue(EventMsg{});
      queue.Enqueue(EventMsg{});

      CHECK_GE(queue.MessageCount<EventMsg>(), 3);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Enqueue (with producer token)") {
    SUBCASE("Enqueue with producer token stores the message") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      auto token = queue.MakeProducerToken<EventMsg>();
      queue.Enqueue(token, EventMsg{});

      CHECK(queue.HasMessages<EventMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::EnqueueBulk") {
    SUBCASE("All messages in a range are enqueued") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      std::vector<EventMsg> batch = {EventMsg{}, EventMsg{}, EventMsg{}};
      queue.EnqueueBulk(batch);

      CHECK_GE(queue.MessageCount<EventMsg>(), 3);
    }

    SUBCASE("Bulk enqueue with producer token stores all messages") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      auto token = queue.MakeProducerToken<EventMsg>();
      std::vector<EventMsg> batch = {EventMsg{}, EventMsg{}};
      queue.EnqueueBulk(token, batch);

      CHECK_GE(queue.MessageCount<EventMsg>(), 2);
    }

    SUBCASE("Bulk enqueue of empty range is a no-op") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      std::vector<EventMsg> empty;
      queue.EnqueueBulk(empty);

      CHECK_EQ(queue.MessageCount<EventMsg>(), 0);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::MakeProducerToken") {
    SUBCASE("Token can be used to enqueue messages for that type") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      auto token = queue.MakeProducerToken<EventMsg>();
      queue.Enqueue(token, EventMsg{});

      CHECK(queue.HasMessages<EventMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::MakeConsumerToken") {
    SUBCASE("Token can be used to dequeue from TypedStorage") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{88});

      auto token = queue.MakeConsumerToken<EventMsg>();
      EventMsg dest;
      const bool ok = queue.TypedStorage<EventMsg>().Dequeue(token, dest);

      CHECK(ok);
      CHECK_EQ(dest.id, 88);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::Swap") {
    SUBCASE("Swap exchanges registrations and messages between two queues") {
      AsyncMessageQueue q1;
      AsyncMessageQueue q2;

      q1.Register<EventMsg>();
      q1.Enqueue(EventMsg{});
      q2.Register<CommandMsg>();
      q2.Enqueue(CommandMsg{});
      q1.Swap(q2);

      CHECK(q1.IsRegistered<CommandMsg>());
      CHECK_FALSE(q1.IsRegistered<EventMsg>());
      CHECK(q2.IsRegistered<EventMsg>());
      CHECK_FALSE(q2.IsRegistered<CommandMsg>());
    }

    SUBCASE("Friend swap() overload works symmetrically") {
      AsyncMessageQueue q1;
      AsyncMessageQueue q2;

      q1.Register<EventMsg>();
      q1.Enqueue(EventMsg{});
      q1.Enqueue(EventMsg{});

      q2.Register<EventMsg>();
      q2.Enqueue(EventMsg{});

      std::swap(q1, q2);

      CHECK_GE(q1.MessageCount<EventMsg>(), 1);
      CHECK_GE(q2.MessageCount<EventMsg>(), 2);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::IsRegistered") {
    SUBCASE("Returns true for a registered type") {
      AsyncMessageQueue queue;
      queue.Register<EventMsg>();
      CHECK(queue.IsRegistered<EventMsg>());
    }

    SUBCASE("Returns false for an unregistered type") {
      const AsyncMessageQueue queue;
      CHECK_FALSE(queue.IsRegistered<EventMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::HasMessages") {
    SUBCASE("Returns false when no messages exist across all types") {
      AsyncMessageQueue queue;
      queue.Register<EventMsg>();
      CHECK_FALSE(queue.HasMessages());
    }

    SUBCASE("Returns true when at least one message exists across all types") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{});

      CHECK(queue.HasMessages());
    }

    SUBCASE(
        "Typed HasMessages<T> returns false for registered but empty type") {
      AsyncMessageQueue queue;
      queue.Register<EventMsg>();
      CHECK_FALSE(queue.HasMessages<EventMsg>());
    }

    SUBCASE("Typed HasMessages<T> returns true after enqueue for that type") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{});

      CHECK(queue.HasMessages<EventMsg>());
    }

    SUBCASE("Typed HasMessages<T> returns false for unregistered type") {
      const AsyncMessageQueue queue;
      CHECK_FALSE(queue.HasMessages<EventMsg>());
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::TypeCount") {
    SUBCASE("Returns zero for empty queue") {
      const AsyncMessageQueue queue;
      CHECK_EQ(queue.TypeCount(), 0);
    }

    SUBCASE("Increments per distinct registered type") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      CHECK_EQ(queue.TypeCount(), 1);
      queue.Register<CommandMsg>();
      CHECK_EQ(queue.TypeCount(), 2);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::MessageCount") {
    SUBCASE("Total MessageCount returns zero for empty queue") {
      const AsyncMessageQueue queue;
      CHECK_EQ(queue.MessageCount(), 0);
    }

    SUBCASE("Total MessageCount accumulates across all types") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<CommandMsg>();
      queue.Enqueue(EventMsg{});
      queue.Enqueue(EventMsg{});
      queue.Enqueue(CommandMsg{});

      CHECK_GE(queue.MessageCount(), 3);
    }

    SUBCASE(
        "Typed MessageCount<T> returns zero for registered but empty type") {
      AsyncMessageQueue queue;
      queue.Register<EventMsg>();
      CHECK_EQ(queue.MessageCount<EventMsg>(), 0);
    }

    SUBCASE("Typed MessageCount<T> returns zero for unregistered type") {
      const AsyncMessageQueue queue;
      CHECK_EQ(queue.MessageCount<EventMsg>(), 0);
    }

    SUBCASE("Typed MessageCount<T> counts only messages of that type") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Register<CommandMsg>();
      queue.Enqueue(EventMsg{});
      queue.Enqueue(EventMsg{});
      queue.Enqueue(CommandMsg{});

      CHECK_GE(queue.MessageCount<EventMsg>(), 2);
      CHECK_GE(queue.MessageCount<CommandMsg>(), 1);
    }
  }

  TEST_CASE("ecs::AsyncMessageQueue::TypedStorage") {
    SUBCASE("Mutable TypedStorage allows direct enqueue and dequeue") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.TypedStorage<EventMsg>().Enqueue(EventMsg{50});
      EventMsg dest;
      const bool ok = queue.TypedStorage<EventMsg>().Dequeue(dest);

      CHECK(ok);
      CHECK_EQ(dest.id, 50);
    }

    SUBCASE("Const TypedStorage allows reading SizeApprox") {
      AsyncMessageQueue queue;

      queue.Register<EventMsg>();
      queue.Enqueue(EventMsg{});

      const auto& const_queue = queue;
      CHECK_GE(const_queue.TypedStorage<EventMsg>().SizeApprox(), 1);
    }
  }
}
