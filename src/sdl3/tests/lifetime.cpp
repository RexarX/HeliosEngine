#include <doctest/doctest.h>

#include <helios/sdl3/details/lifetime.hpp>

#include "available.hpp"

#include <SDL3/SDL.h>

using namespace helios::sdl3;

TEST_SUITE("helios::sdl3::Retain") {
  TEST_CASE("helios::sdl3::Retain") {
    SUBCASE("Double retain shares one SDL init cycle") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      Retain(SDL_INIT_VIDEO);
      CHECK(Initialized());
      Retain(SDL_INIT_VIDEO);
      CHECK(Initialized());

      Release(SDL_INIT_VIDEO);
      CHECK(Initialized());

      Release(SDL_INIT_VIDEO);
      CHECK_FALSE(Initialized());
    }

    SUBCASE("Retains an additional subsystem while video is held") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();
      HELIOS_SKIP_IF_NO_SDL_GAMEPAD();

      Retain(SDL_INIT_VIDEO);
      CHECK(Initialized());

      Retain(SDL_INIT_GAMEPAD);
      CHECK(Initialized());

      Release(SDL_INIT_GAMEPAD);
      CHECK(Initialized());

      Release(SDL_INIT_VIDEO);
      CHECK_FALSE(Initialized());
    }
  }
}

TEST_SUITE("helios::sdl3::Release") {
  TEST_CASE("helios::sdl3::Release") {
    SUBCASE("Last release terminates SDL") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      Retain(SDL_INIT_VIDEO);
      Release(SDL_INIT_VIDEO);
      CHECK_FALSE(Initialized());
    }
  }
}

TEST_SUITE("helios::sdl3::Initialized") {
  TEST_CASE("helios::sdl3::Initialized") {
    SUBCASE("Reports false before the first retain") {
      CHECK_FALSE(Initialized());
    }
  }
}

TEST_SUITE("helios::sdl3::Probe") {
  TEST_CASE("helios::sdl3::Probe") {
    SUBCASE("Leaves SDL uninitialized after a successful video probe") {
      if (!Probe(SDL_INIT_VIDEO)) {
        MESSAGE("SDL video unavailable; skipping");
        return;
      }
      CHECK_FALSE(Initialized());
    }
  }
}
