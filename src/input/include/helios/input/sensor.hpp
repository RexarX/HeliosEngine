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
#include <helios/input/ids.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Standalone motion-sensor kind (not attached to a gamepad slot).
enum class SensorType : uint8_t {
  kUnknown = 0,
  kAccel,
  kGyro,
  kAccelLeft,
  kGyroLeft,
  kAccelRight,
  kGyroRight,
};

/// @brief Snapshot of one standalone sensor slot.
struct Sensor {
  /// @brief Clears identity, type, and sample.
  void Reset() noexcept;

  std::string name;
  std::array<float, 3> value = {};
  std::optional<SensorId> id;
  SensorType type = SensorType::kUnknown;
  bool connected = false;
};

inline void Sensor::Reset() noexcept {
  name.clear();
  value = {};
  id.reset();
  type = SensorType::kUnknown;
  connected = false;
}

/// @brief Fixed standalone-sensor slot table (independent of gamepad ids).
struct Sensors {
  static constexpr std::string_view kName = "helios::input::Sensors";
  static constexpr size_t kSlotCount = 8;

  std::array<Sensor, kSlotCount> devices = {};

  /**
   * @brief Looks up a mutable sensor slot by id.
   * @param id Sensor slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr Sensor* TryGet(SensorId id) noexcept;

  /**
   * @brief Looks up a const sensor slot by id.
   * @param id Sensor slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Sensor* TryGet(SensorId id) const noexcept;
};

constexpr Sensor* Sensors::TryGet(SensorId id) noexcept {
  if (static_cast<size_t>(id) >= devices.size()) {
    return nullptr;
  }
  return &devices[static_cast<size_t>(id)];
}

constexpr const Sensor* Sensors::TryGet(SensorId id) const noexcept {
  if (static_cast<size_t>(id) >= devices.size()) {
    return nullptr;
  }
  return &devices[static_cast<size_t>(id)];
}

/// @brief Standalone sensor connect / disconnect notification.
struct SensorConnectionMsg {
  static constexpr std::string_view kName =
      "helios::input::SensorConnectionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string name;
  SensorId id = 0;
  SensorType type = SensorType::kUnknown;
  bool connected = false;
};

/// @brief Latest sample for a standalone sensor slot.
struct SensorUpdateMsg {
  static constexpr std::string_view kName = "helios::input::SensorUpdateMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::array<float, 3> value = {};
  SensorId id = 0;
  SensorType type = SensorType::kUnknown;
};

/**
 * @brief Returns the string name of a sensor type.
 * @param type Sensor type
 * @return String name of the type
 */
[[nodiscard]] constexpr std::string_view ToString(SensorType type) noexcept {
  switch (type) {
    using enum SensorType;
    case kUnknown:
      return "Unknown";
    case kAccel:
      return "Accel";
    case kGyro:
      return "Gyro";
    case kAccelLeft:
      return "AccelLeft";
    case kGyroLeft:
      return "GyroLeft";
    case kAccelRight:
      return "AccelRight";
    case kGyroRight:
      return "GyroRight";
  }
  return "unknown";
}

/**
 * @brief Outputs a sensor type to an output stream.
 * @param os Output stream
 * @param type Sensor type
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, SensorType type) {
  return os << "SensorType::" << ToString(type);
}

/**
 * @brief Formats a sensor snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param sensor Sensor snapshot
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Sensor& sensor) {
  out = std::format_to(
      out, "Sensor{{name=\"{}\", value=({}, {}, {}), id=", sensor.name,
      sensor.value[0], sensor.value[1], sensor.value[2]);
  if (sensor.id.has_value()) {
    out = std::format_to(out, "{}", *sensor.id);
  } else {
    out = std::format_to(out, "none");
  }
  return std::format_to(out, ", type={}, connected={}}}", ToString(sensor.type),
                        sensor.connected);
}

/**
 * @brief Formats a sensor snapshot as a string.
 * @param sensor Sensor snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Sensor& sensor) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), sensor);
  return result;
}

/**
 * @brief Formats a sensor snapshot as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param sensor Sensor snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Sensor& sensor) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), sensor);
  return result;
}

/**
 * @brief Outputs a sensor snapshot to an output stream.
 * @param os Output stream
 * @param sensor Sensor snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Sensor& sensor) {
  ToString(std::ostreambuf_iterator<char>(os), sensor);
  return os;
}

/**
 * @brief Formats a sensor slot table using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param sensors Sensors resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Sensors& sensors) {
  out = std::format_to(out, "Sensors{{devices = [");

  size_t cnt = 0;
  for (const auto& sensor : sensors.devices) {
    if (!sensor.connected) {
      continue;
    }

    if (cnt++ > 0) [[likely]] {
      out = std::format_to(out, ", ");
    }
    ToString(out, sensor);
  };

  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a sensor slot table as a string.
 * @param sensors Sensors resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Sensors& sensors) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), sensors);
  return result;
}

/**
 * @brief Formats a sensor slot table as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param sensors Sensors resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Sensors& sensors) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), sensors);
  return result;
}

/**
 * @brief Outputs a sensor slot table to an output stream.
 * @param os Output stream
 * @param sensors Sensors resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Sensors& sensors) {
  ToString(std::ostreambuf_iterator<char>(os), sensors);
  return os;
}

/**
 * @brief Formats a sensor connection message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Sensor connection message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const SensorConnectionMsg& msg) {
  return std::format_to(
      out, "SensorConnectionMsg{{name=\"{}\", id={}, type={}, connected={}}}",
      msg.name, msg.id, ToString(msg.type), msg.connected);
}

/**
 * @brief Formats a sensor connection message as a string.
 * @param msg Sensor connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const SensorConnectionMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a sensor connection message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg Sensor connection message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const SensorConnectionMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a sensor connection message to an output stream.
 * @param os Output stream
 * @param msg Sensor connection message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const SensorConnectionMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a sensor update message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Sensor update message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const SensorUpdateMsg& msg) {
  return std::format_to(
      out, "SensorUpdateMsg{{value=({}, {}, {}), id={}, type={}}}",
      msg.value[0], msg.value[1], msg.value[2], msg.id, ToString(msg.type));
}

/**
 * @brief Formats a sensor update message as a string.
 * @param msg Sensor update message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const SensorUpdateMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a sensor update message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg Sensor update message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const SensorUpdateMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a sensor update message to an output stream.
 * @param os Output stream
 * @param msg Sensor update message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const SensorUpdateMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::SensorType> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::SensorType type,
                               format_context& ctx) {
    return format_to(ctx.out(), "SensorType::{}",
                     helios::input::ToString(type));
  }
};

template <>
struct formatter<helios::input::Sensor> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Sensor& sensor, format_context& ctx) {
    return helios::input::ToString(ctx.out(), sensor);
  }
};

template <>
struct formatter<helios::input::Sensors> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Sensors& sensors,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), sensors);
  }
};

template <>
struct formatter<helios::input::SensorConnectionMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::SensorConnectionMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::SensorUpdateMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::SensorUpdateMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
