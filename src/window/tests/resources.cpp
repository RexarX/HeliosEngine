#include <doctest/doctest.h>

#include <helios/window/resources.hpp>

using namespace helios::window;

TEST_SUITE("helios::window::HasFlag") {
  TEST_CASE("helios::window::HasFlag") {
    SUBCASE("Detects a set exit trigger") {
      CHECK(HasFlag(kExitTriggersDefault, ExitTrigger::kPrimaryClosed));
      CHECK(HasFlag(kExitTriggersDefault, ExitTrigger::kAllWindowsClosed));
    }

    SUBCASE("Returns false for an unset exit trigger") {
      CHECK_FALSE(HasFlag(kExitTriggersPrimary, ExitTrigger::kAnyWindowClosed));
    }
  }
}

TEST_SUITE("helios::window::ShouldRequestExitOnClose") {
  TEST_CASE("helios::window::ShouldRequestExitOnClose") {
    SUBCASE("Default triggers exit on primary close") {
      CHECK(ShouldRequestExitOnClose(kExitTriggersDefault, true, false));
    }

    SUBCASE("Default triggers exit on last window") {
      CHECK(ShouldRequestExitOnClose(kExitTriggersDefault, false, true));
    }

    SUBCASE("Default does not exit on an auxiliary window close") {
      CHECK_FALSE(ShouldRequestExitOnClose(kExitTriggersDefault, false, false));
    }

    SUBCASE("Last-window preset ignores a non-last primary close") {
      CHECK_FALSE(
          ShouldRequestExitOnClose(kExitTriggersLastWindow, true, false));
      CHECK(ShouldRequestExitOnClose(kExitTriggersLastWindow, false, true));
    }

    SUBCASE("Primary preset ignores a last non-primary close") {
      CHECK(ShouldRequestExitOnClose(kExitTriggersPrimary, true, false));
      CHECK_FALSE(ShouldRequestExitOnClose(kExitTriggersPrimary, false, true));
    }

    SUBCASE("Any-window trigger exits on every close") {
      CHECK(ShouldRequestExitOnClose(ExitTrigger::kAnyWindowClosed, false,
                                     false));
      CHECK(
          ShouldRequestExitOnClose(ExitTrigger::kAnyWindowClosed, true, true));
    }

    SUBCASE("None preset never exits") {
      CHECK_FALSE(ShouldRequestExitOnClose(kExitTriggersNone, true, true));
    }
  }
}
