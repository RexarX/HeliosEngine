#include <doctest/doctest.h>

#include <helios/input/components.hpp>
#include <helios/input/mouse.hpp>

#include <cstdint>
#include <vector>

using namespace helios::input;

TEST_SUITE("helios::input::Cursor") {
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
