#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/pen.hpp>

using namespace helios::ecs;
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
      CHECK_FALSE(pen.id.has_value());
      CHECK_EQ(pen.device_type, PenDeviceType::kUnknown);
      CHECK_FALSE(pen.in_proximity);
      CHECK_FALSE(pen.down);
      CHECK_FALSE(pen.eraser);
      CHECK_FALSE(pen.has_position);
    }
  }
}

TEST_SUITE("helios::input::Pens") {
  TEST_CASE("helios::input::Pens") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(Pens{});
      CHECK(world.HasResource<Pens>());
    }
  }

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

    SUBCASE("Returns nullptr for an out-of-range id") {
      Pens pens;
      CHECK_EQ(pens.TryGet(static_cast<PenId>(Pens::kSlotCount)), nullptr);
    }
  }
}

TEST_SUITE("helios::input::PenProximityMsg") {
  TEST_CASE("helios::input::PenProximityMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<PenProximityMsg>();
      CHECK(world.HasMessage<PenProximityMsg>());
      world.WriteMessages<PenProximityMsg>().Write(
          {.id = 0, .in_proximity = true});
    }
  }
}

TEST_SUITE("helios::input::PenTouchMsg") {
  TEST_CASE("helios::input::PenTouchMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<PenTouchMsg>();
      CHECK(world.HasMessage<PenTouchMsg>());
      world.WriteMessages<PenTouchMsg>().Write({.id = 0, .down = true});
    }
  }
}

TEST_SUITE("helios::input::PenButtonInputMsg") {
  TEST_CASE("helios::input::PenButtonInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<PenButtonInputMsg>();
      CHECK(world.HasMessage<PenButtonInputMsg>());
      world.WriteMessages<PenButtonInputMsg>().Write(
          {.id = 0,
           .button = PenButton::kBarrel1,
           .state = ButtonState::kPressed});
    }
  }
}

TEST_SUITE("helios::input::PenMovedMsg") {
  TEST_CASE("helios::input::PenMovedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<PenMovedMsg>();
      CHECK(world.HasMessage<PenMovedMsg>());
      world.WriteMessages<PenMovedMsg>().Write({.x = 1.0, .y = 2.0, .id = 0});
    }
  }
}

TEST_SUITE("helios::input::PenAxisChangedMsg") {
  TEST_CASE("helios::input::PenAxisChangedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<PenAxisChangedMsg>();
      CHECK(world.HasMessage<PenAxisChangedMsg>());
      world.WriteMessages<PenAxisChangedMsg>().Write({.value = 0.75F, .id = 0});
    }
  }
}
