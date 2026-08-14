#include <pch.hpp>

#include <helios/glfw/systems/apply.hpp>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/details/glfw_sync.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>

#include <GLFW/glfw3.h>

#include <cstdint>
#include <utility>

namespace helios::glfw {

namespace {

[[nodiscard]] constexpr auto ResolveClientSize(
    const window::Properties& properties) noexcept -> std::pair<int, int> {
  const uint32_t width =
      properties.client_width.value_or(properties.width.value_or(1U));
  const uint32_t height =
      properties.client_height.value_or(properties.height.value_or(1U));
  return {static_cast<int>(width), static_cast<int>(height)};
}

}  // namespace

void ApplyChanges::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    window::Windows windows, window::AppearanceWriters appearance,
    ecs::MessageWriter<window::PosChangedMsg> pos_changed) const {
  if (!context->initialized) [[unlikely]] {
    return;
  }

  for (auto&& [entity, window] : windows.query.WithEntity()) {
    const NativeWindows::Entry* entry = native->TryGet(entity);
    if (entry == nullptr) [[unlikely]] {
      continue;
    }

    GLFWwindow* glfw_window = entry->native.window;
    if (window.close_requested) [[unlikely]] {
      window.ClearDirty();
      continue;
    }

    if (window.dirty_flags == window::DirtyFlag::kNone) {
      continue;
    }

    const bool presentation_mode =
        window::IsFullscreenPresentation(window.properties.mode);
    const bool exclusive = window.properties.mode == window::Mode::kFullscreen;

    if (window.Dirty(window::DirtyFlag::kTitle)) {
      glfwSetWindowTitle(glfw_window, window.properties.title.c_str());
    }

    if (window.Dirty(window::DirtyFlag::kIcon)) {
      ApplyWindowIcons(*glfw_window, window.properties.icons);
      appearance.icon.Write(
          {.entity = entity, .icons = window.properties.icons});
    }

    if (window.Dirty(window::DirtyFlag::kMaximized) && !presentation_mode) {
      ApplyMaximized(*glfw_window, window.properties.maximized);
      appearance.maximized.Write(
          {.entity = entity, .maximized = window.properties.maximized});
    }

    if (window.Dirty(window::DirtyFlag::kMode) ||
        (window.Dirty(window::DirtyFlag::kMonitor) && presentation_mode) ||
        (exclusive && (window.Dirty(window::DirtyFlag::kSize) ||
                       window.Dirty(window::DirtyFlag::kRefreshRate)))) {
      ApplyPresentationMode(*glfw_window, window);
      SyncWindowGeometry(window, *glfw_window);
      if (window.Dirty(window::DirtyFlag::kMode)) {
        appearance.mode.Write(
            {.entity = entity, .mode = window.properties.mode});
      }
    }

    if (window.Dirty(window::DirtyFlag::kMonitor) &&
        window.properties.mode == window::Mode::kWindowed) {
      ApplyMonitorPlacement(*glfw_window, window);
      pos_changed.Write({
          .entity = entity,
          .x = *window.properties.pos_x,
          .y = *window.properties.pos_y,
      });
    }

    if (window.Dirty(window::DirtyFlag::kSize) && !presentation_mode) {
      const auto [client_width, client_height] =
          ResolveClientSize(window.properties);
      glfwSetWindowSize(glfw_window, client_width, client_height);
      SyncWindowGeometry(window, *glfw_window);
    }

    if (window.Dirty(window::DirtyFlag::kPos) && !presentation_mode &&
        window.properties.pos_x.has_value() &&
        window.properties.pos_y.has_value()) {
      glfwSetWindowPos(glfw_window, *window.properties.pos_x,
                       *window.properties.pos_y);
      pos_changed.Write({
          .entity = entity,
          .x = *window.properties.pos_x,
          .y = *window.properties.pos_y,
      });
    }

    if (window.Dirty(window::DirtyFlag::kCursor)) {
      ApplyCursorMode(*glfw_window, window.properties.cursor_mode);
      appearance.cursor_mode.Write(
          {.entity = entity, .cursor_mode = window.properties.cursor_mode});
    }

    if (window.Dirty(window::DirtyFlag::kVisible)) {
      if (window.properties.visible) {
        glfwShowWindow(glfw_window);
      } else {
        glfwHideWindow(glfw_window);
      }
      appearance.visibility.Write(
          {.entity = entity, .visible = window.properties.visible});
    }

    if (window.Dirty(window::DirtyFlag::kResizable) && !presentation_mode) {
      glfwSetWindowAttrib(glfw_window, GLFW_RESIZABLE,
                          window.properties.resizable ? GLFW_TRUE : GLFW_FALSE);
      appearance.resizable.Write(
          {.entity = entity, .resizable = window.properties.resizable});
    }

    if (window.Dirty(window::DirtyFlag::kDecorated) &&
        window.properties.mode == window::Mode::kWindowed) {
      glfwSetWindowAttrib(glfw_window, GLFW_DECORATED,
                          window.properties.decorated ? GLFW_TRUE : GLFW_FALSE);
      appearance.decorated.Write(
          {.entity = entity, .decorated = window.properties.decorated});
    }

    if (window.Dirty(window::DirtyFlag::kSizeLimits)) {
      ApplySizeLimits(*glfw_window, window.properties);
    }

    if (window.Dirty(window::DirtyFlag::kAspectRatio)) {
      ApplyAspectRatio(*glfw_window, window.properties);
    }

    if (window.Dirty(window::DirtyFlag::kOpacity)) {
      ApplyOpacity(*glfw_window, window.properties.opacity);
      appearance.opacity.Write(
          {.entity = entity, .opacity = window.properties.opacity});
    }

    if (window.Dirty(window::DirtyFlag::kFloating)) {
      ApplyFloating(*glfw_window, window.properties.floating);
      appearance.floating.Write(
          {.entity = entity, .floating = window.properties.floating});
    }

    if (window.Dirty(window::DirtyFlag::kAutoIconify)) {
      ApplyAutoIconify(*glfw_window, window.properties.auto_iconify);
    }

    if (window.Dirty(window::DirtyFlag::kFocusOnShow)) {
      ApplyFocusOnShow(*glfw_window, window.properties.focus_on_show);
    }

    if (window.Dirty(window::DirtyFlag::kAttention)) {
      glfwRequestWindowAttention(glfw_window);
    }

    if (window.Dirty(window::DirtyFlag::kFocus)) {
      glfwFocusWindow(glfw_window);
    }

    if (window.Dirty(window::DirtyFlag::kMousePassthrough)) {
#ifdef GLFW_MOUSE_PASSTHROUGH
      glfwSetWindowAttrib(
          glfw_window, GLFW_MOUSE_PASSTHROUGH,
          window.properties.mouse_passthrough ? GLFW_TRUE : GLFW_FALSE);
#endif
      appearance.mouse_passthrough.Write({
          .entity = entity,
          .mouse_passthrough = window.properties.mouse_passthrough,
      });
    }

    window.ClearDirty();
  }
}

}  // namespace helios::glfw
