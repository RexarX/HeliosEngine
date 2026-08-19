#pragma once

#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace helios::input {

/// @brief Pen barrel buttons (SDL buttons 1–5). Tip contact is not a button.
enum class PenButton : uint8_t {
  kBarrel1 = 0,
  kBarrel2,
  kBarrel3,
  kBarrel4,
  kBarrel5,
  kCount,
};

/// @brief Pen axes (SDL order and ranges).
enum class PenAxis : uint8_t {
  kPressure = 0,
  kXTilt,
  kYTilt,
  kDistance,
  kRotation,
  kSlider,
  kTangentialPressure,
  kCount,
};

/// @brief Whether the pen touches a display or an external tablet.
enum class PenDeviceType : uint8_t {
  kUnknown = 0,
  kDirect,
  kIndirect,
};

using PenButtonInput = ButtonInput<PenButton>;

/// @brief Snapshot of one pen / stylus slot.
struct Pen {
  /// @brief Clears identity, buttons, axes, position, and tip state.
  void Reset() noexcept;

  double position_x = 0.0;
  double position_y = 0.0;
  double delta_x = 0.0;
  double delta_y = 0.0;
  Axis<PenAxis> axes;
  PenButtonInput buttons;
  int32_t id = -1;
  PenDeviceType device_type = PenDeviceType::kUnknown;
  bool in_proximity = false;
  bool down = false;
  bool eraser = false;
  bool has_position = false;

  /**
   * @brief Returns the pen position.
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
};

inline void Pen::Reset() noexcept {
  position_x = 0.0;
  position_y = 0.0;
  delta_x = 0.0;
  delta_y = 0.0;
  axes.Clear();
  buttons.Reset();
  id = -1;
  device_type = PenDeviceType::kUnknown;
  in_proximity = false;
  down = false;
  eraser = false;
  has_position = false;
}

/**
 * @brief Returns the string name of a pen button.
 * @param button Pen button
 * @return String name of the button
 */
[[nodiscard]] constexpr std::string_view ToString(PenButton button) noexcept {
  switch (button) {
    using enum PenButton;
    case kBarrel1:
      return "Barrel1";
    case kBarrel2:
      return "Barrel2";
    case kBarrel3:
      return "Barrel3";
    case kBarrel4:
      return "Barrel4";
    case kBarrel5:
      return "Barrel5";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a pen button to an output stream.
 * @param os Output stream
 * @param button Pen button
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PenButton button) {
  return os << "PenButton::" << ToString(button);
}

/**
 * @brief Returns the string name of a pen axis.
 * @param axis Pen axis
 * @return String name of the axis
 */
[[nodiscard]] constexpr std::string_view ToString(PenAxis axis) noexcept {
  switch (axis) {
    using enum PenAxis;
    case kPressure:
      return "Pressure";
    case kXTilt:
      return "XTilt";
    case kYTilt:
      return "YTilt";
    case kDistance:
      return "Distance";
    case kRotation:
      return "Rotation";
    case kSlider:
      return "Slider";
    case kTangentialPressure:
      return "TangentialPressure";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a pen axis to an output stream.
 * @param os Output stream
 * @param axis Pen axis
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PenAxis axis) {
  return os << "PenAxis::" << ToString(axis);
}

/**
 * @brief Returns the string name of a pen device type.
 * @param type Pen device type
 * @return String name of the type
 */
[[nodiscard]] constexpr std::string_view ToString(PenDeviceType type) noexcept {
  switch (type) {
    using enum PenDeviceType;
    case kUnknown:
      return "Unknown";
    case kDirect:
      return "Direct";
    case kIndirect:
      return "Indirect";
  }
  return "unknown";
}

/**
 * @brief Outputs a pen device type to an output stream.
 * @param os Output stream
 * @param type Pen device type
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PenDeviceType type) {
  return os << "PenDeviceType::" << ToString(type);
}

/**
 * @brief Formats a pen snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param pen Pen snapshot
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Pen& pen, It out) {
  return std::format_to(
      out,
      "Pen{{position=({}, {}), delta=({}, {}), id={}, device_type={}, "
      "in_proximity={}, down={}, eraser={}}}",
      pen.position_x, pen.position_y, pen.delta_x, pen.delta_y, pen.id,
      ToString(pen.device_type), pen.in_proximity, pen.down, pen.eraser);
}

/**
 * @brief Formats a pen snapshot as a string.
 * @param pen Pen snapshot
 * @return Formatted pen string
 */
[[nodiscard]] inline std::string ToString(const Pen& pen) {
  std::string result;
  result.reserve(160);
  ToString(pen, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a pen snapshot to an output stream.
 * @param os Output stream
 * @param pen Pen snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Pen& pen) {
  ToString(pen, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::PenButton> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::PenButton button,
                               format_context& ctx) {
    return format_to(ctx.out(), "PenButton::{}",
                     helios::input::ToString(button));
  }
};

template <>
struct formatter<helios::input::PenAxis> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::PenAxis axis,
                               format_context& ctx) {
    return format_to(ctx.out(), "PenAxis::{}", helios::input::ToString(axis));
  }
};

template <>
struct formatter<helios::input::PenDeviceType> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::PenDeviceType type,
                               format_context& ctx) {
    return format_to(ctx.out(), "PenDeviceType::{}",
                     helios::input::ToString(type));
  }
};

template <>
struct formatter<helios::input::Pen> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Pen& pen, format_context& ctx) {
    return helios::input::ToString(pen, ctx.out());
  }
};

}  // namespace std
