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

#include <array>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#endif
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Keyboard key press / release / repeat event for a window entity.
struct KeyboardInputMsg {
  static constexpr std::string_view kName = "helios::input::KeyboardInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  Key key = Key::kUnknown;
  ButtonState state = ButtonState::kReleased;
  Modifiers modifiers = Modifiers::kNone;
};

/// @brief Unicode text input event for a window entity.
struct TextInputMsg {
  static constexpr std::string_view kName = "helios::input::TextInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  uint32_t codepoint = 0;
};

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

/// @brief Gamepad connect / disconnect notification.
struct GamepadConnectionMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadConnectionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  bool connected = false;
  std::string name;
  std::string guid;
};

/// @brief Gamepad button press / release event.
struct GamepadButtonInputMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  GamepadButton button = GamepadButton::kA;
  ButtonState state = ButtonState::kReleased;
};

/// @brief Gamepad axis value change.
/// @details `value` is the raw backend sample (GLFW gamepad range). Filtered
/// stick / trigger values are applied to `Gamepad::axes` by
/// `UpdateGamepadState`.
struct GamepadAxisChangedMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadAxisChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  GamepadAxis axis = GamepadAxis::kLeftX;
  float value = 0.0F;
};

/// @brief Gamepad mapping string changed for a connected slot.
struct GamepadRemappedMsg {
  static constexpr std::string_view kName = "helios::input::GamepadRemappedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  std::string mapping;
};

/// @brief Gamepad battery / power snapshot change.
struct GamepadPowerChangedMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadPowerChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  GamepadPower power;
};

/// @brief Latest gyro or accelerometer sample for a gamepad slot.
struct GamepadSensorUpdateMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadSensorUpdateMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  GamepadSensor sensor = GamepadSensor::kGyro;
  std::array<float, 3> value = {};
};

/// @brief Touchpad finger sample for a gamepad slot.
struct GamepadTouchpadMsg {
  static constexpr std::string_view kName = "helios::input::GamepadTouchpadMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  float x = 0.0F;
  float y = 0.0F;
  float pressure = 0.0F;
  uint8_t finger = 0;
  bool down = false;

