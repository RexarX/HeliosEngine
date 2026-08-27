#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif
#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/window/clipboard.hpp>
#include <helios/window/ids.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/settings.hpp>

#include <cstdint>
#include <span>
#endif
#include <helios/sdl3/window/state.hpp>

HELIOS_MODULE_EXPORT struct SDL_Window;
HELIOS_MODULE_EXPORT using SDL_DisplayID = uint32_t;
HELIOS_MODULE_EXPORT using SDL_WindowFlags = uint64_t;

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;

}

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

[[nodiscard]] SDL_DisplayID DisplayAtIndex(::helios::window::MonitorId index);

[[nodiscard]] SDL_DisplayID ResolveDisplay(
    const ::helios::window::Properties& properties, SDL_Window& window);

void RefreshMonitors(::helios::window::Monitors& monitors);

void MarkMonitorDependentDirty(ecs::World& world, NativeWindows& native);

[[nodiscard]] SDL_WindowFlags BuildCreationFlags(
    const ::helios::window::Window& window);

void ApplyCursorMode(SDL_Window& native_window,
                     ::helios::window::CursorMode mode);

void ResolveCreationSize(::helios::window::Window& window);

void ApplyPresentationMode(SDL_Window& native_window,
                           const ::helios::window::Window& window);

void ApplyMonitorPlacement(SDL_Window& native_window,
                           ::helios::window::Window& window);

void SyncWindowGeometry(::helios::window::Window& window,
                        SDL_Window& native_window);

void ApplyWindowIcons(SDL_Window& native_window,
                      std::span<const ::helios::window::IconImage> icons);

void ApplyMaximized(SDL_Window& native_window, bool maximized);

void ApplySizeLimits(SDL_Window& native_window,
                     const ::helios::window::Properties& properties);

void ApplyAspectRatio(SDL_Window& native_window,
                      const ::helios::window::Properties& properties);

void ApplyOpacity(SDL_Window& native_window, float opacity);

void ApplyFloating(SDL_Window& native_window, bool floating);

void ApplyAutoIconify(SDL_Window& native_window, bool auto_iconify);

void ApplyFocusOnShow(bool focus_on_show);

void ApplyMousePassthrough(SDL_Window& native_window, bool passthrough);

void SyncClipboard(ecs::World& world, const NativeWindows& native,
                   Context& context);

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
