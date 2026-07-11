#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/local_param.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/param_policy.hpp>

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

struct Health {
  int value = 0;
};

struct Disabled {};

struct Camera {
  float fov = 90.0F;
};

struct RenderSettings {
  bool vsync = false;
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
};

// Systems for BuildPolicyFromSystem testing

struct QueryOnlySystem {
  void operator()(Query<const Position&, Velocity&> /*query*/) {}
};

struct ResourceOnlySystem {
  void operator()(Res<const Camera> /*camera*/,
                  Res<RenderSettings> /*settings*/) {}
};

struct LocalOnlySystem {
  void operator()(Local<Camera> /*camera*/) {}
};

struct CommandsOnlySystem {
  void operator()(Commands /*commands*/) {}
};

struct WorldViewOnlySystem {
  void operator()(WorldView /*view*/) {}
};

struct MessageReaderOnlySystem {
  void operator()(MessageReader<MyMessage> /*reader*/) {}
};

struct MixedQueryResourceSystem {
  void operator()(Query<const Position&, Velocity&> /*query*/,
                  Res<const Camera> /*camera*/,
                  Res<RenderSettings> /*settings*/) {}
};

struct MixedAllSystem {
  void operator()(Query<const Position&> /*query*/,
                  Res<const Camera> /*camera*/, Local<RenderSettings> /*local*/,
                  Commands /*commands*/, WorldView /*view*/,
                  MessageReader<MyMessage> /*reader*/) {}
};

struct EmptyOperatorSystem {
  void operator()() {}
};

// Helper: manually build policy from params, equivalent to
// BuildPolicyFromParams
template <typename... Params>
auto BuildPolicyFromParamsManual() -> AccessPolicy {
  AccessPolicyBuilder builder;
  RegisterParamAccess<Params...>(builder);
  return builder.Build();
}

}  // namespace

// Compile-time checks for BuildPolicyFromParams and BuildPolicyFromSystem
// These use decltype to verify return types without triggering consteval
// evaluation issues on toolchains with incomplete constexpr std::vector
// support.

static_assert(
    std::same_as<decltype(BuildPolicyFromParams<Query<const Position&>>()),
                 AccessPolicy>);
static_assert(std::same_as<decltype(BuildPolicyFromParams<Res<const Camera>>()),
                           AccessPolicy>);
static_assert(
    std::same_as<decltype(BuildPolicyFromParams<Query<const Position&>,
                                                Res<const Camera>, Commands>()),
                 AccessPolicy>);
static_assert(std::same_as<decltype(BuildPolicyFromSystem<QueryOnlySystem>()),
                           AccessPolicy>);
static_assert(
    std::same_as<decltype(BuildPolicyFromSystem<EmptyOperatorSystem>()),
                 AccessPolicy>);

