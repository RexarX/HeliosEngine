#include <pch.hpp>

#include <helios/sdl3/window/sync.hpp>

#include <helios/ecs/world.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/ids.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/settings.hpp>

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <cstddef>
#include <cstdint>
#include <helios/assert.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace helios::sdl3::window {

namespace {

[[nodiscard]] SDL_DisplayID DisplayContainingPoint(int x, int y) {
  const auto point = SDL_Point{.x = x, .y = y};
  SDL_DisplayID display = SDL_GetDisplayForPoint(&point);
  if (display != 0) {
    return display;
  }

  return SDL_GetPrimaryDisplay();
}

SDL_DisplayID DisplayForWindow(SDL_Window& window) {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  SDL_GetWindowPosition(&window, &x, &y);
  SDL_GetWindowSize(&window, &width, &height);

  SDL_DisplayID display = DisplayContainingPoint(x + width / 2, y + height / 2);
  if (display == 0) [[unlikely]] {
    display = SDL_GetPrimaryDisplay();
  }

  HELIOS_INVARIANT(display != 0, "No SDL display available!");
  return display;
}

[[nodiscard]] uint32_t RefreshRateToHz(const SDL_DisplayMode& mode) {
  if (mode.refresh_rate > 0.0F) {
    return static_cast<uint32_t>(mode.refresh_rate);
  }
  if (mode.refresh_rate_denominator != 0) {
    return static_cast<uint32_t>(
        static_cast<double>(mode.refresh_rate_numerator) /
        static_cast<double>(mode.refresh_rate_denominator));
  }
  return 0;
}

[[nodiscard]] bool FindFullscreenMode(SDL_DisplayID display, uint32_t width,
                                      uint32_t height, uint32_t refresh_rate,
                                      SDL_DisplayMode& out_mode) {
  int mode_count = 0;
  SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &mode_count);
  if (modes == nullptr || mode_count == 0) [[unlikely]] {
    SDL_free(modes);
    return false;
  }

  const SDL_DisplayMode* best = nullptr;
  SDL_DisplayMode best_mode{};
  for (int index = 0; index < mode_count; ++index) {
    const SDL_DisplayMode& mode = *modes[index];
    if (static_cast<uint32_t>(mode.w) != width ||
        static_cast<uint32_t>(mode.h) != height) {
      continue;
    }

    const uint32_t hz = RefreshRateToHz(mode);
    if (refresh_rate != 0U && hz == refresh_rate) {
      out_mode = mode;
      SDL_free(modes);
      return true;
    }

    if (best == nullptr) {
      best_mode = mode;
      best = &best_mode;
    }
  }

  if (best != nullptr) {
    out_mode = *best;
    SDL_free(modes);
    return true;
  }

  SDL_free(modes);
  return false;
}

}  // namespace

SDL_DisplayID DisplayAtIndex(::helios::window::MonitorId index) {
  int count = 0;
  SDL_DisplayID* displays = SDL_GetDisplays(&count);
  if (displays == nullptr || static_cast<uint32_t>(index) >=
                                 static_cast<uint32_t>(count)) [[unlikely]] {
    SDL_free(displays);
    return 0;
  }

  const SDL_DisplayID display = displays[index];
  SDL_free(displays);
  return display;
}

SDL_DisplayID ResolveDisplay(const ::helios::window::Properties& properties,
                             SDL_Window& window) {
  if (properties.monitor_index.has_value()) {
    SDL_DisplayID display = DisplayAtIndex(*properties.monitor_index);
    if (display != 0) [[likely]] {
      return display;
    }
  }

  return DisplayForWindow(window);
}

