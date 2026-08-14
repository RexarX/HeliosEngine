#include <doctest/doctest.h>

#include <helios/window/properties.hpp>

#include <format>
#include <sstream>
#include <string>

using namespace helios::window;

TEST_SUITE("helios::window::IsFullscreenPresentation") {
  TEST_CASE("helios::window::IsFullscreenPresentation") {
    SUBCASE("Windowed is not fullscreen") {
      CHECK_FALSE(IsFullscreenPresentation(Mode::kWindowed));
    }

    SUBCASE("Borderless is fullscreen") {
      CHECK(IsFullscreenPresentation(Mode::kBorderless));
    }

    SUBCASE("Exclusive fullscreen is fullscreen") {
      CHECK(IsFullscreenPresentation(Mode::kFullscreen));
    }
  }
}

TEST_SUITE("helios::window::HasFlag") {
  TEST_CASE("helios::window::HasFlag") {
    SUBCASE("Detects a set dirty flag") {
      CHECK(HasFlag(DirtyFlag::kTitle | DirtyFlag::kSize, DirtyFlag::kTitle));
    }

    SUBCASE("Returns false for an unset dirty flag") {
      CHECK_FALSE(HasFlag(DirtyFlag::kTitle, DirtyFlag::kSize));
    }

    SUBCASE("kNone never matches a concrete flag") {
      CHECK_FALSE(HasFlag(DirtyFlag::kNone, DirtyFlag::kTitle));
    }
  }
}

TEST_SUITE("helios::window::operator|") {
  TEST_CASE("helios::window::operator|") {
    SUBCASE("Combines dirty flags") {
      const DirtyFlag flags = DirtyFlag::kTitle | DirtyFlag::kSize;
      CHECK(HasFlag(flags, DirtyFlag::kTitle));
      CHECK(HasFlag(flags, DirtyFlag::kSize));
    }
  }
}

TEST_SUITE("helios::window::ToString") {
  TEST_CASE("helios::window::ToString") {
    SUBCASE("Formats presentation modes") {
      CHECK_EQ(ToString(Mode::kWindowed), "Windowed");
      CHECK_EQ(ToString(Mode::kBorderless), "Borderless");
      CHECK_EQ(ToString(Mode::kFullscreen), "Fullscreen");
      CHECK_EQ(ToString(static_cast<Mode>(255)), "unknown");
    }

    SUBCASE("Formats cursor modes") {
      CHECK_EQ(ToString(CursorMode::kVisible), "Visible");
      CHECK_EQ(ToString(CursorMode::kHidden), "Hidden");
      CHECK_EQ(ToString(CursorMode::kDisabled), "Disabled");
      CHECK_EQ(ToString(static_cast<CursorMode>(255)), "unknown");
    }

    SUBCASE("Formats client APIs") {
      CHECK_EQ(ToString(ClientApi::kNone), "None");
      CHECK_EQ(ToString(ClientApi::kOpenGL), "OpenGL");
      CHECK_EQ(ToString(static_cast<ClientApi>(255)), "unknown");
    }

    SUBCASE("Formats dirty flags without prefix") {
      CHECK_EQ(ToString(DirtyFlag::kNone), "None");
      CHECK_EQ(ToString(DirtyFlag::kTitle | DirtyFlag::kSize), "Title | Size");
      CHECK_EQ(ToString(DirtyFlag::kFocus | DirtyFlag::kMousePassthrough),
               "Focus | MousePassthrough");
      CHECK_EQ(ToString(DirtyFlag::kRefreshRate), "RefreshRate");
    }

    SUBCASE("Formats dirty flags with prefix") {
      CHECK_EQ(ToString(DirtyFlag::kNone, true), "DirtyFlag::None");
      CHECK_EQ(ToString(DirtyFlag::kTitle | DirtyFlag::kSize, true),
               "DirtyFlag::Title | DirtyFlag::Size");
    }

    SUBCASE("Formats properties") {
      const Properties properties{
          .title = "Test",
          .width = 800,
          .height = 600,
          .client_width = 790,
          .client_height = 560,
          .pos_x = 10,
          .pos_y = 20,
          .monitor_index = 0,
          .refresh_rate = 144,
          .min_width = 320,
          .min_height = 240,
          .max_width = 1920,
          .max_height = 1080,
          .aspect_numer = 16,
          .aspect_denom = 9,
          .mode = Mode::kBorderless,
          .cursor_mode = CursorMode::kHidden,
          .visible = false,
          .focused = false,
          .resizable = false,
          .decorated = false,
      };

      const auto formatted = ToString(properties);
      CHECK_NE(formatted.find("title=\"Test\""), std::string::npos);
      CHECK_NE(formatted.find("width=800"), std::string::npos);
      CHECK_NE(formatted.find("height=600"), std::string::npos);
      CHECK_NE(formatted.find("client_width=790"), std::string::npos);
      CHECK_NE(formatted.find("pos_x=10"), std::string::npos);
      CHECK_NE(formatted.find("refresh_rate=144"), std::string::npos);
      CHECK_NE(formatted.find("mode=Borderless"), std::string::npos);
      CHECK_NE(formatted.find("cursor_mode=Hidden"), std::string::npos);
      CHECK_NE(formatted.find("visible=false"), std::string::npos);
    }
  }
}

