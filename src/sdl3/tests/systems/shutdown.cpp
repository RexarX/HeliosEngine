#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/systems/shutdown.hpp>

using namespace helios;
using namespace helios::sdl3;

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
