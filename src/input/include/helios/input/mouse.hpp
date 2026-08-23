#pragma once

#include <helios/memory/temporary_storage.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
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

}  // namespace helios::input

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

}  // namespace std
