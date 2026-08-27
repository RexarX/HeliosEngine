#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#endif
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Sent after a backend creates the OS window for an entity.
struct CreatedMsg {
  static constexpr std::string_view kName = "helios::window::CreatedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  Properties properties;
};

/// @brief Sent after a backend destroys the OS window for an entity.
struct ClosedMsg {
  static constexpr std::string_view kName = "helios::window::ClosedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
};

/// @brief Sent when a window framebuffer size changes.
struct ResizedMsg {
  static constexpr std::string_view kName = "helios::window::ResizedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  uint32_t width = 0;
  uint32_t height = 0;

  /**
   * @brief Returns the framebuffer size.
   * @return Width and height in pixels
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/// @brief Sent when a window client-area size changes.
struct ClientResizedMsg {
  static constexpr std::string_view kName = "helios::window::ClientResizedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  uint32_t width = 0;
  uint32_t height = 0;

  /**
   * @brief Returns the client-area size.
   * @return Width and height in pixels
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/// @brief Sent when a window content scale changes.
struct ContentScaleChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ContentScaleChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  float scale_x = 1.0F;
  float scale_y = 1.0F;

  /**
   * @brief Returns the content scale.
   * @return Horizontal and vertical scale
   */
  [[nodiscard]] constexpr auto GetScale() const noexcept
      -> std::pair<float, float> {
    return {scale_x, scale_y};
  }
};

/// @brief Sent when a window position changes.
struct PosChangedMsg {
  static constexpr std::string_view kName = "helios::window::PosChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  int32_t x = 0;
  int32_t y = 0;

  /**
   * @brief Returns the window position.
   * @return X and y in screen coordinates
   */
  [[nodiscard]] constexpr auto GetPos() const noexcept
      -> std::pair<int32_t, int32_t> {
    return {x, y};
  }
};

/// @brief Sent when a window presentation mode changes.
struct ModeChangedMsg {
  static constexpr std::string_view kName = "helios::window::ModeChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  Mode mode = Mode::kWindowed;
};

/// @brief Sent when a window cursor mode changes.
struct CursorModeChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::CursorModeChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  CursorMode cursor_mode = CursorMode::kVisible;
};

/// @brief Sent when a window visibility changes.
struct VisibilityChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::VisibilityChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool visible = true;
};

/// @brief Sent when a window gains or loses focus.
struct FocusChangedMsg {
  static constexpr std::string_view kName = "helios::window::FocusChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool focused = false;
};

/// @brief Sent when a window maximize state changes.
struct MaximizedChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::MaximizedChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool maximized = false;
};

/// @brief Sent when a window icon set changes.
struct IconChangedMsg {
  static constexpr std::string_view kName = "helios::window::IconChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  std::vector<IconImage> icons;
};

/// @brief Sent when a window resizable state changes.
struct ResizableChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ResizableChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool resizable = true;
};

/// @brief Sent when a window decorated state changes.
struct DecoratedChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::DecoratedChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool decorated = true;
};

/// @brief Sent when a window opacity changes.
struct OpacityChangedMsg {
  static constexpr std::string_view kName = "helios::window::OpacityChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  float opacity = 1.0F;
};

/// @brief Sent when a window floating state changes.
struct FloatingChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::FloatingChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool floating = false;
};

/// @brief Sent when the cursor enters or leaves a window.
struct HoverChangedMsg {
  static constexpr std::string_view kName = "helios::window::HoverChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool hovered = false;
};

/// @brief Sent when mouse passthrough state changes.
struct MousePassthroughChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::MousePassthroughChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool mouse_passthrough = false;
};

/// @brief Sent when the OS clipboard text changes.
struct ClipboardChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ClipboardChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string text;
};

/// @brief Sent when files are dropped onto a window.
struct DroppedFilesMsg {
  static constexpr std::string_view kName = "helios::window::DroppedFilesMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  std::vector<std::string> paths;
};

/// @brief Sent when the user requests to close a window.
struct CloseRequestedMsg {
  static constexpr std::string_view kName = "helios::window::CloseRequestedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
};

