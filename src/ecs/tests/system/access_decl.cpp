#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/param_traits.hpp>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
};

struct Camera {
  float fov = 90.0F;
};

struct RenderSettings {
  bool vsync = false;
};

}  // namespace

TEST_SUITE("helios::ecs::RegisterParamAccess") {
  TEST_CASE("ecs::RegisterParamAccess matches BuildPolicyFromParams") {
    const auto from_register = [] {
      AccessPolicyBuilder builder;
      RegisterParamAccess<Query<const Position&, Velocity&>, Res<const Camera>,
                          Res<RenderSettings>>(builder);
      return builder.Build();
    }();

    const auto from_build =
        BuildPolicyFromParams<Query<const Position&, Velocity&>,
                              Res<const Camera>, Res<RenderSettings>>();

    CHECK(from_register.HasReadComponent(ComponentTypeIndex::From<Position>()));
    CHECK(
        from_register.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
    CHECK(from_register.HasReadResource(ResourceTypeIndex::From<Camera>()));
    CHECK(from_register.HasWriteResource(
        ResourceTypeIndex::From<RenderSettings>()));

    CHECK(from_build.HasReadComponent(ComponentTypeIndex::From<Position>()));
    CHECK(from_build.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
    CHECK(from_build.HasReadResource(ResourceTypeIndex::From<Camera>()));
    CHECK(
        from_build.HasWriteResource(ResourceTypeIndex::From<RenderSettings>()));
  }

  TEST_CASE("ecs::DeclareReadComponents and DeclareWriteComponents") {
    AccessPolicyBuilder builder;
    DeclareReadComponents<Position>(builder);
    DeclareWriteComponents<Velocity>(builder);

    const auto policy = builder.Build();
    CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
    CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
  }

  TEST_CASE("ecs::DeclareQueryAccess") {
    AccessPolicyBuilder builder;
    DeclareQueryAccess<const Camera&, RenderSettings&>(builder);

    const auto policy = builder.Build();
    CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Camera>()));
    CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<RenderSettings>()));
  }
}
