#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/mouse.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddMouseMessages(World& world) {
  world.AddMessages<MouseButtonInputMsg, CursorMovedMsg, MouseMotionMsg,
                    MouseWheelMsg, MouseConnectionMsg>();
}

}  // namespace

TEST_SUITE("helios::input::UpdateMouseState") {
  TEST_CASE("helios::input::UpdateMouseState::operator()") {
    SUBCASE("Applies buttons, last cursor position, and accumulated motion") {
      World world;
      world.InsertResources(Mouse{});
      AddMouseMessages(world);

      world.WriteMessages<MouseButtonInputMsg>().Write({
          .button = MouseButton::kLeft,
          .state = ButtonState::kPressed,
      });
      world.WriteMessages<CursorMovedMsg>().Write({.x = 1.0, .y = 2.0});
      world.WriteMessages<CursorMovedMsg>().Write({.x = 10.0, .y = 20.0});
      world.WriteMessages<MouseMotionMsg>().Write(
          {.delta_x = 3.0, .delta_y = -1.5});
      world.WriteMessages<MouseMotionMsg>().Write(
          {.delta_x = 1.0, .delta_y = 0.5});
      world.WriteMessages<MouseWheelMsg>().Write({.x = 0.25, .y = 1.0});
      world.WriteMessages<MouseWheelMsg>().Write({.x = 0.25, .y = 0.5});

      auto local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Mouse>, MouseMessages>();
      UpdateMouseState{}(
          SystemParamTraits<Res<Mouse>>::Make(world, local_data, policy),
          SystemParamTraits<MouseMessages>::Make(world, local_data, policy));

      const auto& mouse = world.ReadResource<Mouse>();
      CHECK(mouse.buttons.Pressed(MouseButton::kLeft));
      CHECK_EQ(mouse.position_x, 10.0);
      CHECK_EQ(mouse.position_y, 20.0);
      CHECK_EQ(mouse.delta_x, 4.0);
      CHECK_EQ(mouse.delta_y, -1.0);
      CHECK_EQ(mouse.scroll_x, 0.5);
      CHECK_EQ(mouse.scroll_y, 1.5);
    }
  }
}
