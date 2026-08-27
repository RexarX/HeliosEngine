#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

struct EventDispatcher;

}

namespace helios::sdl3::window {

/**
 * @brief Registers SDL window and display event handlers on the dispatcher.
 * @param dispatcher SDL event dispatcher from `helios::sdl3`
 */
void RegisterEventHandlers(sdl3::EventDispatcher& dispatcher);

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
