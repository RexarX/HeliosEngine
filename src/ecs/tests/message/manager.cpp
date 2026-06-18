#include <doctest/doctest.h>

#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/queue.hpp>

#include <functional>
#include <string_view>
#include <utility>
#include <vector>

using namespace helios::ecs;
using ConsumedRegistry = helios::ecs::ConsumedMessagesRegistry<>;

namespace {

struct Position {
  static constexpr bool kConsumable = true;

  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct ManualMessage {
  static constexpr auto kClearPolicy = MessageClearPolicy::kManual;

  int value = 0;
};

struct NamedMessage {
  static constexpr std::string_view kName = "NamedMessage";

  int value = 0;
};

struct AsyncEvent {
  static constexpr bool kAsync = true;

  int id = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::MessageManager") {
  TEST_CASE("ecs::MessageManager::ctor") {
    SUBCASE("Default ctor produces empty manager") {
      const MessageManager manager;
      CHECK_EQ(manager.RegisteredMessageCount(), 0);
      CHECK_FALSE(manager.HasMessages());
    }

    SUBCASE("Move ctor") {
      MessageManager original;

      original.Register<Position>();
      original.Write(Position{});

      const MessageManager moved(std::move(original));

      CHECK(moved.IsRegistered<Position>());
      CHECK(moved.HasMessages<Position>());
    }
  }

  TEST_CASE("ecs::MessageManager::operator=") {
    SUBCASE("Move assignment") {
      MessageManager original;

      original.Register<Position>();
      original.Write(Position{});

      MessageManager assigned;
      assigned = std::move(original);

      CHECK(assigned.IsRegistered<Position>());
      CHECK(assigned.HasMessages<Position>());
    }
  }

  TEST_CASE("ecs::MessageManager::Clear") {
    SUBCASE("Clear on empty manager is a no-op") {
      MessageManager manager;

      manager.Clear();

      CHECK_EQ(manager.RegisteredMessageCount(), 0);
      CHECK_FALSE(manager.HasMessages());
    }

    SUBCASE("Clear removes all registrations and messages") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<Velocity>();
      manager.Write(Position{});
      manager.Write(Velocity{});
      manager.Clear();

      CHECK_EQ(manager.RegisteredMessageCount(), 0);
      CHECK_FALSE(manager.IsRegistered<Position>());
      CHECK_FALSE(manager.IsRegistered<Velocity>());
      CHECK_FALSE(manager.HasMessages());
    }

    SUBCASE("After Clear, types can be re-registered") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Clear();
      manager.Register<Position>();

      CHECK(manager.IsRegistered<Position>());
    }
  }

  TEST_CASE("ecs::MessageManager::ClearAllQueues") {
    SUBCASE("ClearAllQueues empties messages but preserves registrations") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<Velocity>();
      manager.Write(Position{});
      manager.Write(Velocity{});
      manager.ClearAllQueues();

      CHECK(manager.IsRegistered<Position>());
      CHECK(manager.IsRegistered<Velocity>());
      CHECK_EQ(manager.RegisteredMessageCount(), 2);
      CHECK_FALSE(manager.HasMessages());
    }

