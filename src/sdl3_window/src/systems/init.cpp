#include <pch.hpp>

#include <helios/sdl3/window/systems/init.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/window/event_handlers.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/window/resources.hpp>

#include <SDL3/SDL_init.h>

#include <helios/assert.hpp>

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
