#include <doctest/doctest.h>

#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/wrapper.hpp>

#include <memory_resource>
#include <string_view>

using namespace helios::ecs;

namespace {

struct ConsumedRegistry {
  std::pmr::monotonic_buffer_resource resource;
  ConsumedMessagesRegistry registry;

  ConsumedRegistry() : registry(&resource) {}

  operator ConsumedMessagesRegistry&() noexcept { return registry; }
};

struct Position {
  static constexpr bool kConsumable = true;

  float x = 0.0F;
  float y = 0.0F;
};

struct NamedMessage {
  static constexpr std::string_view kName = "NamedMessage";
  static constexpr bool kConsumable = true;

  int value = 0;
};

struct SimpleMessage {
  int id = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::MessageWrapper") {
  TEST_CASE("helios::ecs::MessageWrapper::ctor") {
    SUBCASE("Construct from message and message id") {
      const SimpleMessage msg{42};

      const MessageWrapper<SimpleMessage> wrapper(msg,
                                                  MessageId<SimpleMessage>{0});

      CHECK_EQ(wrapper->id, 42);
      CHECK_EQ(wrapper.Id().value, 0);
    }

    SUBCASE("Copy ctor") {
      const SimpleMessage msg{99};
      const MessageWrapper<SimpleMessage> original(msg,
                                                   MessageId<SimpleMessage>{5});

      const MessageWrapper<SimpleMessage> copy(original);

      CHECK_EQ(copy->id, 99);
      CHECK_EQ(copy.Id().value, 5);
    }

    SUBCASE("Move ctor") {
      const SimpleMessage msg{77};
      MessageWrapper<SimpleMessage> original(msg, MessageId<SimpleMessage>{10});

      const MessageWrapper<SimpleMessage> moved(std::move(original));

      CHECK_EQ(moved->id, 77);
      CHECK_EQ(moved.Id().value, 10);
    }
  }

  TEST_CASE("helios::ecs::MessageWrapper::operators") {
    const SimpleMessage msg{42};
    const MessageWrapper<SimpleMessage> wrapper(msg,
                                                MessageId<SimpleMessage>{3});

    SUBCASE("operator*") {
      const SimpleMessage& unwrapped = *wrapper;
      CHECK_EQ(unwrapped.id, 42);
    }

    SUBCASE("operator->") {
      CHECK_EQ(wrapper->id, 42);
    }
  }

