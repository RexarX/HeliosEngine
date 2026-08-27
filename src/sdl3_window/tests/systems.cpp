#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/application.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/sdl3/window/systems/apply.hpp>
#include <helios/sdl3/window/systems/destroy.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/schedules.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <SDL3/SDL_video.h>

#include <string>
#include <variant>

using namespace helios;
using namespace helios::sdl3::window;

namespace {

[[nodiscard]] window::Properties HiddenTestProperties(
    std::string title = "HeliosTest") {
  return {
      .title = std::move(title),
      .width = 64,
      .height = 64,
      .visible = false,
      .focused = false,
  };
}

struct CaptureCloseRequestedDirty {
  bool* dirty_cleared = nullptr;

  void operator()(window::Windows windows) const {
    for (auto&& [window] : windows.query) {
      if (window.close_requested && dirty_cleared != nullptr) {
        *dirty_cleared = window.dirty_flags == window::DirtyFlag::kNone;
      }
    }
  }
};

}  // namespace

TEST_SUITE("helios::sdl3::window::CreateNativeWindows") {
  TEST_CASE("helios::sdl3::window::CreateNativeWindows::operator()") {
    SUBCASE("Creates a hidden window and emits window::CreatedMsg") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddBundle(
          entity,
          window::PrimaryWindow{
              .window = window::Window::FromProperties(HiddenTestProperties()),
          });
      app.Update();

      CHECK(world.HasComponent<window::NativeHandleComponent>(entity));
      CHECK_FALSE(world.HasComponent<window::CreationFailed>(entity));
      CHECK(world.ReadResource<NativeWindows>().Contains(entity));
      CHECK(world.ReadResource<WindowMap>().Contains(entity));
      CHECK(world.ReadResource<Context>().initialized);
      CHECK_FALSE(world.ReadComponent<window::Window>(entity).Dirty(
          window::DirtyFlag::kTitle));

      const auto created =
          world.Messages().PreviousMessages<window::CreatedMsg>();
      REQUIRE_EQ(created.size(), 1U);
      CHECK_EQ(created[0].entity, entity);
      CHECK_EQ(created[0].properties.title, "HeliosTest");
      CHECK_EQ(world.ReadComponent<window::Window>(entity).properties.title,
               "HeliosTest");

#ifdef HELIOS_PLATFORM_WINDOWS
      CHECK(std::holds_alternative<window::Win32Handle>(
          world.ReadComponent<window::NativeHandleComponent>(entity).handle));
#endif
    }
  }
}

TEST_SUITE("helios::sdl3::window::ApplyChanges") {
  TEST_CASE("helios::sdl3::window::ApplyChanges::operator()") {
    SUBCASE("Applies title, size, and visibility then clears dirty flags") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity, window::Window::FromProperties(
                                      HiddenTestProperties("Apply")));
      app.Update();

      auto& window = world.WriteComponent<window::Window>(entity);
      window.SetTitle("Applied");
      window.SetSize(80, 48);
      window.SetVisible(false);
      window.SetOpacity(0.75F);
      window.SetMousePassthrough(true);
      app.Update();

      const auto& updated = world.ReadComponent<window::Window>(entity);
      CHECK_EQ(updated.properties.title, "Applied");
      CHECK_EQ(updated.dirty_flags, window::DirtyFlag::kNone);
      CHECK_FALSE(updated.properties.visible);
      CHECK(updated.properties.mouse_passthrough);

      const NativeWindows::Entry* entry =
          world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);
      CHECK_EQ(std::string(SDL_GetWindowTitle(entry->native.window)),
               "Applied");
    }

    SUBCASE("Clears dirty flags on close-requested windows") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      bool dirty_cleared = false;
      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      window::RegisterEventsSchedule(app.GetMainSubApp().GetScheduler());
      app.AddSystem(window::kEvents, CaptureCloseRequestedDirty{&dirty_cleared})
          .After<ApplyChanges>()
          .Before<DestroyClosedWindows>();
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity, window::Window::FromProperties(
                                      HiddenTestProperties("Closing")));
      app.Update();

      auto& window = world.WriteComponent<window::Window>(entity);
      window.SetTitle("Closing");
      window.RequestClose();
      app.Update();

      CHECK(dirty_cleared);
    }

    SUBCASE("Applies exclusive fullscreen from the desktop video mode") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      HELIOS_SKIP_IF_NO_DISPLAY();

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity, window::Window::FromProperties(
                                      HiddenTestProperties("Exclusive")));
      app.Update();

      world.WriteComponent<window::Window>(entity).SetMode(
          window::Mode::kFullscreen);
      app.Update();

      const auto& updated = world.ReadComponent<window::Window>(entity);
      CHECK_EQ(updated.dirty_flags, window::DirtyFlag::kNone);
      CHECK_EQ(updated.properties.mode, window::Mode::kFullscreen);

      const NativeWindows::Entry* entry =
          world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);

      if ((SDL_GetWindowFlags(entry->native.window) & SDL_WINDOW_FULLSCREEN) ==
          0U) {
        MESSAGE(
            "Exclusive fullscreen was not applied (virtual/CI display); "
            "skipping Hz change");
        world.WriteComponent<window::Window>(entity).SetMode(
            window::Mode::kWindowed);
        app.Update();
        return;
      }

      const SDL_DisplayMode* current =
          SDL_GetWindowFullscreenMode(entry->native.window);
      if (current == nullptr) {
        MESSAGE("SDL did not report a fullscreen mode; skipping Hz change");
        world.WriteComponent<window::Window>(entity).SetMode(
            window::Mode::kWindowed);
        app.Update();
        return;
      }

      world.WriteComponent<window::Window>(entity).SetMode(
          window::Mode::kWindowed);
      app.Update();
    }
  }
}

