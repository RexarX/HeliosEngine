#include <doctest/doctest.h>

#include <helios/ecs/message/message.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using namespace helios::ecs;

namespace {

struct SimpleMessage {
  int value = 0;
};

struct NamedMessage {
  static constexpr std::string_view kName = "NamedMessage";

  float data = 0.0F;
};

struct ManualClearMessage {
  static constexpr auto kClearPolicy = MessageClearPolicy::kManual;

  int counter = 0;
};

struct AsyncMessage {
  static constexpr bool kAsync = true;

  double value = 0.0;
};

struct NamedAsyncMessage {
  static constexpr std::string_view kName = "NamedAsyncMessage";
  static constexpr bool kAsync = true;

  int data = 0;
};

struct CompleteMessage {
  static constexpr std::string_view kName = "CompleteMessage";
  static constexpr auto kClearPolicy = MessageClearPolicy::kManual;
  static constexpr bool kAsync = false;

  std::string text;
};

struct ConsumableMessage {
  static constexpr bool kConsumable = true;

  int value = 0;
};

struct NonConsumableMessage {
  static constexpr bool kConsumable = false;

  int value = 0;
};

struct ConsumableNamedMessage {
  static constexpr std::string_view kName = "ConsumableNamedMessage";
  static constexpr bool kConsumable = true;

  float data = 0.0F;
};

struct LargeMessage {
  double data[100] = {};
};

struct AlignedMessage {
  alignas(16) float data[4] = {};
};

// Non-message types for negative testing
class PolymorphicMessage {
public:
  virtual ~PolymorphicMessage() = default;
};

struct NonCopyableNonMovableMessage {
  NonCopyableNonMovableMessage() = default;
  NonCopyableNonMovableMessage(const NonCopyableNonMovableMessage&) = delete;
  NonCopyableNonMovableMessage(NonCopyableNonMovableMessage&&) = delete;

  NonCopyableNonMovableMessage& operator=(const NonCopyableNonMovableMessage&) =
      delete;
  NonCopyableNonMovableMessage& operator=(NonCopyableNonMovableMessage&&) =
      delete;
};

}  // namespace

TEST_SUITE("helios::ecs::MessageTrait") {
  TEST_CASE("ecs::MessageTrait::concept") {
    SUBCASE("Regular message types satisfy MessageTrait") {
      CHECK(MessageTrait<SimpleMessage>);
      CHECK(MessageTrait<NamedMessage>);
      CHECK(MessageTrait<ManualClearMessage>);
      CHECK(MessageTrait<CompleteMessage>);
      CHECK(MessageTrait<LargeMessage>);
      CHECK(MessageTrait<AlignedMessage>);
    }

    SUBCASE("Fundamental types satisfy MessageTrait") {
      CHECK(MessageTrait<int>);
      CHECK(MessageTrait<float>);
      CHECK(MessageTrait<double>);
      CHECK(MessageTrait<std::string>);
    }

    SUBCASE("Polymorphic types do not satisfy MessageTrait") {
      CHECK_FALSE(MessageTrait<PolymorphicMessage>);
    }

    SUBCASE("Non-copyable and non-movable types do not satisfy MessageTrait") {
      CHECK_FALSE(MessageTrait<NonCopyableNonMovableMessage>);
    }
  }
}

TEST_SUITE("helios::ecs::AsyncMessageTrait") {
  TEST_CASE("ecs::AsyncMessageTrait::concept") {
    SUBCASE("Types with kAsync = true satisfy AsyncMessageTrait") {
      CHECK(AsyncMessageTrait<AsyncMessage>);
      CHECK(AsyncMessageTrait<NamedAsyncMessage>);
    }

    SUBCASE("Types without kAsync do not satisfy AsyncMessageTrait") {
      CHECK_FALSE(AsyncMessageTrait<SimpleMessage>);
      CHECK_FALSE(AsyncMessageTrait<NamedMessage>);
      CHECK_FALSE(AsyncMessageTrait<ManualClearMessage>);
    }

    SUBCASE("Types with kAsync = false do not satisfy AsyncMessageTrait") {
      CHECK_FALSE(AsyncMessageTrait<CompleteMessage>);
    }

    SUBCASE("Async messages must be default initializable") {
      CHECK(std::default_initializable<AsyncMessage>);
      CHECK(std::default_initializable<NamedAsyncMessage>);
    }
  }
}

