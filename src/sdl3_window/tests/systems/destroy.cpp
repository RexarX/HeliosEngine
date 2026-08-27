#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/systems/destroy.hpp>
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
#endif
