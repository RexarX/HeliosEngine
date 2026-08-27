#include <pch.hpp>

#include <helios/sdl3/window/systems/create.hpp>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/params.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/log/log.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/window/native_handle.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace helios::sdl3::window {

void CreateNativeWindows::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ecs::Res<WindowMap> window_map, ecs::Res<const sdl3::Context> sdl_context,
    ecs::Query<::helios::window::Window&,
               ecs::Without<::helios::window::CreationFailed>>
        windows,
    ::helios::window::CreationWriters writers) const {
  if (!context->initialized || sdl_context->world == nullptr) [[unlikely]] {
    return;
  }

  auto filtered = windows.WithEntity().Filter(
      [native](ecs::Entity entity, ::helios::window::Window& window) {
        return !window.close_requested && !native->Contains(entity);
      });

  for (auto&& [entity, window] : filtered) {
    ResolveCreationSize(window);

    const uint32_t width = window.properties.width.value_or(1);
    const uint32_t height = window.properties.height.value_or(1);
    const SDL_WindowFlags flags = BuildCreationFlags(window);

    const char* previous_hint =
        SDL_GetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN);
    const std::string previous =
        previous_hint != nullptr ? previous_hint : std::string{};
    const bool activate =
        window.properties.focused && window.properties.visible;
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, activate ? "1" : "0");

    SDL_Window* sdl_window = SDL_CreateWindow(window.properties.title.c_str(),
                                              static_cast<int>(width),
                                              static_cast<int>(height), flags);
    if (!previous.empty()) {
      SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, previous.c_str());
    } else {
      SDL_ResetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN);
    }
    if (sdl_window == nullptr) [[unlikely]] {
      constexpr std::string_view kReason = "SDL_CreateWindow returned null";
      log::Error("Failed to create SDL window for entity '{}': {}", entity,
                 kReason);
      sdl_context->world->AddComponents(entity,
                                        ::helios::window::CreationFailed{});
      writers.failed.Write({.entity = entity, .reason = std::string(kReason)});
      continue;
    }

    if (window.properties.pos_x.has_value() &&
        window.properties.pos_y.has_value()) {
      SDL_SetWindowPosition(sdl_window, *window.properties.pos_x,
                            *window.properties.pos_y);
    }

    if (window.properties.mode == ::helios::window::Mode::kWindowed &&
        window.properties.monitor_index.has_value()) {
      ApplyMonitorPlacement(*sdl_window, window);
    }

    ApplyPresentationMode(*sdl_window, window);
    SyncWindowGeometry(window, *sdl_window);
    ApplySizeLimits(*sdl_window, window.properties);
    ApplyAspectRatio(*sdl_window, window.properties);
    ApplyOpacity(*sdl_window, window.properties.opacity);
    ApplyCursorMode(*sdl_window, window.properties.cursor_mode);
    ApplyFocusOnShow(window.properties.focus_on_show);
    ApplyAutoIconify(*sdl_window, window.properties.auto_iconify);
    ApplyMousePassthrough(*sdl_window, window.properties.mouse_passthrough);
    SDL_StartTextInput(sdl_window);
    if (!window.properties.icons.empty()) {
      ApplyWindowIcons(*sdl_window, window.properties.icons);
    }
    if (window.properties.maximized) {
      ApplyMaximized(*sdl_window, true);
      SyncWindowGeometry(window, *sdl_window);
    }

    const SDL_WindowFlags window_flags = SDL_GetWindowFlags(sdl_window);
    window.properties.focused = (window_flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    window.properties.hovered = (window_flags & SDL_WINDOW_MOUSE_FOCUS) != 0;

    window.ClearDirty();

    writers.content_scale.Write({
        .entity = entity,
        .scale_x = window.properties.content_scale_x,
        .scale_y = window.properties.content_scale_y,
    });
    writers.created.Write({.entity = entity, .properties = window.properties});

    // NativeHandleComponent is archetype-stored; adding it migrates the
    // entity and invalidates `window`.
    sdl_context->world->AddComponents(
        entity, ::helios::window::NativeHandleComponent{
                    .handle = QueryNativeHandle(*sdl_window)});

    native->Insert(entity, NativeEntry{.window = sdl_window});
    window_map->Insert(entity, sdl_window, SDL_GetWindowID(sdl_window));
  }
}

}  // namespace helios::sdl3::window
