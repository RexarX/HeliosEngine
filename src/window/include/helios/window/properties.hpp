#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace helios::window {

/// @brief Window presentation mode.
enum class Mode : uint8_t {
  kWindowed = 0,    ///< Standard windowed mode.
  kBorderless = 1,  ///< Borderless fullscreen window.
  kFullscreen = 2,  ///< Exclusive fullscreen mode.
};

/**
 * @brief Tests whether a mode covers an entire monitor surface.
 * @param mode Presentation mode
 * @return True for borderless or exclusive fullscreen
 */
[[nodiscard]] constexpr bool IsFullscreenPresentation(Mode mode) noexcept {
  return mode == Mode::kBorderless || mode == Mode::kFullscreen;
}

/**
 * @brief Returns the string name of a window presentation mode.
 * @param mode Presentation mode
 * @return String name of the mode
 */
[[nodiscard]] constexpr std::string_view ToString(Mode mode) noexcept {
  switch (mode) {
    using enum Mode;
    case kWindowed:
      return "Windowed";
    case kBorderless:
      return "Borderless";
    case kFullscreen:
      return "Fullscreen";
  }
  return "unknown";
}

/**
 * @brief Outputs a window presentation mode to an output stream.
 * @param os Output stream
 * @param mode Presentation mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, Mode mode) {
  return os << "Mode::" << ToString(mode);
}

/// @brief Cursor visibility and capture mode.
enum class CursorMode : uint8_t {
  kVisible = 0,   ///< Cursor is visible and free.
  kHidden = 1,    ///< Cursor is hidden but not captured.
  kDisabled = 2,  ///< Cursor is hidden and captured (raw input).
};

/**
 * @brief Returns the string name of a cursor mode.
 * @param mode Cursor mode
 * @return String name of the mode
 */
[[nodiscard]] constexpr std::string_view ToString(CursorMode mode) noexcept {
  switch (mode) {
    using enum CursorMode;
    case kVisible:
      return "Visible";
    case kHidden:
      return "Hidden";
    case kDisabled:
      return "Disabled";
  }
  return "unknown";
}

/**
 * @brief Outputs a cursor mode to an output stream.
 * @param os Output stream
 * @param mode Cursor mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CursorMode mode) {
  return os << "CursorMode::" << ToString(mode);
}

/// @brief Graphics client API requested at window creation.
enum class ClientApi : uint8_t {
  kNone = 0,    ///< No client API (`GLFW_NO_API`); for Vulkan/DX/NRI backends.
  kOpenGL = 1,  ///< OpenGL context (`GLFW_OPENGL_API`).
};

/**
 * @brief Returns the string name of a client API.
 * @param api Client API
 * @return String name of the API
 */
[[nodiscard]] constexpr std::string_view ToString(ClientApi api) noexcept {
  switch (api) {
    using enum ClientApi;
    case kNone:
      return "None";
    case kOpenGL:
      return "OpenGL";
  }
  return "unknown";
}

/**
 * @brief Outputs a client API to an output stream.
 * @param os Output stream
 * @param api Client API
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ClientApi api) {
  return os << "ClientApi::" << ToString(api);
}

/// @brief Bitmask of pending property changes for backend synchronization.
enum class DirtyFlag : uint32_t {
  kNone = 0,
  kTitle = 1U << 0U,
  kSize = 1U << 1U,
  kPos = 1U << 2U,
  kMode = 1U << 3U,
  kCursor = 1U << 4U,
  kVisible = 1U << 5U,
  kResizable = 1U << 6U,
  kDecorated = 1U << 7U,
  kMonitor = 1U << 8U,
  kIcon = 1U << 9U,
  kMaximized = 1U << 10U,
  kSizeLimits = 1U << 11U,
  kAspectRatio = 1U << 12U,
  kOpacity = 1U << 13U,
  kFloating = 1U << 14U,
  kAutoIconify = 1U << 15U,
  kFocusOnShow = 1U << 16U,
  kAttention = 1U << 17U,
  kFocus = 1U << 18U,
  kMousePassthrough = 1U << 19U,
  kRefreshRate = 1U << 20U,
};

/**
 * @brief Combines dirty flags.
 * @param lhs Left-hand side flag
 * @param rhs Right-hand side flag
 * @return Combined flag
 */
