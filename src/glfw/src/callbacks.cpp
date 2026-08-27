#include <pch.hpp>

#include <helios/glfw/state.hpp>

#include <helios/ecs/world.hpp>
#include <helios/glfw/sync.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/resources.hpp>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/input/messages.hpp>
#endif

#include <GLFW/glfw3.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/glfw/details/input_map.hpp>
#endif

namespace helios::glfw {

namespace {

ecs::World* g_monitor_callback_world = nullptr;

struct CallbackContext {
  ecs::World& world;
  ecs::Entity entity;
};

[[nodiscard]] auto TryGetCallbackContext(GLFWwindow* glfw_window)
    -> std::optional<CallbackContext> {
  auto* user_data =
      static_cast<NativeUserData*>(glfwGetWindowUserPointer(glfw_window));
  if (user_data == nullptr || user_data->world == nullptr ||
      !user_data->entity.Valid()) [[unlikely]] {
    return std::nullopt;
  }

  return CallbackContext{.world = *user_data->world,
                         .entity = user_data->entity};
}

void RequestNestedFramePump(ecs::World& world) {
  auto* glfw_context = world.TryWriteResource<Context>();
  if (glfw_context == nullptr || !glfw_context->in_event_poll ||
      glfw_context->frame_pump == nullptr || glfw_context->in_nested_pump) {
    return;
  }

  glfw_context->in_nested_pump = true;
  glfw_context->frame_pump(glfw_context->frame_pump_user_data);
  glfw_context->in_nested_pump = false;
}

void FramebufferSizeCallback(GLFWwindow* glfw_window, int width, int height) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.width = static_cast<uint32_t>(width);
    component->properties.height = static_cast<uint32_t>(height);
  }

  context->world.WriteMessages<window::ResizedMsg>().Write({
      .entity = context->entity,
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height),
  });
  RequestNestedFramePump(context->world);
}

void WindowSizeCallback(GLFWwindow* glfw_window, int width, int height) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.client_width = static_cast<uint32_t>(width);
    component->properties.client_height = static_cast<uint32_t>(height);
  }

  context->world.WriteMessages<window::ClientResizedMsg>().Write({
      .entity = context->entity,
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height),
  });
  RequestNestedFramePump(context->world);
}

void WindowContentScaleCallback(GLFWwindow* glfw_window, float scale_x,
                                float scale_y) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.content_scale_x = scale_x;
    component->properties.content_scale_y = scale_y;
  }

  context->world.WriteMessages<window::ContentScaleChangedMsg>().Write({
      .entity = context->entity,
      .scale_x = scale_x,
      .scale_y = scale_y,
  });
  RequestNestedFramePump(context->world);
}

void WindowPosCallback(GLFWwindow* glfw_window, int x, int y) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.pos_x = x;
    component->properties.pos_y = y;
  }

  context->world.WriteMessages<window::PosChangedMsg>().Write(
      {.entity = context->entity, .x = x, .y = y});
  RequestNestedFramePump(context->world);
}

void WindowIconifyCallback(GLFWwindow* glfw_window, int iconified) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  const bool visible = iconified == GLFW_FALSE;
  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.visible = visible;
  }

  context->world.WriteMessages<window::VisibilityChangedMsg>().Write(
      {.entity = context->entity, .visible = visible});
}

void WindowMaximizeCallback(GLFWwindow* glfw_window, int maximized) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  const bool is_maximized = maximized == GLFW_TRUE;
  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.maximized = is_maximized;
  }

  context->world.WriteMessages<window::MaximizedChangedMsg>().Write(
      {.entity = context->entity, .maximized = is_maximized});
}

void FocusCallback(GLFWwindow* glfw_window, int focused) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  const bool is_focused = focused == GLFW_TRUE;
  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.focused = is_focused;
  }

  if (is_focused) {
    if (auto* glfw_context = context->world.TryWriteResource<Context>()) {
      glfw_context->clipboard_dirty = true;
    }
  }

  context->world.WriteMessages<window::FocusChangedMsg>().Write(
      {.entity = context->entity, .focused = is_focused});
}

void CloseCallback(GLFWwindow* glfw_window) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  context->world.WriteMessages<window::CloseRequestedMsg>().Write(
      {.entity = context->entity});

  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->RequestClose();
  }

  glfwSetWindowShouldClose(glfw_window, GLFW_FALSE);
}

void CursorEnterCallback(GLFWwindow* glfw_window, int entered) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  const bool hovered = entered == GLFW_TRUE;
  if (auto* component =
          context->world.TryWriteComponent<window::Window>(context->entity)) {
    component->properties.hovered = hovered;
  }

  context->world.WriteMessages<window::HoverChangedMsg>().Write(
      {.entity = context->entity, .hovered = hovered});
}

void DropCallback(GLFWwindow* glfw_window, int count, const char* paths[]) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value()) [[unlikely]] {
    return;
  }

  std::vector<std::string> dropped_paths;
  dropped_paths.reserve(static_cast<size_t>(count));
  for (int index = 0; index < count; ++index) {
    if (paths[index] != nullptr) {
      dropped_paths.emplace_back(paths[index]);
    }
  }

  context->world.WriteMessages<window::DroppedFilesMsg>().Write({
      .entity = context->entity,
      .paths = std::move(dropped_paths),
  });
}

