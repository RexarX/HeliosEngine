#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/sdl3/window/plugin.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/systems/shutdown.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

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

}  // namespace

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
