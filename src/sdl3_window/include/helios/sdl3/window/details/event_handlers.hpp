#pragma once

#include <helios/sdl3/details/event_dispatcher.hpp>

namespace helios::sdl3::window {

/**
 * @brief Registers SDL window and display event handlers on the dispatcher.
 * @param dispatcher SDL event dispatcher from `helios::sdl3`
 */
void RegisterEventHandlers(sdl3::EventDispatcher& dispatcher);

}  // namespace helios::sdl3::window
