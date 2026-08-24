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
 * @brief Tests whether a subsystem can be initialized, then fully tears it
 * down when this process has no outstanding `Retain`.
 * @details Uses `SDL_Init`/`SDL_Quit` so X11/XKB resources from a probe do
 * not leak into LeakSanitizer at process exit. Safe to call after `Retain`:
 * the live session is left intact.
 * @param flags Subsystem flags to probe
 * @return True if the subsystem initialized successfully
 */
[[nodiscard]] bool Probe(SDL_InitFlags flags);

/**
 * @brief Tests whether SDL has been initialized at least once.
 * @return True after the first successful `Retain`
 */
[[nodiscard]] bool Initialized() noexcept;

}  // namespace helios::sdl3
