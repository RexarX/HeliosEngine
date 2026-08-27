#include <pch.hpp>

#include <helios/glfw/sync.hpp>

#include <helios/ecs/world.hpp>
#include <helios/glfw/state.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include <GLFW/glfw3.h>

#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <cstdint>
#include <helios/assert.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace helios::glfw {

namespace {

[[nodiscard]] GLFWmonitor* MonitorContainingPoint(int x, int y) {
  int count = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&count);
  for (int index = 0; index < count; ++index) {
    GLFWmonitor* monitor = monitors[index];
    int monitor_x = 0;
    int monitor_y = 0;
    glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);
    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    if (video_mode == nullptr) [[unlikely]] {
      continue;
    }

    const int monitor_right = monitor_x + video_mode->width;
    const int monitor_bottom = monitor_y + video_mode->height;
    if (x >= monitor_x && x < monitor_right && y >= monitor_y &&
        y < monitor_bottom) {
      return monitor;
    }
  }

  return glfwGetPrimaryMonitor();
}

GLFWmonitor* MonitorForWindow(GLFWwindow& window) {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  glfwGetWindowPos(&window, &x, &y);
  glfwGetWindowSize(&window, &width, &height);

  GLFWmonitor* monitor = MonitorContainingPoint(x + width / 2, y + height / 2);
  if (monitor == nullptr) [[unlikely]] {
    monitor = glfwGetPrimaryMonitor();
  }

  HELIOS_INVARIANT(monitor != nullptr, "No GLFW monitor available!");
  return monitor;
}

#ifdef HELIOS_PLATFORM_WINDOWS
namespace {

void CaptureClipboardSequence(Context& context) noexcept {
  context.clipboard_sequence = GetClipboardSequenceNumber();
  context.clipboard_sequence_valid = true;
}

[[nodiscard]] bool ClipboardHasUnicodeText() noexcept {
  return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

}  // namespace
#endif

}  // namespace

GLFWmonitor* MonitorAtIndex(int32_t index) {
  if (index < 0) [[unlikely]] {
    return nullptr;
  }

  int count = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&count);
  if (index >= count) [[unlikely]] {
    return nullptr;
  }

  return monitors[index];
}

GLFWmonitor* ResolveMonitor(const window::Properties& properties,
                            GLFWwindow& window) {
  if (properties.monitor_index.has_value()) {
    GLFWmonitor* monitor = MonitorAtIndex(*properties.monitor_index);
    if (monitor != nullptr) [[likely]] {
      return monitor;
    }
  }

  return MonitorForWindow(window);
}

void RefreshMonitors(window::Monitors& monitors) {
  monitors.monitors.clear();

  int count = 0;
  GLFWmonitor** glfw_monitors = glfwGetMonitors(&count);
  GLFWmonitor* primary = glfwGetPrimaryMonitor();

  for (int index = 0; index < count; ++index) {
    GLFWmonitor* monitor = glfw_monitors[index];
    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    HELIOS_INVARIANT(video_mode != nullptr,
                     "Failed to query video mode for monitor {}!", index);

    window::Monitor entry;
    entry.index = index;
    entry.primary = monitor == primary;
    glfwGetMonitorPos(monitor, &entry.x, &entry.y);
    entry.width = static_cast<uint32_t>(video_mode->width);
    entry.height = static_cast<uint32_t>(video_mode->height);

    int work_x = 0;
    int work_y = 0;
    int work_width = 0;
    int work_height = 0;
    glfwGetMonitorWorkarea(monitor, &work_x, &work_y, &work_width,
                           &work_height);
    entry.work_x = work_x;
    entry.work_y = work_y;
    entry.work_width = static_cast<uint32_t>(work_width);
    entry.work_height = static_cast<uint32_t>(work_height);

    if (const char* name = glfwGetMonitorName(monitor); name != nullptr) {
      entry.name = name;
    }

    int physical_width = 0;
    int physical_height = 0;
    glfwGetMonitorPhysicalSize(monitor, &physical_width, &physical_height);
    entry.physical_width_mm = physical_width;
    entry.physical_height_mm = physical_height;

    entry.current = {
        .width = static_cast<uint32_t>(video_mode->width),
        .height = static_cast<uint32_t>(video_mode->height),
        .refresh_rate = static_cast<uint32_t>(video_mode->refreshRate),
    };

    int mode_count = 0;
    const GLFWvidmode* video_modes = glfwGetVideoModes(monitor, &mode_count);
    entry.modes.reserve(static_cast<size_t>(mode_count));
    for (int mode_index = 0; mode_index < mode_count; ++mode_index) {
      const GLFWvidmode& mode = video_modes[mode_index];
      entry.modes.push_back({
          .width = static_cast<uint32_t>(mode.width),
          .height = static_cast<uint32_t>(mode.height),
          .refresh_rate = static_cast<uint32_t>(mode.refreshRate),
      });
    }

    monitors.monitors.push_back(entry);
  }
}

