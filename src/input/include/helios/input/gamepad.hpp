#pragma once

#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>

#include <array>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

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
  kMisc1,
  kRightPaddle1,
  kLeftPaddle1,
  kRightPaddle2,
  kLeftPaddle2,
  kTouchpad,
  kMisc2,
  kMisc3,
  kMisc4,
  kMisc5,
  kMisc6,
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

/// @brief Gamepad battery / power snapshot.
enum class GamepadPowerState : uint8_t {
  kUnknown = 0,
  kOnBattery,
  kNoBattery,
  kCharging,
  kCharged,
  kWired,
};

/// @brief Battery state and charge percent (`-1` when unknown).
struct GamepadPower {
  GamepadPowerState state = GamepadPowerState::kUnknown;
  int8_t percent = -1;
};

/// @brief One touchpad finger sample (PlayStation-style pads).
struct GamepadTouchpadFinger {
  float x = 0.0F;
  float y = 0.0F;
  float pressure = 0.0F;
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

/// @brief Motion sensor kind reported by `GamepadSensorUpdateMsg`.
enum class GamepadSensor : uint8_t {
  kGyro = 0,
  kAccel,
  kCount,
};

/// @brief Bitmask of pending gamepad output changes for backend
/// synchronization.
enum class GamepadDirtyFlags : uint8_t {
  kNone = 0,
  kRumble = 1U << 0U,
  kTriggerRumble = 1U << 1U,
  kLed = 1U << 2U,
  kSensors = 1U << 3U,
};

/**
 * @brief Combines gamepad dirty flags.
 * @param lhs Left-hand side flag
 * @param rhs Right-hand side flag
 * @return Combined flag
 */
[[nodiscard]] constexpr GamepadDirtyFlags operator|(
    GamepadDirtyFlags lhs, GamepadDirtyFlags rhs) noexcept {
  return static_cast<GamepadDirtyFlags>(std::to_underlying(lhs) |
                                        std::to_underlying(rhs));
}

/**
 * @brief Tests whether a gamepad dirty flag is set.
 * @param flags Combined dirty flags
 * @param flag Flag to test
 * @return True if the flag is set, false otherwise
 */
[[nodiscard]] constexpr bool HasFlag(GamepadDirtyFlags flags,
                                     GamepadDirtyFlags flag) noexcept {
  return (std::to_underlying(flags) & std::to_underlying(flag)) != 0U;
}

/**
 * @brief Snapshot of one gamepad slot.
 * @details `axes` holds filtered gameplay values (circular sticks, triggers
 * remapped to `[0, 1]`). Raw backend samples live in `Gamepads::filters`.
 * Rumble, LED, and sensor-enable fields are gameplay outputs consumed by
 * backends. GLFW ignores rumble, LED, sensors, touchpad, and battery.
 */
struct Gamepad {
  static constexpr size_t kMaxTouchpadFingers = 2;

  /// @brief Clears identity, buttons, axes, and extra device state.
  constexpr void Reset() noexcept;

  /**
   * @brief Marks one or more outputs as pending backend synchronization.
   * @param flags Dirty flags to set
   */
  constexpr void MarkDirty(GamepadDirtyFlags flags) noexcept {
    dirty_flags = dirty_flags | flags;
  }

  /**
   * @brief Clears dirty flags after backend synchronization.
   * @param flags Dirty flags to clear
   */
  constexpr void ClearDirty(GamepadDirtyFlags flags) noexcept {
    dirty_flags = static_cast<GamepadDirtyFlags>(
        std::to_underlying(dirty_flags) & ~std::to_underlying(flags));
  }

  /// @brief Clears all dirty flags after backend synchronization.
  constexpr void ClearDirty() noexcept {
    dirty_flags = GamepadDirtyFlags::kNone;
  }

  /**
   * @brief Requests dual-motor rumble until `duration_ms` elapses.
   * @param low Low-frequency motor strength
   * @param high High-frequency motor strength
   * @param duration_ms Duration in milliseconds
   */
  constexpr void SetRumble(uint16_t low, uint16_t high,
                           uint32_t duration_ms) noexcept;

  /**
   * @brief Requests trigger rumble until `duration_ms` elapses.
   * @param left Left trigger motor strength
   * @param right Right trigger motor strength
   * @param duration_ms Duration in milliseconds
   */
  constexpr void SetTriggerRumble(uint16_t left, uint16_t right,
                                  uint32_t duration_ms) noexcept;

