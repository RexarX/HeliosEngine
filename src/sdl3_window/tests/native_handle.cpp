#include <doctest/doctest.h>

#include <helios/sdl3/details/lifetime.hpp>
#include <helios/sdl3/window/details/native_handle.hpp>
#include <helios/window/native_handle.hpp>

#include "available.hpp"

#include <SDL3/SDL.h>

#include <variant>

using namespace helios;
using namespace helios::sdl3::window;

TEST_SUITE("helios::sdl3::window::QueryNativeHandle") {
  TEST_CASE("helios::sdl3::window::QueryNativeHandle") {
    SUBCASE("Returns a populated native handle for an SDL window") {
      HELIOS_SKIP_IF_NO_SDL_VIDEO();

      sdl3::Retain(SDL_INIT_VIDEO);
      SDL_Window* sdl_window =
          SDL_CreateWindow("NativeHandle test", 16, 16, SDL_WINDOW_HIDDEN);
      if (sdl_window == nullptr) {
        sdl3::Release(SDL_INIT_VIDEO);
        MESSAGE("SDL_CreateWindow failed; skipping");
        return;
      }

      const window::NativeHandle handle = QueryNativeHandle(*sdl_window);

#if defined(HELIOS_PLATFORM_WINDOWS)
      REQUIRE(std::holds_alternative<window::Win32Handle>(handle));
      const auto& win32 = std::get<window::Win32Handle>(handle);
      CHECK_NE(win32.hwnd, nullptr);
      CHECK_NE(win32.hinstance, nullptr);
#elif defined(HELIOS_PLATFORM_MACOS)
      REQUIRE(std::holds_alternative<window::CocoaHandle>(handle));
      CHECK_NE(std::get<window::CocoaHandle>(handle).ns_window, nullptr);
#else
      REQUIRE_FALSE(std::holds_alternative<std::monostate>(handle));
      bool matched = false;
#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
      if (const auto* wayland = std::get_if<window::WaylandHandle>(&handle);
          wayland != nullptr) {
        CHECK_NE(wayland->display, nullptr);
        CHECK_NE(wayland->surface, nullptr);
        matched = true;
      }
#endif
#ifdef HELIOS_PLATFORM_LINUX_X11
      if (const auto* x11 = std::get_if<window::XlibHandle>(&handle);
          x11 != nullptr) {
        CHECK_NE(x11->display, nullptr);
        CHECK_NE(x11->window, 0UL);
        matched = true;
      }
#endif
      CHECK(matched);
#endif

      SDL_DestroyWindow(sdl_window);
      helios::sdl3::Release(SDL_INIT_VIDEO);
    }
  }
}