[[nodiscard]] constexpr DirtyFlag operator|(DirtyFlag lhs,
                                            DirtyFlag rhs) noexcept {
  return static_cast<DirtyFlag>(std::to_underlying(lhs) |
                                std::to_underlying(rhs));
}

/**
 * @brief Tests whether a dirty flag is set.
 * @param flags Combined dirty flags
 * @param flag Flag to test
 * @return True if the flag is set, false otherwise
 */
[[nodiscard]] constexpr bool HasFlag(DirtyFlag flags, DirtyFlag flag) noexcept {
  return (std::to_underlying(flags) & std::to_underlying(flag)) != 0U;
}

/**
 * @brief Formats dirty flags as a pipe-separated list and writes to an output
 * iterator.
 * @tparam It Output iterator type
 * @param flags Combined dirty flags
 * @param out Output iterator to write the formatted string to
 * @param with_prefix Whether to include a "DirtyFlag::" prefix for each flag
 * @return Updated output iterator after writing the formatted string
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(DirtyFlag flags, It out, bool with_prefix = false) {
  const std::string_view kNoneStr = with_prefix ? "DirtyFlag::None" : "None";

  if (flags == DirtyFlag::kNone) {
    return std::format_to(out, "{}", kNoneStr);
  }

  bool first = true;
  const auto append = [flags, &out, &first, with_prefix](std::string_view name,
                                                         DirtyFlag flag) {
    if (!HasFlag(flags, flag)) {
      return;
    }
    if (!first) {
      out = std::format_to(out, " | ");
    }
    first = false;
    out = with_prefix ? std::format_to(out, "DirtyFlag::{}", name)
                      : std::format_to(out, "{}", name);
  };

  append("Title", DirtyFlag::kTitle);
  append("Size", DirtyFlag::kSize);
  append("Pos", DirtyFlag::kPos);
  append("Mode", DirtyFlag::kMode);
  append("Cursor", DirtyFlag::kCursor);
  append("Visible", DirtyFlag::kVisible);
  append("Resizable", DirtyFlag::kResizable);
  append("Decorated", DirtyFlag::kDecorated);
  append("Monitor", DirtyFlag::kMonitor);
  append("Icon", DirtyFlag::kIcon);
  append("Maximized", DirtyFlag::kMaximized);
  append("SizeLimits", DirtyFlag::kSizeLimits);
  append("AspectRatio", DirtyFlag::kAspectRatio);
  append("Opacity", DirtyFlag::kOpacity);
  append("Floating", DirtyFlag::kFloating);
  append("AutoIconify", DirtyFlag::kAutoIconify);
  append("FocusOnShow", DirtyFlag::kFocusOnShow);
  append("Attention", DirtyFlag::kAttention);
  append("Focus", DirtyFlag::kFocus);
  append("MousePassthrough", DirtyFlag::kMousePassthrough);
  append("RefreshRate", DirtyFlag::kRefreshRate);
  return out;
}

/**
 * @brief Formats dirty flags as a pipe-separated list of flag names.
 * @param flags Combined dirty flags
 * @param with_prefix Whether to include a "DirtyFlag::" prefix for each flag
 * @return Formatted flag names
 */
[[nodiscard]] inline std::string ToString(DirtyFlag flags,
                                          bool with_prefix = false) {
  std::string result;
  result.reserve(32);  // Reserve some space for common cases
  ToString(flags, std::back_inserter(result), with_prefix);
  return result;
}

