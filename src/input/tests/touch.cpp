#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/touch.hpp>

using namespace helios::ecs;
using namespace helios::input;

TEST_SUITE("helios::input::TouchFinger") {
  TEST_CASE("helios::input::TouchFinger::Reset") {
    SUBCASE("Clears identity, position, and contact state") {
      TouchFinger finger;
      finger.position_x = 10.0;
      finger.position_y = 20.0;
      finger.delta_x = 1.0;
      finger.delta_y = 2.0;
      finger.pressure = 0.75F;
      finger.id = 3;
      finger.device_type = TouchDeviceType::kDirect;
      finger.down = true;
      finger.has_position = true;

      finger.Reset();

      CHECK_EQ(finger.position_x, 0.0);
      CHECK_EQ(finger.position_y, 0.0);
      CHECK_EQ(finger.delta_x, 0.0);
      CHECK_EQ(finger.delta_y, 0.0);
      CHECK_EQ(finger.pressure, doctest::Approx(0.0F));
      CHECK_FALSE(finger.id.has_value());
      CHECK_EQ(finger.device_type, TouchDeviceType::kUnknown);
      CHECK_FALSE(finger.down);
      CHECK_FALSE(finger.has_position);
    }
  }
}

TEST_SUITE("helios::input::Touches") {
  TEST_CASE("helios::input::Touches") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(Touches{});
      CHECK(world.HasResource<Touches>());
    }
  }

  TEST_CASE("helios::input::Touches::TryGet") {
    SUBCASE("Returns a mutable slot for a valid id") {
      Touches touches;
      TouchFinger* finger = touches.TryGet(0);
      REQUIRE_NE(finger, nullptr);
      finger->down = true;
      CHECK(touches.fingers[0].down);
    }

    SUBCASE("Returns a const slot for a valid id") {
      Touches touches;
      touches.fingers[3].id = 3;
      const Touches& view = touches;
      const TouchFinger* finger = view.TryGet(3);
      REQUIRE_NE(finger, nullptr);
      CHECK_EQ(finger->id, 3);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Touches touches;
      CHECK_EQ(touches.TryGet(static_cast<TouchId>(Touches::kSlotCount)),
               nullptr);
    }
  }
}

TEST_SUITE("helios::input::TouchInputMsg") {
  TEST_CASE("helios::input::TouchInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<TouchInputMsg>();
      CHECK(world.HasMessage<TouchInputMsg>());
      world.WriteMessages<TouchInputMsg>().Write(
          {.id = 0, .phase = TouchPhase::kStarted});
    }
  }
}
