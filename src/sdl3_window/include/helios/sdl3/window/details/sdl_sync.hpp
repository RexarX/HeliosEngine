#pragma once

#include <helios/ecs/world.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/window/components.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include <SDL3/SDL_video.h>

#include <cstdint>
#include <span>

namespace helios::sdl3::window {

[[nodiscard]] SDL_DisplayID DisplayAtIndex(int32_t index);

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
