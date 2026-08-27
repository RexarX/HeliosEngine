#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/init.hpp>
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

TEST_SUITE("helios::sdl3::input::Init") {
  TEST_CASE("helios::sdl3::input::Init::operator()") {
    SUBCASE("Retains the gamepad subsystem and registers handlers") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      const auto& context = app.GetWorld().ReadResource<Context>();
      CHECK(context.gamepad_subsystem_retained);
      CHECK(context.handlers_registered);
      CHECK_FALSE(app.GetWorld()
                      .ReadResource<sdl3::EventDispatcher>()
                      .handlers.empty());
    }
  }
}
#endif
