#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/systems/init.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

using namespace helios;
using namespace helios::sdl3::window;

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
#endif
