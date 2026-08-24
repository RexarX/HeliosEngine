#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>

#include <SDL3/SDL_events.h>

#include <string>
#include <string_view>

using namespace helios;
using namespace helios::sdl3;

namespace {

struct DispatchLog {
  static constexpr std::string_view kName = "DispatchLog";

  std::string order;
};

void AppendA(const SDL_Event& event, ecs::World& world) {
  world.WriteResource<DispatchLog>().order += 'A';
  world.WriteResource<DispatchLog>().order +=
      static_cast<char>('0' + (event.type % 10));
}

void AppendB(const SDL_Event& /*event*/, ecs::World& world) {
  world.WriteResource<DispatchLog>().order += 'B';
}

}  // namespace

TEST_SUITE("helios::sdl3::EventDispatcher") {
  TEST_CASE("helios::sdl3::EventDispatcher::Register") {
    SUBCASE("Appends handlers in registration order") {
      EventDispatcher dispatcher;
      CHECK(dispatcher.handlers.empty());

      dispatcher.Register(AppendA);
      dispatcher.Register(AppendB);

      REQUIRE_EQ(dispatcher.handlers.size(), 2U);
      CHECK_EQ(dispatcher.handlers[0], &AppendA);
      CHECK_EQ(dispatcher.handlers[1], &AppendB);
    }
  }

  TEST_CASE("helios::sdl3::EventDispatcher::Dispatch") {
    SUBCASE("Invokes every handler with the same event") {
      ecs::World world;
      world.InsertResources(DispatchLog{});

      EventDispatcher dispatcher;
      dispatcher.Register(AppendA);
      dispatcher.Register(AppendB);

      SDL_Event event{};
      event.type = 3;
      dispatcher.Dispatch(event, world);

      CHECK_EQ(world.ReadResource<DispatchLog>().order, "A3B");
    }

    SUBCASE("Does nothing when no handlers are registered") {
      ecs::World world;
      world.InsertResources(DispatchLog{});

      EventDispatcher dispatcher;
      SDL_Event event{};
      event.type = 1;
      dispatcher.Dispatch(event, world);

      CHECK(world.ReadResource<DispatchLog>().order.empty());
    }
  }
}