void RefreshMonitors(::helios::window::Monitors& monitors) {
  monitors.monitors.clear();

  int count = 0;
  SDL_DisplayID* displays = SDL_GetDisplays(&count);
  const SDL_DisplayID primary = SDL_GetPrimaryDisplay();

  if (displays == nullptr || count == 0) [[unlikely]] {
    SDL_free(displays);
    return;
  }

  for (int index = 0; index < count; ++index) {
    const SDL_DisplayID display_id = displays[index];
    ::helios::window::Monitor entry;
    entry.index = static_cast<::helios::window::MonitorId>(index);
    entry.primary = display_id == primary;

    if (const char* name = SDL_GetDisplayName(display_id); name != nullptr) {
      entry.name = name;
    }

    SDL_Rect bounds{};
    if (SDL_GetDisplayBounds(display_id, &bounds)) {
      entry.x = bounds.x;
      entry.y = bounds.y;
      entry.width = static_cast<uint32_t>(bounds.w);
      entry.height = static_cast<uint32_t>(bounds.h);
    }

    SDL_Rect usable{};
    if (SDL_GetDisplayUsableBounds(display_id, &usable)) {
      entry.work_x = usable.x;
      entry.work_y = usable.y;
      entry.work_width = static_cast<uint32_t>(usable.w);
      entry.work_height = static_cast<uint32_t>(usable.h);
    }

    if (const SDL_DisplayMode* current = SDL_GetCurrentDisplayMode(display_id);
        current != nullptr) [[likely]] {
      entry.current = {
          .width = static_cast<uint32_t>(current->w),
          .height = static_cast<uint32_t>(current->h),
          .refresh_rate = RefreshRateToHz(*current),
      };
    }

    int mode_count = 0;
    SDL_DisplayMode** modes =
        SDL_GetFullscreenDisplayModes(display_id, &mode_count);
    if (modes != nullptr) {
      entry.modes.reserve(static_cast<size_t>(mode_count));
      for (int mode_index = 0; mode_index < mode_count; ++mode_index) {
        const SDL_DisplayMode& mode = *modes[mode_index];
        entry.modes.push_back({
            .width = static_cast<uint32_t>(mode.w),
            .height = static_cast<uint32_t>(mode.h),
            .refresh_rate = RefreshRateToHz(mode),
        });
      }
      SDL_free(modes);
    }

    monitors.monitors.push_back(entry);
  }

  SDL_free(displays);
}

void MarkMonitorDependentDirty(ecs::World& world, NativeWindows& native) {
  for (const NativeWindows::Entry& entry : native.entries) {
    auto* component =
        world.TryWriteComponent<::helios::window::Window>(entry.entity);
    if (component == nullptr) [[unlikely]] {
      continue;
    }

    if (component->properties.monitor_index.has_value()) {
      component->MarkDirty(::helios::window::DirtyFlag::kMonitor);
    }
    if (::helios::window::IsFullscreenPresentation(
            component->properties.mode)) {
      component->MarkDirty(::helios::window::DirtyFlag::kMode);
    }
  }
}

SDL_WindowFlags BuildCreationFlags(const ::helios::window::Window& window) {
  SDL_WindowFlags flags = 0;

  if (!window.properties.visible) {
    flags |= SDL_WINDOW_HIDDEN;
  }
  if (!window.properties.decorated) {
    flags |= SDL_WINDOW_BORDERLESS;
  }
  if (window.properties.resizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  }
  if (window.properties.floating) {
    flags |= SDL_WINDOW_ALWAYS_ON_TOP;
  }
  if (window.properties.maximized) {
    flags |= SDL_WINDOW_MAXIMIZED;
  }
  if (window.properties.transparent_framebuffer) {
    flags |= SDL_WINDOW_TRANSPARENT;
  }
  if (window.properties.scale_to_monitor ||
      window.properties.scale_framebuffer) {
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  }
  if (window.properties.client_api == ::helios::window::ClientApi::kOpenGL) {
    flags |= SDL_WINDOW_OPENGL;
  }

  return flags;
}

void ApplyCursorMode(SDL_Window& native_window,
                     ::helios::window::CursorMode mode) {
  switch (mode) {
    using enum ::helios::window::CursorMode;
    case kVisible:
      SDL_SetWindowRelativeMouseMode(&native_window, false);
      SDL_SetWindowMouseGrab(&native_window, false);
      SDL_ShowCursor();
      break;
    case kHidden:
      SDL_SetWindowRelativeMouseMode(&native_window, false);
      SDL_SetWindowMouseGrab(&native_window, false);
      SDL_HideCursor();
      break;
    case kDisabled:
      SDL_SetWindowRelativeMouseMode(&native_window, true);
      break;
    case kCaptured:
      SDL_SetWindowRelativeMouseMode(&native_window, false);
      SDL_SetWindowMouseGrab(&native_window, true);
      SDL_ShowCursor();
      break;
  }
}

void ResolveCreationSize(::helios::window::Window& window) {
  if (window.properties.width.has_value() &&
      window.properties.height.has_value()) {
    return;
  }

  SDL_DisplayID display = 0;
  if (window.properties.monitor_index.has_value()) {
    display = DisplayAtIndex(*window.properties.monitor_index);
  }
  if (display == 0) {
    display = SDL_GetPrimaryDisplay();
  }

  const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
  HELIOS_INVARIANT(mode != nullptr, "Failed to query display desktop mode!");
  const auto [default_width, default_height] =
      ::helios::window::DefaultSizeForScreen(static_cast<uint32_t>(mode->w),
                                             static_cast<uint32_t>(mode->h));

  if (!window.properties.width.has_value()) {
    window.properties.width = default_width;
  }

  if (!window.properties.height.has_value()) {
    window.properties.height = default_height;
  }
}

