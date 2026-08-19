#include <pch.hpp>

#include <helios/sdl3/window/systems/init.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>
#include <helios/sdl3/details/lifetime.hpp>
#include <helios/sdl3/window/details/event_handlers.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/sdl3/window/details/sdl_sync.hpp>

#include <SDL3/SDL.h>

namespace helios::sdl3::window {

void Init::operator()(ecs::Res<Context> context,
                      ecs::Res<::helios::window::Monitors> monitors,
                      ecs::Res<sdl3::EventDispatcher> dispatcher) const {
  HELIOS_VERIFY(!context->initialized,
                "SDL3 window backend already initialized!");

  sdl3::Retain(SDL_INIT_VIDEO);
  context->initialized = true;

  RegisterEventHandlers(*dispatcher);
  RefreshMonitors(*monitors);
}

}  // namespace helios::sdl3::window