    SUBCASE("ClearAllQueues with no messages is a no-op") {
      MessageManager manager;

      manager.Register<Position>();
      manager.ClearAllQueues();

      CHECK(manager.IsRegistered<Position>());
      CHECK_FALSE(manager.HasMessages());
    }
  }

  TEST_CASE("ecs::MessageManager::Update (no consumed)") {
    SUBCASE(
        "Messages written this frame are readable from previous queue after "
        "one Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{1.0F, 2.0F});
      manager.Update();

      const auto prev = manager.PreviousMessages<Position>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].x, 1.0F);
      CHECK_EQ(prev[0].y, 2.0F);
    }

    SUBCASE(
        "Automatic messages are cleared from previous queue after two "
        "Updates") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();
      manager.Update();

      CHECK_FALSE(manager.HasMessages<Position>());
    }

    SUBCASE("Manual policy messages survive two Updates") {
      MessageManager manager;

      manager.Register<ManualMessage>();
      manager.Write(ManualMessage{42});
      manager.Update();
      manager.Update();

      const auto prev = manager.PreviousMessages<ManualMessage>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].value, 42);
    }

    SUBCASE("Current queue is empty after Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      const auto curr = manager.CurrentMessages<Position>();
      CHECK(curr.empty());
    }

    SUBCASE("Multiple types updated independently") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<Velocity>();
      manager.Write(Position{});
      manager.Write(Velocity{});
      manager.Update();

      CHECK_EQ(manager.PreviousMessages<Position>().size(), 1);
      CHECK_EQ(manager.PreviousMessages<Velocity>().size(), 1);
    }
  }

  TEST_CASE("ecs::MessageManager::Update (with consumed registry)") {
    SUBCASE("Consumed previous message is removed after Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(0);  // Index 0 in previous queue
      manager.Update(registry);

      CHECK_FALSE(manager.HasMessages<Position>());
    }

    SUBCASE("Unconsumed messages survive Update with consumed registries") {
      MessageManager manager;
      manager.Register<Position>();
      manager.Write(Position{});
      manager.Write(Position{2.0F, 0.0F});
      manager.Update();

      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(0);  // Consume only the first
      manager.Update(registry);

      const auto prev = manager.PreviousMessages<Position>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].x, 2.0F);
    }

    SUBCASE("Empty consumed registries do not affect messages") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      ConsumedRegistry empty_registry;
      manager.Update(empty_registry);

      CHECK_EQ(manager.PreviousMessages<Position>().size(), 1);
    }

    SUBCASE("Consumed current message is removed during same Update cycle") {
      MessageManager manager;

      manager.Register<Position>();

      // Write current frame messages and immediately consume one via a registry
      manager.Update();  // prev is empty; current gets Position below
      manager.Write(Position{});
      manager.Write(Position{20.0F, 0.0F});

      // Index layout before Update: prev=0 items, current=[10, 20] -> global
      // indices 0, 1
      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(
          0);  // consume global index 0 -> current[0]

      manager.Update(registry);

      const auto prev = manager.PreviousMessages<Position>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].x, 20.0F);
    }
  }

  TEST_CASE("ecs::MessageManager::ApplyConsumed") {
    SUBCASE(
        "ApplyConsumed removes consumed messages from both queues without "
        "buffer swap") {
      MessageManager manager;

      manager.Register<Position>();

      manager.Write(Position{1.0F, 0.0F});
      manager.Update();
      manager.Write(Position{2.0F, 0.0F});

      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(0);
      registry.MarkConsumed<Position>(1);

      manager.ApplyConsumed(registry);

      CHECK_FALSE(manager.HasMessages<Position>());
      CHECK_EQ(manager.PreviousMessages<Position>().size(), 0);
      CHECK_EQ(manager.CurrentMessages<Position>().size(), 0);
    }

    SUBCASE("ApplyConsumed does not swap buffers") {
      MessageManager manager;

      manager.Register<Position>();

      manager.Write(Position{10.0F, 0.0F});
      manager.Update();

      manager.Write(Position{20.0F, 0.0F});

      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(0);

      manager.ApplyConsumed(registry);

      CHECK_EQ(manager.PreviousMessages<Position>().size(), 0);
      CHECK_EQ(manager.CurrentMessages<Position>().size(), 1);
      CHECK_EQ(manager.CurrentMessages<Position>()[0].x, 20.0F);
    }

    SUBCASE("ApplyConsumed with empty registry is a no-op") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      ConsumedRegistry empty;
      manager.ApplyConsumed(empty);

      CHECK_EQ(manager.PreviousMessages<Position>().size(), 1);
    }

    SUBCASE(
        "ApplyConsumed removes only consumed messages from previous queue") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{1.0F, 0.0F});
      manager.Write(Position{2.0F, 0.0F});
      manager.Write(Position{3.0F, 0.0F});
      manager.Update();

      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(1);

      manager.ApplyConsumed(registry);

      const auto prev = manager.PreviousMessages<Position>();
      CHECK_EQ(prev.size(), 2);
      CHECK_EQ(prev[0].x, 1.0F);
      CHECK_EQ(prev[1].x, 3.0F);
    }

    SUBCASE(
        "ApplyConsumed removes consumed messages from current queue using "
        "global indices") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{10.0F, 0.0F});
      manager.Update();
      manager.Write(Position{20.0F, 0.0F});
      manager.Write(Position{30.0F, 0.0F});

      ConsumedRegistry registry;
      registry.MarkConsumed<Position>(1);
      registry.MarkConsumed<Position>(2);

      manager.ApplyConsumed(registry);

      const auto prev = manager.PreviousMessages<Position>();
      const auto curr = manager.CurrentMessages<Position>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].x, 10.0F);
      CHECK_EQ(curr.size(), 0);
    }
  }

  TEST_CASE("ecs::MessageManager::Register") {
    SUBCASE("Register single sync type") {
      MessageManager manager;

      manager.Register<Position>();

      CHECK(manager.IsRegistered<Position>());
      CHECK_EQ(manager.RegisteredMessageCount(), 1);
    }

    SUBCASE("RegisterMessages registers all provided types") {
      MessageManager manager;

      manager.Register<Position, Velocity>();

      CHECK(manager.IsRegistered<Position>());
      CHECK(manager.IsRegistered<Velocity>());
      CHECK_EQ(manager.RegisteredMessageCount(), 2);
    }

    SUBCASE("Register multiple sync types") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<Velocity>();

      CHECK(manager.IsRegistered<Position>());
      CHECK(manager.IsRegistered<Velocity>());
      CHECK_EQ(manager.RegisteredMessageCount(), 2);
    }

    SUBCASE("Register async type") {
      MessageManager manager;

      manager.Register<AsyncEvent>();

      CHECK(manager.IsRegistered<AsyncEvent>());
      CHECK_EQ(manager.RegisteredMessageCount(), 1);
    }

    SUBCASE("Metadata is stored correctly for sync type") {
      MessageManager manager;

      manager.Register<NamedMessage>();

      const auto* meta = manager.Metadata<NamedMessage>();
      REQUIRE(meta != nullptr);
      CHECK_EQ(meta->name, "NamedMessage");
      CHECK_EQ(meta->clear_policy, MessageClearPolicy::kAutomatic);
      CHECK_FALSE(meta->is_async);
    }

    SUBCASE("Metadata is stored correctly for async type") {
      MessageManager manager;

      manager.Register<AsyncEvent>();

      const auto* meta = manager.Metadata<AsyncEvent>();
      REQUIRE(meta != nullptr);
      CHECK(meta->is_async);
    }

    SUBCASE("Metadata is stored correctly for manual clear policy type") {
      MessageManager manager;

      manager.Register<ManualMessage>();

      const auto* meta = manager.Metadata<ManualMessage>();
      REQUIRE(meta != nullptr);
      CHECK_EQ(meta->clear_policy, MessageClearPolicy::kManual);
    }
  }

  TEST_CASE("ecs::MessageManager::Write") {
    SUBCASE("Write single message into current queue") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{1.0F, 2.0F});

      const auto curr = manager.CurrentMessages<Position>();
      CHECK_EQ(curr.size(), 1);
      CHECK_EQ(curr[0].x, 1.0F);
      CHECK_EQ(curr[0].y, 2.0F);
    }

    SUBCASE("Write multiple messages accumulate in current queue") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Write(Position{});
      manager.Write(Position{});

      CHECK_EQ(manager.CurrentMessages<Position>().size(), 3);
    }

    SUBCASE("Write rvalue message is stored correctly") {
      MessageManager manager;

      manager.Register<Velocity>();
      manager.Write(Velocity{5.0F, 6.0F});

      const auto curr = manager.CurrentMessages<Velocity>();
      CHECK_EQ(curr.size(), 1);
      CHECK_EQ(curr[0].dx, 5.0F);
      CHECK_EQ(curr[0].dy, 6.0F);
    }
  }

  TEST_CASE("ecs::MessageManager::WriteAsync") {
    SUBCASE("WriteAsync enqueues message to async queue") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      manager.WriteAsync(AsyncEvent{42});

      CHECK(manager.HasAsyncMessages<AsyncEvent>());
    }
  }

  TEST_CASE("ecs::MessageManager::WriteBulk") {
    SUBCASE("WriteBulk enqueues all messages in the range") {
      MessageManager manager;

      manager.Register<Position>();
      const std::vector<Position> msgs(3);
      manager.WriteBulk(msgs);

      CHECK_EQ(manager.CurrentMessages<Position>().size(), 3);
    }

    SUBCASE("WriteBulk with empty range does not add messages") {
      MessageManager manager;

      manager.Register<Position>();
      const std::vector<Position> empty;
      manager.WriteBulk(empty);

      CHECK_FALSE(manager.HasMessages<Position>());
    }
  }

  TEST_CASE("ecs::MessageManager::WriteAsyncBulk") {
    SUBCASE("WriteAsyncBulk enqueues all async messages") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      const std::vector<AsyncEvent> msgs(3);
      manager.WriteAsyncBulk(msgs);

      CHECK(manager.HasAsyncMessages<AsyncEvent>());
    }
  }

  TEST_CASE("ecs::MessageManager::ManualClear") {
    SUBCASE("ManualClear removes messages from both queues for sync type") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();
      manager.Write(Position{});
      manager.ManualClear<Position>();

      CHECK_FALSE(manager.HasMessages<Position>());
    }

    SUBCASE("ManualClear does not affect other registered types") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<Velocity>();
      manager.Write(Position{});
      manager.Write(Velocity{});
      manager.ManualClear<Position>();

      CHECK_FALSE(manager.HasMessages<Position>());
      CHECK(manager.HasMessages<Velocity>());
    }
  }

  TEST_CASE("ecs::MessageManager::ManualAsyncClear") {
    SUBCASE(
        "ManualAsyncClear removes messages from both queues for async type") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      manager.WriteAsync(AsyncEvent{});
      manager.Update();
      manager.WriteAsync(AsyncEvent{});
      manager.ManualAsyncClear<AsyncEvent>();

      CHECK_FALSE(manager.HasAsyncMessages<AsyncEvent>());
    }

    SUBCASE("ManualAsyncClear does not affect other registered types") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      manager.Register<Position>();
      manager.WriteAsync(AsyncEvent{});
      manager.Write(Position{});
      manager.ManualAsyncClear<AsyncEvent>();

      CHECK_FALSE(manager.HasAsyncMessages<AsyncEvent>());
      CHECK(manager.HasMessages<Position>());
    }
  }

  TEST_CASE("ecs::MessageManager::MergeLocalMessages") {
    SUBCASE(
        "Const lvalue local queue is merged into current queue without "
        "modifying source") {
      MessageManager manager;
      MessageQueue local_mut;
      const MessageQueue<>& local = local_mut;

      manager.Register<Position>();
      local_mut.Register<Position>();
      local_mut.Enqueue(Position{7.0F, 8.0F});

      manager.MergeLocalMessages(local);

      const auto curr = manager.CurrentMessages<Position>();
      CHECK_EQ(curr.size(), 1);
      CHECK_EQ(curr[0].x, 7.0F);
      CHECK_EQ(local.MessageCount<Position>(), 1);
    }

    SUBCASE("Rvalue local queue is merged and consumed") {
      MessageManager manager;
      MessageQueue local;

      manager.Register<Position>();
      local.Register<Position>();
      local.Enqueue(Position{7.0F, 8.0F});

      manager.MergeLocalMessages(std::move(local));

      const auto curr = manager.CurrentMessages<Position>();
      CHECK_EQ(curr.size(), 1);
      CHECK_EQ(curr[0].x, 7.0F);
      CHECK_EQ(local.TypeCount(), 0);
      CHECK_EQ(local.MessageCount(), 0);
    }

    SUBCASE("MergeLocalMessages with multiple messages") {
      MessageManager manager;
      MessageQueue local;

      manager.Register<Position>();
      local.Register<Position>();
      local.Enqueue(Position{});
      local.Enqueue(Position{});

      manager.MergeLocalMessages(std::move(local));

      CHECK_EQ(manager.CurrentMessages<Position>().size(), 2);
    }
  }

  TEST_CASE("ecs::MessageManager::IsRegistered") {
    SUBCASE("Returns false for unregistered type") {
      const MessageManager manager;
      CHECK_FALSE(manager.IsRegistered<Position>());
    }

    SUBCASE("Returns true after registration") {
      MessageManager manager;
      manager.Register<Position>();
      CHECK(manager.IsRegistered<Position>());
    }

    SUBCASE("Returns true for async type after registration") {
      MessageManager manager;
      manager.Register<AsyncEvent>();
      CHECK(manager.IsRegistered<AsyncEvent>());
    }

    SUBCASE("Returns false for a different unregistered type") {
      MessageManager manager;
      manager.Register<Position>();
      CHECK_FALSE(manager.IsRegistered<Velocity>());
    }
  }

  TEST_CASE("ecs::MessageManager::HasMessages (global)") {
    SUBCASE("Returns false when no messages exist") {
      MessageManager manager;
      manager.Register<Position>();
      CHECK_FALSE(manager.HasMessages());
    }

    SUBCASE("Returns true after writing a message") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});

      CHECK(manager.HasMessages());
    }

    SUBCASE("Returns true when messages exist in previous queue") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      CHECK(manager.HasMessages());
    }

    SUBCASE("Returns false after clearing all queues") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.ClearAllQueues();

      CHECK_FALSE(manager.HasMessages());
    }
  }

  TEST_CASE("ecs::MessageManager::HasMessages (by type)") {
    SUBCASE("Returns false for unregistered type") {
      const MessageManager manager;
      CHECK_FALSE(manager.HasMessages<Position>());
    }

    SUBCASE("Returns false for registered type with no messages") {
      MessageManager manager;
      manager.Register<Position>();
      CHECK_FALSE(manager.HasMessages<Position>());
    }

    SUBCASE("Returns true after writing a message") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});

      CHECK(manager.HasMessages<Position>());
    }
  }

  TEST_CASE("ecs::MessageManager::HasAsyncMessages") {
    SUBCASE("Returns false for unregistered type") {
      const MessageManager manager;
      CHECK_FALSE(manager.HasMessages<Position>());
    }

    SUBCASE("Returns false before writing") {
      MessageManager manager;
      manager.Register<AsyncEvent>();
      CHECK_FALSE(manager.HasAsyncMessages<AsyncEvent>());
    }

    SUBCASE("Returns true after writing") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      manager.WriteAsync(AsyncEvent{});

      CHECK(manager.HasAsyncMessages<AsyncEvent>());
    }
  }

  TEST_CASE("ecs::MessageManager::PreviousMessages") {
    SUBCASE("Empty before any Update") {
      MessageManager manager;
      manager.Register<Position>();
      manager.Write(Position{});

      CHECK(manager.PreviousMessages<Position>().empty());
    }

    SUBCASE("Contains written messages after one Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{1.0F, 2.0F});
      manager.Update();

      const auto prev = manager.PreviousMessages<Position>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].x, 1.0F);
    }
  }

  TEST_CASE("ecs::MessageManager::CurrentMessages") {
    SUBCASE("Contains written messages before Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{3.0F, 4.0F});

      const auto curr = manager.CurrentMessages<Position>();
      CHECK_EQ(curr.size(), 1);
      CHECK_EQ(curr[0].x, 3.0F);
    }

    SUBCASE("Empty after Update (messages moved to previous)") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      CHECK(manager.CurrentMessages<Position>().empty());
    }
  }

  TEST_CASE("ecs::MessageManager::Metadata") {
    SUBCASE("Returns nullptr for unregistered type") {
      const MessageManager manager;
      CHECK(manager.Metadata<Position>() == nullptr);
    }

    SUBCASE("Returns correct metadata after registration") {
      MessageManager manager;
      manager.Register<NamedMessage>();

      const auto* meta = manager.Metadata<NamedMessage>();
      REQUIRE(meta != nullptr);
      CHECK_EQ(meta->name, "NamedMessage");
      CHECK_EQ(meta->type_index, MessageTypeIndex::From<NamedMessage>());
    }
  }

  TEST_CASE("ecs::MessageManager::RegisteredMessageCount") {
    SUBCASE("Zero initially") {
      const MessageManager manager;
      CHECK_EQ(manager.RegisteredMessageCount(), 0);
    }

    SUBCASE("Increases with each registration") {
      MessageManager manager;

      manager.Register<Position>();
      CHECK_EQ(manager.RegisteredMessageCount(), 1);

      manager.Register<Velocity>();
      CHECK_EQ(manager.RegisteredMessageCount(), 2);

      manager.Register<AsyncEvent>();
      CHECK_EQ(manager.RegisteredMessageCount(), 3);
    }

    SUBCASE("Resets to zero after Clear") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<Velocity>();
      manager.Clear();

      CHECK_EQ(manager.RegisteredMessageCount(), 0);
    }
  }

  TEST_CASE("ecs::MessageManager::CurrentQueue") {
    SUBCASE("Returns reference to current queue") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});

      const auto& queue = manager.CurrentQueue();
      CHECK_EQ(queue.MessageCount<Position>(), 1);
    }
  }

  TEST_CASE("ecs::MessageManager::PreviousQueue") {
    SUBCASE("Returns reference to previous queue, empty before any Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});

      CHECK(manager.PreviousQueue().Messages<Position>().empty());
    }

    SUBCASE("Contains messages after one Update") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Write(Position{});
      manager.Update();

      CHECK_EQ(manager.PreviousQueue().MessageCount<Position>(), 1);
    }
  }

  TEST_CASE("ecs::MessageManager::AsyncQueue") {
    SUBCASE("Non-const AsyncQueue returns mutable reference") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      AsyncMessageQueue& queue = manager.AsyncQueue();
      queue.Enqueue<AsyncEvent>(AsyncEvent{});

      CHECK(manager.HasAsyncMessages<AsyncEvent>());
    }

    SUBCASE("Const AsyncQueue returns const reference") {
      MessageManager manager;

      manager.Register<AsyncEvent>();
      manager.WriteAsync(AsyncEvent{});
      const MessageManager& const_manager = manager;
      const AsyncMessageQueue& queue = const_manager.AsyncQueue();

      CHECK(queue.HasMessages<AsyncEvent>());
    }
  }

  TEST_CASE("ecs::MessageManager::lifecycle") {
    SUBCASE("Full double-buffer cycle with automatic clear") {
      MessageManager manager;

      manager.Register<Position>();

      // Frame 1: write
      manager.Write(Position{});
      CHECK(manager.CurrentMessages<Position>().size() == 1);

      // After frame 1 update: current -> previous
      manager.Update();
      CHECK(manager.PreviousMessages<Position>().size() == 1);
      CHECK(manager.CurrentMessages<Position>().empty());

      // Frame 2: write new message; previous still visible
      manager.Write(Position{2.0F, 0.0F});

      // After frame 2 update: auto clear previous, current (frame2 msg) ->
      // previous
      manager.Update();

      const auto prev = manager.PreviousMessages<Position>();
      CHECK_EQ(prev.size(), 1);
      CHECK_EQ(prev[0].x, 2.0F);
    }

    SUBCASE("Manual clear policy message persists across many Updates") {
      MessageManager manager;

      manager.Register<ManualMessage>();
      manager.Write(ManualMessage{});

      for (int i = 0; i < 5; ++i) {
        manager.Update();
      }

      CHECK(manager.HasMessages<ManualMessage>());
    }

    SUBCASE("Mix of automatic and manual messages handled independently") {
      MessageManager manager;

      manager.Register<Position>();
      manager.Register<ManualMessage>();
      manager.Write(Position{});
      manager.Write(ManualMessage{});
      manager.Update();
      manager.Update();

      CHECK_FALSE(manager.HasMessages<Position>());
      CHECK(manager.HasMessages<ManualMessage>());
    }
  }
}
