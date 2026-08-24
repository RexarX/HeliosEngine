#include <pch.hpp>

#include <helios/glfw/systems/create.hpp>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/details/glfw_sync.hpp>
#include <helios/glfw/details/native_handle.hpp>
#include <helios/log/log.hpp>
#include <helios/memory/ref_counted.hpp>
#include <helios/window/components.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>

#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace helios::glfw {

void CreateNativeWindows::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ecs::Query<window::Window&, ecs::Without<window::CreationFailed>> windows,
    window::CreationWriters writers) const {
  if (!context->initialized || context->world == nullptr) [[unlikely]] {
    return;
  }

  auto filtered = windows.WithEntity().Filter(
      [native](ecs::Entity entity, window::Window& window) {
        return !window.close_requested && !native->Contains(entity);
      });

  for (auto&& [entity, window] : filtered) {
    ResolveCreationSize(window);
    ApplyWindowHints(window);

    const uint32_t width = window.properties.width.value_or(1);
    const uint32_t height = window.properties.height.value_or(1);
    GLFWwindow* glfw_window =
        glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                         window.properties.title.c_str(), nullptr, nullptr);
    if (glfw_window == nullptr) [[unlikely]] {
      constexpr std::string_view kReason = "glfwCreateWindow returned null";
      log::Error("Failed to create GLFW window for entity '{}': {}", entity,
                 kReason);
      context->world->AddComponents(entity, window::CreationFailed{});
      writers.failed.Write({.entity = entity, .reason = std::string(kReason)});
      continue;
    }

    NativeEntry entry{};
    entry.window = glfw_window;
    entry.user_data = mem::MakeArc<NativeUserData>(
        NativeUserData{.world = context->world, .entity = entity});
    RegisterCallbacks(*glfw_window, *entry.user_data);

    if (window.properties.pos_x.has_value() &&
        window.properties.pos_y.has_value()) {
      glfwSetWindowPos(glfw_window, *window.properties.pos_x,
                       *window.properties.pos_y);
    }

    if (window.properties.mode == window::Mode::kWindowed &&
        window.properties.monitor_index.has_value()) {
      ApplyMonitorPlacement(*glfw_window, window);
    }

    ApplyPresentationMode(*glfw_window, window);
    SyncWindowGeometry(window, *glfw_window);
    ApplySizeLimits(*glfw_window, window.properties);
    ApplyAspectRatio(*glfw_window, window.properties);
    ApplyOpacity(*glfw_window, window.properties.opacity);
    ApplyCursorMode(*glfw_window, window.properties.cursor_mode);
    if (!window.properties.icons.empty()) {
      ApplyWindowIcons(*glfw_window, window.properties.icons);
    }
    if (window.properties.maximized) {
      ApplyMaximized(*glfw_window, true);
      SyncWindowGeometry(window, *glfw_window);
    }
    window.properties.focused =
        glfwGetWindowAttrib(glfw_window, GLFW_FOCUSED) == GLFW_TRUE;
    window.properties.hovered =
        glfwGetWindowAttrib(glfw_window, GLFW_HOVERED) == GLFW_TRUE;
    window.ClearDirty();

    writers.content_scale.Write({
        .entity = entity,
        .scale_x = window.properties.content_scale_x,
        .scale_y = window.properties.content_scale_y,
    });
    writers.created.Write({.entity = entity, .properties = window.properties});

    // NativeHandleComponent is archetype-stored; adding it migrates the
    // entity and invalidates `window`.
    context->world->AddComponents(
        entity, window::NativeHandleComponent{
                    .handle = QueryNativeHandle(*glfw_window)});
    native->Insert(entity, std::move(entry));
  }
}

}  // namespace helios::glfw
