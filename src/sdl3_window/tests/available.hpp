#pragma once

#include <helios/app/application.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/window/plugin.hpp>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <doctest/doctest.h>

namespace helios::sdl3::window::test {

[[nodiscard]] inline bool SdlVideoAvailable() {
  static const bool available = []() -> bool {
    return sdl3::Probe(SDL_INIT_VIDEO);
  }();
  return available;
}

struct ScopedShutdown {
  app::App& app;

  ~ScopedShutdown() {
    Plugin{}.Destroy(app);
    sdl3::Plugin{}.Destroy(app);
  }
};

}  // namespace helios::sdl3::window::test

#define HELIOS_SKIP_IF_NO_SDL_VIDEO()                         \
  do {                                                        \
    if (!::helios::sdl3::window::test::SdlVideoAvailable()) { \
      MESSAGE("SDL video unavailable; skipping");             \
      return;                                                 \
    }                                                         \
  } while (false)

// SDL video must already be initialized. Use only when shutdown is owned
// (RAII).
#define HELIOS_SKIP_IF_NO_DISPLAY()                                \
  do {                                                             \
    if (SDL_GetPrimaryDisplay() == 0) {                            \
      MESSAGE("No SDL display available (headless CI); skipping"); \
      return;                                                      \
    }                                                              \
  } while (false)
