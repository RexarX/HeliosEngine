#include <doctest/doctest.h>

#include <helios/input/keyboard.hpp>

#include <cstdint>
#include <format>
#include <sstream>
#include <string>
#include <utility>

using namespace helios::input;

TEST_SUITE("helios::input::HasFlag") {
  TEST_CASE("helios::input::HasFlag") {
    SUBCASE("Detects a set modifier") {
      CHECK(
          HasFlag(Modifiers::kShift | Modifiers::kControl, Modifiers::kShift));
    }

    SUBCASE("Returns false for an unset modifier") {
      CHECK_FALSE(HasFlag(Modifiers::kShift, Modifiers::kAlt));
    }
  }
}

TEST_SUITE("helios::input::operator|") {
  TEST_CASE("helios::input::operator|") {
    SUBCASE("Combines modifier flags") {
      const auto modifiers = Modifiers::kShift | Modifiers::kAlt;
      CHECK(HasFlag(modifiers, Modifiers::kShift));
      CHECK(HasFlag(modifiers, Modifiers::kAlt));
    }
  }
}

TEST_SUITE("helios::input::ToString") {
  TEST_CASE("helios::input::ToString") {
    SUBCASE("Formats every defined key") {
      for (uint8_t i = 0; i <= std::to_underlying(Key::kCount); ++i) {
        const auto name = ToString(static_cast<Key>(i));
        CHECK_FALSE(name.empty());
        CHECK_NE(name, "unknown");
      }
    }

    SUBCASE("Formats representative keys") {
      CHECK_EQ(ToString(Key::kUnknown), "Unknown");
      CHECK_EQ(ToString(Key::kA), "A");
      CHECK_EQ(ToString(Key::kEscape), "Escape");
      CHECK_EQ(ToString(Key::kF12), "F12");
      CHECK_EQ(ToString(Key::kNumPadEnter), "NumPadEnter");
      CHECK_EQ(ToString(Key::kCount), "Count");
    }

    SUBCASE("Formats an invalid key as unknown") {
      CHECK_EQ(ToString(static_cast<Key>(255)), "unknown");
    }

    SUBCASE("Formats modifiers without prefix") {
      CHECK_EQ(ToString(Modifiers::kNone), "None");
      CHECK_EQ(ToString(Modifiers::kShift | Modifiers::kControl),
               "Shift | Control");
      CHECK_EQ(ToString(Modifiers::kCapsLock | Modifiers::kNumLock),
               "CapsLock | NumLock");
    }

    SUBCASE("Formats modifiers with prefix") {
      CHECK_EQ(ToString(Modifiers::kNone, true), "Modifiers::None");
      CHECK_EQ(ToString(Modifiers::kShift | Modifiers::kAlt, true),
               "Modifiers::Shift | Modifiers::Alt");
    }

    SUBCASE("Formats button states") {
      CHECK_EQ(ToString(ButtonState::kReleased), "Released");
      CHECK_EQ(ToString(ButtonState::kPressed), "Pressed");
      CHECK_EQ(ToString(ButtonState::kRepeat), "Repeat");
      CHECK_EQ(ToString(static_cast<ButtonState>(255)), "unknown");
    }
  }
}

TEST_SUITE("helios::input::operator<<") {
  TEST_CASE("helios::input::operator<<") {
    SUBCASE("Streams keys") {
      std::ostringstream stream;
      stream << Key::kEscape;
      CHECK_EQ(stream.str(), "Key::Escape");
    }

    SUBCASE("Streams modifiers") {
      std::ostringstream stream;
      stream << (Modifiers::kShift | Modifiers::kControl);
      CHECK_EQ(stream.str(), "Shift | Control");
    }

    SUBCASE("Streams button states") {
      std::ostringstream stream;
      stream << ButtonState::kPressed;
      CHECK_EQ(stream.str(), "ButtonState::Pressed");
    }
  }
}

TEST_SUITE("std::formatter") {
  TEST_CASE("std::formatter") {
    SUBCASE("Formats Key") {
      CHECK_EQ(std::format("{}", Key::kA), "Key::A");
    }

    SUBCASE("Formats Modifiers with prefix") {
      CHECK_EQ(std::format("{}", Modifiers::kShift | Modifiers::kControl),
               "Modifiers::Shift | Modifiers::Control");
    }

    SUBCASE("Formats ButtonState") {
      CHECK_EQ(std::format("{}", ButtonState::kRepeat), "ButtonState::Repeat");
    }
  }
}
