#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/pen.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddPenMessages(World& world) {
  world.AddMessages<PenProximityMsg, PenTouchMsg, PenButtonInputMsg,
                    PenMovedMsg, PenAxisChangedMsg>();
}

void RunUpdatePen(World& world) {
  auto local_data = SystemLocalData::From();
  const AccessPolicy policy = BuildPolicyFromParams<Res<Pens>, PenMessages>();
  UpdatePenState{}(
      SystemParamTraits<Res<Pens>>::Make(world, local_data, policy),
      SystemParamTraits<PenMessages>::Make(world, local_data, policy));
}

}  // namespace

TEST_SUITE("helios::input::UpdatePenState") {
  TEST_CASE("helios::input::UpdatePenState::operator()") {
    SUBCASE(
        "Applies proximity, first position without a delta spike, then "
        "motion") {
      World world;
      world.InsertResources(Pens{});
      AddPenMessages(world);

      world.WriteMessages<PenProximityMsg>().Write({
          .id = 0,
          .device_type = PenDeviceType::kIndirect,
          .in_proximity = true,
      });
      world.WriteMessages<PenMovedMsg>().Write({
          .x = 10.0,
          .y = 20.0,
          .id = 0,
      });
      world.WriteMessages<PenMovedMsg>().Write({
          .x = 14.0,
          .y = 24.0,
          .id = 0,
      });
      world.WriteMessages<PenTouchMsg>().Write({
          .x = 14.0,
          .y = 24.0,
          .id = 0,
          .down = true,
          .eraser = false,
      });
      world.WriteMessages<PenButtonInputMsg>().Write({
          .id = 0,
          .button = PenButton::kBarrel1,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<PenAxisChangedMsg>().Write({
          .x = 14.0,
          .y = 24.0,
          .value = 0.75F,
          .id = 0,
          .axis = PenAxis::kPressure,
      });

      RunUpdatePen(world);

      const Pen& pen = world.ReadResource<Pens>().pens[0];
      CHECK(pen.in_proximity);
      CHECK_EQ(pen.id, 0);
      CHECK_EQ(pen.device_type, PenDeviceType::kIndirect);
      CHECK(pen.down);
      CHECK_FALSE(pen.eraser);
      CHECK(pen.buttons.Pressed(PenButton::kBarrel1));
      CHECK_EQ(pen.position_x, 14.0);
      CHECK_EQ(pen.position_y, 24.0);
      CHECK_EQ(pen.delta_x, 4.0);
      CHECK_EQ(pen.delta_y, 4.0);
      CHECK_EQ(pen.axes.Get(PenAxis::kPressure), doctest::Approx(0.75F));
    }

    SUBCASE("Proximity out resets the slot") {
      World world;
      Pens pens;
      pens.pens[1].id = 1;
      pens.pens[1].in_proximity = true;
      pens.pens[1].down = true;
      pens.pens[1].buttons.Press(PenButton::kBarrel2);
      pens.pens[1].position_x = 5.0;
      world.InsertResources(std::move(pens));
      AddPenMessages(world);

      world.WriteMessages<PenProximityMsg>().Write({
          .id = 1,
          .device_type = PenDeviceType::kDirect,
          .in_proximity = false,
      });

      RunUpdatePen(world);

      const Pen& pen = world.ReadResource<Pens>().pens[1];
      CHECK_FALSE(pen.in_proximity);
      CHECK_FALSE(pen.id.has_value());
      CHECK_FALSE(pen.down);
      CHECK_FALSE(pen.buttons.Pressed(PenButton::kBarrel2));
      CHECK_EQ(pen.position_x, 0.0);
    }

    SUBCASE("Ignores samples while not in proximity") {
      World world;
      world.InsertResources(Pens{});
      AddPenMessages(world);

      world.WriteMessages<PenMovedMsg>().Write({
          .x = 1.0,
          .y = 2.0,
          .id = 0,
      });
      world.WriteMessages<PenButtonInputMsg>().Write({
          .id = 0,
          .button = PenButton::kBarrel1,
          .state = ButtonState::kPressed,
      });

      RunUpdatePen(world);

      const Pen& pen = world.ReadResource<Pens>().pens[0];
      CHECK_FALSE(pen.in_proximity);
      CHECK_EQ(pen.position_x, 0.0);
      CHECK_FALSE(pen.buttons.AnyPressed());
    }

    SUBCASE("Ignores out-of-range pen ids") {
      World world;
      world.InsertResources(Pens{});
      AddPenMessages(world);

      world.WriteMessages<PenProximityMsg>().Write({
          .id = static_cast<PenId>(Pens::kSlotCount),
          .device_type = PenDeviceType::kDirect,
          .in_proximity = true,
      });

      RunUpdatePen(world);

      CHECK_FALSE(world.ReadResource<Pens>().pens[0].in_proximity);
    }
  }
}