TEST_SUITE("helios::window::operator<<") {
  TEST_CASE("helios::window::operator<<") {
    SUBCASE("Streams presentation mode") {
      std::ostringstream stream;
      stream << Mode::kWindowed;
      CHECK_EQ(stream.str(), "Mode::Windowed");
    }

    SUBCASE("Streams cursor mode") {
      std::ostringstream stream;
      stream << CursorMode::kDisabled;
      CHECK_EQ(stream.str(), "CursorMode::Disabled");
    }

    SUBCASE("Streams client API") {
      std::ostringstream stream;
      stream << ClientApi::kNone;
      CHECK_EQ(stream.str(), "ClientApi::None");
    }

    SUBCASE("Streams dirty flags") {
      std::ostringstream stream;
      stream << (DirtyFlag::kTitle | DirtyFlag::kSize);
      CHECK_EQ(stream.str(), "Title | Size");
    }

    SUBCASE("Streams properties") {
      const Properties properties{.title = "Test", .width = 800, .height = 600};
      std::ostringstream stream;
      stream << properties;
      CHECK_EQ(stream.str(), ToString(properties));
    }
  }
}

TEST_SUITE("helios::window::DefaultSizeForScreen") {
  TEST_CASE("helios::window::DefaultSizeForScreen") {
    SUBCASE("Falls back when the screen size is zero") {
      const auto [width, height] = DefaultSizeForScreen(0, 0);
      CHECK_EQ(width, 1280U);
      CHECK_EQ(height, 720U);
    }

    SUBCASE("Uses two thirds of a landscape monitor") {
      const auto [width, height] = DefaultSizeForScreen(1920, 1080);
      CHECK_EQ(width, 1280U);
      CHECK_EQ(height, 720U);
    }

    SUBCASE("Adapts to a portrait monitor") {
      const auto [width, height] = DefaultSizeForScreen(1080, 1920);
      CHECK_LT(width, 1080U);
      CHECK_LT(height, 1920U);
      CHECK_GE(width, 320U);
      CHECK_GE(height, 320U);
    }

    SUBCASE("Clamps to the screen on smaller landscape displays") {
      const auto [width, height] = DefaultSizeForScreen(800, 600);
      CHECK_LE(width, 800U);
      CHECK_LE(height, 600U);
      CHECK_GE(width, 320U);
    }
  }
}

TEST_SUITE("helios::window::ResolveExclusiveVideoMode") {
  TEST_CASE("helios::window::ResolveExclusiveVideoMode") {
    SUBCASE("Unset fields use the desktop video mode") {
      const auto requested =
          ResolveExclusiveVideoMode(Properties{}, ExclusiveVideoMode{
                                                      .width = 1920,
                                                      .height = 1080,
                                                      .refresh_rate = 60,
                                                  });
      CHECK_EQ(requested.width, 1920U);
      CHECK_EQ(requested.height, 1080U);
      CHECK_EQ(requested.refresh_rate, 60U);
    }

    SUBCASE("Explicit size and refresh override independently") {
      const auto requested = ResolveExclusiveVideoMode(
          Properties{
              .width = 1280,
              .height = 720,
              .refresh_rate = 144,
          },
          ExclusiveVideoMode{
              .width = 1920,
              .height = 1080,
              .refresh_rate = 60,
          });
      CHECK_EQ(requested.width, 1280U);
      CHECK_EQ(requested.height, 720U);
      CHECK_EQ(requested.refresh_rate, 144U);
    }

    SUBCASE("Hz-only keeps the desktop resolution") {
      const auto requested = ResolveExclusiveVideoMode(
          Properties{.refresh_rate = 144}, ExclusiveVideoMode{
                                               .width = 2560,
                                               .height = 1440,
                                               .refresh_rate = 60,
                                           });
      CHECK_EQ(requested.width, 2560U);
      CHECK_EQ(requested.height, 1440U);
      CHECK_EQ(requested.refresh_rate, 144U);
    }
  }
}

TEST_SUITE("std::formatter") {
  TEST_CASE("std::formatter") {
    SUBCASE("Formats Mode") {
      CHECK_EQ(std::format("{}", Mode::kWindowed), "Mode::Windowed");
    }

    SUBCASE("Formats ClientApi") {
      CHECK_EQ(std::format("{}", ClientApi::kOpenGL), "ClientApi::OpenGL");
    }

    SUBCASE("Formats CursorMode") {
      CHECK_EQ(std::format("{}", CursorMode::kDisabled),
               "CursorMode::Disabled");
    }

    SUBCASE("Formats DirtyFlag with prefix") {
      CHECK_EQ(std::format("{}", DirtyFlag::kNone), "DirtyFlag::None");
      CHECK_EQ(std::format("{}", DirtyFlag::kTitle | DirtyFlag::kSize),
               "DirtyFlag::Title | DirtyFlag::Size");
    }

    SUBCASE("Formats Properties") {
      const Properties properties{.title = "Test"};
      CHECK_EQ(std::format("{}", properties), ToString(properties));
    }
  }
}
