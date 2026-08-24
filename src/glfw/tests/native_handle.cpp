#include <doctest/doctest.h>

#include <helios/glfw/details/native_handle.hpp>
#include <helios/window/native_handle.hpp>

#include "available.hpp"

#include <GLFW/glfw3.h>

#include <variant>

using namespace helios;
using namespace helios::glfw;

TEST_SUITE("helios::glfw::QueryNativeHandle") {
  TEST_CASE("helios::glfw::QueryNativeHandle") {
    SUBCASE("Returns a populated native handle for a GLFW window") {
      HELIOS_SKIP_IF_NO_GLFW();

      REQUIRE_EQ(glfwInit(), GLFW_TRUE);
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
      glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
      GLFWwindow* glfw_window =
          glfwCreateWindow(16, 16, "NativeHandle test", nullptr, nullptr);
      REQUIRE_NE(glfw_window, nullptr);

      const window::NativeHandle handle = QueryNativeHandle(*glfw_window);

#if defined(HELIOS_PLATFORM_WINDOWS)
      REQUIRE(std::holds_alternative<window::Win32Handle>(handle));
      const auto& win32 = std::get<window::Win32Handle>(handle);
      CHECK_NE(win32.hwnd, nullptr);
      CHECK_NE(win32.hinstance, nullptr);
#elif defined(HELIOS_PLATFORM_MACOS)
      REQUIRE(std::holds_alternative<window::CocoaHandle>(handle));
      CHECK_NE(std::get<window::CocoaHandle>(handle).ns_window, nullptr);
#elif defined(HELIOS_PLATFORM_LINUX)
      if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        REQUIRE(std::holds_alternative<window::WaylandHandle>(handle));
        const auto& wayland = std::get<window::WaylandHandle>(handle);
        CHECK_NE(wayland.display, nullptr);
        CHECK_NE(wayland.surface, nullptr);
      } else {
        REQUIRE(std::holds_alternative<window::XlibHandle>(handle));
        const auto& x11 = std::get<window::XlibHandle>(handle);
        CHECK_NE(x11.display, nullptr);
        CHECK_NE(x11.window, 0UL);
      }
#else
      CHECK(std::holds_alternative<std::monostate>(handle));
#endif

      glfwDestroyWindow(glfw_window);
      glfwTerminate();
    }
  }
}