/**
 * @brief Outputs dirty flags to an output stream.
 * @param os Output stream
 * @param flags Combined dirty flags
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, DirtyFlag flags) {
  ToString(flags, std::ostreambuf_iterator<char>(os));
  return os;
}

/// @brief RGBA8 window icon image.
struct IconImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> rgba;
};

/// @brief Creation-time and mutable window properties.
struct Properties {
  std::string title = "Helios";
  std::vector<IconImage> icons;
  std::optional<uint32_t> width;
  std::optional<uint32_t> height;
  std::optional<uint32_t> client_width;
  std::optional<uint32_t> client_height;
  std::optional<int32_t> pos_x;
  std::optional<int32_t> pos_y;
  std::optional<int32_t> monitor_index;
  std::optional<uint32_t> refresh_rate;
  std::optional<uint32_t> min_width;
  std::optional<uint32_t> min_height;
  std::optional<uint32_t> max_width;
  std::optional<uint32_t> max_height;
  std::optional<int32_t> aspect_numer;
  std::optional<int32_t> aspect_denom;
  float content_scale_x = 1.0F;
  float content_scale_y = 1.0F;
  float opacity = 1.0F;
  Mode mode = Mode::kWindowed;
  CursorMode cursor_mode = CursorMode::kVisible;
  ClientApi client_api = ClientApi::kNone;
  bool visible = true;
  bool focused = true;
  bool resizable = true;
  bool decorated = true;
  bool maximized = false;
  bool floating = false;
  bool auto_iconify = true;
  bool focus_on_show = true;
  bool hovered = false;
  bool transparent_framebuffer = false;
  bool scale_to_monitor = false;
  bool scale_framebuffer = true;
  bool mouse_passthrough = false;
};

/**
 * @brief Computes a default client size for a monitor resolution.
 * @details Uses most of the screen on portrait displays and a balanced fraction
 * on landscape displays so fixed 1280x720 is not assumed.
 * @param screen_width Monitor width in pixels
 * @param screen_height Monitor height in pixels
 * @return Default width and height in pixels
 */
[[nodiscard]] constexpr auto DefaultSizeForScreen(
    uint32_t screen_width, uint32_t screen_height) noexcept
    -> std::pair<uint32_t, uint32_t> {
  constexpr uint32_t kMinSize = 320;

  if (screen_width == 0 || screen_height == 0) {
    return {1280, 720};
  }

  if (screen_height > screen_width) {
    const uint32_t width =
        std::clamp((screen_width * 9U) / 10U, kMinSize, screen_width);
    const uint32_t height =
        std::clamp((width * 16U) / 10U, kMinSize, (screen_height * 9U) / 10U);
    return {width, height};
  }

  const uint32_t width =
      std::clamp((screen_width * 2U) / 3U, kMinSize, screen_width);
  const uint32_t height =
      std::clamp((screen_height * 2U) / 3U, kMinSize, screen_height);
  return {width, height};
}

/// @brief Exclusive-fullscreen width, height, and refresh rate.
struct ExclusiveVideoMode {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t refresh_rate = 0;
};

/**
 * @brief Resolves exclusive-fullscreen size and refresh rate.
 * @details Unset `width`, `height`, and `refresh_rate` fall back to the
 * current desktop video mode. Borderless fullscreen ignores this helper.
 * @param properties Window properties
 * @param desktop Current desktop video mode
 * @return Size and refresh rate to request from the backend
 */
[[nodiscard]] constexpr ExclusiveVideoMode ResolveExclusiveVideoMode(
    const Properties& properties, ExclusiveVideoMode desktop) noexcept {
  return {
      .width = properties.width.value_or(desktop.width),
      .height = properties.height.value_or(desktop.height),
      .refresh_rate = properties.refresh_rate.value_or(desktop.refresh_rate),
  };
}

