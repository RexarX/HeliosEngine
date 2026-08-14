#include <doctest/doctest.h>

#include <helios/input/gamepad.hpp>

#include <cstdint>
#include <format>
#include <sstream>
#include <utility>

using namespace helios::input;

TEST_SUITE("helios::input::Trigger") {
  TEST_CASE("helios::input::Trigger") {
    SUBCASE("Returns true for trigger axes") {
      CHECK(Trigger(GamepadAxis::kLeftTrigger));
      CHECK(Trigger(GamepadAxis::kRightTrigger));
    }

    SUBCASE("Returns false for stick axes") {
      CHECK_FALSE(Trigger(GamepadAxis::kLeftX));
      CHECK_FALSE(Trigger(GamepadAxis::kRightY));
    }
  }
}

TEST_SUITE("helios::input::LeftStick") {
  TEST_CASE("helios::input::LeftStick") {
    SUBCASE("Returns true for left stick axes") {
      CHECK(LeftStick(GamepadAxis::kLeftX));
      CHECK(LeftStick(GamepadAxis::kLeftY));
    }

    SUBCASE("Returns false for other axes") {
      CHECK_FALSE(LeftStick(GamepadAxis::kRightX));
      CHECK_FALSE(LeftStick(GamepadAxis::kLeftTrigger));
    }
  }
}

TEST_SUITE("helios::input::RightStick") {
  TEST_CASE("helios::input::RightStick") {
    SUBCASE("Returns true for right stick axes") {
      CHECK(RightStick(GamepadAxis::kRightX));
      CHECK(RightStick(GamepadAxis::kRightY));
    }

    SUBCASE("Returns false for other axes") {
      CHECK_FALSE(RightStick(GamepadAxis::kLeftY));
      CHECK_FALSE(RightStick(GamepadAxis::kRightTrigger));
    }
  }
}

TEST_SUITE("helios::input::GamepadAxisFilter") {
  TEST_CASE("helios::input::GamepadAxisFilter::ctor") {
    SUBCASE("Initializes trigger rest to -1") {
      const GamepadAxisFilter filter;
      CHECK_EQ(filter.raw.Get(GamepadAxis::kLeftTrigger),
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kRightTrigger)],
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.0F));
    }
  }

  TEST_CASE("helios::input::GamepadAxisFilter::Reset") {
    SUBCASE("Clears samples and restores default rest centers") {
      GamepadAxisFilter filter;
      filter.raw.Set(GamepadAxis::kLeftX, 0.5F);
      filter.center[static_cast<size_t>(GamepadAxis::kLeftX)] = 0.2F;
      filter.rest_frames[0] = 9;
      filter.seen[0] = 1;

      filter.Reset();

      CHECK_EQ(filter.raw.Get(GamepadAxis::kLeftX), doctest::Approx(0.0F));
      CHECK_EQ(filter.raw.Get(GamepadAxis::kLeftTrigger),
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftX)],
               doctest::Approx(0.0F));
      CHECK_EQ(filter.center[static_cast<size_t>(GamepadAxis::kLeftTrigger)],
               doctest::Approx(GamepadAxisFilter::kTriggerRest));
      CHECK_EQ(filter.rest_frames[0], 0);
      CHECK_EQ(filter.seen[0], 0);
    }
  }
}

TEST_SUITE("helios::input::ToString") {
  TEST_CASE("helios::input::ToString") {
    SUBCASE("Formats every defined gamepad button") {
      for (uint8_t i = 0; i <= std::to_underlying(GamepadButton::kCount); ++i) {
        const auto name = ToString(static_cast<GamepadButton>(i));
        CHECK_FALSE(name.empty());
        CHECK_NE(name, "unknown");
      }
    }

    SUBCASE("Formats representative gamepad buttons") {
      CHECK_EQ(ToString(GamepadButton::kA), "A");
      CHECK_EQ(ToString(GamepadButton::kStart), "Start");
      CHECK_EQ(ToString(GamepadButton::kDpadLeft), "DpadLeft");
      CHECK_EQ(ToString(GamepadButton::kCount), "Count");
    }

    SUBCASE("Formats an invalid gamepad button as unknown") {
      CHECK_EQ(ToString(static_cast<GamepadButton>(255)), "unknown");
    }

    SUBCASE("Formats every defined gamepad axis") {
      for (uint8_t i = 0; i <= std::to_underlying(GamepadAxis::kCount); ++i) {
        const auto name = ToString(static_cast<GamepadAxis>(i));
        CHECK_FALSE(name.empty());
        CHECK_NE(name, "unknown");
      }
    }

    SUBCASE("Formats representative gamepad axes") {
      CHECK_EQ(ToString(GamepadAxis::kLeftX), "LeftX");
      CHECK_EQ(ToString(GamepadAxis::kRightTrigger), "RightTrigger");
      CHECK_EQ(ToString(GamepadAxis::kCount), "Count");
    }

    SUBCASE("Formats an invalid gamepad axis as unknown") {
      CHECK_EQ(ToString(static_cast<GamepadAxis>(255)), "unknown");
    }
  }
}

TEST_SUITE("helios::input::operator<<") {
  TEST_CASE("helios::input::operator<<") {
    SUBCASE("Streams gamepad buttons") {
      std::ostringstream stream;
      stream << GamepadButton::kLeftBumper;
      CHECK_EQ(stream.str(), "GamepadButton::LeftBumper");
    }

    SUBCASE("Streams gamepad axes") {
      std::ostringstream stream;
      stream << GamepadAxis::kLeftTrigger;
      CHECK_EQ(stream.str(), "GamepadAxis::LeftTrigger");
    }
  }
}

TEST_SUITE("std::formatter") {
  TEST_CASE("std::formatter") {
    SUBCASE("Formats GamepadButton") {
      CHECK_EQ(std::format("{}", GamepadButton::kY), "GamepadButton::Y");
    }

    SUBCASE("Formats GamepadAxis") {
      CHECK_EQ(std::format("{}", GamepadAxis::kRightY), "GamepadAxis::RightY");
    }
  }
}
