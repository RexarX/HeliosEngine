#include <doctest/doctest.h>

#include <helios/ecs/message/cursor.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/reader.hpp>

#include <array>
#include <compare>
#include <memory_resource>
#include <utility>
#include <vector>

using namespace helios::ecs;

namespace {

struct ConsumedRegistry {
  std::pmr::monotonic_buffer_resource resource;
  ConsumedMessagesRegistry registry;

  ConsumedRegistry() : registry(&resource) {}

  operator ConsumedMessagesRegistry&() noexcept { return registry; }
};

struct Score {
  static constexpr bool kConsumable = true;

  int value = 0;
};

struct Ping {
  int value = 0;
};

// Helper: build a MessageManager with two frames worth of messages written.
// After one Update(), previous=[prev_msgs], current=[].
// Then write curr_msgs so: previous=[prev_msgs], current=[curr_msgs].
MessageManager MakeManager(const std::vector<Score>& prev_msgs,
                           const std::vector<Score>& curr_msgs) {
  MessageManager manager;
  manager.Register<Score>();
  for (const auto& msg : prev_msgs) {
    manager.Write(msg);
  }

  manager.Update();
  for (const auto& msg : curr_msgs) {
    manager.Write(msg);
  }
  return manager;
}

MessageManager MakePingManager(const std::vector<Ping>& prev_msgs,
                               const std::vector<Ping>& curr_msgs) {
  MessageManager manager;
  manager.Register<Ping>();
  for (const auto& msg : prev_msgs) {
    manager.Write(msg);
  }

  manager.Update();
  for (const auto& msg : curr_msgs) {
    manager.Write(msg);
  }
  return manager;
}

MessageManager MakeEmptyScoreManager() {
  MessageManager manager;
  manager.Register<Score>();
  return manager;
}

template <MessageTrait T>
std::vector<AnyMessageId> MakeMessageIds(std::initializer_list<size_t> values) {
  std::vector<AnyMessageId> ids;
  ids.reserve(values.size());
  for (const size_t value : values) {
    ids.push_back({.value = value, .type = MessageTypeIndex::From<T>()});
  }
  return ids;
}

}  // namespace

