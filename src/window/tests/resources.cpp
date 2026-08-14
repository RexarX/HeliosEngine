#include <doctest/doctest.h>

#include <helios/window/resources.hpp>

#include <format>
#include <sstream>
#include <string>

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

TEST_SUITE("helios::window::operator|") {
  TEST_CASE("helios::window::operator|") {
    SUBCASE("Combines exit triggers") {
      const auto triggers =
          ExitTrigger::kPrimaryClosed | ExitTrigger::kAnyWindowClosed;
      CHECK(HasFlag(triggers, ExitTrigger::kPrimaryClosed));
      CHECK(HasFlag(triggers, ExitTrigger::kAnyWindowClosed));
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

TEST_SUITE("helios::window::ToString") {
  TEST_CASE("helios::window::ToString") {
    SUBCASE("Formats exit triggers without prefix") {
      CHECK_EQ(ToString(ExitTrigger::kNone), "None");
      CHECK_EQ(ToString(kExitTriggersPrimary), "PrimaryClosed");
      CHECK_EQ(ToString(kExitTriggersLastWindow), "AllWindowsClosed");
      CHECK_EQ(ToString(kExitTriggersDefault),
               "PrimaryClosed | AllWindowsClosed");
      CHECK_EQ(ToString(ExitTrigger::kAnyWindowClosed), "AnyWindowClosed");
    }

    SUBCASE("Formats exit triggers with prefix") {
      CHECK_EQ(ToString(kExitTriggersNone, true), "ExitTrigger::None");
      CHECK_EQ(ToString(kExitTriggersDefault, true),
               "ExitTrigger::PrimaryClosed | ExitTrigger::AllWindowsClosed");
    }

    SUBCASE("Formats event modes") {
      CHECK_EQ(ToString(EventMode::kPoll), "Poll");
      CHECK_EQ(ToString(EventMode::kWaitTimeout), "WaitTimeout");
      CHECK_EQ(ToString(static_cast<EventMode>(255)), "unknown");
    }

    SUBCASE("Formats monitor events") {
      CHECK_EQ(ToString(MonitorEvent::kConnected), "Connected");
      CHECK_EQ(ToString(MonitorEvent::kDisconnected), "Disconnected");
      CHECK_EQ(ToString(static_cast<MonitorEvent>(255)), "unknown");
    }

    SUBCASE("Formats video modes") {
      CHECK_EQ(
          ToString(VideoMode{.width = 800, .height = 600, .refresh_rate = 144}),
          "VideoMode{width=800, height=600, refresh_rate=144}");
    }

    SUBCASE("Formats monitors") {
      const Monitor monitor{
          .name = "Test Display",
          .modes = {VideoMode{
                        .width = 1920, .height = 1080, .refresh_rate = 60},
                    VideoMode{
                        .width = 1280, .height = 720, .refresh_rate = 60}},
          .index = 1,
          .x = 100,
          .y = 50,
          .width = 1920,
          .height = 1080,
          .work_x = 0,
          .work_y = 0,
          .work_width = 1920,
          .work_height = 1040,
          .physical_width_mm = 600,
          .physical_height_mm = 340,
          .current =
              VideoMode{.width = 1920, .height = 1080, .refresh_rate = 60},
          .primary = true,
      };

      const auto formatted = ToString(monitor);
      CHECK_NE(formatted.find("name=\"Test Display\""), std::string::npos);
      CHECK_NE(formatted.find("modes=2"), std::string::npos);
      CHECK_NE(formatted.find("refresh_rate=60"), std::string::npos);
      CHECK_NE(formatted.find("primary=true"), std::string::npos);
    }
  }
}

TEST_SUITE("helios::window::operator<<") {
  TEST_CASE("helios::window::operator<<") {
    SUBCASE("Streams exit triggers") {
      std::ostringstream stream;
      stream << kExitTriggersPrimary;
      CHECK_EQ(stream.str(), "PrimaryClosed");
    }

    SUBCASE("Streams event modes") {
      std::ostringstream stream;
      stream << EventMode::kWaitTimeout;
      CHECK_EQ(stream.str(), "EventMode::WaitTimeout");
    }

    SUBCASE("Streams monitor events") {
      std::ostringstream stream;
      stream << MonitorEvent::kConnected;
      CHECK_EQ(stream.str(), "MonitorEvent::Connected");
    }

    SUBCASE("Streams video modes") {
      const VideoMode mode{.width = 800, .height = 600, .refresh_rate = 60};
      std::ostringstream stream;
      stream << mode;
      CHECK_EQ(stream.str(), ToString(mode));
    }

    SUBCASE("Streams monitors") {
      const Monitor monitor{.name = "Test Display"};
      std::ostringstream stream;
      stream << monitor;
      CHECK_EQ(stream.str(), ToString(monitor));
    }
  }
}

TEST_SUITE("std::formatter") {
  TEST_CASE("std::formatter") {
    SUBCASE("Formats ExitTrigger with prefix") {
      CHECK_EQ(std::format("{}", kExitTriggersDefault),
               "ExitTrigger::PrimaryClosed | ExitTrigger::AllWindowsClosed");
    }

    SUBCASE("Formats EventMode") {
      CHECK_EQ(std::format("{}", EventMode::kPoll), "EventMode::Poll");
    }

    SUBCASE("Formats MonitorEvent") {
      CHECK_EQ(std::format("{}", MonitorEvent::kConnected),
               "MonitorEvent::Connected");
    }

    SUBCASE("Formats VideoMode") {
      CHECK_EQ(std::format(
                   "{}",
                   VideoMode{.width = 800, .height = 600, .refresh_rate = 144}),
               "VideoMode{width=800, height=600, refresh_rate=144}");
    }

    SUBCASE("Formats Monitor") {
      const Monitor monitor{.name = "Test Display"};
      CHECK_EQ(std::format("{}", monitor), ToString(monitor));
    }
  }
}
