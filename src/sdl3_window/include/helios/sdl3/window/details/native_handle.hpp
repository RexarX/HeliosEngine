#pragma once

#include <helios/window/native_handle.hpp>

#include <SDL3/SDL_video.h>

namespace helios::sdl3::window {

/**
 * @brief Queries the OS-native handle for an SDL window.
 * @param window SDL window
 * @return Platform-specific native handle
 */
[[nodiscard]] ::helios::window::NativeHandle QueryNativeHandle(
    SDL_Window& window);

}  // namespace helios::sdl3::window
