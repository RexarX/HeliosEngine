#pragma once

#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>

#include <array>
#include <cstdint>
#include <format>
#include <ostream>
#include <string>
#include <string_view>

namespace helios::input {

/// @brief Contiguous gamepad button identifiers (GLFW order).
enum class GamepadButton : uint8_t {
  kA = 0,
  kB,
  kX,
  kY,
  kLeftBumper,
  kRightBumper,
  kBack,
  kStart,
  kGuide,
  kLeftThumb,
  kRightThumb,
  kDpadUp,
  kDpadRight,
  kDpadDown,
  kDpadLeft,
  kCount,
};

/// @brief Contiguous gamepad axis identifiers (GLFW order).
enum class GamepadAxis : uint8_t {
  kLeftX = 0,
  kLeftY,
  kRightX,
  kRightY,
  kLeftTrigger,
  kRightTrigger,
  kCount,
};

using GamepadButtonInput = ButtonInput<GamepadButton>;

/**
 * @brief Returns whether `axis` is a trigger.
 * @param axis Gamepad axis
 * @return `true` for left / right trigger
 */
[[nodiscard]] constexpr bool Trigger(GamepadAxis axis) noexcept {
  return axis == GamepadAxis::kLeftTrigger ||
         axis == GamepadAxis::kRightTrigger;
}

/**
 * @brief Returns whether `axis` is on the left stick.
 * @param axis Gamepad axis
 * @return `true` for left X / Y
 */
[[nodiscard]] constexpr bool LeftStick(GamepadAxis axis) noexcept {
  return axis == GamepadAxis::kLeftX || axis == GamepadAxis::kLeftY;
}

/**
 * @brief Returns whether `axis` is on the right stick.
 * @param axis Gamepad axis
 * @return `true` for right X / Y
 */
[[nodiscard]] constexpr bool RightStick(GamepadAxis axis) noexcept {
  return axis == GamepadAxis::kRightX || axis == GamepadAxis::kRightY;
}

/**
 * @brief Per-slot rest-center bookkeeping for `UpdateGamepadState`.
 * @details `raw` stores the last backend samples (GLFW range). `center` is the
 * calibrated rest for each axis (sticks default `0`, triggers `-1`).
 */
struct GamepadAxisFilter {
  static constexpr size_t kSize = Axis<GamepadAxis>::kSize;
  static constexpr float kTriggerRest = -1.0F;
  static constexpr float kRestCapture = 0.35F;

  /// @brief Initializes trigger rest to GLFW `-1`.
  GamepadAxisFilter() noexcept { Reset(); }

  /// @brief Clears samples and restores default rest centers.
  void Reset() noexcept;

  Axis<GamepadAxis> raw;
  std::array<float, kSize> center{};
  std::array<uint16_t, kSize> rest_frames{};
  std::array<uint8_t, kSize> seen{};
};

inline void GamepadAxisFilter::Reset() noexcept {
  raw.Clear();
  raw.Set(GamepadAxis::kLeftTrigger, kTriggerRest);
  raw.Set(GamepadAxis::kRightTrigger, kTriggerRest);
  center.fill(0.0F);
  center[static_cast<size_t>(GamepadAxis::kLeftTrigger)] = kTriggerRest;
  center[static_cast<size_t>(GamepadAxis::kRightTrigger)] = kTriggerRest;
  rest_frames.fill(0);
  seen.fill(0);
}

/// @brief Snapshot of one gamepad slot.
/// @details `axes` holds filtered gameplay values (circular sticks, triggers
/// remapped to `[0, 1]`). Raw backend samples live in `Gamepads::filters`.
struct Gamepad {
  std::string name;
  GamepadButtonInput buttons;
  Axis<GamepadAxis> axes;
  int32_t id = -1;
  bool connected = false;
};

/**
 * @brief Returns the string name of a gamepad button.
 * @param button Gamepad button
 * @return String name of the button
 */
[[nodiscard]] constexpr std::string_view ToString(
    GamepadButton button) noexcept {
  switch (button) {
    using enum GamepadButton;
    case kA:
      return "A";
    case kB:
      return "B";
    case kX:
      return "X";
    case kY:
      return "Y";
    case kLeftBumper:
      return "LeftBumper";
    case kRightBumper:
      return "RightBumper";
    case kBack:
      return "Back";
    case kStart:
      return "Start";
    case kGuide:
      return "Guide";
    case kLeftThumb:
      return "LeftThumb";
    case kRightThumb:
      return "RightThumb";
    case kDpadUp:
      return "DpadUp";
    case kDpadRight:
      return "DpadRight";
    case kDpadDown:
      return "DpadDown";
    case kDpadLeft:
      return "DpadLeft";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a gamepad button to an output stream.
 * @param os Output stream
 * @param button Gamepad button
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, GamepadButton button) {
  return os << "GamepadButton::" << ToString(button);
}

/**
 * @brief Returns the string name of a gamepad axis.
 * @param axis Gamepad axis
 * @return String name of the axis
 */
[[nodiscard]] constexpr std::string_view ToString(GamepadAxis axis) noexcept {
  switch (axis) {
    using enum GamepadAxis;
    case kLeftX:
      return "LeftX";
    case kLeftY:
      return "LeftY";
    case kRightX:
      return "RightX";
    case kRightY:
      return "RightY";
    case kLeftTrigger:
      return "LeftTrigger";
    case kRightTrigger:
      return "RightTrigger";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a gamepad axis to an output stream.
 * @param os Output stream
 * @param axis Gamepad axis
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, GamepadAxis axis) {
  return os << "GamepadAxis::" << ToString(axis);
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::GamepadButton> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::GamepadButton button,
                               format_context& ctx) {
    return format_to(ctx.out(), "GamepadButton::{}",
                     helios::input::ToString(button));
  }
};

template <>
struct formatter<helios::input::GamepadAxis> {
  static constexpr auto parse(std::format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::GamepadAxis axis,
                               format_context& ctx) {
    return format_to(ctx.out(), "GamepadAxis::{}",
                     helios::input::ToString(axis));
  }
};

}  // namespace std
