#include <doctest/doctest.h>

#include <helios/window/properties.hpp>

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