TEST_SUITE("helios::sdl3::window::DestroyClosedWindows") {
  TEST_CASE("helios::sdl3::window::DestroyClosedWindows::operator()") {
    SUBCASE("Destroys a closed primary window and requests app::AppExit") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddBundle(
          entity,
          window::PrimaryWindow{
              .window = window::Window::FromProperties(HiddenTestProperties()),
          });
      app.Update();
      world.WriteComponent<window::Window>(entity).RequestClose();
      app.Update();

      CHECK_FALSE(world.Exists(entity));
      CHECK(world.ReadResource<NativeWindows>().Empty());
      CHECK_FALSE(world.ReadResource<WindowMap>().Contains(entity));

      const auto closed =
          world.Messages().PreviousMessages<window::ClosedMsg>();
      REQUIRE_EQ(closed.size(), 1U);
      CHECK_EQ(closed[0].entity, entity);

      const auto exits = world.Messages().PreviousMessages<app::AppExit>();
      CHECK_FALSE(exits.empty());
    }

    SUBCASE("Does not request exit when triggers are none") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(
          WindowPlugin{{.exit_triggers = window::kExitTriggersNone}});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();
      world.WriteComponent<window::Window>(entity).RequestClose();
      app.Update();

      CHECK_FALSE(world.Exists(entity));
      CHECK(world.Messages().PreviousMessages<app::AppExit>().empty());
    }
  }
}

TEST_SUITE("helios::sdl3::window::PollEvents") {
  TEST_CASE("helios::sdl3::window::PollEvents::operator()") {
    SUBCASE("Writes pending clipboard text") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      auto& clipboard = world.WriteResource<window::Clipboard>();
      clipboard.text = "helios-clipboard";
      clipboard.pending_write = true;
      app.Update();

      CHECK_FALSE(world.ReadResource<window::Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<window::Clipboard>().text,
               "helios-clipboard");
    }

    SUBCASE("WaitTimeout returns and writes pending clipboard text") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{{
          .event_wait_timeout = 0.001,
          .event_mode = window::EventMode::kWaitTimeout,
      }});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      auto& clipboard = world.WriteResource<window::Clipboard>();
      clipboard.text = "helios-wait";
      clipboard.pending_write = true;
      app.Update();

      CHECK_FALSE(world.ReadResource<window::Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<window::Clipboard>().text, "helios-wait");
    }
  }
}

TEST_SUITE("helios::sdl3::window::Init") {
  TEST_CASE("helios::sdl3::window::Init::operator()") {
    SUBCASE("Marks context initialized and snapshots monitors") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      CHECK(app.GetWorld().ReadResource<Context>().initialized);
      CHECK_FALSE(app.GetWorld()
                      .ReadResource<sdl3::EventDispatcher>()
                      .handlers.empty());
      HELIOS_SKIP_IF_NO_DISPLAY();
      CHECK_FALSE(
          app.GetWorld().ReadResource<window::Monitors>().monitors.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::window::Shutdown") {
  TEST_CASE("helios::sdl3::window::Shutdown::operator()") {
    SUBCASE("Releases video and clears native windows") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();
      CHECK(world.ReadResource<NativeWindows>().Contains(entity));

      Plugin{}.Destroy(app);

      CHECK_FALSE(world.ReadResource<Context>().initialized);
      CHECK(world.ReadResource<NativeWindows>().Empty());
      CHECK(world.ReadResource<WindowMap>().entries.empty());
    }
  }
}
#endif
