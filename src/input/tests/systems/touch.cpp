#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/touch.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddTouchMessages(World& world) {
  world.AddMessages<TouchInputMsg>();
}

void RunUpdateTouch(World& world) {
  auto local_data = SystemLocalData::From();
  const AccessPolicy policy =
      BuildPolicyFromParams<Res<Touches>, TouchMessages>();
  UpdateTouchState{}(
      SystemParamTraits<Res<Touches>>::Make(world, local_data, policy),
      SystemParamTraits<TouchMessages>::Make(world, local_data, policy));
}

}  // namespace

TEST_SUITE("helios::input::UpdateTouchState") {
  TEST_CASE("helios::input::UpdateTouchState::operator()") {
    SUBCASE("Applies start without a delta from the origin") {
      World world;
      world.InsertResources(Touches{});
      AddTouchMessages(world);

      world.WriteMessages<TouchInputMsg>().Write({
          .x = 10.0,
          .y = 20.0,
          .id = 0,
          .phase = TouchPhase::kStarted,
          .device_type = TouchDeviceType::kDirect,
      });
      RunUpdateTouch(world);

      const TouchFinger& finger = world.ReadResource<Touches>().fingers[0];
      CHECK_EQ(finger.id, 0);
      CHECK(finger.down);
      CHECK(finger.has_position);
      CHECK_EQ(finger.position_x, 10.0);
      CHECK_EQ(finger.position_y, 20.0);
      CHECK_EQ(finger.delta_x, 0.0);
      CHECK_EQ(finger.delta_y, 0.0);
      CHECK_EQ(finger.device_type, TouchDeviceType::kDirect);
    }

    SUBCASE("Accumulates motion delta after the first sample") {
      World world;
      world.InsertResources(Touches{});
      AddTouchMessages(world);

      world.WriteMessages<TouchInputMsg>().Write({
          .x = 10.0,
          .y = 20.0,
          .id = 1,
          .phase = TouchPhase::kStarted,
      });
      RunUpdateTouch(world);
      world.WriteMessages<TouchInputMsg>().Write({
          .x = 12.0,
          .y = 24.0,
          .id = 1,
          .phase = TouchPhase::kMoved,
      });
      RunUpdateTouch(world);

      const TouchFinger& finger = world.ReadResource<Touches>().fingers[1];
      CHECK_EQ(finger.position_x, 12.0);
      CHECK_EQ(finger.position_y, 24.0);
      CHECK_EQ(finger.delta_x, 2.0);
      CHECK_EQ(finger.delta_y, 4.0);
    }

    SUBCASE("Ended fingers stay visible until cleared") {
      World world;
      world.InsertResources(Touches{});
      AddTouchMessages(world);

      world.WriteMessages<TouchInputMsg>().Write({
          .x = 1.0,
          .y = 2.0,
          .id = 0,
          .phase = TouchPhase::kStarted,
      });
      RunUpdateTouch(world);
      world.WriteMessages<TouchInputMsg>().Write({
          .x = 1.0,
          .y = 2.0,
          .id = 0,
          .phase = TouchPhase::kEnded,
      });
      RunUpdateTouch(world);

      const TouchFinger& finger = world.ReadResource<Touches>().fingers[0];
      CHECK_FALSE(finger.down);
      CHECK_EQ(finger.id, 0);
      CHECK(finger.has_position);
    }
  }
}
