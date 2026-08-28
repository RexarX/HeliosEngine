#include <doctest/doctest.h>

#include <helios/window/settings.hpp>

using namespace helios::window;

TEST_SUITE("helios::window::ShouldRequestExitOnClose") {
  TEST_CASE("helios::window::ShouldRequestExitOnClose") {
    SUBCASE("Default triggers exit on primary close") {
      CHECK(ShouldRequestExitOnClose(kExitTriggersDefault, true, false));
    }

    SUBCASE("Default triggers exit on last window") {
      CHECK(ShouldRequestExitOnClose(kExitTriggersDefault, false, true));
    }

    SUBCASE("Default does not exit on auxiliary window close") {
      CHECK_FALSE(ShouldRequestExitOnClose(kExitTriggersDefault, false, false));
    }

    SUBCASE("Last-window preset ignores primary-only close") {
      CHECK_FALSE(
          ShouldRequestExitOnClose(kExitTriggersLastWindow, true, false));
      CHECK(ShouldRequestExitOnClose(kExitTriggersLastWindow, false, true));
    }

    SUBCASE("Primary preset ignores last-window-only close") {
      CHECK(ShouldRequestExitOnClose(kExitTriggersPrimary, true, false));
      CHECK_FALSE(ShouldRequestExitOnClose(kExitTriggersPrimary, false, true));
    }

    SUBCASE("Any-window trigger exits on every close") {
      CHECK(ShouldRequestExitOnClose(ExitTrigger::kAnyWindowClosed, false,
                                     false));
    }

    SUBCASE("None preset never exits") {
      CHECK_FALSE(ShouldRequestExitOnClose(kExitTriggersNone, true, true));
    }
  }
}

TEST_SUITE("helios::window::HasFlag") {
  TEST_CASE("helios::window::HasFlag") {
    SUBCASE("Detects a set exit trigger") {
      CHECK(HasFlag(kExitTriggersDefault, ExitTrigger::kPrimaryClosed));
      CHECK(HasFlag(kExitTriggersDefault, ExitTrigger::kAllWindowsClosed));
    }

    SUBCASE("Returns false for an unset exit trigger") {
      CHECK_FALSE(HasFlag(kExitTriggersPrimary, ExitTrigger::kAnyWindowClosed));
    }

    SUBCASE("kNone never matches a concrete flag") {
      CHECK_FALSE(HasFlag(kExitTriggersNone, ExitTrigger::kPrimaryClosed));
    }
  }
}