  /**
   * @brief Requests an LED color.
   * @param red Red channel
   * @param green Green channel
   * @param blue Blue channel
   */
  constexpr void SetLed(uint8_t red, uint8_t green, uint8_t blue) noexcept;

  /**
   * @brief Requests gyroscope sampling.
   * @param enabled Whether gyro reports should be enabled
   */
  constexpr void SetGyroEnabled(bool enabled) noexcept;

  /**
   * @brief Requests accelerometer sampling.
   * @param enabled Whether accelerometer reports should be enabled
   */
  constexpr void SetAccelEnabled(bool enabled) noexcept;

  /**
   * @brief Tests whether a dirty flag is set.
   * @param flag Dirty flag to test
   * @return True when the flag is set
   */
  [[nodiscard]] constexpr bool Dirty(GamepadDirtyFlags flag) const noexcept {
    return HasFlag(dirty_flags, flag);
  }

  /**
   * @brief Returns dual-motor rumble strengths.
   * @return Low-frequency and high-frequency motor strengths
   */
  [[nodiscard]] constexpr auto GetRumble() const noexcept
      -> std::pair<uint16_t, uint16_t> {
    return {rumble_low, rumble_high};
  }

  /**
   * @brief Returns trigger rumble strengths.
   * @return Left and right trigger motor strengths
   */
  [[nodiscard]] constexpr auto GetTriggerRumble() const noexcept
      -> std::pair<uint16_t, uint16_t> {
    return {trigger_rumble_left, trigger_rumble_right};
  }

  /**
   * @brief Returns the requested LED color.
   * @return Red, green, and blue channels
   */
  [[nodiscard]] constexpr auto GetLed() const noexcept
      -> std::array<uint8_t, 3> {
    return {led_r, led_g, led_b};
  }

