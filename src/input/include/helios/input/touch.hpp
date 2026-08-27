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
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#endif
#include <helios/input/ids.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Lifetime of one finger contact on a touch device.
enum class TouchPhase : uint8_t {
  kStarted = 0,
  kMoved,
  kEnded,
  kCanceled,
};

/// @brief Whether a touch device is a display or an indirect pad.
enum class TouchDeviceType : uint8_t {
  kUnknown = 0,
  kDirect,
  kIndirectAbsolute,
  kIndirectRelative,
};

/// @brief Snapshot of one finger slot.
struct TouchFinger {
  /// @brief Clears identity, position, and contact state.
  void Reset() noexcept;

  double position_x = 0.0;
  double position_y = 0.0;
  double delta_x = 0.0;
  double delta_y = 0.0;
  float pressure = 0.0F;
  std::optional<TouchId> id;
  TouchDeviceType device_type = TouchDeviceType::kUnknown;
  bool down = false;
  bool has_position = false;

  /**
   * @brief Returns the finger position.
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

inline void TouchFinger::Reset() noexcept {
  position_x = 0.0;
  position_y = 0.0;
  delta_x = 0.0;
  delta_y = 0.0;
  pressure = 0.0F;
  id.reset();
  device_type = TouchDeviceType::kUnknown;
  down = false;
  has_position = false;
}

/// @brief Fixed touch-finger slot table (independent of gamepad / joystick
/// ids).
struct Touches {
  static constexpr std::string_view kName = "helios::input::Touches";
  static constexpr size_t kSlotCount = 16;

  std::array<TouchFinger, kSlotCount> fingers = {};

  /**
   * @brief Looks up a mutable finger slot by id.
   * @param id Finger slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr TouchFinger* TryGet(TouchId id) noexcept;

  /**
   * @brief Looks up a const finger slot by id.
   * @param id Finger slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const TouchFinger* TryGet(TouchId id) const noexcept;
};

constexpr TouchFinger* Touches::TryGet(TouchId id) noexcept {
  if (static_cast<size_t>(id) >= fingers.size()) {
    return nullptr;
  }
  return &fingers[static_cast<size_t>(id)];
}

constexpr const TouchFinger* Touches::TryGet(TouchId id) const noexcept {
  if (static_cast<size_t>(id) >= fingers.size()) {
    return nullptr;
  }
  return &fingers[static_cast<size_t>(id)];
}

/// @brief Touch finger started / moved / ended / canceled for a window entity.
struct TouchInputMsg {
  static constexpr std::string_view kName = "helios::input::TouchInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  double dx = 0.0;
  double dy = 0.0;
  float pressure = 0.0F;
  TouchId id = 0;
  TouchPhase phase = TouchPhase::kStarted;
  TouchDeviceType device_type = TouchDeviceType::kUnknown;

  /**
   * @brief Returns the finger position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }

  /**
   * @brief Returns the motion delta.
   * @return Delta x and y
   */
  [[nodiscard]] constexpr auto GetDelta() const noexcept
      -> std::pair<double, double> {
    return {dx, dy};
  }
};

/**
 * @brief Returns the string name of a touch phase.
 * @param phase Touch phase
 * @return String name of the phase
 */
[[nodiscard]] constexpr std::string_view ToString(TouchPhase phase) noexcept {
  switch (phase) {
    using enum TouchPhase;
    case kStarted:
      return "Started";
    case kMoved:
      return "Moved";
    case kEnded:
      return "Ended";
    case kCanceled:
      return "Canceled";
  }
  return "unknown";
}

/**
 * @brief Outputs a touch phase to an output stream.
 * @param os Output stream
 * @param phase Touch phase
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, TouchPhase phase) {
  return os << "TouchPhase::" << ToString(phase);
}

/**
 * @brief Returns the string name of a touch device type.
 * @param type Touch device type
 * @return String name of the type
 */