void MarkMonitorDependentDirty(ecs::World& world, NativeWindows& native) {
  for (const NativeWindows::Entry& entry : native.entries) {
    auto* component = world.TryWriteComponent<window::Window>(entry.entity);
    if (component == nullptr) [[unlikely]] {
      continue;
    }

    if (component->properties.monitor_index.has_value()) {
      component->MarkDirty(window::DirtyFlag::kMonitor);
    }
    if (window::IsFullscreenPresentation(component->properties.mode)) {
      component->MarkDirty(window::DirtyFlag::kMode);
    }
  }
}

void ApplyWindowHints(const window::Window& window) {
  glfwWindowHint(GLFW_VISIBLE,
                 window.properties.visible ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUSED,
                 window.properties.focused ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_RESIZABLE,
                 window.properties.resizable ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_DECORATED,
                 window.properties.decorated ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_FLOATING,
                 window.properties.floating ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_AUTO_ICONIFY,
                 window.properties.auto_iconify ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUS_ON_SHOW,
                 window.properties.focus_on_show ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(
      GLFW_TRANSPARENT_FRAMEBUFFER,
      window.properties.transparent_framebuffer ? GLFW_TRUE : GLFW_FALSE);
#ifdef GLFW_SCALE_TO_MONITOR
  glfwWindowHint(GLFW_SCALE_TO_MONITOR,
                 window.properties.scale_to_monitor ? GLFW_TRUE : GLFW_FALSE);
#endif
#ifdef GLFW_SCALE_FRAMEBUFFER
  glfwWindowHint(GLFW_SCALE_FRAMEBUFFER,
                 window.properties.scale_framebuffer ? GLFW_TRUE : GLFW_FALSE);
#endif
#ifdef GLFW_MOUSE_PASSTHROUGH
  glfwWindowHint(GLFW_MOUSE_PASSTHROUGH,
                 window.properties.mouse_passthrough ? GLFW_TRUE : GLFW_FALSE);
#endif

  int client_api = GLFW_NO_API;
  if (window.properties.client_api == window::ClientApi::kOpenGL) {
    client_api = GLFW_OPENGL_API;
  }
  glfwWindowHint(GLFW_CLIENT_API, client_api);
}

void ApplyCursorMode(GLFWwindow& native_window, window::CursorMode mode) {
  int glfw_mode = GLFW_CURSOR_NORMAL;
  switch (mode) {
    using enum window::CursorMode;
    case kVisible:
      glfw_mode = GLFW_CURSOR_NORMAL;
      break;
    case kHidden:
      glfw_mode = GLFW_CURSOR_HIDDEN;
      break;
    case kDisabled:
      glfw_mode = GLFW_CURSOR_DISABLED;
      break;
    case kCaptured:
      glfw_mode = GLFW_CURSOR_CAPTURED;
      break;
  }
  glfwSetInputMode(&native_window, GLFW_CURSOR, glfw_mode);
}

void ResolveCreationSize(window::Window& window) {
  if (window.properties.width.has_value() &&
      window.properties.height.has_value()) {
    return;
  }

  GLFWmonitor* monitor = nullptr;
  if (window.properties.monitor_index.has_value()) {
    monitor = MonitorAtIndex(*window.properties.monitor_index);
  }

  if (monitor == nullptr) {
    monitor = glfwGetPrimaryMonitor();
  }

  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  HELIOS_INVARIANT(mode != nullptr, "Failed to query monitor video mode!");
  const auto [default_width, default_height] = window::DefaultSizeForScreen(
      static_cast<uint32_t>(mode->width), static_cast<uint32_t>(mode->height));

  if (!window.properties.width.has_value()) {
    window.properties.width = default_width;
  }

  if (!window.properties.height.has_value()) {
    window.properties.height = default_height;
  }
}

void ApplyPresentationMode(GLFWwindow& native_window,
                           const window::Window& window) {
  const window::Mode mode = window.properties.mode;

  if (mode == window::Mode::kFullscreen) {
    GLFWmonitor* monitor = ResolveMonitor(window.properties, native_window);
    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    HELIOS_INVARIANT(video_mode != nullptr,
                     "Failed to query video mode for fullscreen monitor!");
    const auto requested = window::ResolveExclusiveVideoMode(
        window.properties,
        {
            .width = static_cast<uint32_t>(video_mode->width),
            .height = static_cast<uint32_t>(video_mode->height),
            .refresh_rate = static_cast<uint32_t>(video_mode->refreshRate),
        });
    glfwSetWindowMonitor(&native_window, monitor, 0, 0,
                         static_cast<int>(requested.width),
                         static_cast<int>(requested.height),
                         static_cast<int>(requested.refresh_rate));
    return;
  }

  if (mode == window::Mode::kBorderless) {
    GLFWmonitor* monitor = ResolveMonitor(window.properties, native_window);
    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    HELIOS_INVARIANT(video_mode != nullptr,
                     "Failed to query video mode for borderless monitor!");
    int monitor_x = 0;
    int monitor_y = 0;
    glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);
    glfwSetWindowMonitor(&native_window, nullptr, monitor_x, monitor_y,
                         video_mode->width, video_mode->height, 0);
    glfwSetWindowAttrib(&native_window, GLFW_DECORATED, GLFW_FALSE);
    return;
  }

  const int width = static_cast<int>(window.properties.width.value_or(1));
  const int height = static_cast<int>(window.properties.height.value_or(1));
  int x = 0;
  int y = 0;
  if (window.properties.pos_x.has_value() &&
      window.properties.pos_y.has_value()) {
    x = *window.properties.pos_x;
    y = *window.properties.pos_y;
  } else {
    glfwGetWindowPos(&native_window, &x, &y);
  }

  glfwSetWindowMonitor(&native_window, nullptr, x, y, width, height, 0);
  glfwSetWindowAttrib(&native_window, GLFW_DECORATED,
                      window.properties.decorated ? GLFW_TRUE : GLFW_FALSE);
  glfwSetWindowAttrib(&native_window, GLFW_RESIZABLE,
                      window.properties.resizable ? GLFW_TRUE : GLFW_FALSE);
}