  std::string name;
  std::string guid;
  std::string mapping;
  Axis<GamepadAxis> axes;
  std::array<float, 3> gyro = {};
  std::array<float, 3> accel = {};
  std::array<GamepadTouchpadFinger, kMaxTouchpadFingers> touchpad = {};
  GamepadButtonInput buttons;
  uint32_t rumble_duration_ms = 0;
  uint32_t trigger_rumble_duration_ms = 0;
  int32_t id = -1;
  uint16_t rumble_low = 0;
  uint16_t rumble_high = 0;
  uint16_t trigger_rumble_left = 0;
  uint16_t trigger_rumble_right = 0;
  GamepadPower power;
  uint8_t touchpad_count = 0;
  uint8_t led_r = 0;
  uint8_t led_g = 0;
  uint8_t led_b = 0;
  GamepadDirtyFlags dirty_flags = GamepadDirtyFlags::kNone;
  bool connected = false;
  bool gyro_enabled = false;
  bool accel_enabled = false;
};

constexpr void Gamepad::Reset() noexcept {
  name.clear();
  guid.clear();
  mapping.clear();
  buttons.Reset();
  axes.Clear();
  power = {};
  gyro.fill(0.0F);
  accel.fill(0.0F);
  touchpad = {};
  touchpad_count = 0;
  rumble_low = 0;
  rumble_high = 0;
  trigger_rumble_left = 0;
  trigger_rumble_right = 0;
  rumble_duration_ms = 0;
  trigger_rumble_duration_ms = 0;
  led_r = 0;
  led_g = 0;
  led_b = 0;
  id = -1;
  connected = false;
  gyro_enabled = false;
  accel_enabled = false;
  dirty_flags = GamepadDirtyFlags::kNone;
}

constexpr void Gamepad::SetRumble(uint16_t low, uint16_t high,
                                  uint32_t duration_ms) noexcept {
  rumble_low = low;
  rumble_high = high;
  rumble_duration_ms = duration_ms;
  MarkDirty(GamepadDirtyFlags::kRumble);
}

constexpr void Gamepad::SetTriggerRumble(uint16_t left, uint16_t right,
                                         uint32_t duration_ms) noexcept {
  trigger_rumble_left = left;
  trigger_rumble_right = right;
  trigger_rumble_duration_ms = duration_ms;
  MarkDirty(GamepadDirtyFlags::kTriggerRumble);
}

constexpr void Gamepad::SetLed(uint8_t red, uint8_t green,
                               uint8_t blue) noexcept {
  led_r = red;
  led_g = green;
  led_b = blue;
  MarkDirty(GamepadDirtyFlags::kLed);
}

constexpr void Gamepad::SetGyroEnabled(bool enabled) noexcept {
  gyro_enabled = enabled;
  MarkDirty(GamepadDirtyFlags::kSensors);
}

constexpr void Gamepad::SetAccelEnabled(bool enabled) noexcept {
  accel_enabled = enabled;
  MarkDirty(GamepadDirtyFlags::kSensors);
}

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
    case kMisc1:
      return "Misc1";
    case kRightPaddle1:
      return "RightPaddle1";
    case kLeftPaddle1:
      return "LeftPaddle1";
    case kRightPaddle2:
      return "RightPaddle2";
    case kLeftPaddle2:
      return "LeftPaddle2";
    case kTouchpad:
      return "Touchpad";
    case kMisc2:
      return "Misc2";
    case kMisc3:
      return "Misc3";
    case kMisc4:
      return "Misc4";
    case kMisc5:
      return "Misc5";
    case kMisc6:
      return "Misc6";
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

/**
 * @brief Returns the string name of a gamepad power state.
 * @param state Power state
 * @return String name of the state
 */
[[nodiscard]] constexpr std::string_view ToString(
    GamepadPowerState state) noexcept {
  switch (state) {
    using enum GamepadPowerState;
    case kUnknown:
      return "Unknown";
    case kOnBattery:
      return "OnBattery";
    case kNoBattery:
      return "NoBattery";
    case kCharging:
      return "Charging";
    case kCharged:
      return "Charged";
    case kWired:
      return "Wired";
  }
  return "unknown";
}

/**
 * @brief Outputs a gamepad power state to an output stream.
 * @param os Output stream
 * @param state Power state
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, GamepadPowerState state) {
  return os << "GamepadPowerState::" << ToString(state);
}

/**
 * @brief Formats a gamepad power snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param power Power snapshot
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadPower& power, It out) {
  return std::format_to(out, "GamepadPower{{state={}, percent={}}}",
                        ToString(power.state), power.percent);
}

/**
 * @brief Formats a gamepad power snapshot as a string.
 * @param power Power snapshot
 * @return Formatted power string
 */
[[nodiscard]] inline std::string ToString(const GamepadPower& power) {
  std::string result;
  result.reserve(64);
  ToString(power, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a gamepad power snapshot to an output stream.
 * @param os Output stream
 * @param power Power snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const GamepadPower& power) {
  ToString(power, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a touchpad finger sample using an output iterator.
 * @tparam It Output iterator type
 * @param finger Touchpad finger sample
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadTouchpadFinger& finger, It out) {
  return std::format_to(
      out, "GamepadTouchpadFinger{{x={}, y={}, pressure={}, down={}}}",
      finger.x, finger.y, finger.pressure, finger.down);
}

/**
 * @brief Formats a touchpad finger sample as a string.
 * @param finger Touchpad finger sample
 * @return Formatted finger string
 */
[[nodiscard]] inline std::string ToString(const GamepadTouchpadFinger& finger) {
  std::string result;
  result.reserve(128);
  ToString(finger, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a touchpad finger sample to an output stream.
 * @param os Output stream
 * @param finger Touchpad finger sample
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadTouchpadFinger& finger) {
  ToString(finger, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Returns the string name of a gamepad sensor.
 * @param sensor Sensor kind
 * @return String name of the sensor
 */
[[nodiscard]] constexpr std::string_view ToString(
    GamepadSensor sensor) noexcept {
  switch (sensor) {
    using enum GamepadSensor;
    case kGyro:
      return "Gyro";
    case kAccel:
      return "Accel";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a gamepad sensor to an output stream.
 * @param os Output stream
 * @param sensor Sensor kind
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, GamepadSensor sensor) {
  return os << "GamepadSensor::" << ToString(sensor);
}

/**
 * @brief Formats gamepad dirty flags as a pipe-separated list and writes to an
 * output iterator.
 * @tparam It Output iterator type
 * @param flags Combined dirty flags
 * @param out Output iterator to write the formatted string to
 * @param with_prefix Whether to include a "GamepadDirtyFlags::" prefix
 * @return Updated output iterator after writing the formatted string
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(GamepadDirtyFlags flags, It out, bool with_prefix = false) {
  const std::string_view kNoneStr =
      with_prefix ? "GamepadDirtyFlags::None" : "None";

  if (flags == GamepadDirtyFlags::kNone) {
    return std::format_to(out, "{}", kNoneStr);
  }

  bool first = true;
  const auto append = [flags, &out, &first, with_prefix](
                          std::string_view name, GamepadDirtyFlags flag) {
    if (!HasFlag(flags, flag)) {
      return;
    }
    if (!first) {
      out = std::format_to(out, " | ");
    }
    first = false;
    out = with_prefix ? std::format_to(out, "GamepadDirtyFlags::{}", name)
                      : std::format_to(out, "{}", name);
  };

  append("Rumble", GamepadDirtyFlags::kRumble);
  append("TriggerRumble", GamepadDirtyFlags::kTriggerRumble);
  append("Led", GamepadDirtyFlags::kLed);
  append("Sensors", GamepadDirtyFlags::kSensors);
  return out;
}

/**
 * @brief Formats gamepad dirty flags as a pipe-separated list of flag names.
 * @param flags Combined dirty flags
 * @param with_prefix Whether to include a "GamepadDirtyFlags::" prefix
 * @return Formatted flag names
 */
[[nodiscard]] inline std::string ToString(GamepadDirtyFlags flags,
                                          bool with_prefix = false) {
  std::string result;
  result.reserve(64);
  ToString(flags, std::back_inserter(result), with_prefix);
  return result;
}

/**
 * @brief Outputs gamepad dirty flags to an output stream.
 * @param os Output stream
 * @param flags Combined dirty flags
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, GamepadDirtyFlags flags) {
  ToString(flags, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats a gamepad snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param pad Gamepad snapshot
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Gamepad& pad, It out) {
  out = std::format_to(out,
                       "Gamepad{{id={}, name=\"{}\", guid=\"{}\", connected={}",
                       pad.id, pad.name, pad.guid, pad.connected);
  out = std::format_to(out, ", power=");
  out = ToString(pad.power, out);
  out = std::format_to(out, ", dirty_flags=");
  out = ToString(pad.dirty_flags, out);
  return std::format_to(
      out, ", gyro_enabled={}, accel_enabled={}, touchpad_count={}}}",
      pad.gyro_enabled, pad.accel_enabled, pad.touchpad_count);
}

/**
 * @brief Formats a gamepad snapshot as a string.
 * @param pad Gamepad snapshot
 * @return Formatted gamepad string
 */
[[nodiscard]] inline std::string ToString(const Gamepad& pad) {
  std::string result;
  result.reserve(128);
  ToString(pad, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a gamepad snapshot to an output stream.
 * @param os Output stream
 * @param pad Gamepad snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Gamepad& pad) {
  ToString(pad, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::GamepadButton> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
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
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::GamepadAxis axis,
                               format_context& ctx) {
    return format_to(ctx.out(), "GamepadAxis::{}",
                     helios::input::ToString(axis));
  }
};

template <>
struct formatter<helios::input::GamepadPowerState> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::GamepadPowerState state,
                               format_context& ctx) {
    return format_to(ctx.out(), "GamepadPowerState::{}",
                     helios::input::ToString(state));
  }
};

template <>
struct formatter<helios::input::GamepadPower> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadPower& power,
                     format_context& ctx) {
    return helios::input::ToString(power, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadTouchpadFinger> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadTouchpadFinger& finger,
                     format_context& ctx) {
    return helios::input::ToString(finger, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadSensor> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::GamepadSensor sensor,
                               format_context& ctx) {
    return format_to(ctx.out(), "GamepadSensor::{}",
                     helios::input::ToString(sensor));
  }
};

template <>
struct formatter<helios::input::GamepadDirtyFlags> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(helios::input::GamepadDirtyFlags flags,
                     format_context& ctx) {
    return helios::input::ToString(flags, ctx.out(), /*with_prefix=*/true);
  }
};

template <>
struct formatter<helios::input::Gamepad> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Gamepad& pad, format_context& ctx) {
    return helios::input::ToString(pad, ctx.out());
  }
};

}  // namespace std
