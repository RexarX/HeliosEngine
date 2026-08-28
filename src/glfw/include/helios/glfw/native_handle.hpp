#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.glfw;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/window/native_handle.hpp>
#endif

HELIOS_MODULE_EXPORT struct GLFWwindow;

HELIOS_MODULE_EXPORT
namespace helios::glfw {

/**
 * @brief Queries the OS-native handle for a GLFW window.
 * @param window GLFW window
 * @return Platform-specific native handle
 */
[[nodiscard]] window::NativeHandle QueryNativeHandle(GLFWwindow& window);

}  // namespace helios::glfw
#endif  // HELIOS_MODULE_CONSUMER_SHIM
