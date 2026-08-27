#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/systems/init.hpp>
#include <helios/sdl3/systems/pump_events.hpp>
#include <helios/sdl3/systems/shutdown.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/resources.hpp>

#include "available.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

#include <string_view>

using namespace helios;
using namespace helios::sdl3;

namespace {

struct UserEventFlag {
  static constexpr std::string_view kName = "UserEventFlag";
  bool seen = false;
};

}  // namespace

TEST_SUITE("helios::sdl3::Init") {
  TEST_CASE("helios::sdl3::Init::operator()") {
    SUBCASE("Stores the main world pointer") {
      ecs::World world;
      Context context;
      Init{}(world, ecs::Res<Context>(context));

      CHECK_EQ(context.world, &world);
    }
  }
}

TEST_SUITE("helios::sdl3::PumpEvents") {
  TEST_CASE("helios::sdl3::PumpEvents::operator()") {
    SUBCASE("Returns immediately when SDL is not retained") {
      CHECK_FALSE(Initialized());

      Context context;
      EventDispatcher dispatcher;
      window::Settings settings;
      ecs::World world;
      context.world = &world;

      PumpEvents{}(ecs::Res<Context>(context),
                   ecs::Res<EventDispatcher>(dispatcher),
                   ecs::Res<const window::Settings>(settings));

      CHECK_FALSE(context.in_event_poll);
    }

    SUBCASE("Returns immediately when a poll is already in progress") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      test::ScopedRetain retain{SDL_INIT_VIDEO};

      ecs::World world;
      Context context;
      context.world = &world;
      context.in_event_poll = true;
      EventDispatcher dispatcher;
      window::Settings settings;

      PumpEvents{}(ecs::Res<Context>(context),
                   ecs::Res<EventDispatcher>(dispatcher),
                   ecs::Res<const window::Settings>(settings));

      CHECK(context.in_event_poll);
    }

    SUBCASE("Drains the SDL queue after a wait-timeout") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      window::Plugin{}.Build(app);
      Plugin plugin;
      plugin.Build(app);
      plugin.Finish(app);
      app.GetWorld().WriteResource<window::Settings>().event_mode =
          window::EventMode::kWaitTimeout;
      app.GetWorld().WriteResource<window::Settings>().event_wait_timeout =
          0.001;
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      test::ScopedRetain retain{SDL_INIT_VIDEO};

      Context& context = app.GetWorld().WriteResource<Context>();
      REQUIRE_NE(context.world, nullptr);

      PumpEvents{}(ecs::Res<Context>(context),
                   ecs::Res<EventDispatcher>(
                       app.GetWorld().WriteResource<EventDispatcher>()),
                   ecs::Res<const window::Settings>(
                       app.GetWorld().ReadResource<window::Settings>()));

      CHECK_FALSE(context.in_event_poll);
    }

    SUBCASE("Dispatches a pushed user event") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      app::App app;
      window::Plugin{}.Build(app);
      Plugin plugin;
      plugin.Build(app);
      plugin.Finish(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      test::ScopedRetain retain{SDL_INIT_VIDEO};

      app.GetWorld().TryInsertResources(UserEventFlag{});

      auto& dispatcher = app.GetWorld().WriteResource<EventDispatcher>();
      dispatcher.Register([](const SDL_Event& event, ecs::World& world) {
        if (event.type == SDL_EVENT_USER) {
          world.WriteResource<UserEventFlag>().seen = true;
        }
      });

      SDL_Event event{};
      event.type = SDL_EVENT_USER;
      REQUIRE(SDL_PushEvent(&event));

      PumpEvents{}(ecs::Res<Context>(app.GetWorld().WriteResource<Context>()),
                   ecs::Res<EventDispatcher>(dispatcher),
                   ecs::Res<const window::Settings>(
                       app.GetWorld().ReadResource<window::Settings>()));

      CHECK(app.GetWorld().ReadResource<UserEventFlag>().seen);
      CHECK_FALSE(app.GetWorld().ReadResource<Context>().in_event_poll);
    }
  }
}

TEST_SUITE("helios::sdl3::Shutdown") {
  TEST_CASE("helios::sdl3::Shutdown::operator()") {
    SUBCASE("Clears runtime pointers owned by Context") {
      ecs::World world;
      Context context;
      context.world = &world;
      context.frame_pump = [](void*) {};
      context.frame_pump_user_data = &world;
      context.in_event_poll = true;
      context.in_nested_pump = true;

      Shutdown{}(ecs::Res<Context>(context));

      CHECK_EQ(context.world, nullptr);
      CHECK_EQ(context.frame_pump, nullptr);
      CHECK_EQ(context.frame_pump_user_data, nullptr);
      CHECK_FALSE(context.in_event_poll);
      CHECK_FALSE(context.in_nested_pump);
    }
  }
}
#endif