TEST_SUITE("helios::ecs::AnyMessageTrait") {
  TEST_CASE("ecs::AnyMessageTrait::concept") {
    SUBCASE("Regular messages satisfy AnyMessageTrait") {
      CHECK(AnyMessageTrait<SimpleMessage>);
      CHECK(AnyMessageTrait<NamedMessage>);
    }

    SUBCASE("Async messages satisfy AnyMessageTrait") {
      CHECK(AnyMessageTrait<AsyncMessage>);
      CHECK(AnyMessageTrait<NamedAsyncMessage>);
    }

    SUBCASE("Non-message types do not satisfy AnyMessageTrait") {
      CHECK_FALSE(AnyMessageTrait<PolymorphicMessage>);
      CHECK_FALSE(AnyMessageTrait<NonCopyableNonMovableMessage>);
    }
  }
}

TEST_SUITE("helios::ecs::MessageWithNameTrait") {
  TEST_CASE("ecs::MessageWithNameTrait::concept") {
    SUBCASE("Messages with kName satisfy MessageWithNameTrait") {
      CHECK(MessageWithNameTrait<NamedMessage>);
      CHECK(MessageWithNameTrait<NamedAsyncMessage>);
      CHECK(MessageWithNameTrait<CompleteMessage>);
    }

    SUBCASE("Messages without kName do not satisfy MessageWithNameTrait") {
      CHECK_FALSE(MessageWithNameTrait<SimpleMessage>);
      CHECK_FALSE(MessageWithNameTrait<AsyncMessage>);
      CHECK_FALSE(MessageWithNameTrait<ManualClearMessage>);
    }
  }
}

TEST_SUITE("helios::ecs::MessageWithClearPolicy") {
  TEST_CASE("ecs::MessageWithClearPolicy::concept") {
    SUBCASE("Messages with kClearPolicy satisfy MessageWithClearPolicy") {
      CHECK(MessageWithClearPolicy<ManualClearMessage>);
      CHECK(MessageWithClearPolicy<CompleteMessage>);
    }

    SUBCASE(
        "Messages without kClearPolicy do not satisfy MessageWithClearPolicy") {
      CHECK_FALSE(MessageWithClearPolicy<SimpleMessage>);
      CHECK_FALSE(MessageWithClearPolicy<NamedMessage>);
      CHECK_FALSE(MessageWithClearPolicy<AsyncMessage>);
    }
  }
}

TEST_SUITE("helios::ecs::MessageNameOf") {
  TEST_CASE("ecs::MessageNameOf::basic") {
    SUBCASE("Message with custom name returns custom name") {
      constexpr auto name = MessageNameOf<NamedMessage>();
      CHECK_EQ(name, "NamedMessage");
    }

    SUBCASE("Message with custom name (NamedAsyncMessage)") {
      constexpr auto name = MessageNameOf<NamedAsyncMessage>();
      CHECK_EQ(name, "NamedAsyncMessage");
    }

    SUBCASE("Message with custom name (CompleteMessage)") {
      constexpr auto name = MessageNameOf<CompleteMessage>();
      CHECK_EQ(name, "CompleteMessage");
    }

    SUBCASE("Message without custom name returns type name") {
      constexpr auto name = MessageNameOf<SimpleMessage>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Message name with instance parameter") {
      NamedMessage msg{};

      const auto name_from_type = MessageNameOf<NamedMessage>();
      const auto name_from_instance = MessageNameOf(msg);

      CHECK_EQ(name_from_type, name_from_instance);
    }

    SUBCASE("Async message without custom name returns type name") {
      constexpr auto name = MessageNameOf<AsyncMessage>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Message with manual clear policy without custom name") {
      constexpr auto name = MessageNameOf<ManualClearMessage>();
      CHECK_FALSE(name.empty());
    }
  }
}

