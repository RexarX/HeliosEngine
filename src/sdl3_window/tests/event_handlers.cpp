#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/window/event_handlers.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <string>

using namespace helios;
using namespace helios::sdl3::window;

namespace {

[[nodiscard]] window::Properties HiddenTestProperties() {
  return {
      .title = "HeliosEvents",
      .width = 64,
      .height = 64,
      .visible = false,
      .focused = false,
  };
}

[[nodiscard]] SDL_WindowID WindowIdFor(ecs::World& world, ecs::Entity entity) {
  const WindowMap::Entry* entry =
      world.ReadResource<WindowMap>().TryGet(entity);
  REQUIRE_NE(entry, nullptr);
  return entry->window_id;
}

}  // namespace

TEST_SUITE("helios::sdl3::window::RegisterEventHandlers") {
  TEST_CASE("helios::sdl3::window::RegisterEventHandlers") {
    SUBCASE("Emits window::ResizedMsg for a pixel-size-changed event") {
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

      SDL_Event event{};
      event.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
      event.window.windowID = WindowIdFor(world, entity);
      event.window.data1 = 96;
      event.window.data2 = 54;
      world.WriteResource<sdl3::EventDispatcher>().Dispatch(event, world);

      const auto resized =
          world.Messages().CurrentMessages<window::ResizedMsg>();
      REQUIRE_EQ(resized.size(), 1U);
      CHECK_EQ(resized[0].entity, entity);
      CHECK_EQ(resized[0].width, 96U);
      CHECK_EQ(resized[0].height, 54U);
      CHECK_EQ(*world.ReadComponent<window::Window>(entity).properties.width,
               96U);
    }

    SUBCASE("Requests close on SDL_EVENT_WINDOW_CLOSE_REQUESTED") {
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

      SDL_Event event{};
      event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
      event.window.windowID = WindowIdFor(world, entity);
      world.WriteResource<sdl3::EventDispatcher>().Dispatch(event, world);

      CHECK(world.ReadComponent<window::Window>(entity).close_requested);
      const auto requested =
          world.Messages().CurrentMessages<window::CloseRequestedMsg>();
      REQUIRE_EQ(requested.size(), 1U);
      CHECK_EQ(requested[0].entity, entity);
    }

    SUBCASE("Emits window::DroppedFilesMsg for a drop-file event") {
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

      const char* path = "C:/tmp/dropped.txt";
      SDL_Event event{};
      event.type = SDL_EVENT_DROP_FILE;
      event.drop.windowID = WindowIdFor(world, entity);
      event.drop.data = path;
      world.WriteResource<sdl3::EventDispatcher>().Dispatch(event, world);

      const auto dropped =
          world.Messages().CurrentMessages<window::DroppedFilesMsg>();
      REQUIRE_EQ(dropped.size(), 1U);
      CHECK_EQ(dropped[0].entity, entity);
      REQUIRE_EQ(dropped[0].paths.size(), 1U);
      CHECK_EQ(dropped[0].paths[0], std::string(path));
    }

    SUBCASE("Marks the clipboard dirty on clipboard-update") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      SDL_Event event{};
      event.type = SDL_EVENT_CLIPBOARD_UPDATE;
      app.GetWorld().WriteResource<sdl3::EventDispatcher>().Dispatch(
          event, app.GetWorld());

      CHECK(app.GetWorld().ReadResource<Context>().clipboard_dirty);
    }
  }
}
#endif
