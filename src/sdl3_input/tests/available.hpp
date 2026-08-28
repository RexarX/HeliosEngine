#pragma once

#include <helios/app/application.hpp>
#include <helios/sdl3/input/plugin.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/plugin.hpp>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#endif

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <doctest/doctest.h>

namespace helios::sdl3::input::test {

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

  ~ScopedShutdown() {
    helios::sdl3::input::Plugin{}.Destroy(app);
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
    helios::sdl3::window::Plugin{}.Destroy(app);
#endif
    helios::sdl3::Plugin{}.Destroy(app);
  }
};

}  // namespace helios::sdl3::input::test

#define HELIOS_SKIP_IF_NO_SDL_VIDEO()                        \
  do {                                                       \
    if (!::helios::sdl3::input::test::SdlVideoAvailable()) { \
      MESSAGE("SDL video unavailable; skipping");            \
      return;                                                \
    }                                                        \
  } while (false)

#define HELIOS_SKIP_IF_NO_SDL_GAMEPAD()                        \
  do {                                                         \
    if (!::helios::sdl3::input::test::SdlGamepadAvailable()) { \
      MESSAGE("SDL gamepad unavailable; skipping");            \
      return;                                                  \
    }                                                          \
  } while (false)

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#define HELIOS_SKIP_IF_NO_SDL_RUNTIME() \
  do {                                  \
    HELIOS_SKIP_IF_NO_SDL_VIDEO();      \
    HELIOS_SKIP_IF_NO_SDL_GAMEPAD();    \
  } while (false)
#else
#define HELIOS_SKIP_IF_NO_SDL_RUNTIME() HELIOS_SKIP_IF_NO_SDL_GAMEPAD()
#endif