TEST_SUITE("helios::ecs::MessageClearPolicyOf") {
  TEST_CASE("ecs::MessageClearPolicyOf::basic") {
    SUBCASE("Message with custom clear policy returns custom policy") {
      constexpr auto policy = MessageClearPolicyOf<ManualClearMessage>();
      CHECK_EQ(policy, MessageClearPolicy::kManual);
    }

    SUBCASE("Message with custom clear policy (CompleteMessage)") {
      constexpr auto policy = MessageClearPolicyOf<CompleteMessage>();
      CHECK_EQ(policy, MessageClearPolicy::kManual);
    }

    SUBCASE("Message without custom clear policy defaults to automatic") {
      constexpr auto policy = MessageClearPolicyOf<SimpleMessage>();
      CHECK_EQ(policy, MessageClearPolicy::kAutomatic);
    }

    SUBCASE("Named message defaults to automatic") {
      constexpr auto policy = MessageClearPolicyOf<NamedMessage>();
      CHECK_EQ(policy, MessageClearPolicy::kAutomatic);
    }

    SUBCASE("Async message defaults to automatic") {
      constexpr auto policy = MessageClearPolicyOf<AsyncMessage>();
      CHECK_EQ(policy, MessageClearPolicy::kAutomatic);
    }

    SUBCASE("Clear policy with instance parameter") {
      ManualClearMessage msg{};

      const auto policy_from_type = MessageClearPolicyOf<ManualClearMessage>();
      const auto policy_from_instance = MessageClearPolicyOf(msg);

      CHECK_EQ(policy_from_type, policy_from_instance);
    }
  }
}

TEST_SUITE("helios::ecs::Message: edge cases") {
  TEST_CASE("ecs::Message::edge_cases") {
    SUBCASE("Message with all traits specified") {
      CHECK(MessageTrait<CompleteMessage>);
      CHECK(MessageWithNameTrait<CompleteMessage>);
      CHECK(MessageWithClearPolicy<CompleteMessage>);
      CHECK_EQ(MessageNameOf<CompleteMessage>(), "CompleteMessage");
      CHECK_EQ(MessageClearPolicyOf<CompleteMessage>(),
               MessageClearPolicy::kManual);
    }

    SUBCASE("Async message with name") {
      CHECK(AsyncMessageTrait<NamedAsyncMessage>);
      CHECK_EQ(MessageNameOf<NamedAsyncMessage>(), "NamedAsyncMessage");
    }

    SUBCASE("Large message satisfies MessageTrait") {
      CHECK(MessageTrait<LargeMessage>);
      CHECK(AnyMessageTrait<LargeMessage>);
    }

    SUBCASE("Aligned message satisfies MessageTrait") {
      CHECK(MessageTrait<AlignedMessage>);
      CHECK(AnyMessageTrait<AlignedMessage>);
    }

    SUBCASE("Multiple messages have unique type indices") {
      const auto indices = {
          MessageTypeIndex::From<SimpleMessage>(),
          MessageTypeIndex::From<NamedMessage>(),
          MessageTypeIndex::From<ManualClearMessage>(),
          MessageTypeIndex::From<AsyncMessage>(),
          MessageTypeIndex::From<NamedAsyncMessage>(),
          MessageTypeIndex::From<CompleteMessage>(),
      };

      // Check all indices are unique
      std::vector<MessageTypeIndex> index_vec(indices);
      std::ranges::sort(index_vec);
      const auto last = std::ranges::unique(index_vec);

      CHECK_EQ(last.begin(), index_vec.end());
    }

    SUBCASE("Clear policy enum values") {
      CHECK_NE(MessageClearPolicy::kAutomatic, MessageClearPolicy::kManual);
    }
  }
}

TEST_SUITE("ecs::ConsumableMessageTrait") {
  TEST_CASE("ecs::ConsumableMessageTrait::concept") {
    SUBCASE("Messages with kConsumable = true satisfy ConsumableMessageTrait") {
      CHECK(ConsumableMessageTrait<ConsumableMessage>);
      CHECK(ConsumableMessageTrait<ConsumableNamedMessage>);
    }

    SUBCASE(
        "Messages with kConsumable = false do not satisfy "
        "ConsumableMessageTrait") {
      CHECK_FALSE(ConsumableMessageTrait<NonConsumableMessage>);
    }

    SUBCASE(
        "Messages without kConsumable do not satisfy ConsumableMessageTrait") {
      CHECK_FALSE(ConsumableMessageTrait<SimpleMessage>);
      CHECK_FALSE(ConsumableMessageTrait<NamedMessage>);
      CHECK_FALSE(ConsumableMessageTrait<ManualClearMessage>);
    }

    SUBCASE("Async messages cannot be ConsumableMessageTrait") {
      CHECK_FALSE(ConsumableMessageTrait<AsyncMessage>);
      CHECK_FALSE(ConsumableMessageTrait<NamedAsyncMessage>);
    }
  }
}