[[nodiscard]] constexpr std::string_view ToString(
    TouchDeviceType type) noexcept {
  switch (type) {
    using enum TouchDeviceType;
    case kUnknown:
      return "Unknown";
    case kDirect:
      return "Direct";
    case kIndirectAbsolute:
      return "IndirectAbsolute";
    case kIndirectRelative:
      return "IndirectRelative";
  }
  return "unknown";
}

/**
 * @brief Outputs a touch device type to an output stream.
 * @param os Output stream
 * @param type Touch device type
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, TouchDeviceType type) {
  return os << "TouchDeviceType::" << ToString(type);
}

/**
 * @brief Formats a touch finger snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param finger Touch finger snapshot
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const TouchFinger& finger) {
  out = std::format_to(
      out, "TouchFinger{{position=({}, {}), delta=({}, {}), pressure={}, id=",
      finger.position_x, finger.position_y, finger.delta_x, finger.delta_y,
      finger.pressure);
  if (finger.id.has_value()) {
    out = std::format_to(out, "{}", *finger.id);
  } else {
    out = std::format_to(out, "none");
  }
  return std::format_to(out, ", device_type={}, down={}}}",
                        ToString(finger.device_type), finger.down);
}

/**
 * @brief Formats a touch finger snapshot as a string.
 * @param finger Touch finger snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const TouchFinger& finger) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), finger);
  return result;
}

/**
 * @brief Formats a touch finger snapshot as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param finger Touch finger snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const TouchFinger& finger) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), finger);
  return result;
}

/**
 * @brief Outputs a touch finger snapshot to an output stream.
 * @param os Output stream
 * @param finger Touch finger snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const TouchFinger& finger) {
  ToString(std::ostreambuf_iterator<char>(os), finger);
  return os;
}

/**
 * @brief Formats a touch slot table using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param touches Touches resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Touches& touches) {
  out = std::format_to(out, "Touches{{fingers = [");

  size_t cnt = 0;
  for (const auto& finger : touches.fingers) {
    if (!finger.down) {
      continue;
    }

    if (cnt++ > 0) [[likely]] {
      out = std::format_to(out, ", ");
    }
    ToString(out, finger);
  };

  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a touch slot table as a string.
 * @param touches Touches resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Touches& touches) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), touches);
  return result;
}

/**
 * @brief Formats a touch slot table as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param touches Touches resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Touches& touches) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), touches);
  return result;
}

/**
 * @brief Outputs a touch slot table to an output stream.
 * @param os Output stream
 * @param touches Touches resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Touches& touches) {
  ToString(std::ostreambuf_iterator<char>(os), touches);
  return os;
}

/**
 * @brief Formats a touch input message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Touch input message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const TouchInputMsg& msg) {
  return std::format_to(
      out,
      "TouchInputMsg{{entity={}, x={}, y={}, dx={}, dy={}, pressure={}, "
      "id={}, phase={}, device_type={}}}",
      msg.entity, msg.x, msg.y, msg.dx, msg.dy, msg.pressure, msg.id,
      ToString(msg.phase), ToString(msg.device_type));
}

/**
 * @brief Formats a touch input message as a string.
 * @param msg Touch input message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const TouchInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a touch input message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg Touch input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const TouchInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a touch input message to an output stream.
 * @param os Output stream
 * @param msg Touch input message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const TouchInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::TouchPhase> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::TouchPhase phase,
                               format_context& ctx) {
    return format_to(ctx.out(), "TouchPhase::{}",
                     helios::input::ToString(phase));
  }
};

template <>
struct formatter<helios::input::TouchDeviceType> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::TouchDeviceType type,
                               format_context& ctx) {
    return format_to(ctx.out(), "TouchDeviceType::{}",
                     helios::input::ToString(type));
  }
};

template <>
struct formatter<helios::input::TouchFinger> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::TouchFinger& finger,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), finger);
  }
};

template <>
struct formatter<helios::input::Touches> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Touches& touches,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), touches);
  }
};

template <>
struct formatter<helios::input::TouchInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::TouchInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
