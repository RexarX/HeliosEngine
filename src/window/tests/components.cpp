#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/window/components.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace helios;
using namespace helios::window;

TEST_SUITE("helios::window::Window") {
  TEST_CASE("helios::window::Window::FromProperties") {
    SUBCASE("Copies properties and starts with no dirty flags") {
      const auto window = Window::FromProperties(Properties{.title = "Test"});

      CHECK_EQ(window.properties.title, "Test");
      CHECK_EQ(window.dirty_flags, DirtyFlag::kNone);
      CHECK_FALSE(window.close_requested);
    }

    SUBCASE("Preserves background creation defaults") {
      const auto window = Window::FromProperties(Properties{
          .visible = false,
          .focused = false,
      });

      CHECK_FALSE(window.properties.visible);
      CHECK_FALSE(window.properties.focused);
    }
  }

  TEST_CASE("helios::window::Window::RequestClose") {
    SUBCASE("Sets close_requested without dirty flags") {
      auto window = Window::FromProperties(Properties{});
      window.RequestClose();

      CHECK(window.close_requested);
      CHECK_EQ(window.dirty_flags, DirtyFlag::kNone);
    }
  }

  TEST_CASE("helios::window::Window::MarkDirty") {
    SUBCASE("Sets a single dirty flag") {
      auto window = Window::FromProperties(Properties{});
      window.MarkDirty(DirtyFlag::kTitle);

      CHECK(window.Dirty(DirtyFlag::kTitle));
      CHECK_FALSE(window.Dirty(DirtyFlag::kSize));
    }

    SUBCASE("Combines additional dirty flags") {
      auto window = Window::FromProperties(Properties{});
      window.MarkDirty(DirtyFlag::kTitle);
      window.MarkDirty(DirtyFlag::kSize | DirtyFlag::kPos);

      CHECK(window.Dirty(DirtyFlag::kTitle));
      CHECK(window.Dirty(DirtyFlag::kSize));
      CHECK(window.Dirty(DirtyFlag::kPos));
    }
  }

  TEST_CASE("helios::window::Window::ClearDirty") {
    SUBCASE("Clears selected flags and leaves others") {
      auto window = Window::FromProperties(Properties{});
      window.MarkDirty(DirtyFlag::kTitle | DirtyFlag::kSize | DirtyFlag::kPos);
      window.ClearDirty(DirtyFlag::kSize);

      CHECK(window.Dirty(DirtyFlag::kTitle));
      CHECK_FALSE(window.Dirty(DirtyFlag::kSize));
      CHECK(window.Dirty(DirtyFlag::kPos));
    }

    SUBCASE("Clears all flags") {
      auto window = Window::FromProperties(Properties{});
      window.MarkDirty(DirtyFlag::kTitle | DirtyFlag::kFocus);
      window.ClearDirty();

      CHECK_EQ(window.dirty_flags, DirtyFlag::kNone);
      CHECK_FALSE(window.Dirty(DirtyFlag::kTitle));
    }
  }

  TEST_CASE("helios::window::Window::Dirty") {
    SUBCASE("Returns false when the flag is unset") {
      const auto window = Window::FromProperties(Properties{});
      CHECK_FALSE(window.Dirty(DirtyFlag::kTitle));
    }

    SUBCASE("Returns true when the flag is set") {
      auto window = Window::FromProperties(Properties{});
      window.MarkDirty(DirtyFlag::kOpacity);
      CHECK(window.Dirty(DirtyFlag::kOpacity));
    }
  }

  TEST_CASE("helios::window::Window::SetTitle") {
    SUBCASE("Updates title and marks title dirty") {
      auto window = Window::FromProperties(Properties{.title = "Old"});
      window.SetTitle("New");

      CHECK_EQ(window.properties.title, "New");
      CHECK(window.Dirty(DirtyFlag::kTitle));
    }
  }

  TEST_CASE("helios::window::Window::SetSize") {
    SUBCASE("Updates size and marks size dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetSize(800, 600);

      CHECK(window.properties.width.has_value());
      CHECK(window.properties.height.has_value());
      CHECK_EQ(*window.properties.width, 800U);
      CHECK_EQ(*window.properties.height, 600U);
      CHECK(window.Dirty(DirtyFlag::kSize));
    }
  }

  TEST_CASE("helios::window::Window::SetPos") {
    SUBCASE("Updates position and marks pos dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetPos(32, 48);

      CHECK(window.properties.pos_x.has_value());
      CHECK(window.properties.pos_y.has_value());
      CHECK_EQ(*window.properties.pos_x, 32);
      CHECK_EQ(*window.properties.pos_y, 48);
      CHECK(window.Dirty(DirtyFlag::kPos));
    }
  }

  TEST_CASE("helios::window::Window::ClearPos") {
    SUBCASE("Clears an explicit position and marks pos dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetPos(10, 20);
      window.ClearDirty();
      window.ClearPos();

      CHECK_FALSE(window.properties.pos_x.has_value());
      CHECK_FALSE(window.properties.pos_y.has_value());
      CHECK(window.Dirty(DirtyFlag::kPos));
    }
  }

  TEST_CASE("helios::window::Window::SetMonitorIndex") {
    SUBCASE("Stores the monitor index and marks monitor dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMonitorIndex(1);

      CHECK(window.properties.monitor_index.has_value());
      CHECK_EQ(*window.properties.monitor_index, 1);
      CHECK(window.Dirty(DirtyFlag::kMonitor));
    }
  }

  TEST_CASE("helios::window::Window::ClearMonitorIndex") {
    SUBCASE("Clears the monitor index and marks monitor dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMonitorIndex(0);
      window.ClearDirty();
      window.ClearMonitorIndex();

      CHECK_FALSE(window.properties.monitor_index.has_value());
      CHECK(window.Dirty(DirtyFlag::kMonitor));
    }
  }

  TEST_CASE("helios::window::Window::SetMode") {
    SUBCASE("Updates presentation mode and marks mode dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMode(Mode::kBorderless);

      CHECK_EQ(window.properties.mode, Mode::kBorderless);
      CHECK(window.Dirty(DirtyFlag::kMode));
    }
  }

  TEST_CASE("helios::window::Window::SetRefreshRate") {
    SUBCASE("Stores the refresh rate and marks refresh dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetRefreshRate(144);

      CHECK(window.properties.refresh_rate.has_value());
      CHECK_EQ(*window.properties.refresh_rate, 144U);
      CHECK(window.Dirty(DirtyFlag::kRefreshRate));
    }
  }

  TEST_CASE("helios::window::Window::ClearRefreshRate") {
    SUBCASE("Clears the refresh rate and marks refresh dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetRefreshRate(120);
      window.ClearDirty();
      window.ClearRefreshRate();

      CHECK_FALSE(window.properties.refresh_rate.has_value());
      CHECK(window.Dirty(DirtyFlag::kRefreshRate));
    }
  }

  TEST_CASE("helios::window::Window::SetVideoMode") {
    SUBCASE("Applies size and refresh without changing presentation mode") {
      auto window = Window::FromProperties(Properties{});
      window.SetVideoMode(VideoMode{
          .width = 1920,
          .height = 1080,
          .refresh_rate = 144,
      });

      CHECK(window.properties.width.has_value());
      CHECK(window.properties.height.has_value());
      CHECK_EQ(*window.properties.width, 1920U);
      CHECK_EQ(*window.properties.height, 1080U);
      CHECK(window.properties.refresh_rate.has_value());
      CHECK_EQ(*window.properties.refresh_rate, 144U);
      CHECK_EQ(window.properties.mode, Mode::kWindowed);
      CHECK_EQ(window.dirty_flags, DirtyFlag::kSize | DirtyFlag::kRefreshRate);
      CHECK_FALSE(window.Dirty(DirtyFlag::kMode));
    }

    SUBCASE("Leaves an existing presentation mode unchanged") {
      auto window = Window::FromProperties(Properties{
          .mode = Mode::kFullscreen,
      });
      window.SetVideoMode(VideoMode{
          .width = 1280,
          .height = 720,
          .refresh_rate = 60,
      });

      CHECK_EQ(window.properties.mode, Mode::kFullscreen);
      CHECK_FALSE(window.Dirty(DirtyFlag::kMode));
    }
  }

  TEST_CASE("helios::window::Window::SetCursorMode") {
    SUBCASE("Updates cursor mode and marks cursor dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetCursorMode(CursorMode::kDisabled);

      CHECK_EQ(window.properties.cursor_mode, CursorMode::kDisabled);
      CHECK(window.Dirty(DirtyFlag::kCursor));
    }

    SUBCASE("Sets captured cursor mode") {
      auto window = Window::FromProperties(Properties{});
      window.SetCursorMode(CursorMode::kCaptured);

      CHECK_EQ(window.properties.cursor_mode, CursorMode::kCaptured);
      CHECK(window.Dirty(DirtyFlag::kCursor));
    }
  }

  TEST_CASE("helios::window::Window::SetVisible") {
    SUBCASE("Updates visibility and marks visible dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetVisible(false);

      CHECK_FALSE(window.properties.visible);
      CHECK(window.Dirty(DirtyFlag::kVisible));
    }
  }

  TEST_CASE("helios::window::Window::SetResizable") {
    SUBCASE("Updates resizable and marks resizable dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetResizable(false);

      CHECK_FALSE(window.properties.resizable);
      CHECK(window.Dirty(DirtyFlag::kResizable));
    }
  }

  TEST_CASE("helios::window::Window::SetDecorated") {
    SUBCASE("Updates decorated and marks decorated dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetDecorated(false);

      CHECK_FALSE(window.properties.decorated);
      CHECK(window.Dirty(DirtyFlag::kDecorated));
    }
  }

  TEST_CASE("helios::window::Window::SetIcons") {
    SUBCASE("Stores icons and marks icon dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetIcons({IconImage{.width = 16, .height = 16, .rgba = {}}});

      CHECK_EQ(window.properties.icons.size(), 1U);
      CHECK_EQ(window.properties.icons[0].width, 16U);
      CHECK(window.Dirty(DirtyFlag::kIcon));
    }
  }

  TEST_CASE("helios::window::Window::SetMaximized") {
    SUBCASE("Updates maximized and marks maximized dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMaximized(true);

      CHECK(window.properties.maximized);
      CHECK(window.Dirty(DirtyFlag::kMaximized));
    }
  }

  TEST_CASE("helios::window::Window::SetMinSize") {
    SUBCASE("Stores limits and marks size-limits dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMinSize(320, 240);

      CHECK(window.properties.min_width.has_value());
      CHECK(window.properties.min_height.has_value());
      CHECK_EQ(*window.properties.min_width, 320U);
      CHECK_EQ(*window.properties.min_height, 240U);
      CHECK(window.Dirty(DirtyFlag::kSizeLimits));
    }

    SUBCASE("Clears limits when passed nullopt") {
      auto window = Window::FromProperties(Properties{});
      window.SetMinSize(320, 240);
      window.ClearDirty();
      window.SetMinSize(std::nullopt, std::nullopt);

      CHECK_FALSE(window.properties.min_width.has_value());
      CHECK_FALSE(window.properties.min_height.has_value());
      CHECK(window.Dirty(DirtyFlag::kSizeLimits));
    }
  }

  TEST_CASE("helios::window::Window::SetMaxSize") {
    SUBCASE("Stores limits and marks size-limits dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMaxSize(1920, 1080);

      CHECK(window.properties.max_width.has_value());
      CHECK(window.properties.max_height.has_value());
      CHECK_EQ(*window.properties.max_width, 1920U);
      CHECK_EQ(*window.properties.max_height, 1080U);
      CHECK(window.Dirty(DirtyFlag::kSizeLimits));
    }
  }

  TEST_CASE("helios::window::Window::SetAspectRatio") {
    SUBCASE("Stores ratio and marks aspect-ratio dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetAspectRatio(16, 9);

      CHECK(window.properties.aspect_numer.has_value());
      CHECK(window.properties.aspect_denom.has_value());
      CHECK_EQ(*window.properties.aspect_numer, 16);
      CHECK_EQ(*window.properties.aspect_denom, 9);
      CHECK(window.Dirty(DirtyFlag::kAspectRatio));
    }
  }

  TEST_CASE("helios::window::Window::ClearAspectRatio") {
    SUBCASE("Clears ratio and marks aspect-ratio dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetAspectRatio(16, 9);
      window.ClearDirty();
      window.ClearAspectRatio();

      CHECK_FALSE(window.properties.aspect_numer.has_value());
      CHECK_FALSE(window.properties.aspect_denom.has_value());
      CHECK(window.Dirty(DirtyFlag::kAspectRatio));
    }
  }

  TEST_CASE("helios::window::Window::SetOpacity") {
    SUBCASE("Updates opacity and marks opacity dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetOpacity(0.5F);

      CHECK_EQ(window.properties.opacity, doctest::Approx(0.5F));
      CHECK(window.Dirty(DirtyFlag::kOpacity));
    }
  }

  TEST_CASE("helios::window::Window::SetFloating") {
    SUBCASE("Updates floating and marks floating dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetFloating(true);

      CHECK(window.properties.floating);
      CHECK(window.Dirty(DirtyFlag::kFloating));
    }
  }

  TEST_CASE("helios::window::Window::SetAutoIconify") {
    SUBCASE("Updates auto-iconify and marks auto-iconify dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetAutoIconify(false);

      CHECK_FALSE(window.properties.auto_iconify);
      CHECK(window.Dirty(DirtyFlag::kAutoIconify));
    }
  }

  TEST_CASE("helios::window::Window::SetFocusOnShow") {
    SUBCASE("Updates focus-on-show and marks focus-on-show dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetFocusOnShow(false);

      CHECK_FALSE(window.properties.focus_on_show);
      CHECK(window.Dirty(DirtyFlag::kFocusOnShow));
    }
  }

  TEST_CASE("helios::window::Window::RequestAttention") {
    SUBCASE("Marks attention dirty without changing properties") {
      auto window = Window::FromProperties(Properties{});
      window.RequestAttention();

      CHECK(window.Dirty(DirtyFlag::kAttention));
    }
  }

  TEST_CASE("helios::window::Window::RequestFocus") {
    SUBCASE("Marks focus dirty") {
      auto window = Window::FromProperties(Properties{});
      window.RequestFocus();

      CHECK(window.Dirty(DirtyFlag::kFocus));
    }
  }

  TEST_CASE("helios::window::Window::SetMousePassthrough") {
    SUBCASE("Updates passthrough and marks mouse-passthrough dirty") {
      auto window = Window::FromProperties(Properties{});
      window.SetMousePassthrough(true);

      CHECK(window.properties.mouse_passthrough);
      CHECK(window.Dirty(DirtyFlag::kMousePassthrough));
    }
  }
}

TEST_SUITE("helios::window::PrimaryWindow") {
  TEST_CASE("helios::window::PrimaryWindow::Build") {
    SUBCASE("Adds Window and Primary components") {
      ecs::World world;
      const auto entity = world.CreateEntity();
      world.AddBundle(
          entity,
          PrimaryWindow{
              .window = Window::FromProperties(Properties{.title = "Primary"}),
          });

      CHECK(world.HasComponent<Window>(entity));
      CHECK(world.HasComponent<Primary>(entity));
      CHECK_EQ(world.ReadComponent<Window>(entity).properties.title, "Primary");
    }
  }
}
