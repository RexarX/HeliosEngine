#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/apply_raw_mouse.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#endif

#include <SDL3/SDL_mouse.h>

#include <string>

using namespace helios;
using namespace helios::sdl3::input;

namespace {

void AddInputPlugins(app::App& app) {
  app.AddPlugins(sdl3::Plugin{}, window::Plugin{},
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
                 sdl3::window::Plugin{},
#endif
                 input::Plugin{}, Plugin{});
}

}  // namespace

TEST_SUITE("helios::sdl3::input::ApplyRawMouseMotion") {
  TEST_CASE("helios::sdl3::input::ApplyRawMouseMotion::operator()") {
    SUBCASE("Writes the relative mouse scale hint") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      app.GetWorld().WriteResource<input::Settings>().raw_mouse_motion = true;
      app.Update();

      const char* raw = SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE);
      REQUIRE_NE(raw, nullptr);
      CHECK_EQ(std::string(raw), "0");
      CHECK(app.GetWorld().ReadResource<Context>().raw_mouse_hint_set);
      CHECK(app.GetWorld().ReadResource<Context>().last_raw_mouse_motion);

      app.GetWorld().WriteResource<input::Settings>().raw_mouse_motion = false;
      app.Update();

      const char* scaled = SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE);
      REQUIRE_NE(scaled, nullptr);
      CHECK_EQ(std::string(scaled), "1");
      CHECK_FALSE(app.GetWorld().ReadResource<Context>().last_raw_mouse_motion);
    }
  }
}
#endif
