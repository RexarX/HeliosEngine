#pragma once

#include <helios/app/application.hpp>
#include <helios/sdl3/details/lifetime.hpp>
#include <helios/sdl3/plugin.hpp>

#include <SDL3/SDL_init.h>
#include <doctest/doctest.h>

namespace helios::sdl3::test {

[[nodiscard]] inline bool SdlVideoAvailable() {
  static const bool available = []() -> bool {
    return helios::sdl3::Probe(SDL_INIT_VIDEO);
  }();
  return available;
}

[[nodiscard]] inline bool SdlGamepadAvailable() {
  static const bool available = []() -> bool {
    return helios::sdl3::Probe(SDL_INIT_GAMEPAD);
  }();
  return available;
}

struct ScopedShutdown {
  helios::app::App& app;

  ~ScopedShutdown() { helios::sdl3::Plugin{}.Destroy(app); }
};

struct ScopedRetain {
  SDL_InitFlags flags;

  explicit ScopedRetain(SDL_InitFlags retain_flags) : flags(retain_flags) {
    helios::sdl3::Retain(flags);
  }

  ~ScopedRetain() { helios::sdl3::Release(flags); }
};

}  // namespace helios::sdl3::test

#define HELIOS_SKIP_IF_NO_SDL_VIDEO()                 \
  do {                                                \
    if (!::helios::sdl3::test::SdlVideoAvailable()) { \
      MESSAGE("SDL video unavailable; skipping");     \
      return;                                         \
    }                                                 \
  } while (false)

#define HELIOS_SKIP_IF_NO_SDL_GAMEPAD()                 \
  do {                                                  \
    if (!::helios::sdl3::test::SdlGamepadAvailable()) { \
      MESSAGE("SDL gamepad unavailable; skipping");     \
      return;                                           \
    }                                                   \
  } while (false)
