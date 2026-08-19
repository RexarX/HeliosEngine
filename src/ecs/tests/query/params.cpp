#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/query/params.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param.hpp>

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

template <typename T>
concept HasSystemParamTraits = requires { typename SystemParamTraits<T>; };

template <typename T>
concept HasRegisterAccess = requires(AccessPolicyBuilder& builder) {
  SystemParamTraits<T>::RegisterAccess(builder);
};

}  // namespace

TEST_SUITE("helios::ecs::SystemParamTraits") {
  TEST_CASE("helios::ecs::SystemParamTraits: Query") {
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
}
