#pragma once

#include <cstdint>
#include <format>
#include <ostream>
#include <string_view>
#include <vector>

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

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::MouseButton> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
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
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::CursorIcon icon,
                               format_context& ctx) {
    return format_to(ctx.out(), "CursorIcon::{}",
                     helios::input::ToString(icon));
  }
};

}  // namespace std
