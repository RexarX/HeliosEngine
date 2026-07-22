#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/param_traits.hpp>

#include <optional>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct Disabled {};

struct Camera {
  float fov = 90.0F;
};

struct AsyncCounter {
  static constexpr bool kThreadSafe = true;

  int value = 0;
};

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
  TEST_CASE("ecs::SystemParamTraits::Query") {
    SUBCASE("Query exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Query<const Position&>>);
      CHECK(HasSystemParamTraits<Query<Position&, const Velocity&>>);
      CHECK(HasSystemParamTraits<Query<>>);
    }

    SUBCASE("RegisterAccess adds components to builder") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Query<const Position&, Velocity&>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
    }

    SUBCASE("Read-only Query produces only read components") {
      AccessPolicyBuilder builder;
      SystemParamTraits<
          Query<const Position&, const Velocity&>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_EQ(policy.GetReadComponents().size(), 2);
      CHECK(policy.GetWriteComponents().empty());
    }

    SUBCASE("Write Query produces only write components") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Query<Position&, Velocity&>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.GetReadComponents().empty());
      CHECK_EQ(policy.GetWriteComponents().size(), 2);
    }

    SUBCASE("Tag components are excluded from policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Query<Disabled, const Position&>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK(policy.GetWriteComponents().empty());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::Resources") {
    SUBCASE("Res<const T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Res<const Camera>>);
      CHECK(HasRegisterAccess<Res<const Camera>>);
    }

    SUBCASE("Res<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Res<Camera>>);
      CHECK(HasRegisterAccess<Res<Camera>>);
    }

    SUBCASE("optional<Res<const T>> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<std::optional<Res<const Camera>>>);
      CHECK(HasRegisterAccess<std::optional<Res<const Camera>>>);
    }

    SUBCASE("optional<Res<T>> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<std::optional<Res<Camera>>>);
      CHECK(HasRegisterAccess<std::optional<Res<Camera>>>);
    }

    SUBCASE("Res<const T> RegisterAccess adds read resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Res<const Camera>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Res<T> RegisterAccess adds write resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Res<Camera>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("optional<Res<const T>> RegisterAccess adds read resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<std::optional<Res<const Camera>>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("optional<Res<T>> RegisterAccess adds write resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<std::optional<Res<Camera>>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Async resource is not registered by RegisterAccess") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Res<AsyncCounter>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Async resource in optional is not registered") {
      AccessPolicyBuilder builder;
      SystemParamTraits<std::optional<Res<AsyncCounter>>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::Local") {
    SUBCASE("Local<const T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Local<const Camera>>);
      CHECK(HasRegisterAccess<Local<const Camera>>);
    }

    SUBCASE("Local<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Local<Camera>>);
      CHECK(HasRegisterAccess<Local<Camera>>);
    }

    SUBCASE("optional<Local<T>> is not a system parameter") {
      CHECK_FALSE(SystemParam<std::optional<Local<const Camera>>>);
      CHECK_FALSE(SystemParam<std::optional<Local<Camera>>>);
      CHECK_FALSE(HasRegisterAccess<std::optional<Local<const Camera>>>);
      CHECK_FALSE(HasRegisterAccess<std::optional<Local<Camera>>>);
    }

    SUBCASE("Local<T> Make default-creates system-local resources") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      CHECK_FALSE(data.resource_manager.Has<Camera>());

      const Local<Camera> local =
          SystemParamTraits<Local<Camera>>::Make(world, data, policy);
      local = Camera{.fov = 75.0F};

      CHECK_EQ(data.resource_manager.Get<Camera>().fov, 75.0F);
    }

    SUBCASE("Local<const T> Make default-creates system-local resources") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      CHECK_FALSE(data.resource_manager.Has<Camera>());

      const Local<const Camera> local =
          SystemParamTraits<Local<const Camera>>::Make(world, data, policy);

      CHECK_EQ(local->fov, 90.0F);
      CHECK(data.resource_manager.Has<Camera>());
    }

    SUBCASE("Local<const T> Make reads existing system-local resources") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      data.resource_manager.Insert(Camera{.fov = 80.0F});

      const Local<const Camera> local =
          SystemParamTraits<Local<const Camera>>::Make(world, data, policy);

      CHECK_EQ(local->fov, 80.0F);
    }

    SUBCASE("Local<const T> RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Local<const Camera>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Local<T> RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Local<Camera>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::Commands") {
    SUBCASE("Commands exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Commands>);
      CHECK(HasRegisterAccess<Commands>);
    }

    SUBCASE("Commands RegisterAccess leaves policy empty") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Commands>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::WorldView") {
    SUBCASE("WorldView exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<WorldView>);
      CHECK(HasRegisterAccess<WorldView>);
    }

    SUBCASE("WorldView RegisterAccess leaves policy empty") {
      AccessPolicyBuilder builder;
      SystemParamTraits<WorldView>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::Messages") {
    SUBCASE("MessageReader<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<MessageReader<MyMessage>>);
      CHECK(HasRegisterAccess<MessageReader<MyMessage>>);
    }

    SUBCASE("ConsumableMessageReader<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<ConsumableMessageReader<MyConsumableMessage>>);
      CHECK(HasRegisterAccess<ConsumableMessageReader<MyConsumableMessage>>);
    }

    SUBCASE("MessageWriter<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<MessageWriter<MyMessage>>);
      CHECK(HasRegisterAccess<MessageWriter<MyMessage>>);
    }

    SUBCASE("AsyncMessageReader<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<AsyncMessageReader<MyAsyncMessage>>);
      CHECK(HasRegisterAccess<AsyncMessageReader<MyAsyncMessage>>);
    }

    SUBCASE("AsyncMessageWriter<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<AsyncMessageWriter<MyAsyncMessage>>);
      CHECK(HasRegisterAccess<AsyncMessageWriter<MyAsyncMessage>>);
    }

    SUBCASE("MessageReader RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<MessageReader<MyMessage>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("ConsumableMessageReader RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<ConsumableMessageReader<MyConsumableMessage>>::
          RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("MessageWriter RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<MessageWriter<MyMessage>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("AsyncMessageReader RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<AsyncMessageReader<MyAsyncMessage>>::RegisterAccess(
          builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
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

  TEST_CASE("ecs::SystemParamTraits::combination") {
    SUBCASE("Multiple trait types coexist") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Query<const Position&>>::RegisterAccess(builder);
      SystemParamTraits<Res<const Camera>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Query and mutable resource coexist") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Query<Velocity&>>::RegisterAccess(builder);
      SystemParamTraits<Res<Camera>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }
  }
}