/// @brief Sent when OS window creation fails for an entity.
struct CreationFailedMsg {
  static constexpr std::string_view kName = "helios::window::CreationFailedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  std::string reason;
};

/// @brief Sent when the connected monitor layout changes.
struct MonitorConnectedMsg {
  static constexpr std::string_view kName =
      "helios::window::MonitorConnectedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t index = 0;
};

/// @brief Sent when the connected monitor layout changes.
struct MonitorDisconnectedMsg {
  static constexpr std::string_view kName =
      "helios::window::MonitorDisconnectedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t index = 0;
};

/**
 * @brief Formats `CreatedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CreatedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CreatedMsg& msg) {
  out = std::format_to(out, "CreatedMsg{{entity={}, properties=", msg.entity);
  out = ToString(out, msg.properties);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a `CreatedMsg` message as a string.
 * @param msg `CreatedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CreatedMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CreatedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CreatedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const CreatedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CreatedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CreatedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CreatedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ClosedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Closed message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ClosedMsg msg) {
  return std::format_to(out, "ClosedMsg{{entity={}}}", msg.entity);
}

/**
 * @brief Formats a `ClosedMsg` message as a string.
 * @param msg `ClosedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ClosedMsg msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ClosedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ClosedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ClosedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ClosedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ClosedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ClosedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ResizedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ResizedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ResizedMsg msg) {
  return std::format_to(out, "ResizedMsg{{entity={}, width={}, height={}}}",
                        msg.entity, msg.width, msg.height);
}

/**
 * @brief Formats a `ResizedMsg` message as a string.
 * @param msg `ResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ResizedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ResizedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ResizedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ResizedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ResizedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ResizedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ClientResizedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ClientResizedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ClientResizedMsg msg) {
  return std::format_to(out,
                        "ClientResizedMsg{{entity={}, width={}, height={}}}",
                        msg.entity, msg.width, msg.height);
}

/**
 * @brief Formats a `ClientResizedMsg` message as a string.
 * @param msg `ClientResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ClientResizedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ClientResizedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ClientResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ClientResizedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ClientResizedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ClientResizedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ClientResizedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ContentScaleChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ContentScaleChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ContentScaleChangedMsg msg) {
  return std::format_to(
      out, "ContentScaleChangedMsg{{entity={}, scale_x={}, scale_y={}}}",
      msg.entity, msg.scale_x, msg.scale_y);
}

/**
 * @brief Formats a `ContentScaleChangedMsg` message as a string.
 * @param msg `ContentScaleChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ContentScaleChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ContentScaleChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ContentScaleChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ContentScaleChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ContentScaleChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ContentScaleChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ContentScaleChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `PosChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `PosChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, PosChangedMsg msg) {
  return std::format_to(out, "PosChangedMsg{{entity={}, x={}, y={}}}",
                        msg.entity, msg.x, msg.y);
}

/**
 * @brief Formats a `PosChangedMsg` message as a string.
 * @param msg `PosChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(PosChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `PosChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `PosChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(PosChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `PosChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `PosChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PosChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ModeChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ModeChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ModeChangedMsg msg) {
  return std::format_to(out, "ModeChangedMsg{{entity={}, mode={}}}", msg.entity,
                        ToString(msg.mode));
}

/**
 * @brief Formats a `ModeChangedMsg` message as a string.
 * @param msg `ModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ModeChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ModeChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ModeChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ModeChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ModeChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ModeChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `CursorModeChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CursorModeChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, CursorModeChangedMsg msg) {
  return std::format_to(out,
                        "CursorModeChangedMsg{{entity={}, cursor_mode={}}}",
                        msg.entity, ToString(msg.cursor_mode));
}

/**
 * @brief Formats a `CursorModeChangedMsg` message as a string.
 * @param msg `CursorModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(CursorModeChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CursorModeChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CursorModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(CursorModeChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CursorModeChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CursorModeChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CursorModeChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `VisibilityChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `VisibilityChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, VisibilityChangedMsg msg) {
  return std::format_to(out, "VisibilityChangedMsg{{entity={}, visible={}}}",
                        msg.entity, msg.visible);
}

/**
 * @brief Formats a `VisibilityChangedMsg` message as a string.
 * @param msg `VisibilityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(VisibilityChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `VisibilityChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `VisibilityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(VisibilityChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `VisibilityChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `VisibilityChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, VisibilityChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `FocusChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `FocusChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, FocusChangedMsg msg) {
  return std::format_to(out, "FocusChangedMsg{{entity={}, focused={}}}",
                        msg.entity, msg.focused);
}

/**
 * @brief Formats a `FocusChangedMsg` message as a string.
 * @param msg `FocusChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(FocusChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `FocusChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `FocusChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(FocusChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `FocusChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `FocusChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, FocusChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MaximizedChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MaximizedChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MaximizedChangedMsg msg) {
  return std::format_to(out, "MaximizedChangedMsg{{entity={}, maximized={}}}",
                        msg.entity, msg.maximized);
}

/**
 * @brief Formats a `MaximizedChangedMsg` message as a string.
 * @param msg `MaximizedChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MaximizedChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MaximizedChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MaximizedChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(MaximizedChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MaximizedChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MaximizedChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MaximizedChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `IconChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `IconChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const IconChangedMsg& msg) {
  return std::format_to(out, "IconChangedMsg{{entity={}, icons={}}}",
                        msg.entity, msg.icons.size());
}

/**
 * @brief Formats a `IconChangedMsg` message as a string.
 * @param msg `IconChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const IconChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `IconChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `IconChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const IconChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `IconChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `IconChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const IconChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ResizableChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ResizableChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ResizableChangedMsg msg) {
  return std::format_to(out, "ResizableChangedMsg{{entity={}, resizable={}}}",
                        msg.entity, msg.resizable);
}

/**
 * @brief Formats a `ResizableChangedMsg` message as a string.
 * @param msg `ResizableChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ResizableChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ResizableChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ResizableChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ResizableChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ResizableChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ResizableChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ResizableChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `DecoratedChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `DecoratedChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, DecoratedChangedMsg msg) {
  return std::format_to(out, "DecoratedChangedMsg{{entity={}, decorated={}}}",
                        msg.entity, msg.decorated);
}

/**
 * @brief Formats a DecoratedChanged message as a string.
 * @param msg DecoratedChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(DecoratedChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a DecoratedChanged message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `DecoratedChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(DecoratedChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a DecoratedChanged message to an output stream.
 * @param os Output stream
 * @param msg DecoratedChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, DecoratedChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `OpacityChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `OpacityChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, OpacityChangedMsg msg) {
  return std::format_to(out, "OpacityChangedMsg{{entity={}, opacity={}}}",
                        msg.entity, msg.opacity);
}

/**
 * @brief Formats a `OpacityChangedMsg` message as a string.
 * @param msg `OpacityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(OpacityChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `OpacityChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `OpacityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(OpacityChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `OpacityChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `OpacityChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, OpacityChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `FloatingChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `FloatingChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, FloatingChangedMsg msg) {
  return std::format_to(out, "FloatingChangedMsg{{entity={}, floating={}}}",
                        msg.entity, msg.floating);
}

/**
 * @brief Formats a `FloatingChangedMsg` message as a string.
 * @param msg `FloatingChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(FloatingChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `FloatingChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `FloatingChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(FloatingChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `FloatingChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `FloatingChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, FloatingChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `HoverChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `HoverChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, HoverChangedMsg msg) {
  return std::format_to(out, "HoverChangedMsg{{entity={}, hovered={}}}",
                        msg.entity, msg.hovered);
}

/**
 * @brief Formats a `HoverChangedMsg` message as a string.
 * @param msg `HoverChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(HoverChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `HoverChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `HoverChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(HoverChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `HoverChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `HoverChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, HoverChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MousePassthroughChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MousePassthroughChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MousePassthroughChangedMsg msg) {
  return std::format_to(
      out, "MousePassthroughChangedMsg{{entity={}, mouse_passthrough={}}}",
      msg.entity, msg.mouse_passthrough);
}

/**
 * @brief Formats a `MousePassthroughChangedMsg` message as a string.
 * @param msg `MousePassthroughChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MousePassthroughChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MousePassthroughChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MousePassthroughChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    MousePassthroughChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MousePassthroughChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MousePassthroughChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                MousePassthroughChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ClipboardChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ClipboardChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const ClipboardChangedMsg& msg) {
  return std::format_to(out, "ClipboardChangedMsg{{text=\"{}\"}}", msg.text);
}

/**
 * @brief Formats a `ClipboardChangedMsg` message as a string.
 * @param msg `ClipboardChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const ClipboardChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ClipboardChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ClipboardChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const ClipboardChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ClipboardChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ClipboardChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const ClipboardChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `DroppedFilesMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `DroppedFilesMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const DroppedFilesMsg& msg) {
  out = std::format_to(out, "DroppedFilesMsg{{entity={}, paths=[", msg.entity);
  bool first = true;
  for (const std::string& path : msg.paths) {
    if (!first) {
      out = std::format_to(out, ", ");
    }
    first = false;
    out = std::format_to(out, "\"{}\"", path);
  }
  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a `DroppedFilesMsg` message as a string.
 * @param msg `DroppedFilesMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const DroppedFilesMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `DroppedFilesMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `DroppedFilesMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const DroppedFilesMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `DroppedFilesMsg` message to an output stream.
 * @param os Output stream
 * @param msg `DroppedFilesMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const DroppedFilesMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MonitorConnectedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MonitorConnectedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MonitorConnectedMsg msg) {
  return std::format_to(out, "MonitorConnectedMsg{{index={}}}", msg.index);
}

/**
 * @brief Formats a `MonitorConnectedMsg` message as a string.
 * @param msg `MonitorConnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MonitorConnectedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MonitorConnectedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MonitorConnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(MonitorConnectedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MonitorConnectedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MonitorConnectedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MonitorConnectedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MonitorDisconnectedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MonitorDisconnectedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MonitorDisconnectedMsg msg) {
  return std::format_to(out, "MonitorConnectedMsg{{index={}}}", msg.index);
}

/**
 * @brief Formats a `MonitorDisconnectedMsg` message as a string.
 * @param msg `MonitorDisconnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MonitorDisconnectedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MonitorDisconnectedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MonitorDisconnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(MonitorDisconnectedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MonitorDisconnectedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MonitorDisconnectedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MonitorDisconnectedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `CloseRequestedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CloseRequestedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, CloseRequestedMsg msg) {
  return std::format_to(out, "CloseRequestedMsg{{entity={}}}", msg.entity);
}

/**
 * @brief Formats a `CloseRequestedMsg` message as a string.
 * @param msg `CloseRequestedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(CloseRequestedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CloseRequestedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CloseRequestedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(CloseRequestedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CloseRequestedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CloseRequestedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CloseRequestedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `CreationFailedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CreationFailedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CreationFailedMsg& msg) {
  return std::format_to(out, "CreationFailedMsg{{entity={}, reason=\"{}\"}}",
                        msg.entity, msg.reason);
}

/**
 * @brief Formats a `CreationFailedMsg` message as a string.
 * @param msg `CreationFailedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CreationFailedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CreationFailedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CreationFailedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const CreationFailedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CreationFailedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CreationFailedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const CreationFailedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::window

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::window::CreatedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CreatedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ClosedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ClosedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ResizedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ResizedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ClientResizedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ClientResizedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ContentScaleChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ContentScaleChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::PosChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::PosChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ModeChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ModeChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::CursorModeChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CursorModeChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::VisibilityChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::VisibilityChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::FocusChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::FocusChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MaximizedChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MaximizedChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::IconChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::IconChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ResizableChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ResizableChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::DecoratedChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::DecoratedChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::OpacityChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::OpacityChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::FloatingChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::FloatingChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::HoverChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::HoverChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MousePassthroughChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MousePassthroughChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ClipboardChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ClipboardChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::DroppedFilesMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::DroppedFilesMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::CloseRequestedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CloseRequestedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::CreationFailedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CreationFailedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MonitorConnectedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MonitorConnectedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MonitorDisconnectedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MonitorDisconnectedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
