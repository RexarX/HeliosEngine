#include <pch.hpp>

#include <helios/sdl3/window/details/native_handle.hpp>

#include <helios/window/native_handle.hpp>

#include <SDL3/SDL.h>

#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#ifndef HELIOS_PLATFORM_WINDOWS
#include <string_view>
#endif

namespace helios::sdl3::window {

::helios::window::NativeHandle QueryNativeHandle(SDL_Window& window) {
  const SDL_PropertiesID props = SDL_GetWindowProperties(&window);

#ifdef HELIOS_PLATFORM_WINDOWS
  void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                      nullptr);
  void* hinstance = SDL_GetPointerProperty(
      props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
  if (hinstance == nullptr) {
    hinstance = GetModuleHandle(nullptr);
  }
  return ::helios::window::Win32Handle{.hwnd = hwnd, .hinstance = hinstance};
#elifdef HELIOS_PLATFORM_MACOS
  void* ns_window = SDL_GetPointerProperty(
      props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  return ::helios::window::CocoaHandle{.ns_window = ns_window};
#else
  const char* driver_name = SDL_GetCurrentVideoDriver();
  if (driver_name != nullptr) {
    const auto driver = std::string_view{driver_name};
#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
    if (driver == "wayland") {
      void* display = SDL_GetPointerProperty(
          props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
      void* surface = SDL_GetPointerProperty(
          props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
      return ::helios::window::WaylandHandle{.display = display,
                                             .surface = surface};
    }
#endif
#ifdef HELIOS_PLATFORM_LINUX_X11
    if (driver == "x11") {
      void* display = SDL_GetPointerProperty(
          props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
      const Sint64 x11_window =
          SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
      return ::helios::window::XlibHandle{
          .display = display, .window = static_cast<unsigned long>(x11_window)};
    }
#endif
  }
  return {};
#endif
}

}  // namespace helios::sdl3::window
