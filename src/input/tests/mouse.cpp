#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/mouse.hpp>

#include <cstdint>
#include <vector>

using namespace helios::ecs;
using namespace helios::input;

TEST_SUITE("helios::input::Mouse") {
  TEST_CASE("helios::input::Mouse") {
    SUBCASE("Inserts as a resource and stores buttons and motion") {
      World world;
      world.InsertResources(Mouse{});
      CHECK(world.HasResource<Mouse>());

      auto& mouse = world.WriteResource<Mouse>();
      mouse.buttons.Press(MouseButton::kLeft);
      mouse.position_x = 10.0;
      mouse.position_y = 20.0;
      mouse.delta_x = 1.0;
      mouse.delta_y = -2.0;
      mouse.scroll_x = 0.25;
      mouse.scroll_y = 0.5;

      const auto& view = world.ReadResource<Mouse>();
      CHECK(view.buttons.Pressed(MouseButton::kLeft));
      CHECK_EQ(view.position_x, 10.0);
      CHECK_EQ(view.position_y, 20.0);
      CHECK_EQ(view.delta_x, 1.0);
      CHECK_EQ(view.delta_y, -2.0);
      CHECK_EQ(view.scroll_x, 0.25);
      CHECK_EQ(view.scroll_y, 0.5);
    }
  }
}

TEST_SUITE("helios::input::Cursor") {
  TEST_CASE("helios::input::Cursor") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(Cursor{});
      CHECK(world.HasResource<Cursor>());
    }
  }

  TEST_CASE("helios::input::Cursor::SetIcon") {
    SUBCASE("Updates the icon and marks the cursor dirty") {
      Cursor cursor;
      cursor.SetIcon(CursorIcon::kIBeam);

      CHECK_EQ(cursor.icon, CursorIcon::kIBeam);
      CHECK(cursor.dirty);
    }
  }

  TEST_CASE("helios::input::Cursor::SetCustom") {
    SUBCASE("Stores a custom image and marks the cursor dirty") {
      Cursor cursor;
      cursor.SetCustom(CursorImage{
          .width = 16,
          .height = 16,
          .hotspot_x = 1,
          .hotspot_y = 2,
          .rgba = std::vector<uint8_t>(16U * 16U * 4U, 255),
      });

      CHECK(cursor.custom.has_value());
      CHECK_EQ(cursor.custom->width, 16U);
      CHECK_EQ(cursor.custom->hotspot_x, 1);
      CHECK(cursor.dirty);
    }
  }

  TEST_CASE("helios::input::Cursor::ClearCustom") {
    SUBCASE("Removes a custom image and marks the cursor dirty") {
      Cursor cursor;
      cursor.SetCustom(CursorImage{.width = 8, .height = 8});
      cursor.dirty = false;
      cursor.ClearCustom();

      CHECK_FALSE(cursor.custom.has_value());
      CHECK(cursor.dirty);
    }

    SUBCASE("SetIcon does not clear a custom image") {
      Cursor cursor;
      cursor.SetCustom(CursorImage{.width = 8, .height = 8});
      cursor.SetIcon(CursorIcon::kArrow);

      CHECK(cursor.custom.has_value());
      CHECK_EQ(cursor.icon, CursorIcon::kArrow);
      CHECK(cursor.dirty);
    }
  }
}

TEST_SUITE("helios::input::MouseButtonInputMsg") {
  TEST_CASE("helios::input::MouseButtonInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<MouseButtonInputMsg>();
      CHECK(world.HasMessage<MouseButtonInputMsg>());
      world.WriteMessages<MouseButtonInputMsg>().Write(
          {.button = MouseButton::kLeft, .state = ButtonState::kPressed});
    }
  }
}

TEST_SUITE("helios::input::CursorMovedMsg") {
  TEST_CASE("helios::input::CursorMovedMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<CursorMovedMsg>();
      CHECK(world.HasMessage<CursorMovedMsg>());
      world.WriteMessages<CursorMovedMsg>().Write({.x = 1.0, .y = 2.0});
    }
  }
}

TEST_SUITE("helios::input::MouseMotionMsg") {
  TEST_CASE("helios::input::MouseMotionMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<MouseMotionMsg>();
      CHECK(world.HasMessage<MouseMotionMsg>());
      world.WriteMessages<MouseMotionMsg>().Write(
          {.delta_x = 3.0, .delta_y = -1.5});
    }
  }
}

TEST_SUITE("helios::input::MouseWheelMsg") {
  TEST_CASE("helios::input::MouseWheelMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<MouseWheelMsg>();
      CHECK(world.HasMessage<MouseWheelMsg>());
      world.WriteMessages<MouseWheelMsg>().Write({.x = 0.25, .y = 1.0});
    }
  }
}

TEST_SUITE("helios::input::MouseConnectionMsg") {
  TEST_CASE("helios::input::MouseConnectionMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<MouseConnectionMsg>();
      CHECK(world.HasMessage<MouseConnectionMsg>());
      world.WriteMessages<MouseConnectionMsg>().Write(
          {.name = "mouse", .id = 0, .connected = true});
    }
  }
}
