#pragma once

#include <variant>

namespace helios::window {

#ifdef HELIOS_PLATFORM_WINDOWS
/// @brief Win32 native window handle.
struct Win32Handle {
  void* hwnd = nullptr;
  void* hinstance = nullptr;
};
#endif

#ifdef HELIOS_PLATFORM_LINUX_X11
/// @brief X11 native window handle.
struct XlibHandle {
  void* display = nullptr;
  unsigned long window = 0;
};
#endif

#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
/// @brief Wayland native window handle.
struct WaylandHandle {
  void* display = nullptr;
  void* surface = nullptr;
};
#endif

#ifdef HELIOS_PLATFORM_MACOS
/// @brief Cocoa native window handle.
struct CocoaHandle {
  void* ns_window = nullptr;
  void* ns_view = nullptr;
};
#endif

/// @brief Backend-agnostic native window handle.
/// @details Populated by whichever window backend plugin is active
/// (`glfw`, future `window_win32`, ...). `std::monostate` means no
/// native handle has been created yet.
using NativeHandle = std::variant<std::monostate
#ifdef HELIOS_PLATFORM_WINDOWS
                                  ,
                                  Win32Handle
#endif
#ifdef HELIOS_PLATFORM_LINUX_X11
                                  ,
                                  XlibHandle
#endif
#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
                                  ,
                                  WaylandHandle
#endif
#ifdef HELIOS_PLATFORM_MACOS
                                  ,
                                  CocoaHandle
#endif
                                  >;

}  // namespace helios::window
