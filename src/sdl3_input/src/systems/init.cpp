#include <pch.hpp>

#include <helios/sdl3/input/systems/init.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/lifetime.hpp>
#include <helios/sdl3/input/details/event_handlers.hpp>

#include <SDL3/SDL.h>

namespace helios::sdl3::input {

void Init::operator()(ecs::Res<Context> context,
                      ecs::Res<EventDispatcher> dispatcher) const {
  if (!context->gamepad_subsystem_retained) {
    Retain(SDL_INIT_GAMEPAD);
    context->gamepad_subsystem_retained = true;
  }

  if (!context->handlers_registered) {
    RegisterEventHandlers(*dispatcher);
    context->handlers_registered = true;
  }
}

}  // namespace helios::sdl3::input
