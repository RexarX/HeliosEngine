#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

struct EventDispatcher;

}

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/**
 * @brief Registers keyboard, mouse, and pen SDL event handlers on the
 * dispatcher.
 * @param dispatcher Shared SDL event dispatcher
 */
void RegisterEventHandlers(EventDispatcher& dispatcher);

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
