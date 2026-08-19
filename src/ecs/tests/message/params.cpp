#include <doctest/doctest.h>

#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/params.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param.hpp>

using namespace helios::ecs;

namespace {

struct MyMessage {
  int value = 0;
};

struct MyConsumableMessage {
  static constexpr bool kConsumable = true;

  int value = 0;
};

struct MyAsyncMessage {
  static constexpr bool kAsync = true;

  int value = 0;
};

template <typename T>
concept HasSystemParamTraits = requires { typename SystemParamTraits<T>; };

template <typename T>
concept HasRegisterAccess = requires(AccessPolicyBuilder& builder) {
  SystemParamTraits<T>::RegisterAccess(builder);
};

}  // namespace

TEST_SUITE("helios::ecs::SystemParamTraits") {
  TEST_CASE("helios::ecs::SystemParamTraits: MessageReader") {
    SUBCASE("MessageReader<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<MessageReader<MyMessage>>);
      CHECK(HasRegisterAccess<MessageReader<MyMessage>>);
    }

    SUBCASE("MessageReader RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<MessageReader<MyMessage>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("helios::ecs::SystemParamTraits::ConsumableMessageReader") {
    SUBCASE("ConsumableMessageReader<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<ConsumableMessageReader<MyConsumableMessage>>);
      CHECK(HasRegisterAccess<ConsumableMessageReader<MyConsumableMessage>>);
    }

    SUBCASE("ConsumableMessageReader RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<ConsumableMessageReader<MyConsumableMessage>>::
          RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("helios::ecs::SystemParamTraits::MessageWriter") {
    SUBCASE("MessageWriter<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<MessageWriter<MyMessage>>);
      CHECK(HasRegisterAccess<MessageWriter<MyMessage>>);
    }

    SUBCASE("MessageWriter RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<MessageWriter<MyMessage>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("helios::ecs::SystemParamTraits::AsyncMessageReader") {
    SUBCASE("AsyncMessageReader<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<AsyncMessageReader<MyAsyncMessage>>);
      CHECK(HasRegisterAccess<AsyncMessageReader<MyAsyncMessage>>);
    }

    SUBCASE("AsyncMessageReader RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<AsyncMessageReader<MyAsyncMessage>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("helios::ecs::SystemParamTraits::AsyncMessageWriter") {
    SUBCASE("AsyncMessageWriter<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<AsyncMessageWriter<MyAsyncMessage>>);
      CHECK(HasRegisterAccess<AsyncMessageWriter<MyAsyncMessage>>);
    }

    SUBCASE("AsyncMessageWriter RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<AsyncMessageWriter<MyAsyncMessage>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }
}
