#pragma once

#include <helios/sdl3/details/event_dispatcher.hpp>

namespace helios::sdl3::input {

/**
 * @brief Registers keyboard, mouse, and pen SDL event handlers on the
 * dispatcher.
 * @param dispatcher Shared SDL event dispatcher
 */
void RegisterEventHandlers(EventDispatcher& dispatcher);

}  // namespace helios::sdl3::input
