#include <pch.hpp>

#include <helios/sdl3/window/systems/apply.hpp>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>

#include <SDL3/SDL.h>

#include <cstdint>
#include <utility>

namespace helios::sdl3::window {

namespace {

[[nodiscard]] constexpr auto ResolveClientSize(
    const ::helios::window::Properties& properties) noexcept
    -> std::pair<int, int> {
  const uint32_t width =
      properties.client_width.value_or(properties.width.value_or(1U));
  const uint32_t height =
      properties.client_height.value_or(properties.height.value_or(1U));
  return {static_cast<int>(width), static_cast<int>(height)};
}

}  // namespace

void ApplyChanges::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ::helios::window::Windows windows,
    ::helios::window::AppearanceWriters appearance,
    ecs::MessageWriter<::helios::window::PosChangedMsg> pos_changed) const {
  if (!context->initialized) [[unlikely]] {
    return;
  }

  for (auto&& [entity, window] : windows.query.WithEntity()) {
    const NativeWindows::Entry* entry = native->TryGet(entity);
    if (entry == nullptr) [[unlikely]] {
      continue;
    }

    SDL_Window* sdl_window = entry->native.window;
    if (window.close_requested) [[unlikely]] {
      window.ClearDirty();
      continue;
    }

    if (window.dirty_flags == ::helios::window::DirtyFlag::kNone) {
      continue;
    }

    const bool presentation_mode =
        ::helios::window::IsFullscreenPresentation(window.properties.mode);
    const bool exclusive =
        window.properties.mode == ::helios::window::Mode::kFullscreen;

    if (window.Dirty(::helios::window::DirtyFlag::kTitle)) {
      SDL_SetWindowTitle(sdl_window, window.properties.title.c_str());
    }

    if (window.Dirty(::helios::window::DirtyFlag::kIcon)) {
      ApplyWindowIcons(*sdl_window, window.properties.icons);
      appearance.icon.Write(
          {.entity = entity, .icons = window.properties.icons});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kMaximized) &&
        !presentation_mode) {
      ApplyMaximized(*sdl_window, window.properties.maximized);
      appearance.maximized.Write(
          {.entity = entity, .maximized = window.properties.maximized});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kMode) ||
        (window.Dirty(::helios::window::DirtyFlag::kMonitor) &&
         presentation_mode) ||
        (exclusive &&
         (window.Dirty(::helios::window::DirtyFlag::kSize) ||
          window.Dirty(::helios::window::DirtyFlag::kRefreshRate)))) {
      ApplyPresentationMode(*sdl_window, window);
      SyncWindowGeometry(window, *sdl_window);
      if (window.Dirty(::helios::window::DirtyFlag::kMode)) {
        appearance.mode.Write(
            {.entity = entity, .mode = window.properties.mode});
      }
    }

    if (window.Dirty(::helios::window::DirtyFlag::kMonitor) &&
        window.properties.mode == ::helios::window::Mode::kWindowed) {
      ApplyMonitorPlacement(*sdl_window, window);
      pos_changed.Write({
          .entity = entity,
          .x = *window.properties.pos_x,
          .y = *window.properties.pos_y,
      });
    }

    if (window.Dirty(::helios::window::DirtyFlag::kSize) &&
        !presentation_mode) {
      const auto [client_width, client_height] =
          ResolveClientSize(window.properties);
      SDL_SetWindowSize(sdl_window, client_width, client_height);
      SyncWindowGeometry(window, *sdl_window);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kPos) && !presentation_mode &&
        window.properties.pos_x.has_value() &&
        window.properties.pos_y.has_value()) {
      SDL_SetWindowPosition(sdl_window, *window.properties.pos_x,
                            *window.properties.pos_y);
      pos_changed.Write({
          .entity = entity,
          .x = *window.properties.pos_x,
          .y = *window.properties.pos_y,
      });
    }

    if (window.Dirty(::helios::window::DirtyFlag::kCursor)) {
      ApplyCursorMode(*sdl_window, window.properties.cursor_mode);
      appearance.cursor_mode.Write(
          {.entity = entity, .cursor_mode = window.properties.cursor_mode});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kVisible)) {
      if (window.properties.visible) {
        SDL_ShowWindow(sdl_window);
      } else {
        SDL_HideWindow(sdl_window);
      }
      appearance.visibility.Write(
          {.entity = entity, .visible = window.properties.visible});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kResizable) &&
        !presentation_mode) {
      SDL_SetWindowResizable(sdl_window, window.properties.resizable);
      appearance.resizable.Write(
          {.entity = entity, .resizable = window.properties.resizable});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kDecorated) &&
        window.properties.mode == ::helios::window::Mode::kWindowed) {
      SDL_SetWindowBordered(sdl_window, window.properties.decorated);
      appearance.decorated.Write(
          {.entity = entity, .decorated = window.properties.decorated});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kSizeLimits)) {
      ApplySizeLimits(*sdl_window, window.properties);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kAspectRatio)) {
      ApplyAspectRatio(*sdl_window, window.properties);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kOpacity)) {
      ApplyOpacity(*sdl_window, window.properties.opacity);
      appearance.opacity.Write(
          {.entity = entity, .opacity = window.properties.opacity});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kFloating)) {
      ApplyFloating(*sdl_window, window.properties.floating);
      appearance.floating.Write(
          {.entity = entity, .floating = window.properties.floating});
    }

    if (window.Dirty(::helios::window::DirtyFlag::kAutoIconify)) {
      ApplyAutoIconify(*sdl_window, window.properties.auto_iconify);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kFocusOnShow)) {
      ApplyFocusOnShow(window.properties.focus_on_show);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kAttention)) {
      SDL_RaiseWindow(sdl_window);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kFocus)) {
      SDL_RaiseWindow(sdl_window);
    }

    if (window.Dirty(::helios::window::DirtyFlag::kMousePassthrough)) {
      ApplyMousePassthrough(*sdl_window, window.properties.mouse_passthrough);
      appearance.mouse_passthrough.Write({
          .entity = entity,
          .mouse_passthrough = window.properties.mouse_passthrough,
      });
    }

    window.ClearDirty();
  }
}

}  // namespace helios::sdl3::window
