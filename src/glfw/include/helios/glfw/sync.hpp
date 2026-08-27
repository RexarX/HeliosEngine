#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.glfw;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/window/components.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include <cstdint>
#endif
#include <helios/glfw/state.hpp>

HELIOS_MODULE_EXPORT struct GLFWmonitor;
HELIOS_MODULE_EXPORT struct GLFWwindow;

HELIOS_MODULE_EXPORT
namespace helios::glfw {

[[nodiscard]] GLFWmonitor* MonitorAtIndex(int32_t index);

[[nodiscard]] GLFWmonitor* ResolveMonitor(const window::Properties& properties,
                                          GLFWwindow& window);

void RefreshMonitors(window::Monitors& monitors);

void MarkMonitorDependentDirty(ecs::World& world, NativeWindows& native);

void ApplyWindowHints(const window::Window& window);

void ApplyCursorMode(GLFWwindow& native_window, window::CursorMode mode);

void ResolveCreationSize(window::Window& window);

void ApplyPresentationMode(GLFWwindow& native_window,
                           const window::Window& window);

void ApplyMonitorPlacement(GLFWwindow& native_window, window::Window& window);

void SyncWindowGeometry(window::Window& window, GLFWwindow& native_window);

void ApplyWindowIcons(GLFWwindow& native_window,
                      std::span<const window::IconImage> icons);

void ApplyMaximized(GLFWwindow& native_window, bool maximized);

void ApplySizeLimits(GLFWwindow& native_window,
                     const window::Properties& properties);

void ApplyAspectRatio(GLFWwindow& native_window,
                      const window::Properties& properties);

void ApplyOpacity(GLFWwindow& native_window, float opacity);

void ApplyFloating(GLFWwindow& native_window, bool floating);

void ApplyAutoIconify(GLFWwindow& native_window, bool auto_iconify);

void ApplyFocusOnShow(GLFWwindow& native_window, bool focus_on_show);

void SyncClipboard(ecs::World& world, const NativeWindows& native,
                   Context& context);

}  // namespace helios::glfw
#endif  // HELIOS_MODULE_CONSUMER_SHIM
