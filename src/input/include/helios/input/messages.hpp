#pragma once

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>

#include <array>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

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
 * @param msg Keyboard input message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const KeyboardInputMsg& msg, It out) {
  out = std::format_to(out, "KeyboardInputMsg{{entity={}, key={}, state={}",
                       msg.entity, ToString(msg.key), ToString(msg.state));
  out = std::format_to(out, ", modifiers=");
  out = ToString(msg.modifiers, out);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a text input message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Text input message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const TextInputMsg& msg, It out) {
  return std::format_to(out, "TextInputMsg{{entity={}, codepoint=U+{:04X}}}",
                        msg.entity, msg.codepoint);
}

/**
 * @brief Formats a mouse button message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Mouse button message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const MouseButtonInputMsg& msg, It out) {
  out =
      std::format_to(out, "MouseButtonInputMsg{{entity={}, button={}, state={}",
                     msg.entity, ToString(msg.button), ToString(msg.state));
  out = std::format_to(out, ", modifiers=");
  out = ToString(msg.modifiers, out);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a cursor moved message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Cursor moved message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const CursorMovedMsg& msg, It out) {
  return std::format_to(out, "CursorMovedMsg{{entity={}, x={}, y={}}}",
                        msg.entity, msg.x, msg.y);
}

/**
 * @brief Formats a mouse motion message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Mouse motion message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const MouseMotionMsg& msg, It out) {
  return std::format_to(out,
                        "MouseMotionMsg{{entity={}, delta_x={}, delta_y={}}}",
                        msg.entity, msg.delta_x, msg.delta_y);
}

/**
 * @brief Formats a mouse wheel message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Mouse wheel message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const MouseWheelMsg& msg, It out) {
  return std::format_to(out, "MouseWheelMsg{{entity={}, x={}, y={}}}",
                        msg.entity, msg.x, msg.y);
}

/**
 * @brief Formats a gamepad connection message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad connection message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadConnectionMsg& msg, It out) {
  return std::format_to(
      out,
      "GamepadConnectionMsg{{id={}, connected={}, name=\"{}\", guid=\"{}\"}}",
      msg.id, msg.connected, msg.name, msg.guid);
}

/**
 * @brief Formats a gamepad button message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad button message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadButtonInputMsg& msg, It out) {
  return std::format_to(out,
                        "GamepadButtonInputMsg{{id={}, button={}, state={}}}",
                        msg.id, ToString(msg.button), ToString(msg.state));
}

/**
 * @brief Formats a gamepad axis message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad axis message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadAxisChangedMsg& msg, It out) {
  return std::format_to(out,
                        "GamepadAxisChangedMsg{{id={}, axis={}, value={}}}",
                        msg.id, ToString(msg.axis), msg.value);
}

/**
 * @brief Formats a gamepad remap message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad remap message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadRemappedMsg& msg, It out) {
  return std::format_to(out, "GamepadRemappedMsg{{id={}, mapping=\"{}\"}}",
                        msg.id, msg.mapping);
}

/**
 * @brief Formats a gamepad power message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad power message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadPowerChangedMsg& msg, It out) {
  out = std::format_to(out, "GamepadPowerChangedMsg{{id={}, power=", msg.id);
  out = ToString(msg.power, out);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a gamepad sensor message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad sensor message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadSensorUpdateMsg& msg, It out) {
  return std::format_to(
      out, "GamepadSensorUpdateMsg{{id={}, sensor={}, value=({}, {}, {})}}",
      msg.id, ToString(msg.sensor), msg.value[0], msg.value[1], msg.value[2]);
}

/**
 * @brief Formats a gamepad touchpad message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Gamepad touchpad message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadTouchpadMsg& msg, It out) {
  return std::format_to(out,
                        "GamepadTouchpadMsg{{id={}, finger={}, x={}, y={}, "
                        "pressure={}, down={}}}",
                        msg.id, msg.finger, msg.x, msg.y, msg.pressure,
                        msg.down);
}

/**
 * @brief Formats a joystick connection message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Joystick connection message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const JoystickConnectionMsg& msg, It out) {
  return std::format_to(
      out,
      "JoystickConnectionMsg{{id={}, connected={}, name=\"{}\", guid=\"{}\", "
      "axes={}, buttons={}, hats={}}}",
      msg.id, msg.connected, msg.name, msg.guid, msg.axis_count,
      msg.button_count, msg.hat_count);
}

/**
 * @brief Formats a joystick button message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Joystick button message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const JoystickButtonInputMsg& msg, It out) {
  return std::format_to(out,
                        "JoystickButtonInputMsg{{id={}, button={}, state={}}}",
                        msg.id, msg.button, ToString(msg.state));
}

/**
 * @brief Formats a joystick axis message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Joystick axis message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const JoystickAxisChangedMsg& msg, It out) {
  return std::format_to(out,
                        "JoystickAxisChangedMsg{{id={}, axis={}, value={}}}",
                        msg.id, msg.axis, msg.value);
}

/**
 * @brief Formats a joystick hat message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Joystick hat message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const JoystickHatChangedMsg& msg, It out) {
  return std::format_to(out, "JoystickHatChangedMsg{{id={}, hat={}, value={}}}",
                        msg.id, msg.hat, ToString(msg.value));
}

/**
 * @brief Formats a pen proximity message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Pen proximity message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const PenProximityMsg& msg, It out) {
  return std::format_to(
      out,
      "PenProximityMsg{{entity={}, id={}, device_type={}, in_proximity={}}}",
      msg.entity, msg.id, ToString(msg.device_type), msg.in_proximity);
}

/**
 * @brief Formats a pen touch message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Pen touch message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const PenTouchMsg& msg, It out) {
  return std::format_to(
      out, "PenTouchMsg{{entity={}, x={}, y={}, id={}, down={}, eraser={}}}",
      msg.entity, msg.x, msg.y, msg.id, msg.down, msg.eraser);
}

/**
 * @brief Formats a pen button message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Pen button message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const PenButtonInputMsg& msg, It out) {
  return std::format_to(out,
                        "PenButtonInputMsg{{entity={}, id={}, button={}, "
                        "state={}}}",
                        msg.entity, msg.id, ToString(msg.button),
                        ToString(msg.state));
}

/**
 * @brief Formats a pen moved message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Pen moved message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const PenMovedMsg& msg, It out) {
  return std::format_to(out, "PenMovedMsg{{entity={}, x={}, y={}, id={}}}",
                        msg.entity, msg.x, msg.y, msg.id);
}

/**
 * @brief Formats a pen axis message using an output iterator.
 * @tparam It Output iterator type
 * @param msg Pen axis message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const PenAxisChangedMsg& msg, It out) {
  return std::format_to(
      out,
      "PenAxisChangedMsg{{entity={}, x={}, y={}, value={}, id={}, axis={}}}",
      msg.entity, msg.x, msg.y, msg.value, msg.id, ToString(msg.axis));
}

/**
 * @brief Formats a KeyboardInput message as a string.
 * @param msg KeyboardInput message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const KeyboardInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a KeyboardInput message to an output stream.
 * @param os Output stream
 * @param msg KeyboardInputMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const KeyboardInputMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a TextInput message as a string.
 * @param msg TextInput message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const TextInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a TextInput message to an output stream.
 * @param os Output stream
 * @param msg TextInputMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const TextInputMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a MouseButtonInput message as a string.
 * @param msg MouseButtonInput message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a MouseButtonInput message to an output stream.
 * @param os Output stream
 * @param msg MouseButtonInputMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const MouseButtonInputMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a CursorMoved message as a string.
 * @param msg CursorMoved message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CursorMovedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a CursorMoved message to an output stream.
 * @param os Output stream
 * @param msg CursorMovedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CursorMovedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a MouseMotion message as a string.
 * @param msg MouseMotion message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseMotionMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a MouseMotion message to an output stream.
 * @param os Output stream
 * @param msg MouseMotionMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const MouseMotionMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a MouseWheel message as a string.
 * @param msg MouseWheel message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const MouseWheelMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a MouseWheel message to an output stream.
 * @param os Output stream
 * @param msg MouseWheelMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const MouseWheelMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadConnection message as a string.
 * @param msg GamepadConnection message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadConnectionMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadConnection message to an output stream.
 * @param os Output stream
 * @param msg GamepadConnectionMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadConnectionMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadButtonInput message as a string.
 * @param msg GamepadButtonInput message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadButtonInput message to an output stream.
 * @param os Output stream
 * @param msg GamepadButtonInputMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadButtonInputMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadAxisChanged message as a string.
 * @param msg GamepadAxisChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadAxisChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadAxisChanged message to an output stream.
 * @param os Output stream
 * @param msg GamepadAxisChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadAxisChangedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadRemapped message as a string.
 * @param msg GamepadRemapped message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadRemappedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadRemapped message to an output stream.
 * @param os Output stream
 * @param msg GamepadRemappedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadRemappedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadPowerChanged message as a string.
 * @param msg GamepadPowerChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadPowerChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadPowerChanged message to an output stream.
 * @param os Output stream
 * @param msg GamepadPowerChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadPowerChangedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadSensorUpdate message as a string.
 * @param msg GamepadSensorUpdate message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadSensorUpdateMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadSensorUpdate message to an output stream.
 * @param os Output stream
 * @param msg GamepadSensorUpdateMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadSensorUpdateMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a GamepadTouchpad message as a string.
 * @param msg GamepadTouchpad message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadTouchpadMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a GamepadTouchpad message to an output stream.
 * @param os Output stream
 * @param msg GamepadTouchpadMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadTouchpadMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a JoystickConnection message as a string.
 * @param msg JoystickConnection message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickConnectionMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a JoystickConnection message to an output stream.
 * @param os Output stream
 * @param msg JoystickConnectionMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickConnectionMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a JoystickButtonInput message as a string.
 * @param msg JoystickButtonInput message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a JoystickButtonInput message to an output stream.
 * @param os Output stream
 * @param msg JoystickButtonInputMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickButtonInputMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a JoystickAxisChanged message as a string.
 * @param msg JoystickAxisChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickAxisChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a JoystickAxisChanged message to an output stream.
 * @param os Output stream
 * @param msg JoystickAxisChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickAxisChangedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a JoystickHatChanged message as a string.
 * @param msg JoystickHatChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const JoystickHatChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a JoystickHatChanged message to an output stream.
 * @param os Output stream
 * @param msg JoystickHatChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const JoystickHatChangedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a PenProximity message as a string.
 * @param msg PenProximity message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenProximityMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a PenProximity message to an output stream.
 * @param os Output stream
 * @param msg PenProximityMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenProximityMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a PenTouch message as a string.
 * @param msg PenTouch message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenTouchMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a PenTouch message to an output stream.
 * @param os Output stream
 * @param msg PenTouchMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenTouchMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a PenButtonInput message as a string.
 * @param msg PenButtonInput message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a PenButtonInput message to an output stream.
 * @param os Output stream
 * @param msg PenButtonInputMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const PenButtonInputMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a PenMoved message as a string.
 * @param msg PenMoved message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenMovedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a PenMoved message to an output stream.
 * @param os Output stream
 * @param msg PenMovedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenMovedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a PenAxisChanged message as a string.
 * @param msg PenAxisChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenAxisChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a PenAxisChanged message to an output stream.
 * @param os Output stream
 * @param msg PenAxisChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const PenAxisChangedMsg& msg) {
  ToString(msg, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::KeyboardInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::KeyboardInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::TextInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::TextInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::MouseButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::CursorMovedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::CursorMovedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::MouseMotionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseMotionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::MouseWheelMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::MouseWheelMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadConnectionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadConnectionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadRemappedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadRemappedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadPowerChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadPowerChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadSensorUpdateMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadSensorUpdateMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadTouchpadMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadTouchpadMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::JoystickConnectionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickConnectionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::JoystickButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::JoystickAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::JoystickHatChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::JoystickHatChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::PenProximityMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenProximityMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::PenTouchMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenTouchMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::PenButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::PenMovedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenMovedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

template <>
struct formatter<helios::input::PenAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(msg, ctx.out());
  }
};

}  // namespace std
