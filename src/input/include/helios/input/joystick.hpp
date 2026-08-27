#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/message/message.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#endif
#include <helios/input/button_input.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/ids.hpp>
#include <helios/input/keyboard.hpp>

HELIOS_MODULE_EXPORT
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
  std::optional<JoystickId> id;
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
  id.reset();
  connected = false;
}

/// @brief Fixed unmapped-joystick slot table (same `0..15` ids as `Gamepads`).
/// @details A physical device occupies either a gamepad slot or a joystick
/// slot, never both.
struct Joysticks {
  static constexpr std::string_view kName = "helios::input::Joysticks";
  static constexpr size_t kSlotCount = Gamepads::kSlotCount;

  std::array<Joystick, kSlotCount> sticks = {};

  /**
   * @brief Looks up a mutable joystick slot by id.
   * @param id Joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr Joystick* TryGet(JoystickId id) noexcept;

  /**
   * @brief Looks up a const joystick slot by id.
   * @param id Joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Joystick* TryGet(JoystickId id) const noexcept;
};

constexpr Joystick* Joysticks::TryGet(JoystickId id) noexcept {
  if (static_cast<size_t>(id) >= sticks.size()) {
    return nullptr;
  }
  return &sticks[static_cast<size_t>(id)];
}

constexpr const Joystick* Joysticks::TryGet(JoystickId id) const noexcept {
  if (static_cast<size_t>(id) >= sticks.size()) {
    return nullptr;
  }
  return &sticks[static_cast<size_t>(id)];
}

/// @brief Unmapped joystick connect / disconnect notification.
struct JoystickConnectionMsg {
  static constexpr std::string_view kName =
      "helios::input::JoystickConnectionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string name;
  std::string guid;
  JoystickId id = 0;
  uint8_t axis_count = 0;
  uint8_t button_count = 0;
  uint8_t hat_count = 0;
  bool connected = false;
};

/// @brief Unmapped joystick button press / release event.
struct JoystickButtonInputMsg {
  static constexpr std::string_view kName =
      "helios::input::JoystickButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  JoystickId id = 0;
  uint8_t button = 0;
  ButtonState state = ButtonState::kReleased;
};

/// @brief Unmapped joystick axis value change (raw `-1..1`).
struct JoystickAxisChangedMsg {
  static constexpr std::string_view kName =
      "helios::input::JoystickAxisChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  JoystickId id = 0;
  uint8_t axis = 0;
  float value = 0.0F;
};

/// @brief Unmapped joystick hat-switch change.
struct JoystickHatChangedMsg {
  static constexpr std::string_view kName =
      "helios::input::JoystickHatChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  JoystickId id = 0;
  uint8_t hat = 0;
  JoystickHat value = JoystickHat::kCentered;
};

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
 * @param out Output iterator to write the formatted string to
 * @param stick Joystick snapshot
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Joystick& stick) {
  out = std::format_to(out, "Joystick{{id=");
  if (stick.id.has_value()) {
    out = std::format_to(out, "{}", *stick.id);
  } else {
    out = std::format_to(out, "none");
  }
  return std::format_to(out,
                        ", name=\"{}\", guid=\"{}\", "
                        "connected={}, axes={}, buttons={}, hats={}}}",
                        stick.name, stick.guid, stick.connected,
                        stick.axis_count, stick.button_count, stick.hat_count);
}

/**
 * @brief Formats a joystick snapshot as a string.
 * @param stick Joystick snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Joystick& stick) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), stick);
  return result;
}

/**
 * @brief Formats a joystick snapshot as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param stick Joystick snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Joystick& stick) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), stick);
  return result;
}

/**
 * @brief Outputs a joystick snapshot to an output stream.
 * @param os Output stream
 * @param stick Joystick snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Joystick& stick) {
  ToString(std::ostreambuf_iterator<char>(os), stick);
  return os;
}

/**
 * @brief Formats a joystick slot table using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param joysticks Joysticks resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Joysticks& joysticks) {
  out = std::format_to(out, "Joysticks{{sticks = [");

  size_t cnt = 0;
  for (const auto& stick : joysticks.sticks) {
    if (!stick.connected) {
      continue;
    }

    if (cnt++ > 0) [[likely]] {
      out = std::format_to(out, ", ");
    }
    ToString(out, stick);
  };

  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a joystick slot table as a string.
 * @param joysticks Joysticks resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Joysticks& joysticks) {
  std::string result;
  result.reserve(512);
  ToString(std::back_inserter(result), joysticks);
  return result;
}

/**
 * @brief Formats a joystick slot table as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param joysticks Joysticks resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Joysticks& joysticks) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(512);
  ToString(std::back_inserter(result), joysticks);
  return result;
}

/**
 * @brief Outputs a joystick slot table to an output stream.
 * @param os Output stream
 * @param joysticks Joysticks resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Joysticks& joysticks) {
  ToString(std::ostreambuf_iterator<char>(os), joysticks);
  return os;
}

/**
 * @brief Formats a joystick connection message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Joystick connection message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const JoystickConnectionMsg& msg) {
  return std::format_to(
      out,
      "JoystickConnectionMsg{{id={}, connected={}, name=\"{}\", guid=\"{}\", "
      "axes={}, buttons={}, hats={}}}",
      msg.id, msg.connected, msg.name, msg.guid, msg.axis_count,
      msg.button_count, msg.hat_count);
}

/**
 * @brief Formats a joystick connection message as a string.
 * @param msg Joystick connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickConnectionMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a joystick connection message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Joystick connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const JoystickConnectionMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a joystick connection message to an output stream.
 * @param os Output stream
 * @param msg Joystick connection message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickConnectionMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a joystick button message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Joystick button message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const JoystickButtonInputMsg& msg) {
  return std::format_to(out,
                        "JoystickButtonInputMsg{{id={}, button={}, state={}}}",
                        msg.id, msg.button, ToString(msg.state));
}

/**
 * @brief Formats a joystick button input message as a string.
 * @param msg Joystick button message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a joystick button input message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Joystick button input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const JoystickButtonInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a joystick button message to an output stream.
 * @param os Output stream
 * @param msg Joystick button message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickButtonInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a joystick axis message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Joystick axis message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const JoystickAxisChangedMsg& msg) {
  return std::format_to(out,
                        "JoystickAxisChangedMsg{{id={}, axis={}, value={}}}",
                        msg.id, msg.axis, msg.value);
}

/**
 * @brief Formats a joystick axis message as a string.
 * @param msg Joystick axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickAxisChangedMsg& msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a joystick axis message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Joystick axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const JoystickAxisChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a joystick axis message to an output stream.
 * @param os Output stream
 * @param msg Joystick axis message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickAxisChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a joystick hat message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Joystick hat message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const JoystickHatChangedMsg& msg) {
  return std::format_to(out, "JoystickHatChangedMsg{{id={}, hat={}, value={}}}",
                        msg.id, msg.hat, ToString(msg.value));
}

/**
 * @brief Formats a joystick hat message as a string.
 * @param msg Joystick hat message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickHatChangedMsg& msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a joystick hat message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Joystick hat message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const JoystickHatChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a joystick hat message to an output stream.
 * @param os Output stream
 * @param msg Joystick hat message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickHatChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
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
    return helios::input::ToString(ctx.out(), stick);
  }
};

template <>
struct formatter<helios::input::Joysticks> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Joysticks& joysticks,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), joysticks);
  }
};

template <>
struct formatter<helios::input::JoystickConnectionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickConnectionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::JoystickButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::JoystickAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::JoystickHatChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickHatChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