TEST_SUITE("helios::ecs::BuildPolicyFromParams") {
  TEST_CASE("ecs::BuildPolicyFromParams::Query") {
    SUBCASE("Read-only query produces read component policy") {
      const auto policy = BuildPolicyFromParamsManual<Query<const Position&>>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK_FALSE(
          policy.HasWriteComponent(ComponentTypeIndex::From<Position>()));
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Write query produces write component policy") {
      const auto policy = BuildPolicyFromParamsManual<Query<Position&>>();
      CHECK_FALSE(
          policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Mixed read-write query splits correctly") {
      auto policy =
          BuildPolicyFromParamsManual<Query<const Position&, Velocity&>>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
    }

    SUBCASE("Tag component is excluded") {
      auto policy =
          BuildPolicyFromParamsManual<Query<Disabled, const Position&>>();
      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK(policy.GetWriteComponents().empty());
    }
  }

  TEST_CASE("ecs::BuildPolicyFromParams::Resources") {
    SUBCASE("Res<const T> produces read resource policy") {
      const auto policy = BuildPolicyFromParamsManual<Res<const Camera>>();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Res<T> produces write resource policy") {
      const auto policy = BuildPolicyFromParamsManual<Res<Camera>>();
      CHECK_FALSE(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("optional<Res<const T>> produces read resource policy") {
      auto policy =
          BuildPolicyFromParamsManual<std::optional<Res<const Camera>>>();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("optional<Res<T>> produces write resource policy") {
      const auto policy =
          BuildPolicyFromParamsManual<std::optional<Res<Camera>>>();
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Async resource is not registered") {
      const auto policy = BuildPolicyFromParamsManual<Res<AsyncCounter>>();
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::BuildPolicyFromParams::Local") {
    SUBCASE("Local<T> produces empty policy") {
      const auto policy = BuildPolicyFromParamsManual<Local<Camera>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Local<const T> produces empty policy") {
      const auto policy = BuildPolicyFromParamsManual<Local<const Camera>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::BuildPolicyFromParams::Commands") {
    SUBCASE("Commands produces empty policy") {
      const auto policy = BuildPolicyFromParamsManual<Commands>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::BuildPolicyFromParams::WorldView") {
    SUBCASE("WorldView produces empty policy") {
      const auto policy = BuildPolicyFromParamsManual<WorldView>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::BuildPolicyFromParams::Messages") {
    SUBCASE("MessageReader produces empty policy") {
      const auto policy =
          BuildPolicyFromParamsManual<MessageReader<MyMessage>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("ConsumableMessageReader produces empty policy") {
      const auto policy = BuildPolicyFromParamsManual<
          ConsumableMessageReader<MyConsumableMessage>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("MessageWriter produces empty policy") {
      const auto policy =
          BuildPolicyFromParamsManual<MessageWriter<MyMessage>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("AsyncMessageReader produces empty policy") {
      auto policy =
          BuildPolicyFromParamsManual<AsyncMessageReader<MyAsyncMessage>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("AsyncMessageWriter produces empty policy") {
      auto policy =
          BuildPolicyFromParamsManual<AsyncMessageWriter<MyAsyncMessage>>();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::BuildPolicyFromParams::mixed") {
    SUBCASE(
        "Query and Res params produce combined component and resource "
        "policy") {
      auto policy =
          BuildPolicyFromParamsManual<Query<const Position&, Velocity&>,
                                      Res<const Camera>, Res<RenderSettings>>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<RenderSettings>()));
    }

    SUBCASE("Query, Res, Local, Commands produce combined policy") {
      auto policy =
          BuildPolicyFromParamsManual<Query<const Position&>, Res<const Camera>,
                                      Local<RenderSettings>, Commands>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("All param types combined produce correct policy") {
      const auto policy = BuildPolicyFromParamsManual<
          Query<const Position&, Velocity&>, Res<const Camera>,
          Res<RenderSettings>, Local<Health>, Commands, WorldView,
          MessageReader<MyMessage>, MessageWriter<MyMessage>,
          AsyncMessageReader<MyAsyncMessage>,
          AsyncMessageWriter<MyAsyncMessage>>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<RenderSettings>()));
    }

    SUBCASE("Multiple Queries with same component are deduped") {
      auto policy =
          BuildPolicyFromParamsManual<Query<const Position&>,
                                      Query<const Position&, Velocity&>>();
      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
    }

    SUBCASE("Multiple Resources with same resource are deduped") {
      auto policy =
          BuildPolicyFromParamsManual<Res<Camera>, Res<const Camera>>();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }
  }
}

TEST_SUITE("helios::ecs::SystemParamTraits::RegisterAccess") {
  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::Query") {
    SUBCASE("Query-only system produces component policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&QueryOnlySystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::Resources") {
    SUBCASE("Resource-only system produces resource policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&ResourceOnlySystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<RenderSettings>()));
    }
  }

  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::Local") {
    SUBCASE("Local-only system produces empty policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&LocalOnlySystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::Commands") {
    SUBCASE("Commands-only system produces empty policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&CommandsOnlySystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::WorldView") {
    SUBCASE("WorldView-only system produces empty policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&WorldViewOnlySystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::Messages") {
    SUBCASE("MessageReader-only system produces empty policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&MessageReaderOnlySystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::SystemParamTraits::RegisterAccess::mixed") {
    SUBCASE("Mixed Query and Resource system produces combined policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&MixedQueryResourceSystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<RenderSettings>()));
    }

    SUBCASE("All param types combined produce correct policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&MixedAllSystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Empty operator system produces empty policy") {
      AccessPolicyBuilder builder;
      using Args = details::MemberFnArgs<
          decltype(&EmptyOperatorSystem::operator())>::ArgsTuple;
      []<typename... Params>(std::tuple<Params...>*, AccessPolicyBuilder& b) {
        (SystemParamTraits<std::remove_cvref_t<Params>>::RegisterAccess(b),
         ...);
      }(static_cast<Args*>(nullptr), builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }
}
