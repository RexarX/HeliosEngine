#include <doctest/doctest.h>

#include <compare>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/reader.hpp>

#include <array>
#include <string_view>
#include <utility>
#include <vector>

using namespace helios::ecs;
using ConsumedRegistry = helios::ecs::ConsumedMessagesRegistry<>;

namespace {

struct Score {
  static constexpr bool kConsumable = true;
  int value = 0;
};

struct Tagged {
  static constexpr bool kConsumable = true;
  static constexpr std::string_view kName = "Tagged";

  int tag = 0;
  bool active = false;
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

}  // namespace

TEST_SUITE("helios::ecs::ConsumableMessageWrapperIter") {
  TEST_CASE("ecs::ConsumableMessageWrapperIter::ctor") {
    SUBCASE("Copy ctor preserves position") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      const ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 1);
      const ConsumableMessageWrapperIter<Score> copy(iter);

      CHECK_EQ(copy.Position(), 1);
    }

    SUBCASE("Move ctor preserves position") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(1);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 0);
      const ConsumableMessageWrapperIter<Score> moved(std::move(iter));

      CHECK_EQ(moved.Position(), 0);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::assignment") {
    SUBCASE("Copy assignment preserves position") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(3);
      const ConsumableMessageWrapperIter<Score> original(msgs, {}, registry, 2);
      ConsumableMessageWrapperIter<Score> assigned(msgs, {}, registry, 0);
      assigned = original;

      CHECK_EQ(assigned.Position(), 2);
    }

    SUBCASE("Move assignment preserves position") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(1);
      ConsumableMessageWrapperIter<Score> original(msgs, {}, registry, 1);
      ConsumableMessageWrapperIter<Score> assigned(msgs, {}, registry, 0);
      assigned = std::move(original);

      CHECK_EQ(assigned.Position(), 1);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator*") {
    SUBCASE(
        "Dereference yields wrapper pointing to correct message in first "
        "span") {
      ConsumedRegistry registry;

      const std::vector<Score> first = {{10}, {20}};
      const ConsumableMessageWrapperIter<Score> iter(first, {}, registry, 0);
      CHECK_EQ((*iter)->value, 10);
    }

    SUBCASE("Dereference at second-span position yields correct message") {
      ConsumedRegistry registry;

      const std::vector<Score> first = {{1}};
      const std::vector<Score> second = {{99}};

      // position 1 = first past first span → second[0]
      const ConsumableMessageWrapperIter<Score> iter(first, second, registry,
                                                     1);
      CHECK_EQ((*iter)->value, 99);
    }

    SUBCASE("Dereference yields correct global index in wrapper") {
      ConsumedRegistry registry;

      const std::vector<Score> first(2);
      const ConsumableMessageWrapperIter<Score> iter(first, {}, registry, 1);
      CHECK_EQ((*iter).GlobalIndex(), 1);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator++ (pre)") {
    SUBCASE("Pre-increment advances position by one") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 0);
      ++iter;

      CHECK_EQ(iter.Position(), 1);
    }

    SUBCASE("Pre-increment returns reference to self") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 0);
      auto& returned = ++iter;

      CHECK_EQ(&returned, &iter);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator++ (post)") {
    SUBCASE("Post-increment returns copy at old position") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 0);
      const auto old = iter++;

      CHECK_EQ(old.Position(), 0);
      CHECK_EQ(iter.Position(), 1);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator-- (pre)") {
    SUBCASE("Pre-decrement retreats position by one") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 1);
      --iter;

      CHECK_EQ(iter.Position(), 0);
    }

    SUBCASE("Pre-decrement returns reference to self") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 1);
      auto& returned = --iter;

      CHECK_EQ(&returned, &iter);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator-- (post)") {
    SUBCASE("Post-decrement returns copy at old position") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(2);
      ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 2);
      const auto old = iter--;

      CHECK_EQ(old.Position(), 2);
      CHECK_EQ(iter.Position(), 1);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator==") {
    SUBCASE("Two iterators at the same position are equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 3);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 3);

      CHECK(lhs == rhs);
    }

    SUBCASE("Two iterators at different positions are not equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 0);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 1);

      CHECK_FALSE(lhs == rhs);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator!=") {
    SUBCASE("Iterators at different positions compare not-equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 0);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 2);

      CHECK(lhs != rhs);
    }

    SUBCASE("Iterators at the same position are not not-equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 5);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 5);

      CHECK_FALSE(lhs != rhs);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::operator<=>") {
    SUBCASE("Earlier position compares less than later position") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 0);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 2);

      CHECK_EQ(lhs <=> rhs, std::strong_ordering::less);
    }

    SUBCASE("Later position compares greater than earlier position") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 3);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 1);
      CHECK_EQ(lhs <=> rhs, std::strong_ordering::greater);
    }

    SUBCASE("Same positions compare equal") {
      ConsumedRegistry registry;

      const ConsumableMessageWrapperIter<Score> lhs({}, {}, registry, 4);
      const ConsumableMessageWrapperIter<Score> rhs({}, {}, registry, 4);

      CHECK_EQ(lhs <=> rhs, std::strong_ordering::equal);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::Position") {
    SUBCASE("Returns the position supplied at construction") {
      ConsumedRegistry registry;
      const ConsumableMessageWrapperIter<Score> iter({}, {}, registry, 7);
      CHECK_EQ(iter.Position(), 7);
    }
  }

  TEST_CASE("ecs::ConsumableMessageWrapperIter::begin / end") {
    SUBCASE("begin returns iterator at position 0") {
      ConsumedRegistry registry;

      const std::vector<Score> msgs(3);
      const ConsumableMessageWrapperIter<Score> iter(msgs, {}, registry, 2);

      CHECK_EQ(iter.begin().Position(), 0);
    }

    SUBCASE("end returns iterator at position equal to total message count") {
      ConsumedRegistry registry;

      const std::vector<Score> first(2);
      const std::vector<Score> second(1);
      const ConsumableMessageWrapperIter<Score> iter(first, second, registry,
                                                     0);

      CHECK_EQ(iter.end().Position(), 3);
    }

    SUBCASE("Range-for over the iterator visits all messages") {
      ConsumedRegistry registry;

      const std::vector<Score> first = {{10}, {20}};
      const std::vector<Score> second = {{30}};
      const ConsumableMessageWrapperIter<Score> iter(first, second, registry,
                                                     0);

      int sum = 0;
      for (const auto wrapper : iter) {
        sum += wrapper->value;
      }
      CHECK_EQ(sum, 60);
    }
  }
}