void MonitorCallback(GLFWmonitor* monitor, int event) {
  if (g_monitor_callback_world == nullptr || monitor == nullptr) [[unlikely]] {
    return;
  }

  ecs::World& world = *g_monitor_callback_world;

  int monitor_count = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
  int monitor_index = 0;
  for (int index = 0; index < monitor_count; ++index) {
    if (monitors[index] == monitor) {
      monitor_index = index;
      break;
    }
  }

  if (auto* layout = world.TryWriteResource<window::Monitors>();
      layout != nullptr) {
    RefreshMonitors(*layout);
  }

  if (event == GLFW_CONNECTED) {
    world.WriteMessages<::helios::window::MonitorConnectedMsg>().Write(
        {.index = monitor_index});
  } else if (event == GLFW_DISCONNECTED) {
    world.WriteMessages<::helios::window::MonitorDisconnectedMsg>().Write(
        {.index = monitor_index});
  }

  if (auto* native = world.TryWriteResource<NativeWindows>();
      native != nullptr) {
    MarkMonitorDependentDirty(world, *native);
  }
}

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

[[nodiscard]] bool InputEnabled(ecs::World& world) noexcept {
  const auto* ctx = world.TryReadResource<Context>();
  return ctx != nullptr && ctx->input_enabled;
}

void KeyCallback(GLFWwindow* glfw_window, int key, int /*scancode*/, int action,
                 int mods) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value() || !InputEnabled(context->world)) [[unlikely]] {
    return;
  }

  context->world.WriteMessages<input::KeyboardInputMsg>().Write({
      .entity = context->entity,
      .key = KeyFromGlfw(key),
      .state = ButtonStateFromGlfw(action),
      .modifiers = ModifiersFromGlfw(mods),
  });
}

void CharCallback(GLFWwindow* glfw_window, unsigned int codepoint) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value() || !InputEnabled(context->world)) [[unlikely]] {
    return;
  }

  context->world.WriteMessages<input::TextInputMsg>().Write({
      .entity = context->entity,
      .codepoint = codepoint,
  });
}

void MouseButtonCallback(GLFWwindow* glfw_window, int button, int action,
                         int mods) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value() || !InputEnabled(context->world)) [[unlikely]] {
    return;
  }

  const auto mapped = MouseButtonFromGlfw(button);
  if (!mapped.has_value()) {
    return;
  }

  context->world.WriteMessages<input::MouseButtonInputMsg>().Write({
      .entity = context->entity,
      .button = *mapped,
      .state = ButtonStateFromGlfw(action),
      .modifiers = ModifiersFromGlfw(mods),
  });
}

void CursorPosCallback(GLFWwindow* glfw_window, double x, double y) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value() || !InputEnabled(context->world)) [[unlikely]] {
    return;
  }

  context->world.WriteMessages<input::CursorMovedMsg>().Write({
      .entity = context->entity,
      .x = x,
      .y = y,
  });

  auto* native = context->world.TryWriteResource<NativeWindows>();
  if (native == nullptr) [[unlikely]] {
    return;
  }

  NativeWindows::Entry* entry = native->TryGet(context->entity);
  if (entry == nullptr) [[unlikely]] {
    return;
  }

  if (entry->native.has_cursor) {
    context->world.WriteMessages<input::MouseMotionMsg>().Write({
        .entity = context->entity,
        .delta_x = x - entry->native.last_cursor_x,
        .delta_y = y - entry->native.last_cursor_y,
    });
  }

  entry->native.last_cursor_x = x;
  entry->native.last_cursor_y = y;
  entry->native.has_cursor = true;
}

void ScrollCallback(GLFWwindow* glfw_window, double x, double y) {
  const auto context = TryGetCallbackContext(glfw_window);
  if (!context.has_value() || !InputEnabled(context->world)) [[unlikely]] {
    return;
  }

  context->world.WriteMessages<input::MouseWheelMsg>().Write({
      .entity = context->entity,
      .x = x,
      .y = y,
  });
}

#endif  // HELIOS_MODULE_INPUT_AVAILABLE

}  // namespace

void RegisterCallbacks(GLFWwindow& window, NativeUserData& user_data) {
  glfwSetWindowUserPointer(&window, &user_data);
  glfwSetFramebufferSizeCallback(&window, FramebufferSizeCallback);
  glfwSetWindowSizeCallback(&window, WindowSizeCallback);
  glfwSetWindowContentScaleCallback(&window, WindowContentScaleCallback);
  glfwSetWindowPosCallback(&window, WindowPosCallback);
  glfwSetWindowIconifyCallback(&window, WindowIconifyCallback);
  glfwSetWindowMaximizeCallback(&window, WindowMaximizeCallback);
  glfwSetWindowFocusCallback(&window, FocusCallback);
  glfwSetWindowCloseCallback(&window, CloseCallback);
  glfwSetCursorEnterCallback(&window, CursorEnterCallback);
  glfwSetDropCallback(&window, DropCallback);
  // No glfwSetWindowRefreshCallback: nested FramePumpOrder runs from size /
  // pos / scale only. WM_PAINT would nest Update inside every compositor paint
  // processed by glfwPollEvents.
#ifdef HELIOS_MODULE_INPUT_AVAILABLE
  glfwSetKeyCallback(&window, KeyCallback);
  glfwSetCharCallback(&window, CharCallback);
  glfwSetMouseButtonCallback(&window, MouseButtonCallback);
  glfwSetCursorPosCallback(&window, CursorPosCallback);
  glfwSetScrollCallback(&window, ScrollCallback);
#endif
}

void RegisterMonitorCallback(ecs::World& world) {
  g_monitor_callback_world = &world;
  glfwSetMonitorCallback(MonitorCallback);
}

void UnregisterMonitorCallback() {
  glfwSetMonitorCallback(nullptr);
  g_monitor_callback_world = nullptr;
}

}  // namespace helios::glfw
