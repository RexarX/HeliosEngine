#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/systems/apply.hpp>
#include <helios/sdl3/window/systems/destroy.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/window/schedules.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <SDL3/SDL_video.h>

#include <string>

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
#endif
