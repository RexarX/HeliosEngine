#include <pch.hpp>

#include <helios/glfw/details/native_handle.hpp>

#include <helios/window/native_handle.hpp>

#ifdef HELIOS_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elifdef HELIOS_GLFW_FORCE_X11
#define GLFW_EXPOSE_NATIVE_X11
#elifdef HELIOS_GLFW_FORCE_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3native.h>

#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace helios::glfw {

window::NativeHandle QueryNativeHandle(GLFWwindow& window) {
#ifdef HELIOS_PLATFORM_WINDOWS
  return window::Win32Handle{.hwnd = glfwGetWin32Window(&window),
                             .hinstance = GetModuleHandle(nullptr)};
#elifdef HELIOS_PLATFORM_MACOS
  return window::CocoaHandle{.ns_window = glfwGetCocoaWindow(&window)};
#elifdef HELIOS_PLATFORM_LINUX
  if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
    return window::WaylandHandle{.display = glfwGetWaylandDisplay(),
                                 .surface = glfwGetWaylandWindow(&window)};
  }
  return window::XlibHandle{.display = glfwGetX11Display(),
                            .window = glfwGetX11Window(&window)};
#else
  return {};
#endif
}

}  // namespace helios::glfw