void ApplyPresentationMode(SDL_Window& native_window,
                           const ::helios::window::Window& window) {
  const ::helios::window::Mode mode = window.properties.mode;

  if (mode == ::helios::window::Mode::kFullscreen) {
    const SDL_DisplayID display =
        ResolveDisplay(window.properties, native_window);
    const SDL_DisplayMode* desktop = SDL_GetDesktopDisplayMode(display);
    HELIOS_INVARIANT(desktop != nullptr,
                     "Failed to query desktop mode for fullscreen display!");
    const auto requested = ::helios::window::ResolveExclusiveVideoMode(
        window.properties, {
                               .width = static_cast<uint32_t>(desktop->w),
                               .height = static_cast<uint32_t>(desktop->h),
                               .refresh_rate = RefreshRateToHz(*desktop),
                           });

    SDL_DisplayMode fullscreen_mode{};
    if (!FindFullscreenMode(display, requested.width, requested.height,
                            requested.refresh_rate, fullscreen_mode)) {
      fullscreen_mode = *desktop;
    }

    SDL_SetWindowFullscreenMode(&native_window, &fullscreen_mode);
    SDL_SetWindowFullscreen(&native_window, true);
    return;
  }

  if (mode == ::helios::window::Mode::kBorderless) {
    SDL_SetWindowFullscreenMode(&native_window, nullptr);
    SDL_SetWindowFullscreen(&native_window, true);
    SDL_SetWindowBordered(&native_window, false);
    return;
  }

  SDL_SetWindowFullscreen(&native_window, false);
  SDL_SetWindowBordered(&native_window, window.properties.decorated);
  SDL_SetWindowResizable(&native_window, window.properties.resizable);

  const int width = static_cast<int>(window.properties.width.value_or(1));
  const int height = static_cast<int>(window.properties.height.value_or(1));
  int x = 0;
  int y = 0;
  if (window.properties.pos_x.has_value() &&
      window.properties.pos_y.has_value()) {
    x = *window.properties.pos_x;
    y = *window.properties.pos_y;
  } else {
    SDL_GetWindowPosition(&native_window, &x, &y);
  }

  SDL_SetWindowSize(&native_window, width, height);
  SDL_SetWindowPosition(&native_window, x, y);
}

void ApplyMonitorPlacement(SDL_Window& native_window,
                           ::helios::window::Window& window) {
  const SDL_DisplayID display =
      ResolveDisplay(window.properties, native_window);
  SDL_Rect usable{};
  HELIOS_VERIFY(SDL_GetDisplayUsableBounds(display, &usable),
                "Failed to query display usable bounds!");

  const int width = static_cast<int>(window.properties.width.value_or(
      static_cast<uint32_t>(usable.w > 0 ? usable.w : 1)));
  const int height = static_cast<int>(window.properties.height.value_or(
      static_cast<uint32_t>(usable.h > 0 ? usable.h : 1)));

  int x = usable.x + ((usable.w - width) / 2);
  int y = usable.y + ((usable.h - height) / 2);
  if (window.properties.pos_x.has_value() &&
      window.properties.pos_y.has_value()) {
    x = *window.properties.pos_x;
    y = *window.properties.pos_y;
  }

  SDL_SetWindowPosition(&native_window, x, y);
  window.properties.pos_x = x;
  window.properties.pos_y = y;
}

void SyncWindowGeometry(::helios::window::Window& window,
                        SDL_Window& native_window) {
  int pixel_width = 0;
  int pixel_height = 0;
  SDL_GetWindowSizeInPixels(&native_window, &pixel_width, &pixel_height);
  window.properties.width = static_cast<uint32_t>(pixel_width);
  window.properties.height = static_cast<uint32_t>(pixel_height);

  int client_width = 0;
  int client_height = 0;
  SDL_GetWindowSize(&native_window, &client_width, &client_height);
  window.properties.client_width = static_cast<uint32_t>(client_width);
  window.properties.client_height = static_cast<uint32_t>(client_height);

  int x = 0;
  int y = 0;
  SDL_GetWindowPosition(&native_window, &x, &y);
  window.properties.pos_x = x;
  window.properties.pos_y = y;

  const float scale = SDL_GetWindowDisplayScale(&native_window);
  window.properties.content_scale_x = scale;
  window.properties.content_scale_y = scale;

  const SDL_WindowFlags flags = SDL_GetWindowFlags(&native_window);
  window.properties.maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
}

