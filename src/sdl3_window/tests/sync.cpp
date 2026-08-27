#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/world.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/settings.hpp>

#include "available.hpp"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

using namespace helios;
using namespace helios::sdl3::window;

namespace {

struct ScopedSdlVideo {
  ScopedSdlVideo() { sdl3::Retain(SDL_INIT_VIDEO); }
  ~ScopedSdlVideo() { sdl3::Release(SDL_INIT_VIDEO); }
};

[[nodiscard]] SDL_Window* CreateHiddenSdlWindow(
    const char* title = "HeliosSync") {
  return SDL_CreateWindow(title, 64, 48, SDL_WINDOW_HIDDEN);
}

}  // namespace

TEST_SUITE("helios::sdl3::window::DisplayAtIndex") {
  TEST_CASE("helios::sdl3::window::DisplayAtIndex") {
    SUBCASE("Returns the primary display at index 0") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;
      HELIOS_SKIP_IF_NO_DISPLAY();

      CHECK_NE(DisplayAtIndex(0), 0U);
      CHECK_EQ(DisplayAtIndex(10'000), 0U);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ResolveDisplay") {
  TEST_CASE("helios::sdl3::window::ResolveDisplay") {
    SUBCASE("Uses an explicit monitor index when it is valid") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;
      HELIOS_SKIP_IF_NO_DISPLAY();

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      window::Properties properties{.monitor_index = 0};
      CHECK_EQ(ResolveDisplay(properties, *sdl_window), DisplayAtIndex(0));

      SDL_DestroyWindow(sdl_window);
    }

    SUBCASE("Falls back to the window display when the index is invalid") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;
      HELIOS_SKIP_IF_NO_DISPLAY();

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      window::Properties properties{.monitor_index = 10'000};
      CHECK_NE(ResolveDisplay(properties, *sdl_window), 0U);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::RefreshMonitors") {
  TEST_CASE("helios::sdl3::window::RefreshMonitors") {
    SUBCASE("Populates the monitors resource from SDL") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;
      HELIOS_SKIP_IF_NO_DISPLAY();

      window::Monitors monitors;
      RefreshMonitors(monitors);

      CHECK_FALSE(monitors.monitors.empty());
      CHECK_GE(monitors.monitors[0].width, 1U);
      CHECK_GE(monitors.monitors[0].height, 1U);
      CHECK_FALSE(monitors.monitors[0].modes.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::window::MarkMonitorDependentDirty") {
  TEST_CASE("helios::sdl3::window::MarkMonitorDependentDirty") {
    SUBCASE("Marks monitor dirty when a window pins a monitor index") {
      ecs::World world;
      NativeWindows native;
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          window::Window::FromProperties(window::Properties{
                              .monitor_index = 0,
                              .mode = window::Mode::kWindowed,
                          }));
      native.Insert(entity, {});

      MarkMonitorDependentDirty(world, native);

      const auto& window = world.ReadComponent<window::Window>(entity);
      CHECK(window.Dirty(window::DirtyFlag::kMonitor));
      CHECK_FALSE(window.Dirty(window::DirtyFlag::kMode));
    }

    SUBCASE("Marks mode dirty for fullscreen presentation") {
      ecs::World world;
      NativeWindows native;
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          window::Window::FromProperties(window::Properties{
                              .mode = window::Mode::kFullscreen,
                          }));
      native.Insert(entity, {});

      MarkMonitorDependentDirty(world, native);

      const auto& window = world.ReadComponent<window::Window>(entity);
      CHECK(window.Dirty(window::DirtyFlag::kMode));
      CHECK_FALSE(window.Dirty(window::DirtyFlag::kMonitor));
    }

    SUBCASE("Skips entities without a window::Window component") {
      ecs::World world;
      NativeWindows native;
      const ecs::Entity entity = world.CreateEntity();
      native.Insert(entity, {});

      MarkMonitorDependentDirty(world, native);
      CHECK_FALSE(world.HasComponent<window::Window>(entity));
    }
  }
}

TEST_SUITE("helios::sdl3::window::BuildCreationFlags") {
  TEST_CASE("helios::sdl3::window::BuildCreationFlags") {
    SUBCASE("Sets hidden, borderless, resizable, and OpenGL bits") {
      auto window = window::Window::FromProperties(window::Properties{
          .client_api = window::ClientApi::kOpenGL,
          .visible = false,
          .resizable = true,
          .decorated = false,
          .maximized = true,
          .floating = true,
          .transparent_framebuffer = true,
          .scale_to_monitor = true,
      });

      const SDL_WindowFlags flags = BuildCreationFlags(window);
      CHECK_NE(flags & SDL_WINDOW_HIDDEN, 0U);
      CHECK_NE(flags & SDL_WINDOW_BORDERLESS, 0U);
      CHECK_NE(flags & SDL_WINDOW_RESIZABLE, 0U);
      CHECK_NE(flags & SDL_WINDOW_ALWAYS_ON_TOP, 0U);
      CHECK_NE(flags & SDL_WINDOW_MAXIMIZED, 0U);
      CHECK_NE(flags & SDL_WINDOW_TRANSPARENT, 0U);
      CHECK_NE(flags & SDL_WINDOW_HIGH_PIXEL_DENSITY, 0U);
      CHECK_NE(flags & SDL_WINDOW_OPENGL, 0U);
    }

    SUBCASE("Omits hidden and OpenGL for a visible no-API window") {
      auto window = window::Window::FromProperties(window::Properties{
          .client_api = window::ClientApi::kNone,
          .visible = true,
          .resizable = false,
          .decorated = true,
      });

      const SDL_WindowFlags flags = BuildCreationFlags(window);
      CHECK_EQ(flags & SDL_WINDOW_HIDDEN, 0U);
      CHECK_EQ(flags & SDL_WINDOW_OPENGL, 0U);
      CHECK_EQ(flags & SDL_WINDOW_BORDERLESS, 0U);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyCursorMode") {
  TEST_CASE("helios::sdl3::window::ApplyCursorMode") {
    SUBCASE("Applies captured and relative cursor modes") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyCursorMode(*sdl_window, window::CursorMode::kDisabled);
      CHECK(SDL_GetWindowRelativeMouseMode(sdl_window));

      ApplyCursorMode(*sdl_window, window::CursorMode::kCaptured);
      CHECK_FALSE(SDL_GetWindowRelativeMouseMode(sdl_window));
      if (!SDL_GetWindowMouseGrab(sdl_window)) {
        MESSAGE("Mouse grab was not applied on a hidden window; continuing");
      }

      ApplyCursorMode(*sdl_window, window::CursorMode::kVisible);
      CHECK_FALSE(SDL_GetWindowRelativeMouseMode(sdl_window));
      CHECK_FALSE(SDL_GetWindowMouseGrab(sdl_window));

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ResolveCreationSize") {
  TEST_CASE("helios::sdl3::window::ResolveCreationSize") {
    SUBCASE("Leaves an explicit size unchanged") {
      auto window = window::Window::FromProperties(window::Properties{
          .width = 64,
          .height = 48,
      });
      ResolveCreationSize(window);
      CHECK_EQ(*window.properties.width, 64U);
      CHECK_EQ(*window.properties.height, 48U);
    }

    SUBCASE("Fills missing dimensions from the primary display") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;
      HELIOS_SKIP_IF_NO_DISPLAY();

      auto window = window::Window::FromProperties(window::Properties{});
      ResolveCreationSize(window);

      CHECK(window.properties.width.has_value());
      CHECK(window.properties.height.has_value());
      CHECK_GE(*window.properties.width, 1U);
      CHECK_GE(*window.properties.height, 1U);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyPresentationMode") {
  TEST_CASE("helios::sdl3::window::ApplyPresentationMode") {
    SUBCASE("Restores a windowed decorated window") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      auto window = window::Window::FromProperties(window::Properties{
          .width = 64,
          .height = 48,
          .mode = window::Mode::kWindowed,
          .visible = false,
          .resizable = true,
          .decorated = true,
      });
      ApplyPresentationMode(*sdl_window, window);

      const SDL_WindowFlags flags = SDL_GetWindowFlags(sdl_window);
      CHECK_EQ(flags & SDL_WINDOW_FULLSCREEN, 0U);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyMonitorPlacement") {
  TEST_CASE("helios::sdl3::window::ApplyMonitorPlacement") {
    SUBCASE("Writes the resolved position back onto the window") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;
      HELIOS_SKIP_IF_NO_DISPLAY();

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      auto window = window::Window::FromProperties(window::Properties{
          .width = 64,
          .height = 48,
          .pos_x = 12,
          .pos_y = 24,
      });
      ApplyMonitorPlacement(*sdl_window, window);

      REQUIRE(window.properties.pos_x.has_value());
      REQUIRE(window.properties.pos_y.has_value());
      CHECK_EQ(*window.properties.pos_x, 12);
      CHECK_EQ(*window.properties.pos_y, 24);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::SyncWindowGeometry") {
  TEST_CASE("helios::sdl3::window::SyncWindowGeometry") {
    SUBCASE("Copies SDL size and position into window::Window properties") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      auto window = window::Window::FromProperties(window::Properties{});
      SyncWindowGeometry(window, *sdl_window);

      CHECK(window.properties.width.has_value());
      CHECK(window.properties.height.has_value());
      CHECK(window.properties.client_width.has_value());
      CHECK(window.properties.client_height.has_value());
      CHECK(window.properties.pos_x.has_value());
      CHECK(window.properties.pos_y.has_value());
      CHECK_GT(window.properties.content_scale_x, 0.0F);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyWindowIcons") {
  TEST_CASE("helios::sdl3::window::ApplyWindowIcons") {
    SUBCASE("Accepts an empty icon list") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyWindowIcons(*sdl_window, {});
      SDL_DestroyWindow(sdl_window);
    }

    SUBCASE("Creates a surface from a valid RGBA icon") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      window::IconImage icon{
          .width = 2,
          .height = 2,
          .rgba = std::vector<uint8_t>(16, 255),
      };
      ApplyWindowIcons(*sdl_window,
                       std::span<const window::IconImage>(&icon, 1));
      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyMaximized") {
  TEST_CASE("helios::sdl3::window::ApplyMaximized") {
    SUBCASE("Restores a hidden window") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyMaximized(*sdl_window, false);
      CHECK_EQ(SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_MAXIMIZED, 0U);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplySizeLimits") {
  TEST_CASE("helios::sdl3::window::ApplySizeLimits") {
    SUBCASE("Applies minimum and maximum client sizes") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      window::Properties properties{
          .min_width = 32,
          .min_height = 16,
          .max_width = 128,
          .max_height = 96,
      };
      ApplySizeLimits(*sdl_window, properties);

      int min_w = 0;
      int min_h = 0;
      int max_w = 0;
      int max_h = 0;
      SDL_GetWindowMinimumSize(sdl_window, &min_w, &min_h);
      SDL_GetWindowMaximumSize(sdl_window, &max_w, &max_h);
      CHECK_EQ(min_w, 32);
      CHECK_EQ(min_h, 16);
      CHECK_EQ(max_w, 128);
      CHECK_EQ(max_h, 96);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyAspectRatio") {
  TEST_CASE("helios::sdl3::window::ApplyAspectRatio") {
    SUBCASE("Sets and clears a locked aspect ratio") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      window::Properties locked{.aspect_numer = 16, .aspect_denom = 9};
      ApplyAspectRatio(*sdl_window, locked);

      float min_aspect = 0.0F;
      float max_aspect = 0.0F;
      SDL_GetWindowAspectRatio(sdl_window, &min_aspect, &max_aspect);
      CHECK_EQ(min_aspect, doctest::Approx(16.0F / 9.0F));

      ApplyAspectRatio(*sdl_window, window::Properties{});
      SDL_GetWindowAspectRatio(sdl_window, &min_aspect, &max_aspect);
      CHECK_EQ(min_aspect, doctest::Approx(0.0F));

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyOpacity") {
  TEST_CASE("helios::sdl3::window::ApplyOpacity") {
    SUBCASE("Writes the SDL window opacity") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyOpacity(*sdl_window, 0.5F);
      CHECK_EQ(SDL_GetWindowOpacity(sdl_window), doctest::Approx(0.5F));

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyFloating") {
  TEST_CASE("helios::sdl3::window::ApplyFloating") {
    SUBCASE("Toggles the always-on-top flag") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyFloating(*sdl_window, true);
      CHECK_NE(SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_ALWAYS_ON_TOP, 0U);

      ApplyFloating(*sdl_window, false);
      CHECK_EQ(SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_ALWAYS_ON_TOP, 0U);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyAutoIconify") {
  TEST_CASE("helios::sdl3::window::ApplyAutoIconify") {
    SUBCASE("Writes the minimize-on-focus-loss hint") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyAutoIconify(*sdl_window, false);
      const char* off = SDL_GetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
      REQUIRE_NE(off, nullptr);
      CHECK_EQ(std::string(off), "0");

      ApplyAutoIconify(*sdl_window, true);
      const char* on = SDL_GetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
      REQUIRE_NE(on, nullptr);
      CHECK_EQ(std::string(on), "1");

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyFocusOnShow") {
  TEST_CASE("helios::sdl3::window::ApplyFocusOnShow") {
    SUBCASE("Writes the activate-when-shown hint") {
      ApplyFocusOnShow(false);
      const char* off = SDL_GetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN);
      REQUIRE_NE(off, nullptr);
      CHECK_EQ(std::string(off), "0");

      ApplyFocusOnShow(true);
      const char* on = SDL_GetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN);
      REQUIRE_NE(on, nullptr);
      CHECK_EQ(std::string(on), "1");
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyMousePassthrough") {
  TEST_CASE("helios::sdl3::window::ApplyMousePassthrough") {
    SUBCASE("Toggles click-through on a native window") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ApplyMousePassthrough(*sdl_window, true);
      ApplyMousePassthrough(*sdl_window, false);

      SDL_DestroyWindow(sdl_window);
    }
  }
}

TEST_SUITE("helios::sdl3::window::SyncClipboard") {
  TEST_CASE("helios::sdl3::window::SyncClipboard") {
    SUBCASE("Writes pending clipboard text through SDL") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      ScopedSdlVideo video;

      SDL_Window* sdl_window = CreateHiddenSdlWindow();
      REQUIRE_NE(sdl_window, nullptr);

      ecs::World world;
      world.InsertResources(window::Clipboard{});
      auto& clipboard = world.WriteResource<window::Clipboard>();
      clipboard.text = "helios-clipboard";
      clipboard.pending_write = true;

      NativeWindows native;
      native.Insert(ecs::Entity{1, 1}, NativeEntry{.window = sdl_window});
      Context context;

      SyncClipboard(world, native, context);

      CHECK_FALSE(world.ReadResource<window::Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<window::Clipboard>().text,
               "helios-clipboard");

      SDL_DestroyWindow(sdl_window);
    }

    SUBCASE("Returns when the clipboard resource is missing") {
      ecs::World world;
      NativeWindows native;
      Context context;
      SyncClipboard(world, native, context);
    }
  }
}
#endif
