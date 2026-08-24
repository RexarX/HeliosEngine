#include <doctest/doctest.h>

#include <helios/input/pen.hpp>
#include <helios/input/resources.hpp>

using namespace helios::input;

TEST_SUITE("helios::input::Pen") {
  TEST_CASE("helios::input::Pen::Reset") {
    SUBCASE("Clears identity, position, axes, buttons, and tip state") {
      Pen pen;
      pen.position_x = 10.0;
      pen.position_y = 20.0;
      pen.delta_x = 1.0;
      pen.delta_y = 2.0;
      pen.axes.Set(PenAxis::kPressure, 0.75F);
      pen.buttons.Press(PenButton::kBarrel1);
      pen.id = 3;
      pen.device_type = PenDeviceType::kDirect;
      pen.in_proximity = true;
      pen.down = true;
      pen.eraser = true;
      pen.has_position = true;

      pen.Reset();

      CHECK_EQ(pen.position_x, 0.0);
      CHECK_EQ(pen.position_y, 0.0);
      CHECK_EQ(pen.delta_x, 0.0);
      CHECK_EQ(pen.delta_y, 0.0);
      CHECK_EQ(pen.axes.Get(PenAxis::kPressure), doctest::Approx(0.0F));
      CHECK_FALSE(pen.buttons.Pressed(PenButton::kBarrel1));
      CHECK_EQ(pen.id, -1);
      CHECK_EQ(pen.device_type, PenDeviceType::kUnknown);
      CHECK_FALSE(pen.in_proximity);
      CHECK_FALSE(pen.down);
      CHECK_FALSE(pen.eraser);
      CHECK_FALSE(pen.has_position);
    }
  }
}

TEST_SUITE("helios::input::Pens") {
  TEST_CASE("helios::input::Pens::TryGet") {
    SUBCASE("Returns a mutable slot for a valid id") {
      Pens pens;
      Pen* pen = pens.TryGet(0);
      REQUIRE_NE(pen, nullptr);
      pen->in_proximity = true;
      CHECK(pens.pens[0].in_proximity);
    }

    SUBCASE("Returns a const slot for a valid id") {
      Pens pens;
      pens.pens[3].id = 3;
      const Pens& view = pens;
      const Pen* pen = view.TryGet(3);
      REQUIRE_NE(pen, nullptr);
      CHECK_EQ(pen->id, 3);
    }

    SUBCASE("Returns nullptr for a negative id") {
      Pens pens;
      CHECK_EQ(pens.TryGet(-1), nullptr);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Pens pens;
      CHECK_EQ(pens.TryGet(static_cast<int32_t>(Pens::kSlotCount)), nullptr);
    }
  }
}
