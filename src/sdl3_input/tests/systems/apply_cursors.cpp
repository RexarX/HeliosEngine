#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/input/cursor_cache.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/input/systems/apply_cursors.hpp>
#include <helios/sdl3/window/plugin.hpp>
#include <helios/sdl3/window/window.hpp>
#endif

#include <SDL3/SDL_mouse.h>

using namespace helios;
using namespace helios::sdl3::input;

TEST_SUITE("helios::sdl3::input::DestroyCursorCache") {
  TEST_CASE("helios::sdl3::input::DestroyCursorCache") {
    SUBCASE("Clears cached cursor pointers") {
      CursorCache cache;
      cache.custom.push_back({.entity = ecs::Entity{1, 1}, .cursor = nullptr});

      DestroyCursorCache(cache);

      CHECK(cache.custom.empty());
      for (SDL_Cursor* cursor : cache.standard) {
        CHECK_EQ(cursor, nullptr);
      }
    }
  }
}

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
namespace {

[[nodiscard]] window::Properties HiddenTestProperties() {
  return {
      .title = "HeliosInput",
      .width = 64,
      .height = 64,
      .visible = false,
      .focused = false,
  };
}

void AddInputPlugins(app::App& app) {
  app.AddPlugins(sdl3::Plugin{}, window::Plugin{}, sdl3::window::Plugin{},
                 input::Plugin{}, Plugin{});
}

}  // namespace

TEST_SUITE("helios::sdl3::input::ApplyCursors") {
  TEST_CASE("helios::sdl3::input::ApplyCursors::operator()") {
    SUBCASE("Applies a standard cursor icon and clears dirty") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()),
          input::Cursor{});
      app.Update();

      world.WriteComponent<input::Cursor>(entity).SetIcon(
          input::CursorIcon::kIBeam);
      app.Update();

      CHECK_FALSE(world.ReadComponent<input::Cursor>(entity).dirty);
      CHECK_EQ(world.ReadComponent<input::Cursor>(entity).icon,
               input::CursorIcon::kIBeam);
    }
  }
}
#endif
#endif
