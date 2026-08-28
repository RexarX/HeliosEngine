#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
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
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#endif
#include <helios/input/button_input.hpp>
#include <helios/input/ids.hpp>
#include <helios/input/keyboard.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Contiguous mouse button identifiers.
enum class MouseButton : uint8_t {
  kLeft = 0,
  kRight,
  kMiddle,
  kExtra1,
  kExtra2,
  kExtra3,
  kExtra4,
  kExtra5,
  kCount,
};

/// @brief Standard cursor icons (GLFW 3.4 standard set plus default).
enum class CursorIcon : uint8_t {
  kDefault = 0,
  kArrow,
  kIBeam,
  kCrosshair,
  kPointingHand,
  kResizeEw,
  kResizeNs,
  kResizeNwse,
  kResizeNesw,
  kResizeAll,
  kNotAllowed,
  kCount,
};

/// @brief Custom RGBA cursor image with hotspot.
struct CursorImage {
  uint32_t width = 0;
  uint32_t height = 0;
  int32_t hotspot_x = 0;
  int32_t hotspot_y = 0;
  std::vector<uint8_t> rgba;

  /**
   * @brief Returns the image size in pixels.
   * @return Width and height
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }

  /**
   * @brief Returns the cursor hotspot.
   * @return Hotspot x and y in pixels
   */
  [[nodiscard]] constexpr auto GetHotspot() const noexcept
      -> std::pair<int32_t, int32_t> {
    return {hotspot_x, hotspot_y};
  }
};

using MouseButtonInput = ButtonInput<MouseButton>;

/// @brief Aggregated mouse state for the main window / app.
struct Mouse {
  static constexpr std::string_view kName = "helios::input::Mouse";

  MouseButtonInput buttons;
  double position_x = 0.0;
  double position_y = 0.0;
  double delta_x = 0.0;
  double delta_y = 0.0;
  double scroll_x = 0.0;
  double scroll_y = 0.0;

  /**
   * @brief Returns the cursor position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {position_x, position_y};
  }

  /**
   * @brief Returns the per-frame motion delta.
   * @return Delta x and y
   */
  [[nodiscard]] constexpr auto GetDelta() const noexcept
      -> std::pair<double, double> {
    return {delta_x, delta_y};
  }

  /**
   * @brief Returns the per-frame scroll delta.
   * @return Scroll x and y
   */
  [[nodiscard]] constexpr auto GetScroll() const noexcept
      -> std::pair<double, double> {
    return {scroll_x, scroll_y};
  }
};

/// @brief Per-window cursor presentation state.
struct Cursor {
  static constexpr std::string_view kName = "helios::input::Cursor";

  std::optional<CursorImage> custom;
  CursorIcon icon = CursorIcon::kDefault;
  bool dirty = false;

  /// @brief Clears any custom cursor image and marks the cursor dirty.
  constexpr void ClearCustom() noexcept;

  /**
   * @brief Sets the standard cursor icon and marks the cursor dirty.
   * @param value Cursor icon to apply
   */
  constexpr void SetIcon(CursorIcon value) noexcept;

  /**
   * @brief Sets a custom cursor image and marks the cursor dirty.
   * @param image Custom RGBA cursor image
   */
  constexpr void SetCustom(CursorImage image) noexcept;
};

constexpr void Cursor::SetIcon(CursorIcon value) noexcept {
  icon = value;
  dirty = true;
}

constexpr void Cursor::SetCustom(CursorImage image) noexcept {
  custom = std::move(image);
  dirty = true;
}

constexpr void Cursor::ClearCustom() noexcept {
  custom.reset();
  dirty = true;
}

/// @brief Mouse button press / release / repeat event for a window entity.
struct MouseButtonInputMsg {
  static constexpr std::string_view kName =
      "helios::input::MouseButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  MouseButton button = MouseButton::kLeft;
  ButtonState state = ButtonState::kReleased;
  Modifiers modifiers = Modifiers::kNone;
};

/// @brief Absolute cursor position change for a window entity.
struct CursorMovedMsg {
  static constexpr std::string_view kName = "helios::input::CursorMovedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;

  /**
   * @brief Returns the cursor position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/// @brief Raw relative mouse motion for a window entity.
struct MouseMotionMsg {
  static constexpr std::string_view kName = "helios::input::MouseMotionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double delta_x = 0.0;
  double delta_y = 0.0;

  /**
   * @brief Returns the relative motion delta.
   * @return Delta x and y
   */
  [[nodiscard]] constexpr auto GetDelta() const noexcept
      -> std::pair<double, double> {
    return {delta_x, delta_y};
  }
};