TEST_SUITE("helios::ecs::ConsumableMessageReader") {
  TEST_CASE("ecs::ConsumableMessageReader::ctor") {
    SUBCASE("Construct from explicit spans and registry") {
      ConsumedRegistry registry;

      const std::vector<Score> prev(1);
      const std::vector<Score> curr(1);
      const ConsumableMessageReader<Score> reader(prev, curr, registry);

      CHECK_EQ(reader.Count(), 2);
    }

    SUBCASE("Construct from MessageManager and registry") {
      const auto manager = MakeManager({{}, {}}, {{}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK_EQ(reader.Count(), 3);
    }

    SUBCASE("Move ctor") {
      const auto manager = MakeManager({{}}, {{}});
      ConsumedRegistry registry;

      ConsumableMessageReader<Score> original(manager, registry);
      const ConsumableMessageReader<Score> moved(std::move(original));

      CHECK_EQ(moved.Count(), 2);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::operator=") {
    SUBCASE("Move assignment") {
      const auto manager = MakeManager({{}}, {{}});
      ConsumedRegistry registry;

      ConsumableMessageReader<Score> original(manager, registry);

      ConsumedRegistry registry2;
      ConsumableMessageReader<Score> assigned({}, {}, registry2);
      assigned = std::move(original);

      CHECK_EQ(assigned.Count(), 2);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::ConsumeAll") {
    SUBCASE("Marks every message as consumed") {
      const auto manager = MakeManager({{}, {}}, {{}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      reader.ConsumeAll();

      CHECK_EQ(registry.ConsumedCount<Score>(), 3);
    }

    SUBCASE("ConsumeAll on empty reader does nothing") {
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader({}, {}, registry);
      reader.ConsumeAll();

      CHECK(registry.Empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::ConsumeIf") {
    SUBCASE("Marks matching messages as consumed and returns count") {
      const auto manager = MakeManager({{1}, {5}, {3}}, {{7}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto count =
          reader.ConsumeIf([](const Score& score) { return score.value > 4; });

      CHECK_EQ(count, 2);                    // {5} and {7} match
      CHECK(registry.IsConsumed<Score>(1));  // global index 1 → prev[1]={5}
      CHECK(registry.IsConsumed<Score>(3));  // global index 3 → curr[0]={7}
    }

    SUBCASE("Returns zero when no messages match the predicate") {
      const auto manager = MakeManager({{1}, {2}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto count = reader.ConsumeIf(
          [](const Score& score) { return score.value > 100; });

      CHECK_EQ(count, 0);
      CHECK(registry.Empty());
    }

    SUBCASE("Returns total count when all messages match") {
      const auto manager = MakeManager({{}}, {{}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto count =
          reader.ConsumeIf([](const Score& /*score*/) { return true; });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Collect") {
    SUBCASE(
        "Collect returns all messages from previous and current as a vector") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto collected = reader.Collect();

      REQUIRE_EQ(collected.size(), 3);
      CHECK_EQ(collected[0].value, 1);
      CHECK_EQ(collected[1].value, 2);
      CHECK_EQ(collected[2].value, 3);
    }

    SUBCASE("Collect on empty reader returns empty vector") {
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);
      CHECK(reader.Collect().empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::CollectWith") {
    SUBCASE("CollectWith returns all messages using custom allocator") {
      const auto manager = MakeManager({{10}, {20}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto collected = reader.CollectWith(std::allocator<Score>{});

      REQUIRE_EQ(collected.size(), 2);
      CHECK_EQ(collected[0].value, 10);
      CHECK_EQ(collected[1].value, 20);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::ReadInto") {
    SUBCASE("ReadInto copies all messages into output iterator") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      std::vector<Score> out;
      reader.ReadInto(std::back_inserter(out));

      REQUIRE_EQ(out.size(), 3);
      CHECK_EQ(out[0].value, 1);
      CHECK_EQ(out[2].value, 3);
    }

    SUBCASE("ReadInto into array") {
      const auto manager = MakeManager({{5}, {6}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      std::vector<Score> out(2);
      reader.ReadInto(out.begin());

      CHECK_EQ(out[0].value, 5);
      CHECK_EQ(out[1].value, 6);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Filter") {
    SUBCASE("Filter yields only matching wrappers") {
      const auto manager = MakeManager({{1}, {10}}, {{5}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::Map") {
    SUBCASE("Map transforms each wrapper") {
      const auto manager = MakeManager({{2}}, {{3}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::Take") {
    SUBCASE("Take limits number of yielded wrappers") {
      const auto manager = MakeManager({{}, {}, {}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      int count = 0;
      reader.Take(2).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 2);
    }

    SUBCASE("Take(0) yields nothing") {
      const auto manager = MakeManager({{}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      int count = 0;
      reader.Take(0).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Skip") {
    SUBCASE("Skip skips the first N wrappers") {
      const auto manager = MakeManager({{}, {}, {}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      int count = 0;
      reader.Skip(2).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 1);
    }

    SUBCASE("Skip more than available yields nothing") {
      const auto manager = MakeManager({{}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      int count = 0;
      reader.Skip(100).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::TakeWhile") {
    SUBCASE("Stops yielding once predicate becomes false") {
      const auto manager = MakeManager({{1}, {2}, {10}, {3}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::SkipWhile") {
    SUBCASE("Skips elements while predicate holds, then yields the rest") {
      const auto manager = MakeManager({{1}, {2}, {10}, {3}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::Enumerate") {
    SUBCASE("Enumerate pairs each wrapper with its zero-based index") {
      const auto manager = MakeManager({{}, {}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::Inspect") {
    SUBCASE("Inspect calls side-effect without consuming the sequence") {
      const auto manager = MakeManager({{}, {}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::StepBy") {
    SUBCASE("StepBy(1) yields all messages") {
      const auto manager = MakeManager({{}, {}, {}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      int count = 0;
      reader.StepBy(1).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 3);
    }

    SUBCASE("StepBy(2) yields every other message") {
      const auto manager = MakeManager({{}, {}, {}, {}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      int count = 0;
      reader.StepBy(2).ForEach(
          [&count](const ConsumableMessageWrapper<Score>& /*wrapper*/) {
            ++count;
          });

      CHECK_EQ(count, 2);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Reverse") {
    SUBCASE("Reverse yields wrappers in reverse order") {
      const auto manager = MakeManager({{1}, {2}, {3}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

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

  TEST_CASE("ecs::ConsumableMessageReader::Slide") {
    SUBCASE("Slide yields windows of `window_size` wrappers") {
      const auto manager = MakeManager({{1}, {2}, {3}, {4}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
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

  TEST_CASE("ecs::ConsumableMessageReader::Stride") {
    SUBCASE("Stride yields every nth element") {
      const auto manager = MakeManager({{1}, {2}, {3}, {4}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto stride = reader.Stride(2);

      CHECK_EQ((*stride)->value, 1);
      CHECK_EQ((*std::next(stride))->value, 3);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Zip") {
    const auto manager = MakeManager({{1}, {2}, {3}, {4}}, {});
    ConsumedRegistry registry;
    constexpr std::array expected = {0, 1, 2, 3};

    SUBCASE("Iterator oveload") {
      const ConsumableMessageReader<Score> reader(manager, registry);
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

    SUBCASE("Range oveload") {
      const ConsumableMessageReader<Score> reader(manager, registry);
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

  TEST_CASE("ecs::ConsumableMessageReader::Fold") {
    SUBCASE("Fold accumulates all messages from previous and current") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const int sum = reader.Fold(
          0, [](int acc, const ConsumableMessageWrapper<Score>& score) {
            return acc + score->value;
          });

      CHECK_EQ(sum, 6);
    }

    SUBCASE("Fold with empty reader returns initial value") {
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader({}, {}, registry);
      const int result = reader.Fold(
          42, [](int acc, const ConsumableMessageWrapper<Score>& /*score*/) {
            return acc + 1;
          });

      CHECK_EQ(result, 42);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Find") {
    SUBCASE("Find returns pointer to the first matching message") {
      const auto manager = MakeManager({{3}, {7}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto result =
          reader.Find([](const ConsumableMessageWrapper<Score>& score) {
            return score->value == 7;
          });

      REQUIRE(result.has_value());
      CHECK_EQ((*result)->value, 7);
    }

    SUBCASE("Find returns nullptr when no message matches") {
      const auto manager = MakeManager({{1}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto result =
          reader.Find([](const ConsumableMessageWrapper<Score>& score) {
            return score->value == 99;
          });

      CHECK_EQ(result, std::nullopt);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::CountIf") {
    SUBCASE("CountIf returns number of matching messages across both spans") {
      const auto manager = MakeManager({{1}, {6}}, {{8}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const size_t count =
          reader.CountIf([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 5;
          });

      CHECK_EQ(count, 2);
    }

    SUBCASE("CountIf returns zero when nothing matches") {
      const auto manager = MakeManager({{1}, {2}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const size_t count =
          reader.CountIf([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 100;
          });

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Partition") {
    SUBCASE("Partition splits wrappers into matching and non-matching groups") {
      const auto manager = MakeManager({{1}, {7}}, {{3}, {9}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
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
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);

      const auto [matched, not_matched] = reader.Partition(
          [](const ConsumableMessageWrapper<Score>& /*score*/) {
            return true;
          });

      CHECK(matched.empty());
      CHECK(not_matched.empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::MaxBy") {
    SUBCASE("MaxBy returns wrapper with maximum key") {
      const auto manager = MakeManager({{2}, {11}}, {{5}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto max =
          reader.MaxBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      REQUIRE(max.has_value());
      CHECK_EQ((*max)->value, 11);
    }

    SUBCASE("MaxBy on empty reader returns std::nullopt") {
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);

      const auto max =
          reader.MaxBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      CHECK_EQ(max, std::nullopt);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::MinBy") {
    SUBCASE("MinBy returns wrapper with minimum key") {
      const auto manager = MakeManager({{4}, {1}}, {{7}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto min =
          reader.MinBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      REQUIRE(min.has_value());
      CHECK_EQ((*min)->value, 1);
    }

    SUBCASE("MinBy on empty reader returns std::nullopt") {
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);

      const auto min =
          reader.MinBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      CHECK_EQ(min, std::nullopt);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::GroupBy") {
    SUBCASE("GroupBy groups wrappers by extracted key") {
      const auto manager = MakeManager({{1}, {2}, {3}}, {{4}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
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
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);

      const auto grouped =
          reader.GroupBy([](const ConsumableMessageWrapper<Score>& score) {
            return score->value;
          });

      CHECK(grouped.empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::ForEach") {
    SUBCASE("ForEach visits all messages in previous then current order") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
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
      ConsumedRegistry registry;
      const std::vector<Score> empty;

      const ConsumableMessageReader<Score> reader({}, {}, registry);

      int call_count = 0;
      reader.ForEach(
          [&call_count](const ConsumableMessageWrapper<Score>& /*score*/) {
            ++call_count;
          });

      CHECK_EQ(call_count, 0);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Any") {
    SUBCASE("Returns true when at least one message matches") {
      const auto manager = MakeManager({{1}}, {{10}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const bool any =
          reader.Any([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 5;
          });

      CHECK(any);
    }

    SUBCASE("Returns false when no message matches") {
      const auto manager = MakeManager({{1}, {2}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const bool any =
          reader.Any([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 100;
          });

      CHECK_FALSE(any);
    }

    SUBCASE("Returns false on empty reader") {
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader({}, {}, registry);
      const bool any =
          reader.Any([](const ConsumableMessageWrapper<Score>& /*score*/) {
            return true;
          });

      CHECK_FALSE(any);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::All") {
    SUBCASE("Returns true when all messages match") {
      const auto manager = MakeManager({{5}, {6}}, {{7}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const bool all =
          reader.All([](const ConsumableMessageWrapper<Score>& score) {
            return score->value >= 5;
          });

      CHECK(all);
    }

    SUBCASE("Returns false when at least one message does not match") {
      const auto manager = MakeManager({{5}, {1}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const bool all =
          reader.All([](const ConsumableMessageWrapper<Score>& score) {
            return score->value >= 5;
          });

      CHECK_FALSE(all);
    }

    SUBCASE("Returns true vacuously for empty reader") {
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader({}, {}, registry);
      const bool all =
          reader.All([](const ConsumableMessageWrapper<Score>& /*score*/) {
            return false;
          });

      CHECK(all);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::None") {
    SUBCASE("Returns true when no message matches") {
      const auto manager = MakeManager({{1}, {2}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const bool none =
          reader.None([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 100;
          });

      CHECK(none);
    }

    SUBCASE("Returns false when at least one message matches") {
      const auto manager = MakeManager({{1}, {10}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const bool none =
          reader.None([](const ConsumableMessageWrapper<Score>& score) {
            return score->value > 5;
          });

      CHECK_FALSE(none);
    }

    SUBCASE("Returns true vacuously for empty reader") {
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader({}, {}, registry);
      const bool none =
          reader.None([](const ConsumableMessageWrapper<Score>& /*score*/) {
            return true;
          });

      CHECK(none);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Empty") {
    SUBCASE("Returns true when both spans are empty") {
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);
      CHECK(reader.Empty());
    }

    SUBCASE("Returns false when previous span has messages") {
      const auto manager = MakeManager({{}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK_FALSE(reader.Empty());
    }

    SUBCASE("Returns false when current span has messages") {
      MessageManager manager;
      ConsumedRegistry registry;

      manager.Register<Score>();
      manager.Write(Score{});

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK_FALSE(reader.Empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::Count") {
    SUBCASE("Returns zero for empty reader") {
      ConsumedRegistry registry;
      const ConsumableMessageReader<Score> reader({}, {}, registry);
      CHECK_EQ(reader.Count(), 0);
    }

    SUBCASE("Returns sum of previous and current message counts") {
      const auto manager = MakeManager({{}, {}}, {{}, {}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK_EQ(reader.Count(), 4);
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::PreviousMessages") {
    SUBCASE("Returns span of previous-frame messages") {
      const auto manager = MakeManager({{10}, {20}}, {{30}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto prev = reader.PreviousMessages();

      REQUIRE_EQ(prev.size(), 2);
      CHECK_EQ(prev[0].value, 10);
      CHECK_EQ(prev[1].value, 20);
    }

    SUBCASE("Returns empty span when no previous messages") {
      MessageManager manager;
      ConsumedRegistry registry;

      manager.Register<Score>();
      manager.Write(Score{});

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK(reader.PreviousMessages().empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::CurrentMessages") {
    SUBCASE("Returns span of current-frame messages") {
      const auto manager = MakeManager({}, {{5}, {6}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      const auto curr = reader.CurrentMessages();

      REQUIRE_EQ(curr.size(), 2);
      CHECK_EQ(curr[0].value, 5);
      CHECK_EQ(curr[1].value, 6);
    }

    SUBCASE("Returns empty span when no current messages") {
      const auto manager = MakeManager({{1}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK(reader.CurrentMessages().empty());
    }
  }

  TEST_CASE("ecs::ConsumableMessageReader::begin / end") {
    SUBCASE("begin returns iterator at position 0") {
      const auto manager = MakeManager({{}}, {});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK_EQ(reader.begin().Position(), 0);
    }

    SUBCASE("end returns iterator at position equal to Count") {
      const auto manager = MakeManager({{}, {}}, {{}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      CHECK_EQ(reader.end().Position(), reader.Count());
    }

    SUBCASE("Range-for over reader visits all messages as wrappers") {
      const auto manager = MakeManager({{1}, {2}}, {{3}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);
      int sum = 0;
      for (const auto wrapper : reader) {
        sum += wrapper->value;
      }

      CHECK_EQ(sum, 6);
    }

    SUBCASE(
        "Wrapper global indices are consecutive across begin/end iteration") {
      const auto manager = MakeManager({{}, {}}, {{}});
      ConsumedRegistry registry;

      const ConsumableMessageReader<Score> reader(manager, registry);

      size_t expected_index = 0;
      for (const auto wrapper : reader) {
        CHECK_EQ(wrapper.GlobalIndex(), expected_index);
        ++expected_index;
      }
    }
  }
}
