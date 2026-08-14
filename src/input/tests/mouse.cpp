#include <doctest/doctest.h>

#include <helios/input/mouse.hpp>

#include <cstdint>
#include <format>
#include <sstream>
#include <utility>

using namespace helios::input;

TEST_SUITE("helios::input::ToString") {
  TEST_CASE("helios::input::ToString") {
    SUBCASE("Formats every defined mouse button") {
      for (uint8_t i = 0; i <= std::to_underlying(MouseButton::kCount); ++i) {
        const auto name = ToString(static_cast<MouseButton>(i));
        CHECK_FALSE(name.empty());
        CHECK_NE(name, "unknown");
      }
    }

    SUBCASE("Formats representative mouse buttons") {
      CHECK_EQ(ToString(MouseButton::kLeft), "Left");
      CHECK_EQ(ToString(MouseButton::kRight), "Right");
      CHECK_EQ(ToString(MouseButton::kMiddle), "Middle");
      CHECK_EQ(ToString(MouseButton::kExtra5), "Extra5");
      CHECK_EQ(ToString(MouseButton::kCount), "Count");
    }

    SUBCASE("Formats an invalid mouse button as unknown") {
      CHECK_EQ(ToString(static_cast<MouseButton>(255)), "unknown");
    }

    SUBCASE("Formats every defined cursor icon") {
      for (uint8_t i = 0; i <= std::to_underlying(CursorIcon::kCount); ++i) {
        const auto name = ToString(static_cast<CursorIcon>(i));
        CHECK_FALSE(name.empty());
        CHECK_NE(name, "unknown");
      }
    }

    SUBCASE("Formats representative cursor icons") {
      CHECK_EQ(ToString(CursorIcon::kDefault), "Default");
      CHECK_EQ(ToString(CursorIcon::kIBeam), "IBeam");
      CHECK_EQ(ToString(CursorIcon::kNotAllowed), "NotAllowed");
      CHECK_EQ(ToString(CursorIcon::kCount), "Count");
    }

    SUBCASE("Formats an invalid cursor icon as unknown") {
      CHECK_EQ(ToString(static_cast<CursorIcon>(255)), "unknown");
    }
  }
}

TEST_SUITE("helios::input::operator<<") {
  TEST_CASE("helios::input::operator<<") {
    SUBCASE("Streams mouse buttons") {
      std::ostringstream stream;
      stream << MouseButton::kLeft;
      CHECK_EQ(stream.str(), "MouseButton::Left");
    }

    SUBCASE("Streams cursor icons") {
      std::ostringstream stream;
      stream << CursorIcon::kCrosshair;
      CHECK_EQ(stream.str(), "CursorIcon::Crosshair");
    }
  }
}

TEST_SUITE("std::formatter") {
  TEST_CASE("std::formatter") {
    SUBCASE("Formats MouseButton") {
      CHECK_EQ(std::format("{}", MouseButton::kMiddle), "MouseButton::Middle");
    }

    SUBCASE("Formats CursorIcon") {
      CHECK_EQ(std::format("{}", CursorIcon::kPointingHand),
               "CursorIcon::PointingHand");
    }
  }
}
