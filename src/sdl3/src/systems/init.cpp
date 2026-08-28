#include <pch.hpp>

#include <helios/sdl3/systems/init.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/context.hpp>

namespace helios::sdl3 {

void Init::operator()(ecs::World& world, ecs::Res<Context> context) const {
  HELIOS_VERIFY(context->world == nullptr, "SDL3 runtime already initialized!");
  context->world = &world;
}

}  // namespace helios::sdl3
