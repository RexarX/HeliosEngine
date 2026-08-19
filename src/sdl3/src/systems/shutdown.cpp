#include <pch.hpp>

#include <helios/sdl3/systems/shutdown.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>

namespace helios::sdl3 {

void Shutdown::operator()(ecs::Res<Context> context) const {
  context->world = nullptr;
  context->frame_pump = nullptr;
  context->frame_pump_user_data = nullptr;
  context->in_event_poll = false;
  context->in_nested_pump = false;
}

}  // namespace helios::sdl3