  /**
   * @brief Returns the finger position.
   * @return Normalized x and y coordinates
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<float, float> {
    return {x, y};
  }
};

/// @brief Unmapped joystick connect / disconnect notification.
struct JoystickConnectionMsg {
  static constexpr std::string_view kName =
      "helios::input::JoystickConnectionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string name;
  std::string guid;
  int32_t id = -1;
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

  int32_t id = -1;
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

  int32_t id = -1;
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

  int32_t id = -1;
  uint8_t hat = 0;
  JoystickHat value = JoystickHat::kCentered;
};

/// @brief Pen proximity in / out notification for a window entity.
struct PenProximityMsg {
  static constexpr std::string_view kName = "helios::input::PenProximityMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  int32_t id = -1;
  PenDeviceType device_type = PenDeviceType::kUnknown;
  bool in_proximity = false;
};

/// @brief Pen tip down / up event for a window entity.
struct PenTouchMsg {
  static constexpr std::string_view kName = "helios::input::PenTouchMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  int32_t id = -1;
  bool down = false;
  bool eraser = false;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/// @brief Pen barrel-button press / release event for a window entity.
struct PenButtonInputMsg {
  static constexpr std::string_view kName = "helios::input::PenButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  int32_t id = -1;
  PenButton button = PenButton::kBarrel1;
  ButtonState state = ButtonState::kReleased;
};

/// @brief Absolute pen position change for a window entity.
struct PenMovedMsg {
  static constexpr std::string_view kName = "helios::input::PenMovedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  int32_t id = -1;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/// @brief Pen axis value change for a window entity.
struct PenAxisChangedMsg {
  static constexpr std::string_view kName = "helios::input::PenAxisChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  float value = 0.0F;
  int32_t id = -1;
  PenAxis axis = PenAxis::kPressure;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/**
 * @brief Formats a keyboard input message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Keyboard input message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const KeyboardInputMsg& msg) {
  out = std::format_to(out, "KeyboardInputMsg{{entity={}, key={}, state={}",
                       msg.entity, ToString(msg.key), ToString(msg.state));
  out = std::format_to(out, ", modifiers=");
  out = ToString(out, msg.modifiers);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a keyboard input message as a string.
 * @param msg Keyboard input message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const KeyboardInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a keyboard input message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Keyboard input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const KeyboardInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a keyboard input message to an output stream.
 * @param os Output stream
 * @param msg Keyboard input message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const KeyboardInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a text input message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Text input message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const TextInputMsg& msg) {
  return std::format_to(out, "TextInputMsg{{entity={}, codepoint=U+{:04X}}}",
                        msg.entity, msg.codepoint);
}

/**
 * @brief Formats a text input message as a string.
 * @param msg Text input message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const TextInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a text input message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Text input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const TextInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a text input message to an output stream.
 * @param os Output stream
 * @param msg Text input message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const TextInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
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
 * @brief Formats a gamepad connection message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad connection message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadConnectionMsg& msg) {
  return std::format_to(
      out,
      "GamepadConnectionMsg{{id={}, connected={}, name=\"{}\", guid=\"{}\"}}",
      msg.id, msg.connected, msg.name, msg.guid);
}

/**
 * @brief Formats a gamepad connection message as a string.
 * @param msg Gamepad connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadConnectionMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad connection message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadConnectionMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad connection message to an output stream.
 * @param os Output stream
 * @param msg Gamepad connection message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadConnectionMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a gamepad button message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad button message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadButtonInputMsg& msg) {
  return std::format_to(out,
                        "GamepadButtonInputMsg{{id={}, button={}, state={}}}",
                        msg.id, ToString(msg.button), ToString(msg.state));
}

/**
 * @brief Formats a gamepad button message as a string.
 * @param msg Gamepad button message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad button input message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad button input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadButtonInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad button message to an output stream.
 * @param os Output stream
 * @param msg Gamepad button message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadButtonInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a gamepad axis message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad axis message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadAxisChangedMsg& msg) {
  return std::format_to(out,
                        "GamepadAxisChangedMsg{{id={}, axis={}, value={}}}",
                        msg.id, ToString(msg.axis), msg.value);
}

/**
 * @brief Formats a gamepad axis message as a string.
 * @param msg Gamepad axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadAxisChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad axis message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadAxisChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad axis message to an output stream.
 * @param os Output stream
 * @param msg Gamepad axis message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadAxisChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a gamepad remap message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad remap message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadRemappedMsg& msg) {
  return std::format_to(out, "GamepadRemappedMsg{{id={}, mapping=\"{}\"}}",
                        msg.id, msg.mapping);
}

/**
 * @brief Formats a gamepad remap message as a string.
 * @param msg Gamepad remap message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadRemappedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad remap message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad remap message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadRemappedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad remap message to an output stream.
 * @param os Output stream
 * @param msg Gamepad remap message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadRemappedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a gamepad power message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad power message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadPowerChangedMsg& msg) {
  out = std::format_to(out, "GamepadPowerChangedMsg{{id={}, power=", msg.id);
  out = ToString(out, msg.power);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a gamepad power message as a string.
 * @param msg Gamepad power message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadPowerChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad power message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad power message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadPowerChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad power message to an output stream.
 * @param os Output stream
 * @param msg Gamepad power message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadPowerChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a gamepad sensor message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad sensor message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadSensorUpdateMsg& msg) {
  return std::format_to(
      out, "GamepadSensorUpdateMsg{{id={}, sensor={}, value=({}, {}, {})}}",
      msg.id, ToString(msg.sensor), msg.value[0], msg.value[1], msg.value[2]);
}

/**
 * @brief Formats a gamepad sensor message as a string.
 * @param msg Gamepad sensor message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadSensorUpdateMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad sensor message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad sensor message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadSensorUpdateMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad sensor message to an output stream.
 * @param os Output stream
 * @param msg Gamepad sensor message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadSensorUpdateMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a gamepad touchpad message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Gamepad touchpad message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadTouchpadMsg& msg) {
  return std::format_to(out,
                        "GamepadTouchpadMsg{{id={}, finger={}, x={}, y={}, "
                        "pressure={}, down={}}}",
                        msg.id, msg.finger, msg.x, msg.y, msg.pressure,
                        msg.down);
}

/**
 * @brief Formats a gamepad touchpad message as a string.
 * @param msg Gamepad touchpad message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadTouchpadMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a gamepad touchpad message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Gamepad touchpad message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadTouchpadMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a gamepad touchpad message to an output stream.
 * @param os Output stream
 * @param msg Gamepad touchpad message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadTouchpadMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
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

/**
 * @brief Formats a pen proximity message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen proximity message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenProximityMsg& msg) {
  return std::format_to(
      out,
      "PenProximityMsg{{entity={}, id={}, device_type={}, in_proximity={}}}",
      msg.entity, msg.id, ToString(msg.device_type), msg.in_proximity);
}

/**
 * @brief Formats a pen proximity message as a string.
 * @param msg Pen proximity message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenProximityMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen proximity message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen proximity message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const PenProximityMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen proximity message to an output stream.
 * @param os Output stream
 * @param msg Pen proximity message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenProximityMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen touch message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen touch message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenTouchMsg& msg) {
  return std::format_to(
      out, "PenTouchMsg{{entity={}, x={}, y={}, id={}, down={}, eraser={}}}",
      msg.entity, msg.x, msg.y, msg.id, msg.down, msg.eraser);
}

/**
 * @brief Formats a pen touch message as a string.
 * @param msg Pen touch message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenTouchMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen touch message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen touch message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const PenTouchMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen touch message to an output stream.
 * @param os Output stream
 * @param msg Pen touch message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenTouchMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen button message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen button message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenButtonInputMsg& msg) {
  return std::format_to(out,
                        "PenButtonInputMsg{{entity={}, id={}, button={}, "
                        "state={}}}",
                        msg.entity, msg.id, ToString(msg.button),
                        ToString(msg.state));
}

/**
 * @brief Formats a pen button input message as a string.
 * @param msg Pen button message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen button input message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen button input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const PenButtonInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen button message to an output stream.
 * @param os Output stream
 * @param msg Pen button message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const PenButtonInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen moved message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen moved message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenMovedMsg& msg) {
  return std::format_to(out, "PenMovedMsg{{entity={}, x={}, y={}, id={}}}",
                        msg.entity, msg.x, msg.y, msg.id);
}

/**
 * @brief Formats a pen moved message as a string.
 * @param msg Pen moved message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenMovedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen moved message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen moved message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const PenMovedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen moved message to an output stream.
 * @param os Output stream
 * @param msg Pen moved message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenMovedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen axis message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen axis message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenAxisChangedMsg& msg) {
  return std::format_to(
      out,
      "PenAxisChangedMsg{{entity={}, x={}, y={}, value={}, id={}, axis={}}}",
      msg.entity, msg.x, msg.y, msg.value, msg.id, ToString(msg.axis));
}

/**
 * @brief Formats a pen axis message as a string.
 * @param msg Pen axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenAxisChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen axis message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const PenAxisChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen axis message to an output stream.
 * @param os Output stream
 * @param msg Pen axis message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const PenAxisChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::KeyboardInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::KeyboardInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::TextInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::TextInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
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
struct formatter<helios::input::GamepadConnectionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadConnectionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::GamepadButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::GamepadAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::GamepadRemappedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadRemappedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::GamepadPowerChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadPowerChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::GamepadSensorUpdateMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadSensorUpdateMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::GamepadTouchpadMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadTouchpadMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
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

template <>
struct formatter<helios::input::PenProximityMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenProximityMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenTouchMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenTouchMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenMovedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenMovedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