TEST_SUITE("helios::ecs::ConsumableMessageWrapperIter") {
  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::ctor") {
    SUBCASE("Copy ctor preserves position") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      const ConsumableMessageWrapperIter<Score> iter(
          msgs, {}, ids, {}, registry, &cursor, 0, 0, 1);
      const ConsumableMessageWrapperIter<Score> copy(iter);

      CHECK_EQ(copy.Position(), 1);
    }

    SUBCASE("Move ctor preserves position") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(1);
      const auto ids = MakeMessageIds<Score>({0});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 0);
      const ConsumableMessageWrapperIter<Score> moved(std::move(iter));

      CHECK_EQ(moved.Position(), 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::assignment") {
    SUBCASE("Copy assignment preserves position") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(3);
      const auto ids = MakeMessageIds<Score>({0, 1, 2});
      const ConsumableMessageWrapperIter<Score> original(
          msgs, {}, ids, {}, registry, &cursor, 0, 0, 2);
      ConsumableMessageWrapperIter<Score> assigned(msgs, {}, ids, {}, registry,
                                                   &cursor, 0, 0, 0);
      assigned = original;

      CHECK_EQ(assigned.Position(), 2);
    }

    SUBCASE("Move assignment preserves position") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(1);
      const auto ids = MakeMessageIds<Score>({0});
      ConsumableMessageWrapperIter<Score> original(msgs, {}, ids, {}, registry,
                                                   &cursor, 0, 0, 1);
      ConsumableMessageWrapperIter<Score> assigned(msgs, {}, ids, {}, registry,
                                                   &cursor, 0, 0, 0);
      assigned = std::move(original);

      CHECK_EQ(assigned.Position(), 1);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator*") {
    SUBCASE(
        "Dereference yields wrapper pointing to correct message in first "
        "span") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> first = {{10}, {20}};
      const auto ids = MakeMessageIds<Score>({0, 1});
      const ConsumableMessageWrapperIter<Score> iter(
          first, {}, ids, {}, registry, &cursor, 0, 0, 0);
      CHECK_EQ((*iter)->value, 10);
    }

    SUBCASE("Dereference at second-span position yields correct message") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> first = {{1}};
      const std::vector<Score> second = {{99}};
      const auto prev_ids = MakeMessageIds<Score>({0});
      const auto curr_ids = MakeMessageIds<Score>({1});

      const ConsumableMessageWrapperIter<Score> iter(
          first, second, prev_ids, curr_ids, registry, &cursor, 0, 0, 1);
      CHECK_EQ((*iter)->value, 99);
    }

    SUBCASE("Dereference yields correct message id in wrapper") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> first(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      const ConsumableMessageWrapperIter<Score> iter(
          first, {}, ids, {}, registry, &cursor, 0, 0, 1);
      CHECK_EQ((*iter).Id().value, 1);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator++ (pre)") {
    SUBCASE("Pre-increment advances position by one") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 0);
      ++iter;

      CHECK_EQ(iter.Position(), 1);
      CHECK_EQ(cursor.last_message_count.value, 1);
    }

    SUBCASE("Pre-increment returns reference to self") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 0);
      auto& returned = ++iter;

      CHECK_EQ(&returned, &iter);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator++ (post)") {
    SUBCASE("Post-increment returns copy at old position") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 0);
      const auto old = iter++;

      CHECK_EQ(old.Position(), 0);
      CHECK_EQ(iter.Position(), 1);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator-- (pre)") {
    SUBCASE("Pre-decrement retreats position by one") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 1);
      --iter;

      CHECK_EQ(iter.Position(), 0);
    }

    SUBCASE("Pre-decrement returns reference to self") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 1);
      auto& returned = --iter;

      CHECK_EQ(&returned, &iter);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator-- (post)") {
    SUBCASE("Post-decrement returns copy at old position") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(2);
      const auto ids = MakeMessageIds<Score>({0, 1});
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, ids, {}, registry,
                                               &cursor, 0, 0, 2);
      const auto old = iter--;

      CHECK_EQ(old.Position(), 2);
      CHECK_EQ(iter.Position(), 1);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator==") {
    SUBCASE("Two iterators at the same position are equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 3);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 3);

      CHECK_EQ(lhs == rhs, true);
    }

    SUBCASE("Two iterators at different positions are not equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 0);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 1);

      CHECK_FALSE(lhs == rhs);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator!=") {
    SUBCASE("Iterators at different positions compare not-equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 0);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 2);

      CHECK_EQ(lhs != rhs, true);
    }

    SUBCASE("Iterators at the same position are not not-equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 5);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 5);

      CHECK_FALSE(lhs != rhs);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::operator<=>") {
    SUBCASE("Earlier position compares less than later position") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 0);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 2);

      CHECK_EQ(lhs <=> rhs, std::strong_ordering::less);
    }

    SUBCASE("Later position compares greater than earlier position") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 3);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 1);
      CHECK_EQ(lhs <=> rhs, std::strong_ordering::greater);
    }

    SUBCASE("Same positions compare equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 4);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, {}, {}, registry,
                                                    nullptr, 0, 0, 4);

      CHECK_EQ(lhs <=> rhs, std::strong_ordering::equal);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::Position") {
    SUBCASE("Returns the position supplied at construction") {
      ConsumedRegistry registry;
      const ConsumableMessageWrapperIter<Score> iter({}, {}, {}, {}, registry,
                                                     nullptr, 0, 0, 7);
      CHECK_EQ(iter.Position(), 7);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapperIter::begin / end") {
    SUBCASE("begin returns iterator at position 0") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> msgs(3);
      const auto ids = MakeMessageIds<Score>({0, 1, 2});
      const ConsumableMessageWrapperIter<Score> iter(
          msgs, {}, ids, {}, registry, &cursor, 0, 0, 2);

      CHECK_EQ(iter.begin().Position(), 0);
    }

    SUBCASE("end returns iterator at position equal to unread count") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> first(2);
      const std::vector<Score> second(1);
      const auto prev_ids = MakeMessageIds<Score>({0, 1});
      const auto curr_ids = MakeMessageIds<Score>({2});
      const ConsumableMessageWrapperIter<Score> iter(
          first, second, prev_ids, curr_ids, registry, &cursor, 0, 0, 0);

      CHECK_EQ(iter.end().Position(), 3);
    }

    SUBCASE("Range-for over the iterator visits all messages") {
      ConsumedRegistry registry;
      auto cursor = MessageCursor<Score>::IncludeBacklog();

      const std::vector<Score> first = {{10}, {20}};
      const std::vector<Score> second = {{30}};
      const auto prev_ids = MakeMessageIds<Score>({0, 1});
      const auto curr_ids = MakeMessageIds<Score>({2});
      const ConsumableMessageWrapperIter<Score> iter(
          first, second, prev_ids, curr_ids, registry, &cursor, 0, 0, 0);

      int sum = 0;
      for (const auto wrapper : iter) {
        sum += wrapper->value;
      }
      CHECK_EQ(sum, 60);
      CHECK_EQ(cursor.last_message_count.value, 3);
    }
  }
}