  TEST_CASE("helios::ecs::MessageWrapper::methods") {
    const SimpleMessage msg{55};
    const MessageWrapper<SimpleMessage> wrapper(msg,
                                                MessageId<SimpleMessage>{7});

    SUBCASE("Name") {
      std::string_view name = wrapper.Name();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("TypeIndex") {
      auto type_idx = wrapper.TypeIndex();
      CHECK_EQ(type_idx, MessageTypeIndex::From<SimpleMessage>());
    }

    SUBCASE("Id") {
      CHECK_EQ(wrapper.Id().value, 7);
    }
  }
}

TEST_SUITE("helios::ecs::ConsumableMessageWrapper") {
  TEST_CASE("helios::ecs::ConsumableMessageWrapper::ctor") {
    SUBCASE("Construct from message, registry and message id") {
      ConsumedRegistry registry;
      const Position msg{1.0F, 2.0F};

      const ConsumableMessageWrapper<Position> wrapper(msg, registry,
                                                       MessageId<Position>{0});

      CHECK_EQ(wrapper->x, 1.0F);
      CHECK_EQ(wrapper->y, 2.0F);
      CHECK_EQ(wrapper.Id().value, 0);
    }

    SUBCASE("Copy ctor") {
      ConsumedRegistry registry;
      const Position msg{3.0F, 4.0F};
      const ConsumableMessageWrapper<Position> original(msg, registry,
                                                        MessageId<Position>{5});

      const ConsumableMessageWrapper<Position> copy(original);

      CHECK_EQ(copy->x, 3.0F);
      CHECK_EQ(copy->y, 4.0F);
      CHECK_EQ(copy.Id().value, 5);
    }

    SUBCASE("Move ctor") {
      ConsumedRegistry registry;
      const Position msg{5.0F, 6.0F};
      ConsumableMessageWrapper<Position> original(msg, registry,
                                                  MessageId<Position>{7});

      const ConsumableMessageWrapper<Position> moved(std::move(original));

      CHECK_EQ(moved->x, 5.0F);
      CHECK_EQ(moved->y, 6.0F);
      CHECK_EQ(moved.Id().value, 7);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapper::operators") {
    ConsumedRegistry registry;
    const Position msg{10.0F, 20.0F};
    const ConsumableMessageWrapper<Position> wrapper(msg, registry,
                                                     MessageId<Position>{2});

    SUBCASE("operator*") {
      const Position& unwrapped = *wrapper;
      CHECK_EQ(unwrapped.x, 10.0F);
      CHECK_EQ(unwrapped.y, 20.0F);
    }

    SUBCASE("operator->") {
      CHECK_EQ(wrapper->x, 10.0F);
      CHECK_EQ(wrapper->y, 20.0F);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapper::Consume") {
    SUBCASE("Consume marks message as consumed") {
      ConsumedRegistry registry;
      const Position msg{};

      const ConsumableMessageWrapper<Position> wrapper(msg, registry,
                                                       MessageId<Position>{0});

      wrapper.Consume();

      CHECK(wrapper.IsConsumed());
    }

    SUBCASE("Multiple Consume calls are idempotent") {
      ConsumedRegistry registry;
      const Position msg{};
      const ConsumableMessageWrapper<Position> wrapper(msg, registry,
                                                       MessageId<Position>{1});

      wrapper.Consume();
      wrapper.Consume();
      wrapper.Consume();

      CHECK(wrapper.IsConsumed());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapper::IsConsumed") {
    SUBCASE("Initially not consumed") {
      ConsumedRegistry registry;
      const Position msg{};
      const ConsumableMessageWrapper<Position> wrapper(msg, registry,
                                                       MessageId<Position>{0});

      CHECK_FALSE(wrapper.IsConsumed());
    }

    SUBCASE("Returns true after Consume") {
      ConsumedRegistry registry;
      const Position msg{};
      const ConsumableMessageWrapper<Position> wrapper(msg, registry,
                                                       MessageId<Position>{2});

      wrapper.Consume();

      CHECK(wrapper.IsConsumed());
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapper::methods") {
    ConsumedRegistry registry;
    const NamedMessage msg{42};
    const ConsumableMessageWrapper<NamedMessage> wrapper(
        msg, registry, MessageId<NamedMessage>{3});

    SUBCASE("Name") {
      CHECK_EQ(wrapper.Name(), "NamedMessage");
    }

    SUBCASE("TypeIndex") {
      auto type_idx = wrapper.TypeIndex();
      CHECK_EQ(type_idx, MessageTypeIndex::From<NamedMessage>());
    }

    SUBCASE("Id") {
      CHECK_EQ(wrapper.Id().value, 3);
    }
  }

  TEST_CASE("helios::ecs::ConsumableMessageWrapper::assignment") {
    ConsumedRegistry registry;
    const Position msg1{1.0F, 2.0F};
    const Position msg2{3.0F, 4.0F};

    ConsumableMessageWrapper<Position> wrapper1(msg1, registry,
                                                MessageId<Position>{0});
    const ConsumableMessageWrapper<Position> wrapper2(msg2, registry,
                                                      MessageId<Position>{1});

    SUBCASE("Copy assignment") {
      wrapper1 = wrapper2;

      CHECK_EQ(wrapper1->x, 3.0F);
      CHECK_EQ(wrapper1->y, 4.0F);
      CHECK_EQ(wrapper1.Id().value, 1);
    }

    SUBCASE("Move assignment") {
      ConsumableMessageWrapper<Position> temp(msg2, registry,
                                              MessageId<Position>{5});
      wrapper1 = std::move(temp);

      CHECK_EQ(wrapper1->x, 3.0F);
      CHECK_EQ(wrapper1->y, 4.0F);
      CHECK_EQ(wrapper1.Id().value, 5);
    }
  }
}
