#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/systems/create.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

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
#endif