TEST_SUITE("helios::ecs::ConsumableMessageReader") {
  TEST_CASE("helios::ecs::ConsumableMessageReader::ctor") {
    SUBCASE("Construct from MessageManager, cursor and registry") {
      const auto manager = MakeManager({{}, {}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_EQ(reader.Count(), 3);
    }

    SUBCASE("Move ctor") {
      const auto manager = MakeManager({{}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      ConsumableMessageReader<Score> original(manager, cursor, registry);
      const ConsumableMessageReader<Score> moved(std::move(original));

      CHECK_EQ(moved.Count(), 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::operator=") {
    SUBCASE("Move assignment") {
      const auto manager = MakeManager({{}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      ConsumableMessageReader<Score> original(manager, cursor, registry);

      auto manager2 = MakeEmptyScoreManager();
      MessageCursor<Score> cursor2 = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry2;
      ConsumableMessageReader<Score> assigned(manager2, cursor2, registry2);
      assigned = std::move(original);

      CHECK_EQ(assigned.Count(), 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Read") {
    SUBCASE("Read yields all unread messages") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      std::vector<int> values;
      for (const auto wrapper : reader.Read()) {
        values.push_back(wrapper->value);
      }

      REQUIRE_EQ(values.size(), 3);
      CHECK_EQ(values[0], 1);
      CHECK_EQ(values[1], 2);
      CHECK_EQ(values[2], 3);
    }

    SUBCASE("Second Read yields nothing after cursor advanced") {
      const auto manager = MakeManager({{1}}, {{2}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      CHECK_EQ(reader.Collect().size(), 2);
      CHECK(reader.Empty());
      CHECK_EQ(reader.Collect().size(), 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::ConsumeAll") {
    SUBCASE("Marks every unread message as consumed by id") {
      const auto manager = MakeManager({{}, {}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      reader.ConsumeAll();

      CHECK_EQ(registry.registry.ConsumedCount<Score>(), 3);
      CHECK(registry.registry.IsConsumed<Score>(MessageId<Score>{0}));
      CHECK(registry.registry.IsConsumed<Score>(MessageId<Score>{1}));
      CHECK(registry.registry.IsConsumed<Score>(MessageId<Score>{2}));
    }

    SUBCASE("ConsumeAll on empty reader does nothing") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      reader.ConsumeAll();

      CHECK(registry.registry.Empty());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::ConsumeIf") {
    SUBCASE("Marks matching messages as consumed and returns count") {
      const auto manager = MakeManager({{1}, {5}, {3}}, {{7}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto count =
          reader.ConsumeIf([](const Score& score) { return score.value > 4; });

      CHECK_EQ(count, 2);
      CHECK(registry.registry.IsConsumed<Score>(MessageId<Score>{1}));
      CHECK(registry.registry.IsConsumed<Score>(MessageId<Score>{3}));
    }

    SUBCASE("Returns zero when no messages match the predicate") {
      const auto manager = MakeManager({{1}, {2}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto count = reader.ConsumeIf(
          [](const Score& score) { return score.value > 100; });

      CHECK_EQ(count, 0);
      CHECK(registry.registry.Empty());
    }

    SUBCASE("Returns total count when all messages match") {
      const auto manager = MakeManager({{}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto count =
          reader.ConsumeIf([](const Score& /*score*/) { return true; });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Clear") {
    SUBCASE("Clear skips retained unread messages until new writes") {
      auto manager = MakeManager({{1}, {2}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      CHECK_EQ(reader.Count(), 2);
      reader.Clear();
      CHECK(reader.Empty());

      manager.Write(Score{9});
      CHECK_EQ(reader.Count(), 1);
      CHECK_EQ(reader.Collect()[0].value, 9);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::MissedMessages") {
    SUBCASE("MissedMessages reports ids dropped before the cursor caught up") {
      MessageManager manager;
      manager.Register<Score>();
      manager.Write(Score{1});
      manager.Write(Score{2});
      manager.Update();
      manager.Update();

      MessageCursor<Score> cursor{.last_message_count = MessageId<Score>{0}};
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_EQ(reader.MissedMessages(), 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Collect") {
    SUBCASE("Collect returns all unread messages from previous and current") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto collected = reader.Collect();

      REQUIRE_EQ(collected.size(), 3);
      CHECK_EQ(collected[0].value, 1);
      CHECK_EQ(collected[1].value, 2);
      CHECK_EQ(collected[2].value, 3);
    }

    SUBCASE("Collect on empty reader returns empty vector") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      CHECK(reader.Collect().empty());
    }

    SUBCASE(
        "Message written, read in current, Update moves to previous, same "
        "cursor does not re-read") {
      MessageManager manager;
      manager.Register<Score>();
      manager.Write(Score{42});

      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_EQ(reader.Collect().size(), 1);
      manager.Update();
      CHECK(reader.Empty());
      CHECK_EQ(reader.Collect().size(), 0);
    }

    SUBCASE("FutureOnly cursor ignores backlog") {
      auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::FutureOnly(manager);
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      CHECK(reader.Empty());

      manager.Write(Score{99});
      CHECK_EQ(reader.Collect().size(), 1);
      CHECK_EQ(reader.Collect().size(), 0);
    }

    SUBCASE("Partial iteration leaves remaining for next begin") {
      const auto manager = MakeManager({{1}, {2}, {3}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      reader.Take(1).ForEach(
          [](const ConsumableMessageWrapper<Score>& /*wrapper*/) {});

      CHECK_EQ(reader.Count(), 2);
      const auto rest = reader.Collect();
      REQUIRE_EQ(rest.size(), 2);
      CHECK_EQ(rest[0].value, 2);
      CHECK_EQ(rest[1].value, 3);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Collect(memory_resource*)") {
    SUBCASE("Collect returns all messages using a memory resource") {
      const auto manager = MakeManager({{10}, {20}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto collected = reader.Collect(std::pmr::get_default_resource());

      REQUIRE_EQ(collected.size(), 2);
      CHECK_EQ(collected[0].value, 10);
      CHECK_EQ(collected[1].value, 20);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::ReadInto") {
    SUBCASE("ReadInto copies all messages into output iterator") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      std::vector<Score> out;
      reader.ReadInto(std::back_inserter(out));

      REQUIRE_EQ(out.size(), 3);
      CHECK_EQ(out[0].value, 1);
      CHECK_EQ(out[2].value, 3);
    }

    SUBCASE("ReadInto into array") {
      const auto manager = MakeManager({{5}, {6}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      std::vector<Score> out(2);
      reader.ReadInto(out.begin());

      CHECK_EQ(out[0].value, 5);
      CHECK_EQ(out[1].value, 6);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Filter") {
    SUBCASE("Filter yields only matching wrappers") {
      const auto manager = MakeManager({{1}, {10}}, {{5}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      std::vector<int> values;
      reader
          .Filter([](const ConsumableMessageWrapper<Score>& wrapper) {
            return wrapper->value >= 5;
          })
          .ForEach([&values](const ConsumableMessageWrapper<Score>& wrapper) {
            values.push_back(wrapper->value);
          });

      CHECK_EQ(values.size(), 2);
    }

    SUBCASE("Filter with always-false predicate yields nothing") {
      const auto manager = MakeManager({{}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader
          .Filter([](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            return false;
          })
          .ForEach(
              [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
                ++count;
              });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Map") {
    SUBCASE("Map transforms each wrapper") {
      const auto manager = MakeManager({{2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      std::vector<int> results;
      reader
          .Map([](const ConsumableMessageWrapper<Score>& wrapper) {
            return wrapper->value * 10;
          })
          .ForEach([&results](int val) { results.push_back(val); });

      CHECK_EQ(results.size(), 2);
      CHECK_NE(std::ranges::find(results, 20), results.end());
      CHECK_NE(std::ranges::find(results, 30), results.end());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Take") {
    SUBCASE("Take limits number of yielded wrappers") {
      const auto manager = MakeManager({{}, {}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader.Take(2).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 2);
    }

    SUBCASE("Take(0) yields nothing") {
      const auto manager = MakeManager({{}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader.Take(0).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Skip") {
    SUBCASE("Skip skips the first N wrappers") {
      const auto manager = MakeManager({{}, {}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader.Skip(2).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 1);
    }

    SUBCASE("Skip more than available yields nothing") {
      const auto manager = MakeManager({{}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader.Skip(100).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::TakeWhile") {
    SUBCASE("Stops yielding once predicate becomes false") {
      const auto manager = MakeManager({{1}, {2}, {10}, {3}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader
          .TakeWhile([](const ConsumableMessageWrapper<Score>& wrapper) {
            return wrapper->value < 5;
          })
          .ForEach(
              [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
                ++count;
              });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::SkipWhile") {
    SUBCASE("Skips elements while predicate holds, then yields the rest") {
      const auto manager = MakeManager({{1}, {2}, {10}, {3}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader
          .SkipWhile([](const ConsumableMessageWrapper<Score>& wrapper) {
            return wrapper->value < 5;
          })
          .ForEach(
              [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
                ++count;
              });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Enumerate") {
    SUBCASE("Enumerate pairs each wrapper with its zero-based index") {
      const auto manager = MakeManager({{}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      std::vector<size_t> indices;
      reader.Enumerate().ForEach(
          [&indices](size_t index,
                     const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            indices.push_back(index);
          });

      CHECK_EQ(indices.size(), 2);
      CHECK_EQ(indices[0], 0);
      CHECK_EQ(indices[1], 1);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Inspect") {
    SUBCASE("Inspect calls side-effect without consuming the sequence") {
      const auto manager = MakeManager({{}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int inspect_count = 0;
      int foreach_count = 0;
      reader
          .Inspect([&inspect_count](
                       const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++inspect_count;
          })
          .ForEach([&foreach_count](
                       const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++foreach_count;
          });

      CHECK_EQ(inspect_count, 2);
      CHECK_EQ(foreach_count, 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::StepBy") {
    SUBCASE("StepBy(1) yields all messages") {
      const auto manager = MakeManager({{}, {}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader.StepBy(1).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 3);
    }

    SUBCASE("StepBy(2) yields every other message") {
      const auto manager = MakeManager({{}, {}, {}, {}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int count = 0;
      reader.StepBy(2).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Reverse") {
    SUBCASE("Reverse yields wrappers in reverse order") {
      const auto manager = MakeManager({{1}, {2}, {3}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      std::vector<int> values;
      reader.Reverse().ForEach(
          [&values](const ConsumableMessageWrapper<Score>& wrapper) {
            values.push_back(wrapper->value);
          });

      REQUIRE_EQ(values.size(), 3);
      CHECK_EQ(values[0], 3);
      CHECK_EQ(values[1], 2);
      CHECK_EQ(values[2], 1);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Slide") {
    SUBCASE("Slide yields windows of `window_size` wrappers") {
      const auto manager = MakeManager({{1}, {2}, {3}, {4}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto windows = reader.Slide(2);

      CHECK_EQ(windows.WindowSize(), 2);

      auto window = *windows;
      CHECK_EQ(window.Size(), 2);
      CHECK_EQ(window[0]->value, 1);
      CHECK_EQ(window[1]->value, 2);

      window = *std::next(windows, 1);
      CHECK_EQ(window.Size(), 2);
      CHECK_EQ(window[0]->value, 2);
      CHECK_EQ(window[1]->value, 3);

      window = *std::next(windows, 2);
      CHECK_EQ(window.Size(), 2);
      CHECK_EQ(window[0]->value, 3);
      CHECK_EQ(window[1]->value, 4);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Stride") {
    SUBCASE("Stride yields every nth element") {
      const auto manager = MakeManager({{1}, {2}, {3}, {4}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto stride = reader.Stride(2);

      CHECK_EQ((*stride)->value, 1);
      CHECK_EQ((*std::next(stride))->value, 3);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Zip") {
    const auto manager = MakeManager({{1}, {2}, {3}, {4}}, {});
    constexpr std::array expected = {0, 1, 2, 3};

    SUBCASE("Iterator overload") {
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto zipped = reader.Zip(expected.begin(), expected.end());

      std::vector<std::pair<int, int>> values;
      for (const auto& [wrapper, num] : zipped) {
        values.emplace_back(wrapper->value, num);
      }

      CHECK_EQ(values.size(), 4);
      CHECK_EQ(values[0], std::pair{1, 0});
      CHECK_EQ(values[1], std::pair{2, 1});
      CHECK_EQ(values[2], std::pair{3, 2});
      CHECK_EQ(values[3], std::pair{4, 3});
    }

    SUBCASE("Range overload") {
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto zipped = reader.Zip(expected.begin(), expected.end());

      std::vector<std::pair<int, int>> values;
      for (const auto& [wrapper, num] : zipped) {
        values.emplace_back(wrapper->value, num);
      }

      CHECK_EQ(values.size(), 4);
      CHECK_EQ(values[0], std::pair{1, 0});
      CHECK_EQ(values[1], std::pair{2, 1});
      CHECK_EQ(values[2], std::pair{3, 2});
      CHECK_EQ(values[3], std::pair{4, 3});
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Fold") {
    SUBCASE("Fold accumulates all messages from previous and current") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const int sum = reader.Fold(
          0, [](int acc, const ConsumableMessageWrapper<Score>& score) {
            return acc + score->value;
          });

      CHECK_EQ(sum, 6);
    }

    SUBCASE("Fold with empty reader returns initial value") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const int result = reader.Fold(
          42, [](int acc, const ConsumableMessageWrapper<Score>& /*score*/) {
            return acc + 1;
          });

      CHECK_EQ(result, 42);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Find") {
    SUBCASE("Find returns optional with the first matching message") {
      const auto manager = MakeManager({{3}, {7}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto result =
          reader.Find([](const ConsumableMessageWrapper<Score>& score) {
            return score->value == 7;
          });

      REQUIRE(result.has_value());
      CHECK_EQ((*result)->value, 7);
    }

    SUBCASE("Find returns nullopt when no message matches") {
      const auto manager = MakeManager({{1}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto result =
          reader.Find([](const ConsumableMessageWrapper<Score>& score) {
            return score->value == 99;
          });

      CHECK_EQ(result, std::nullopt);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::CountIf") {
    SUBCASE("CountIf returns number of matching unread messages") {
      const auto manager = MakeManager({{1}, {6}}, {{8}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const size_t count =
          reader.CountIf([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 5;
          });

      CHECK_EQ(count, 2);
    }

    SUBCASE("CountIf returns zero when nothing matches") {
      const auto manager = MakeManager({{1}, {2}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const size_t count =
          reader.CountIf([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 100;
          });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Partition") {
    SUBCASE("Partition splits wrappers into matching and non-matching groups") {
      const auto manager = MakeManager({{1}, {7}}, {{3}, {9}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto [matched, not_matched] =
          reader.Partition([](const ConsumableMessageWrapper<Score>& score) {
            return score->value >= 5;
          });

      REQUIRE_EQ(matched.size(), 2);
      CHECK_EQ(matched[0]->value, 7);
      CHECK_EQ(matched[1]->value, 9);

      REQUIRE_EQ(not_matched.size(), 2);
      CHECK_EQ(not_matched[0]->value, 1);
      CHECK_EQ(not_matched[1]->value, 3);
    }

    SUBCASE("Partition on empty reader returns two empty vectors") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      const auto [matched, not_matched] = reader.Partition(
          [](const ConsumableMessageWrapper<Score>& /*score*/) {
            return true;
          });

      CHECK(matched.empty());
      CHECK(not_matched.empty());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::MaxBy") {
    SUBCASE("MaxBy returns wrapper with maximum key") {
      const auto manager = MakeManager({{2}, {11}}, {{5}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto max =
          reader.MaxBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      REQUIRE(max.has_value());
      CHECK_EQ((*max)->value, 11);
    }

    SUBCASE("MaxBy on empty reader returns std::nullopt") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      const auto max =
          reader.MaxBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      CHECK_EQ(max, std::nullopt);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::MinBy") {
    SUBCASE("MinBy returns wrapper with minimum key") {
      const auto manager = MakeManager({{4}, {1}}, {{7}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto min =
          reader.MinBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      REQUIRE(min.has_value());
      CHECK_EQ((*min)->value, 1);
    }

    SUBCASE("MinBy on empty reader returns std::nullopt") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      const auto min =
          reader.MinBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      CHECK_EQ(min, std::nullopt);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::GroupBy") {
    SUBCASE("GroupBy groups wrappers by extracted key") {
      const auto manager = MakeManager({{1}, {2}, {3}}, {{4}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const auto grouped =
          reader.GroupBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value % 2 == 0;
          });

      REQUIRE_EQ(grouped.size(), 2);
      REQUIRE_EQ(grouped.at(false).size(), 2);
      CHECK_EQ(grouped.at(false)[0]->value, 1);
      CHECK_EQ(grouped.at(false)[1]->value, 3);

      REQUIRE_EQ(grouped.at(true).size(), 2);
      CHECK_EQ(grouped.at(true)[0]->value, 2);
      CHECK_EQ(grouped.at(true)[1]->value, 4);
    }

    SUBCASE("GroupBy on empty reader returns empty map") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      const auto grouped =
          reader.GroupBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      CHECK(grouped.empty());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::ForEach") {
    SUBCASE("ForEach visits all messages in previous then current order") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      std::vector<int> visited;
      reader.ForEach([&visited](const ConsumableMessageWrapper<Score>& score) {
        visited.push_back(score->value);
      });

      REQUIRE_EQ(visited.size(), 3);
      CHECK_EQ(visited[0], 1);
      CHECK_EQ(visited[1], 2);
      CHECK_EQ(visited[2], 3);
    }

    SUBCASE("ForEach on empty reader does not invoke action") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      int call_count = 0;
      reader.ForEach(
          [&call_count](const ConsumableMessageWrapper<Score>& /*score*/) {
            ++call_count;
          });

      CHECK_EQ(call_count, 0);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Any") {
    SUBCASE("Returns true when at least one message matches") {
      const auto manager = MakeManager({{1}}, {{10}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool any =
          reader.Any([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 5;
          });

      CHECK_EQ(any, true);
    }

    SUBCASE("Returns false when no message matches") {
      const auto manager = MakeManager({{1}, {2}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool any =
          reader.Any([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 100;
          });

      CHECK_FALSE(any);
    }

    SUBCASE("Returns false on empty reader") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool any =
          reader.Any([](const ConsumableMessageWrapper<Score>& /*score*/) {
            return true;
          });

      CHECK_FALSE(any);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::All") {
    SUBCASE("Returns true when all messages match") {
      const auto manager = MakeManager({{5}, {6}}, {{7}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool all =
          reader.All([](const ConsumableMessageWrapper<Score>& score) {
            return score->value >= 5;
          });

      CHECK_EQ(all, true);
    }

    SUBCASE("Returns false when at least one message does not match") {
      const auto manager = MakeManager({{5}, {1}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool all =
          reader.All([](const ConsumableMessageWrapper<Score>& score) {
            return score->value >= 5;
          });

      CHECK_FALSE(all);
    }

    SUBCASE("Returns true vacuously for empty reader") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool all =
          reader.All([](const ConsumableMessageWrapper<Score>& /*score*/) {
            return false;
          });

      CHECK_EQ(all, true);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::None") {
    SUBCASE("Returns true when no message matches") {
      const auto manager = MakeManager({{1}, {2}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool none =
          reader.None([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 100;
          });

      CHECK_EQ(none, true);
    }

    SUBCASE("Returns false when at least one message matches") {
      const auto manager = MakeManager({{1}, {10}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool none =
          reader.None([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 5;
          });

      CHECK_FALSE(none);
    }

    SUBCASE("Returns true vacuously for empty reader") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      const bool none =
          reader.None([](const ConsumableMessageWrapper<Score>& /*score*/) {
            return true;
          });

      CHECK_EQ(none, true);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Empty") {
    SUBCASE("Returns true when no unread messages") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      CHECK(reader.Empty());
    }

    SUBCASE("Returns false when previous span has messages") {
      const auto manager = MakeManager({{}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_FALSE(reader.Empty());
    }

    SUBCASE("Returns false when current span has messages") {
      MessageManager manager;
      ConsumedRegistry registry;

      manager.Register<Score>();
      manager.Write(Score{});

      auto cursor = MessageCursor<Score>::IncludeBacklog();
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_FALSE(reader.Empty());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::Count") {
    SUBCASE("Returns zero for empty reader") {
      const auto manager = MakeEmptyScoreManager();
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      CHECK_EQ(reader.Count(), 0);
    }

    SUBCASE("Returns unread previous and current message counts") {
      const auto manager = MakeManager({{}, {}}, {{}, {}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_EQ(reader.Count(), 4);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageReader::begin / end") {
    SUBCASE("begin returns iterator at position 0") {
      const auto manager = MakeManager({{}}, {});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_EQ(reader.begin().Position(), 0);
    }

    SUBCASE("end returns iterator at position equal to Count") {
      const auto manager = MakeManager({{}, {}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      CHECK_EQ(reader.end().Position(), reader.Count());
    }

    SUBCASE("Range-for over reader visits all messages as wrappers") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);
      int sum = 0;
      for (const auto wrapper : reader) {
        sum += wrapper->value;
      }

      CHECK_EQ(sum, 6);
    }

    SUBCASE("Wrapper ids are consecutive across begin/end iteration") {
      const auto manager = MakeManager({{}, {}}, {{}});
      auto cursor = MessageCursor<Score>::IncludeBacklog();
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, cursor, registry);

      size_t expected_id = 0;
      for (const auto wrapper : reader) {
        CHECK_EQ(wrapper.Id().value, expected_id);
        ++expected_id;
      }
    }
  }
}

TEST_SUITE("helios::ecs::MessageReader") {
  TEST_CASE("helios::ecs::MessageReader::ctor") {
    SUBCASE("Construct from MessageManager and cursor") {
      const auto manager = MakePingManager({{}, {}}, {{}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();

      const MessageReader<Ping> reader(manager, cursor);

      CHECK_EQ(reader.Count(), 3);
    }

    SUBCASE("Move ctor") {
      const auto manager = MakePingManager({{}}, {{}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();

      MessageReader<Ping> original(manager, cursor);
      const MessageReader<Ping> moved(std::move(original));

      CHECK_EQ(moved.Count(), 2);
    }
  }

  TEST_CASE("helios::ecs::MessageReader::Read") {
    SUBCASE("Second Read yields nothing after cursor advanced") {
      const auto manager = MakePingManager({{1}}, {{2}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();

      const MessageReader<Ping> reader(manager, cursor);
      CHECK_EQ(reader.Collect().size(), 2);
      CHECK(reader.Empty());
      CHECK_EQ(reader.Collect().size(), 0);
    }

    SUBCASE(
        "Message written, read in current, Update moves to previous, same "
        "cursor does not re-read") {
      MessageManager manager;
      manager.Register<Ping>();
      manager.Write(Ping{42});

      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      CHECK_EQ(reader.Collect().size(), 1);
      manager.Update();
      CHECK(reader.Empty());
    }

    SUBCASE("New message after Clear is delivered") {
      auto manager = MakePingManager({{1}, {2}}, {});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      reader.Clear();
      CHECK(reader.Empty());
      manager.Write(Ping{9});
      CHECK_EQ(reader.Collect()[0].value, 9);
    }

    SUBCASE("FutureOnly cursor ignores backlog") {
      auto manager = MakePingManager({{1}, {2}}, {{3}});
      auto cursor = MessageCursor<Ping>::FutureOnly(manager);
      const MessageReader<Ping> reader(manager, cursor);

      CHECK(reader.Empty());
      manager.Write(Ping{99});
      CHECK_EQ(reader.Collect().size(), 1);
    }

    SUBCASE("MissedMessages when cursor lags behind OldestMessageCount") {
      MessageManager manager;
      manager.Register<Ping>();
      manager.Write(Ping{1});
      manager.Write(Ping{2});
      manager.Update();
      manager.Update();

      MessageCursor<Ping> cursor{.last_message_count = MessageId<Ping>{0}};
      const MessageReader<Ping> reader(manager, cursor);
      CHECK_EQ(reader.MissedMessages(), 2);
    }

    SUBCASE("Partial iteration leaves remaining for next begin") {
      const auto manager = MakePingManager({{1}, {2}, {3}}, {});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      reader.Take(1).ForEach([](const MessageWrapper<Ping>& /*wrapper*/) {});

      const auto rest = reader.Collect();
      REQUIRE_EQ(rest.size(), 2);
      CHECK_EQ(rest[0].value, 2);
      CHECK_EQ(rest[1].value, 3);
    }
  }

  TEST_CASE("helios::ecs::MessageReader::Filter") {
    SUBCASE("Filter yields only matching wrappers") {
      const auto manager = MakePingManager({{1}, {10}}, {{5}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      std::vector<int> values;
      reader
          .Filter([](const MessageWrapper<Ping>& wrapper) {
            return wrapper->value >= 5;
          })
          .ForEach([&values](const MessageWrapper<Ping>& wrapper) {
            values.push_back(wrapper->value);
          });

      CHECK_EQ(values.size(), 2);
    }
  }

  TEST_CASE("helios::ecs::MessageReader::Map") {
    SUBCASE("Map transforms each wrapper") {
      const auto manager = MakePingManager({{2}}, {{3}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      std::vector<int> results;
      reader
          .Map([](const MessageWrapper<Ping>& wrapper) {
            return wrapper->value * 10;
          })
          .ForEach([&results](int val) { results.push_back(val); });

      CHECK_EQ(results.size(), 2);
    }
  }

  TEST_CASE("helios::ecs::MessageReader::Take") {
    SUBCASE("Take limits number of yielded wrappers") {
      const auto manager = MakePingManager({{}, {}, {}}, {});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      int count = 0;
      reader.Take(2).ForEach(
          [&count](const MessageWrapper<Ping>& /*wrapper*/) { ++count; });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("helios::ecs::MessageReader::Count") {
    SUBCASE("Returns unread message count") {
      const auto manager = MakePingManager({{}, {}}, {{}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);
      CHECK_EQ(reader.Count(), 3);
    }
  }

  TEST_CASE("helios::ecs::MessageReader::begin / end") {
    SUBCASE("Range-for visits unread messages with stable ids") {
      const auto manager = MakePingManager({{10}}, {{20}});
      auto cursor = MessageCursor<Ping>::IncludeBacklog();
      const MessageReader<Ping> reader(manager, cursor);

      std::vector<int> values;
      size_t expected_id = 0;
      for (const auto wrapper : reader) {
        values.push_back(wrapper->value);
        CHECK_EQ(wrapper.Id().value, expected_id);
        ++expected_id;
      }

      REQUIRE_EQ(values.size(), 2);
      CHECK_EQ(values[0], 10);
      CHECK_EQ(values[1], 20);
    }
  }
}
