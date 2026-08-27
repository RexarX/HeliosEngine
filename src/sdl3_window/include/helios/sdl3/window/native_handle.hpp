#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/window/native_handle.hpp>
#endif

HELIOS_MODULE_EXPORT struct SDL_Window;

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/**
 * @brief Queries the OS-native handle for an SDL window.
 * @param window SDL window
 * @return Platform-specific native handle
 */
[[nodiscard]] ::helios::window::NativeHandle QueryNativeHandle(
    SDL_Window& window);

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
