#pragma once

#include <helios/window/native_handle.hpp>

struct GLFWwindow;

namespace helios::glfw {

/**
 * @brief Queries the OS-native handle for a GLFW window.
 * @param window GLFW window
 * @return Platform-specific native handle
 */
[[nodiscard]] window::NativeHandle QueryNativeHandle(GLFWwindow& window);

}  // namespace helios::glfw