/**
 * @brief Formats window properties using an output iterator.
 * @param properties Window properties
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Properties& properties, It out) {
  out = std::format_to(out, "Properties{{");
  out = std::format_to(out, "title=\"{}\"", properties.title);
  out = std::format_to(out, ", icons={}", properties.icons.size());

  if (properties.width.has_value()) [[likely]] {
    out = std::format_to(out, ", width={}", *properties.width);
  }
  if (properties.height.has_value()) [[likely]] {
    out = std::format_to(out, ", height={}", *properties.height);
  }
  if (properties.client_width.has_value()) [[likely]] {
    out = std::format_to(out, ", client_width={}", *properties.client_width);
  }
  if (properties.client_height.has_value()) [[likely]] {
    out = std::format_to(out, ", client_height={}", *properties.client_height);
  }
  if (properties.pos_x.has_value()) [[likely]] {
    out = std::format_to(out, ", pos_x={}", *properties.pos_x);
  }
  if (properties.pos_y.has_value()) [[likely]] {
    out = std::format_to(out, ", pos_y={}", *properties.pos_y);
  }
  if (properties.monitor_index.has_value()) [[likely]] {
    out = std::format_to(out, ", monitor_index={}", *properties.monitor_index);
  }
  if (properties.refresh_rate.has_value()) [[likely]] {
    out = std::format_to(out, ", refresh_rate={}", *properties.refresh_rate);
  }
  if (properties.min_width.has_value()) [[likely]] {
    out = std::format_to(out, ", min_width={}", *properties.min_width);
  }
  if (properties.min_height.has_value()) [[likely]] {
    out = std::format_to(out, ", min_height={}", *properties.min_height);
  }
  if (properties.max_width.has_value()) [[likely]] {
    out = std::format_to(out, ", max_width={}", *properties.max_width);
  }
  if (properties.max_height.has_value()) [[likely]] {
    out = std::format_to(out, ", max_height={}", *properties.max_height);
  }
  if (properties.aspect_numer.has_value()) [[likely]] {
    out = std::format_to(out, ", aspect_numer={}", *properties.aspect_numer);
  }
  if (properties.aspect_denom.has_value()) [[likely]] {
    out = std::format_to(out, ", aspect_denom={}", *properties.aspect_denom);
  }

  out = std::format_to(out, ", content_scale_x={}", properties.content_scale_x);
  out = std::format_to(out, ", content_scale_y={}", properties.content_scale_y);
  out = std::format_to(out, ", opacity={}", properties.opacity);
  out = std::format_to(out, ", mode={}", ToString(properties.mode));
  out =
      std::format_to(out, ", cursor_mode={}", ToString(properties.cursor_mode));
  out = std::format_to(out, ", client_api={}", ToString(properties.client_api));
  out = std::format_to(out, ", visible={}", properties.visible);
  out = std::format_to(out, ", focused={}", properties.focused);
  out = std::format_to(out, ", resizable={}", properties.resizable);
  out = std::format_to(out, ", decorated={}", properties.decorated);
  out = std::format_to(out, ", maximized={}", properties.maximized);
  out = std::format_to(out, ", floating={}", properties.floating);
  out = std::format_to(out, ", auto_iconify={}", properties.auto_iconify);
  out = std::format_to(out, ", focus_on_show={}", properties.focus_on_show);
  out = std::format_to(out, ", hovered={}", properties.hovered);
  out = std::format_to(out, ", transparent_framebuffer={}",
                       properties.transparent_framebuffer);
  out =
      std::format_to(out, ", scale_to_monitor={}", properties.scale_to_monitor);
  out = std::format_to(out, ", scale_framebuffer={}",
                       properties.scale_framebuffer);
  out = std::format_to(out, ", mouse_passthrough={}",
                       properties.mouse_passthrough);
  out = std::format_to(out, "}}");

  return out;
}

/**
 * @brief Formats window properties as a string.
 * @param properties Window properties
 * @return Formatted properties string
 */
[[nodiscard]] inline std::string ToString(const Properties& properties) {
  std::string result;
  result.reserve(256);  // Reserve reasonable space for typical properties
  ToString(properties, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs window properties to an output stream.
 * @param os Output stream
 * @param properties Window properties
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const Properties& properties) {
  ToString(properties, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::window

namespace std {

template <>
struct formatter<helios::window::Mode> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::Mode mode, format_context& ctx) {
    return format_to(ctx.out(), "Mode::{}", helios::window::ToString(mode));
  }
};

template <>
struct formatter<helios::window::ClientApi> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::ClientApi api,
                               format_context& ctx) {
    return format_to(ctx.out(), "ClientApi::{}", helios::window::ToString(api));
  }
};

template <>
struct formatter<helios::window::CursorMode> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::CursorMode mode,
                               format_context& ctx) {
    return format_to(ctx.out(), "CursorMode::{}",
                     helios::window::ToString(mode));
  }
};

template <>
struct formatter<helios::window::DirtyFlag> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(helios::window::DirtyFlag flags, format_context& ctx) {
    return helios::window::ToString(flags, ctx.out(), /*with_prefix=*/true);
  }
};

template <>
struct formatter<helios::window::Properties> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Properties& properties,
                     format_context& ctx) {
    return helios::window::ToString(properties, ctx.out());
  }
};

}  // namespace std
