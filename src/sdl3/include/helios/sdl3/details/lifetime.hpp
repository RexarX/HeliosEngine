#pragma once

#include <SDL3/SDL_init.h>

namespace helios::sdl3 {

/**
 * @brief Retains an SDL subsystem, initializing SDL on the first retain.
 * @param flags Subsystem flags (`SDL_INIT_VIDEO`, `SDL_INIT_GAMEPAD`, ...)
 * @warning Must be called from the process main thread when `flags` includes
 * `SDL_INIT_VIDEO`.
 */
void Retain(SDL_InitFlags flags);

/**
 * @brief Releases a previously retained subsystem.
 * @param flags Subsystem flags that were passed to `Retain`
 */
void Release(SDL_InitFlags flags);

/**
 * @brief Tests whether SDL has been initialized at least once.
 * @return True after the first successful `Retain`
 */
[[nodiscard]] bool Initialized() noexcept;

}  // namespace helios::sdl3
