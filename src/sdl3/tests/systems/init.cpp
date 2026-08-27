#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/systems/init.hpp>

using namespace helios;
using namespace helios::sdl3;

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
#endif