/// @brief Mouse scroll / wheel event for a window entity.
struct MouseWheelMsg {
  static constexpr std::string_view kName = "helios::input::MouseWheelMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;

  /**
   * @brief Returns the scroll delta.
   * @return Scroll x and y
   */
  [[nodiscard]] constexpr auto GetDelta() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/// @brief Mouse connect / disconnect notification.
/// @details Update systems ignore this message. `Mouse` stays process-global.
struct MouseConnectionMsg {
  static constexpr std::string_view kName = "helios::input::MouseConnectionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string name;
  MouseId id = 0;
  bool connected = false;
};

/**
 * @brief Returns the string name of a mouse button.
 * @param button Mouse button
 * @return String name of the button
 */
[[nodiscard]] constexpr std::string_view ToString(MouseButton button) noexcept {
  switch (button) {
    using enum MouseButton;
    case kLeft:
      return "Left";
    case kRight:
      return "Right";
    case kMiddle:
      return "Middle";
    case kExtra1:
      return "Extra1";
    case kExtra2:
      return "Extra2";
    case kExtra3:
      return "Extra3";
    case kExtra4:
      return "Extra4";
    case kExtra5:
      return "Extra5";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a mouse button to an output stream.
 * @param os Output stream
 * @param button Mouse button
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MouseButton button) {
  return os << "MouseButton::" << ToString(button);
}

/**
 * @brief Returns the string name of a cursor icon.
 * @param icon Cursor icon
 * @return String name of the icon
 */
[[nodiscard]] constexpr std::string_view ToString(CursorIcon icon) noexcept {
  switch (icon) {
    using enum CursorIcon;
    case kDefault:
      return "Default";
    case kArrow:
      return "Arrow";
    case kIBeam:
      return "IBeam";
    case kCrosshair:
      return "Crosshair";
    case kPointingHand:
      return "PointingHand";
    case kResizeEw:
      return "ResizeEw";
    case kResizeNs:
      return "ResizeNs";
    case kResizeNwse:
      return "ResizeNwse";
    case kResizeNesw:
      return "ResizeNesw";
    case kResizeAll:
      return "ResizeAll";
    case kNotAllowed:
      return "NotAllowed";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a cursor icon to an output stream.
 * @param os Output stream
 * @param icon Cursor icon
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CursorIcon icon) {
  return os << "CursorIcon::" << ToString(icon);
}

/**
 * @brief Formats a custom cursor image using an output iterator.
 * @tparam It Output iterator type
 * @param image Custom cursor image
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CursorImage& image) {
  return std::format_to(
      out,
      "CursorImage{{width={}, height={}, hotspot_x={}, hotspot_y={}, rgba={}}}",
      image.width, image.height, image.hotspot_x, image.hotspot_y,
      image.rgba.size());
}

/**
 * @brief Formats a custom cursor image as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param image Custom cursor image
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const CursorImage& image) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), image);
  return result;
}

/**
 * @brief Formats a custom cursor image as a string.
 * @param image Custom cursor image
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CursorImage& image) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), image);
  return result;
}

/**
 * @brief Outputs a custom cursor image to an output stream.
 * @param os Output stream
 * @param image Custom cursor image
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CursorImage& image) {
  ToString(std::ostreambuf_iterator<char>(os), image);
  return os;
}

/**
 * @brief Formats a mouse state using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param mouse Mouse resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Mouse& mouse) {
  return std::format_to(
      out, "Mouse{{position=({}, {}), delta=({}, {}), scroll=({}, {})}}",
      mouse.position_x, mouse.position_y, mouse.delta_x, mouse.delta_y,
      mouse.scroll_x, mouse.scroll_y);
}

/**
 * @brief Formats a mouse state as a string.
 * @param mouse Mouse resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Mouse& mouse) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), mouse);
  return result;
}

/**
 * @brief Formats a mouse state as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param mouse Mouse resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Mouse& mouse) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), mouse);
  return result;
}

/**
 * @brief Outputs a mouse state to an output stream.
 * @param os Output stream
 * @param mouse Mouse resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Mouse& mouse) {
  ToString(std::ostreambuf_iterator<char>(os), mouse);
  return os;
}

/**
 * @brief Formats a cursor component using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param cursor Cursor component
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Cursor& cursor) {
  out = std::format_to(out, "Cursor{{icon={}", ToString(cursor.icon));
  out = std::format_to(out, ", dirty={}", cursor.dirty);
  if (cursor.custom.has_value()) {
    out = std::format_to(out, ", custom=");
    out = ToString(out, *cursor.custom);
  }
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a cursor component as a string.
 * @param cursor Cursor component
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Cursor& cursor) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), cursor);
  return result;
}

/**
 * @brief Formats a cursor component as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param cursor Cursor component
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Cursor& cursor) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), cursor);
  return result;
}

/**
 * @brief Outputs a cursor component to an output stream.
 * @param os Output stream
 * @param cursor Cursor component
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Cursor& cursor) {
  ToString(std::ostreambuf_iterator<char>(os), cursor);
  return os;
}

/**
 * @brief Formats a mouse button input message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Mouse button input message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const MouseButtonInputMsg& msg) {
  out =
      std::format_to(out, "MouseButtonInputMsg{{entity={}, button={}, state={}",
                     msg.entity, ToString(msg.button), ToString(msg.state));
  out = std::format_to(out, ", modifiers=");
  out = ToString(out, msg.modifiers);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a mouse button input message as a string.
 * @param msg Mouse button input message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseButtonInputMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a mouse button input message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Mouse button input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const MouseButtonInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a mouse button message to an output stream.
 * @param os Output stream
 * @param msg Mouse button message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const MouseButtonInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a cursor moved message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Cursor moved message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CursorMovedMsg& msg) {
  return std::format_to(out, "CursorMovedMsg{{entity={}, x={}, y={}}}",
                        msg.entity, msg.x, msg.y);
}

/**
 * @brief Formats a cursor moved message as a string.
 * @param msg Cursor moved message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CursorMovedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a cursor moved message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Cursor moved message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const CursorMovedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a cursor moved message to an output stream.
 * @param os Output stream
 * @param msg Cursor moved message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CursorMovedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a mouse motion message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Mouse motion message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const MouseMotionMsg& msg) {
  return std::format_to(out,
                        "MouseMotionMsg{{entity={}, delta_x={}, delta_y={}}}",
                        msg.entity, msg.delta_x, msg.delta_y);
}

/**
 * @brief Formats a mouse motion message as a string.
 * @param msg Mouse motion message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseMotionMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a mouse motion message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Mouse motion message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const MouseMotionMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a mouse motion message to an output stream.
 * @param os Output stream
 * @param msg Mouse motion message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const MouseMotionMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a mouse wheel message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Mouse wheel message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const MouseWheelMsg& msg) {
  return std::format_to(out, "MouseWheelMsg{{entity={}, x={}, y={}}}",
                        msg.entity, msg.x, msg.y);
}

/**
 * @brief Formats a mouse wheel message as a string.
 * @param msg Mouse wheel message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseWheelMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a mouse wheel message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Mouse wheel message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const MouseWheelMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a mouse wheel message to an output stream.
 * @param os Output stream
 * @param msg Mouse wheel message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const MouseWheelMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a mouse connection message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Mouse connection message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const MouseConnectionMsg& msg) {
  return std::format_to(
      out, "MouseConnectionMsg{{id={}, connected={}, name=\"{}\"}}", msg.id,
      msg.connected, msg.name);
}

/**
 * @brief Formats a mouse connection message as a string.
 * @param msg Mouse connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseConnectionMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a mouse connection message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Mouse connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const MouseConnectionMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a mouse connection message to an output stream.
 * @param os Output stream
 * @param msg Mouse connection message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const MouseConnectionMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::MouseButton> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::MouseButton button,
                               format_context& ctx) {
    return format_to(ctx.out(), "MouseButton::{}",
                     helios::input::ToString(button));
  }
};

template <>
struct formatter<helios::input::CursorIcon> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::CursorIcon icon,
                               format_context& ctx) {
    return format_to(ctx.out(), "CursorIcon::{}",
                     helios::input::ToString(icon));
  }
};

template <>
struct formatter<helios::input::CursorImage> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::CursorImage& image,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), image);
  }
};

template <>
struct formatter<helios::input::Mouse> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Mouse& mouse, format_context& ctx) {
    return helios::input::ToString(ctx.out(), mouse);
  }
};

template <>
struct formatter<helios::input::Cursor> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Cursor& cursor, format_context& ctx) {
    return helios::input::ToString(ctx.out(), cursor);
  }
};

template <>
struct formatter<helios::input::MouseButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::CursorMovedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::CursorMovedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::MouseMotionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseMotionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::MouseWheelMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseWheelMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::MouseConnectionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseConnectionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