void ApplyWindowIcons(SDL_Window& native_window,
                      std::span<const ::helios::window::IconImage> icons) {
  if (icons.empty()) {
    SDL_SetWindowIcon(&native_window, nullptr);
    return;
  }

  std::vector<SDL_Surface*> surfaces;
  surfaces.reserve(icons.size());
  for (const ::helios::window::IconImage& icon : icons) {
    if (icon.width == 0 || icon.height == 0 ||
        icon.rgba.size() != static_cast<size_t>(icon.width) *
                                static_cast<size_t>(icon.height) * 4U) {
      continue;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        static_cast<int>(icon.width), static_cast<int>(icon.height),
        SDL_PIXELFORMAT_RGBA32, const_cast<uint8_t*>(icon.rgba.data()),
        static_cast<int>(icon.width) * 4);
    if (surface != nullptr) {
      surfaces.push_back(surface);
    }
  }

  if (!surfaces.empty()) {
    SDL_SetWindowIcon(&native_window, surfaces.front());
  }

  for (SDL_Surface* surface : surfaces) {
    SDL_DestroySurface(surface);
  }
}

void ApplyMaximized(SDL_Window& native_window, bool maximized) {
  if (maximized) {
    SDL_MaximizeWindow(&native_window);
    return;
  }

  SDL_RestoreWindow(&native_window);
}

void ApplySizeLimits(SDL_Window& native_window,
                     const ::helios::window::Properties& properties) {
  if (properties.min_width.has_value() || properties.min_height.has_value()) {
    const int min_width = properties.min_width.has_value()
                              ? static_cast<int>(*properties.min_width)
                              : 0;
    const int min_height = properties.min_height.has_value()
                               ? static_cast<int>(*properties.min_height)
                               : 0;
    SDL_SetWindowMinimumSize(&native_window, min_width, min_height);
  }

  if (properties.max_width.has_value() || properties.max_height.has_value()) {
    const int max_width = properties.max_width.has_value()
                              ? static_cast<int>(*properties.max_width)
                              : 0;
    const int max_height = properties.max_height.has_value()
                               ? static_cast<int>(*properties.max_height)
                               : 0;
    SDL_SetWindowMaximumSize(&native_window, max_width, max_height);
  }
}

void ApplyAspectRatio(SDL_Window& native_window,
                      const ::helios::window::Properties& properties) {
  if (properties.aspect_numer.has_value() &&
      properties.aspect_denom.has_value() && *properties.aspect_denom != 0) {
    const auto aspect = static_cast<float>(*properties.aspect_numer) /
                        static_cast<float>(*properties.aspect_denom);
    SDL_SetWindowAspectRatio(&native_window, aspect, aspect);
    return;
  }

  SDL_SetWindowAspectRatio(&native_window, 0.0F, 0.0F);
}

void ApplyOpacity(SDL_Window& native_window, float opacity) {
  SDL_SetWindowOpacity(&native_window, opacity);
}

void ApplyFloating(SDL_Window& native_window, bool floating) {
  SDL_SetWindowAlwaysOnTop(&native_window, floating);
}

void ApplyAutoIconify(SDL_Window& /*native_window*/, bool auto_iconify) {
  SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, auto_iconify ? "1" : "0");
}

void ApplyFocusOnShow(bool focus_on_show) {
  SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, focus_on_show ? "1" : "0");
}

void ApplyMousePassthrough(SDL_Window& native_window, bool passthrough) {
#ifdef HELIOS_PLATFORM_WINDOWS
  const SDL_PropertiesID props = SDL_GetWindowProperties(&native_window);
  auto* hwnd = static_cast<HWND>(SDL_GetPointerProperty(
      props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
  if (hwnd == nullptr) [[unlikely]] {
    return;
  }

  LONG_PTR ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
  if (passthrough) {
    ex_style |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
  } else {
    ex_style &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
  }
  SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex_style);
#else
  (void)native_window;
  (void)passthrough;
#endif
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

void SyncClipboard(ecs::World& world, const NativeWindows& native,
                   Context& context) {
  auto* clipboard = world.TryWriteResource<::helios::window::Clipboard>();
  if (clipboard == nullptr) [[unlikely]] {
    return;
  }

  if (clipboard->pending_write) {
    SDL_SetClipboardText(clipboard->text.c_str());
    clipboard->pending_write = false;
#ifdef HELIOS_PLATFORM_WINDOWS
    CaptureClipboardSequence(context);
#endif
    context.clipboard_dirty = false;
  }

  if (native.entries.empty()) [[unlikely]] {
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

  char* clipboard_text = SDL_GetClipboardText();
#ifdef HELIOS_PLATFORM_WINDOWS
  CaptureClipboardSequence(context);
#endif
  context.clipboard_dirty = false;

  if (clipboard_text == nullptr) [[unlikely]] {
    return;
  }

  std::string new_text{clipboard_text};
  SDL_free(clipboard_text);

  if (new_text == clipboard->text) {
    return;
  }

  clipboard->text = std::move(new_text);
  world.WriteMessages<::helios::window::ClipboardChangedMsg>().Write(
      {.text = clipboard->text});
}

}  // namespace helios::sdl3::window
