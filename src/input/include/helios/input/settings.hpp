#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/memory/temporary_storage.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#endif
#include <helios/input/axis.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Global input behavior settings.
/// @details Stick and trigger `AxisFilter`s apply to `Gamepad::axes`. Axis
/// messages stay raw. `auto_calibrate` captures rest center on the first
/// near-rest sample and recenters after `rest_frames` at rest.
struct Settings {
  static constexpr std::string_view kName = "helios::input::Settings";
  static constexpr uint16_t kDefaultRestFrames = 60;

  AxisFilter stick{.deadzone = AxisFilter::kDefaultDeadzone,
                   .livezone = AxisFilter::kDefaultLivezone,
                   .rescale = true};
  AxisFilter trigger{.deadzone = AxisFilter::kDefaultTriggerDeadzone,
                     .livezone = AxisFilter::kDefaultLivezone,
                     .rescale = true};
  uint16_t rest_frames = kDefaultRestFrames;
  bool auto_calibrate = true;
  bool raw_mouse_motion = false;
};

/**
 * @brief Formats a input settings using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param settings Input settings
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Settings& settings) {
  out = std::format_to(out, "Settings{{stick=");
  out = ToString(out, settings.stick);
  out = std::format_to(out, ", trigger=");
  out = ToString(out, settings.trigger);
  return std::format_to(
      out, ", rest_frames={}, auto_calibrate={}, raw_mouse_motion={}}}",
      settings.rest_frames, settings.auto_calibrate, settings.raw_mouse_motion);
}

/**
 * @brief Formats a input settings as a string.
 * @param settings Input settings
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Settings& settings) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), settings);
  return result;
}

/**
 * @brief Formats a input settings as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Input settings
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Settings& settings) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), settings);
  return result;
}

/**
 * @brief Outputs a input settings to an output stream.
 * @param os Output stream
 * @param settings Input settings
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  ToString(std::ostreambuf_iterator<char>(os), settings);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::Settings> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Settings& settings,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), settings);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
