#pragma once

#include <helios/input/button_input.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>

namespace helios::input {

/// @brief Hat-switch bit flags (GLFW / SDL layout). Combinations are valid.
enum class JoystickHat : uint8_t {
  kCentered = 0,
  kUp = 1,
  kRight = 2,
  kDown = 4,
  kLeft = 8,
};

/// @brief Snapshot of one unmapped joystick slot.
struct Joystick {
  static constexpr size_t kMaxAxes = 16;
  static constexpr size_t kMaxButtons = 32;
  static constexpr size_t kMaxHats = 4;

  /// @brief Clears identity, counts, axes, buttons, and hats.
  void Reset() noexcept;

  std::string name;
  std::string guid;
  std::array<float, kMaxAxes> axes = {};
  IndexedButtonInput<kMaxButtons> buttons;
  int32_t id = -1;
  std::array<JoystickHat, kMaxHats> hats = {};
  uint8_t axis_count = 0;
  uint8_t button_count = 0;
  uint8_t hat_count = 0;
  bool connected = false;
};

inline void Joystick::Reset() noexcept {
  name.clear();
  guid.clear();
  axes.fill(0.0F);
  buttons.Reset();
  hats.fill(JoystickHat::kCentered);
  axis_count = 0;
  button_count = 0;
  hat_count = 0;
  id = -1;
  connected = false;
}

/**
 * @brief Returns the string name of a hat bitmask.
 * @param hat Hat bits (named flags or combinations)
 * @return String name of the hat
 */
[[nodiscard]] constexpr std::string_view ToString(JoystickHat hat) noexcept {
  switch (static_cast<uint8_t>(hat)) {
    case 0:
      return "Centered";
    case 1:
      return "Up";
    case 2:
      return "Right";
    case 3:
      return "RightUp";
    case 4:
      return "Down";
    case 6:
      return "RightDown";
    case 8:
      return "Left";
    case 9:
      return "LeftUp";
    case 12:
      return "LeftDown";
    default:
      return "unknown";
  }
}

/**
 * @brief Outputs a joystick hat to an output stream.
 * @param os Output stream
 * @param hat Joystick hat
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, JoystickHat hat) {
  return os << "JoystickHat::" << ToString(hat);
}

/**
 * @brief Formats a joystick snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param stick Joystick snapshot
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Joystick& stick, It out) {
  return std::format_to(out,
                        "Joystick{{id={}, name=\"{}\", guid=\"{}\", "
                        "connected={}, axes={}, buttons={}, hats={}}}",
                        stick.id, stick.name, stick.guid, stick.connected,
                        stick.axis_count, stick.button_count, stick.hat_count);
}

/**
 * @brief Formats a joystick snapshot as a string.
 * @param stick Joystick snapshot
 * @return Formatted joystick string
 */
[[nodiscard]] inline std::string ToString(const Joystick& stick) {
  std::string result;
  result.reserve(128);
  ToString(stick, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a joystick snapshot to an output stream.
 * @param os Output stream
 * @param stick Joystick snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Joystick& stick) {
  ToString(stick, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::JoystickHat> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::JoystickHat hat,
                               format_context& ctx) {
    return format_to(ctx.out(), "JoystickHat::{}",
                     helios::input::ToString(hat));
  }
};

template <>
struct formatter<helios::input::Joystick> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Joystick& stick,
                     format_context& ctx) {
    return helios::input::ToString(stick, ctx.out());
  }
};

}  // namespace std