void ApplyMonitorPlacement(GLFWwindow& native_window, window::Window& window) {
  GLFWmonitor* monitor = ResolveMonitor(window.properties, native_window);
  int work_x = 0;
  int work_y = 0;
  int work_width = 0;
  int work_height = 0;
  glfwGetMonitorWorkarea(monitor, &work_x, &work_y, &work_width, &work_height);

  const int width = static_cast<int>(window.properties.width.value_or(
      static_cast<uint32_t>(work_width > 0 ? work_width : 1)));
  const int height = static_cast<int>(window.properties.height.value_or(
      static_cast<uint32_t>(work_height > 0 ? work_height : 1)));

  int x = work_x + (work_width - width) / 2;
  int y = work_y + (work_height - height) / 2;
  if (window.properties.pos_x.has_value() &&
      window.properties.pos_y.has_value()) {
    x = *window.properties.pos_x;
    y = *window.properties.pos_y;
  }

  glfwSetWindowPos(&native_window, x, y);
  window.properties.pos_x = x;
  window.properties.pos_y = y;
}

void SyncWindowGeometry(window::Window& window, GLFWwindow& native_window) {
  int framebuffer_width = 0;
  int framebuffer_height = 0;
  glfwGetFramebufferSize(&native_window, &framebuffer_width,
                         &framebuffer_height);
  window.properties.width = static_cast<uint32_t>(framebuffer_width);
  window.properties.height = static_cast<uint32_t>(framebuffer_height);

  int client_width = 0;
  int client_height = 0;
  glfwGetWindowSize(&native_window, &client_width, &client_height);
  window.properties.client_width = static_cast<uint32_t>(client_width);
  window.properties.client_height = static_cast<uint32_t>(client_height);

  int x = 0;
  int y = 0;
  glfwGetWindowPos(&native_window, &x, &y);
  window.properties.pos_x = x;
  window.properties.pos_y = y;

  float scale_x = 1.0F;
  float scale_y = 1.0F;
  glfwGetWindowContentScale(&native_window, &scale_x, &scale_y);
  window.properties.content_scale_x = scale_x;
  window.properties.content_scale_y = scale_y;

  window.properties.maximized =
      glfwGetWindowAttrib(&native_window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

void ApplyWindowIcons(GLFWwindow& native_window,
                      std::span<const window::IconImage> icons) {
  if (icons.empty()) {
    glfwSetWindowIcon(&native_window, 0, nullptr);
    return;
  }

  std::vector<GLFWimage> glfw_images;
  glfw_images.reserve(icons.size());
  for (const window::IconImage& icon : icons) {
    if (icon.width == 0 || icon.height == 0 ||
        icon.rgba.size() != static_cast<size_t>(icon.width) *
                                static_cast<size_t>(icon.height) * 4U) {
      continue;
    }

    glfw_images.push_back({
        .width = static_cast<int>(icon.width),
        .height = static_cast<int>(icon.height),
        .pixels = const_cast<unsigned char*>(icon.rgba.data()),
    });
  }

  glfwSetWindowIcon(&native_window, static_cast<int>(glfw_images.size()),
                    glfw_images.data());
}

void ApplyMaximized(GLFWwindow& native_window, bool maximized) {
  if (maximized) {
    glfwMaximizeWindow(&native_window);
    return;
  }

  glfwRestoreWindow(&native_window);
}

void ApplySizeLimits(GLFWwindow& native_window,
                     const window::Properties& properties) {
  const int min_width = properties.min_width.has_value()
                            ? static_cast<int>(*properties.min_width)
                            : GLFW_DONT_CARE;
  const int min_height = properties.min_height.has_value()
                             ? static_cast<int>(*properties.min_height)
                             : GLFW_DONT_CARE;
  const int max_width = properties.max_width.has_value()
                            ? static_cast<int>(*properties.max_width)
                            : GLFW_DONT_CARE;
  const int max_height = properties.max_height.has_value()
                             ? static_cast<int>(*properties.max_height)
                             : GLFW_DONT_CARE;
  glfwSetWindowSizeLimits(&native_window, min_width, min_height, max_width,
                          max_height);
}

void ApplyAspectRatio(GLFWwindow& native_window,
                      const window::Properties& properties) {
  if (properties.aspect_numer.has_value() &&
      properties.aspect_denom.has_value()) {
    glfwSetWindowAspectRatio(&native_window, *properties.aspect_numer,
                             *properties.aspect_denom);
    return;
  }

  glfwSetWindowAspectRatio(&native_window, GLFW_DONT_CARE, GLFW_DONT_CARE);
}

void ApplyOpacity(GLFWwindow& native_window, float opacity) {
  glfwSetWindowOpacity(&native_window, opacity);
}

void ApplyFloating(GLFWwindow& native_window, bool floating) {
  glfwSetWindowAttrib(&native_window, GLFW_FLOATING,
                      floating ? GLFW_TRUE : GLFW_FALSE);
}

void ApplyAutoIconify(GLFWwindow& native_window, bool auto_iconify) {
  glfwSetWindowAttrib(&native_window, GLFW_AUTO_ICONIFY,
                      auto_iconify ? GLFW_TRUE : GLFW_FALSE);
}

void ApplyFocusOnShow(GLFWwindow& native_window, bool focus_on_show) {
  glfwSetWindowAttrib(&native_window, GLFW_FOCUS_ON_SHOW,
                      focus_on_show ? GLFW_TRUE : GLFW_FALSE);
}

void SyncClipboard(ecs::World& world, const NativeWindows& native,
                   Context& context) {
  auto* clipboard = world.TryWriteResource<window::Clipboard>();
  if (clipboard == nullptr) [[unlikely]] {
    return;
  }

  GLFWwindow* glfw_window = nullptr;
  if (!native.entries.empty()) {
    glfw_window = native.entries.front().native.window;
  }

  if (clipboard->pending_write && glfw_window != nullptr) {
    glfwSetClipboardString(glfw_window, clipboard->text.c_str());
    clipboard->pending_write = false;
#ifdef HELIOS_PLATFORM_WINDOWS
    CaptureClipboardSequence(context);
#endif
    context.clipboard_dirty = false;
  }

  if (glfw_window == nullptr) [[unlikely]] {
    context.clipboard_dirty = false;
    return;
  }

  bool should_read = context.clipboard_dirty;
#ifdef HELIOS_PLATFORM_WINDOWS
  if (!context.clipboard_sequence_valid ||
      GetClipboardSequenceNumber() != context.clipboard_sequence) {
    should_read = true;
  }
#endif

  if (!should_read) {
    return;
  }

#ifdef HELIOS_PLATFORM_WINDOWS
  if (!ClipboardHasUnicodeText()) {
    CaptureClipboardSequence(context);
    context.clipboard_dirty = false;
    return;
  }
#endif

  const char* clipboard_text = glfwGetClipboardString(glfw_window);
#ifdef HELIOS_PLATFORM_WINDOWS
  // Re-sample after the OS call. Clipboard History can bump the sequence
  // during OpenClipboard; keeping the pre-get value re-reads every frame.
  CaptureClipboardSequence(context);
#endif
  context.clipboard_dirty = false;

  if (clipboard_text == nullptr) [[unlikely]] {
    return;
  }

  const auto new_text = std::string_view{clipboard_text};
  if (new_text == clipboard->text) {
    return;
  }

  clipboard->text.assign(new_text);
  world.WriteMessages<window::ClipboardChangedMsg>().Write(
      {.text = clipboard->text});
}

}  // namespace helios::glfw
