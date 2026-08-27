#include <pch.hpp>

#include <helios/sdl3/lifetime.hpp>

#include <helios/log/log.hpp>

#include <cstdint>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <helios/assert.hpp>

namespace helios::sdl3 {

namespace {

struct LifetimeState {
  uint32_t retain_count = 0;
  SDL_InitFlags retained_flags = 0;
};

[[nodiscard]] LifetimeState& State() noexcept {
  static LifetimeState state;
  return state;
}

void LogSdlError(const char* context) {
  const char* error = SDL_GetError();
  log::Error("SDL {}: {}", context, error != nullptr ? error : "unknown error");
}

}  // namespace

void Retain(SDL_InitFlags flags) {
  auto& state = State();
  if (state.retain_count == 0) {
#ifdef HELIOS_SDL3_FORCE_WAYLAND
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
#elifdef HELIOS_SDL3_FORCE_X11
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
#endif
    if (!SDL_Init(0)) [[unlikely]] {
      LogSdlError("SDL_Init");
      HELIOS_VERIFY(false, "Failed to initialize SDL!");
    }
  }

  const SDL_InitFlags new_flags = flags & ~state.retained_flags;
  if (new_flags != 0) {
    if (!SDL_InitSubSystem(new_flags)) [[unlikely]] {
      LogSdlError("SDL_InitSubSystem");
      HELIOS_VERIFY(false, "Failed to initialize SDL subsystem {}!", new_flags);
    }
    state.retained_flags |= new_flags;
  }

  ++state.retain_count;
}

void Release(SDL_InitFlags flags) {
  auto& state = State();
  HELIOS_VERIFY(state.retain_count > 0, "SDL Release without matching Retain!");

  const SDL_InitFlags active = flags & state.retained_flags;
  if (active != 0) {
    SDL_QuitSubSystem(active);
    state.retained_flags &= ~active;
  }

  --state.retain_count;
  if (state.retain_count == 0) {
    SDL_Quit();
    state.retained_flags = 0;
  }
}

bool Probe(SDL_InitFlags flags) {
  auto& state = State();
  if (state.retain_count > 0) {
    const SDL_InitFlags missing = flags & ~SDL_WasInit(0);
    if (missing == 0) {
      return true;
    }
    if (!SDL_InitSubSystem(missing)) {
      return false;
    }
    SDL_QuitSubSystem(missing);
    return true;
  }

  if (!SDL_Init(flags)) {
    return false;
  }
  SDL_Quit();
  return true;
}

bool Initialized() noexcept {
  return State().retain_count > 0;
}

}  // namespace helios::sdl3
