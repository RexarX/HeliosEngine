#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/poll_sensors.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#endif

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

TEST_SUITE("helios::sdl3::input::PollSensors") {
  TEST_CASE("helios::sdl3::input::PollSensors::operator()") {
    SUBCASE("Runs without requiring a connected sensor") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      app.Update();

      CHECK(app.GetWorld().HasResource<SensorCache>());
    }
  }
}
#endif
