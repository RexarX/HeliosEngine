#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/system_storage.hpp>
#include <helios/ecs/system/composite_param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/system_param.hpp>
#include <helios/ecs/world.hpp>

using namespace helios::ecs;

namespace helios::ecs::composite_param_test {

struct Mesh {
  int id = 0;
};

struct Transform {
  float x = 0.0F;
};

struct Camera {
  float fov = 90.0F;
};

struct RenderParam {
  Res<const Camera> camera;
  Query<const Mesh&, const Transform&> meshes;
};

}  // namespace helios::ecs::composite_param_test

namespace helios::ecs {

template <>
struct SystemParamTraits<composite_param_test::RenderParam>
    : CompositeSystemParam<composite_param_test::RenderParam,
                           Res<const composite_param_test::Camera>,
                           Query<const composite_param_test::Mesh&,
                                 const composite_param_test::Transform&>> {};

}  // namespace helios::ecs

namespace {

using namespace helios::ecs::composite_param_test;

int g_render_mesh_count = 0;

struct RenderSystem {
  void operator()(RenderParam param) {
    g_render_mesh_count = static_cast<int>(param.meshes.Count());
    CHECK_EQ(param.camera->fov, 75.0F);
  }
};

}  // namespace

TEST_SUITE("helios::ecs::CompositeSystemParam") {
  using namespace helios::ecs::composite_param_test;

  TEST_CASE("ecs::CompositeSystemParam::RegisterAccess") {
    const auto policy = BuildPolicyFromParams<RenderParam>();

    CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Mesh>()));
    CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Transform>()));
    CHECK_FALSE(policy.HasWriteComponent(ComponentTypeIndex::From<Mesh>()));
  }

  TEST_CASE("ecs::CompositeSystemParam::Make") {
    World world;
    world.InsertResources(Camera{.fov = 75.0F});

    const auto entity = world.CreateEntity();
    world.AddComponents(entity, Mesh{.id = 1}, Transform{.x = 2.0F});

    SystemLocalData local_data = SystemLocalData::From();
    const AccessPolicy policy = BuildPolicyFromParams<RenderParam>();
    const RenderParam param =
        SystemParamTraits<RenderParam>::Make(world, local_data, policy);

    CHECK_EQ(param.camera->fov, 75.0F);
    CHECK_EQ(param.meshes.Count(), 1);
  }

  TEST_CASE("ecs::CompositeSystemParam through Schedule::Add") {
    World world;
    world.InsertResources(Camera{.fov = 75.0F});

    const auto entity = world.CreateEntity();
    world.AddComponents(entity, Mesh{.id = 1}, Transform{.x = 2.0F});

    Schedule schedule("composite_param_test");
    schedule.Add(RenderSystem{});

    CHECK(schedule.Build().has_value());

    MainThreadExecutor executor;
    schedule.RunAndWait(world, executor);

    CHECK_EQ(g_render_mesh_count, 1);
  }

  TEST_CASE("ecs::SystemParam concept") {
    CHECK(SystemParam<RenderParam>);
    CHECK(SystemParam<Res<const Camera>>);
    CHECK_FALSE(SystemParam<int>);
  }
}
