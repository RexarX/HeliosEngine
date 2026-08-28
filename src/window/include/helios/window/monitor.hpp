#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/message/message.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#endif
#include <helios/window/ids.hpp>

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Monitor connection event reported by the platform backend.
enum class MonitorEvent : uint8_t {
  kConnected = 1,     ///< A monitor was connected.
  kDisconnected = 2,  ///< A monitor was disconnected.
};

/// @brief A display video mode (resolution and refresh rate).
struct VideoMode {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t refresh_rate = 0;

  /**
   * @brief Returns the video-mode size.
   * @return Width and height
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/// @brief Snapshot of a connected display.
struct Monitor {
  std::string name;
  std::vector<VideoMode> modes;
  MonitorId index = 0;
  int32_t x = 0;
  int32_t y = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  int32_t work_x = 0;
  int32_t work_y = 0;
  uint32_t work_width = 0;
  uint32_t work_height = 0;
  int32_t physical_width_mm = 0;
  int32_t physical_height_mm = 0;
  VideoMode current;
  bool primary = false;

  /**
   * @brief Returns the monitor origin.
   * @return X and y in screen coordinates
   */
  [[nodiscard]] constexpr auto GetPos() const noexcept
      -> std::pair<int32_t, int32_t> {
    return {x, y};
  }

  /**
   * @brief Returns the monitor size.
   * @return Width and height in pixels
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }

  /**
   * @brief Returns the work-area origin.
   * @return X and y in screen coordinates
   */
  [[nodiscard]] constexpr auto GetWorkPos() const noexcept
      -> std::pair<int32_t, int32_t> {
    return {work_x, work_y};
  }

  /**
   * @brief Returns the work-area size.
   * @return Width and height in pixels
   */
  [[nodiscard]] constexpr auto GetWorkSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {work_width, work_height};
  }

  /**
   * @brief Returns the physical panel size.
   * @return Width and height in millimetres
   */
  [[nodiscard]] constexpr auto GetPhysicalSize() const noexcept
      -> std::pair<int32_t, int32_t> {
    return {physical_width_mm, physical_height_mm};
  }
};

/// @brief Current connected monitor layout.
struct Monitors {
  static constexpr std::string_view kName = "helios::window::Monitors";

  std::vector<Monitor> monitors;
};

/// @brief Sent when the connected monitor layout changes.
struct MonitorConnectedMsg {
  static constexpr std::string_view kName =
      "helios::window::MonitorConnectedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  MonitorId index = 0;
};

/// @brief Sent when the connected monitor layout changes.
struct MonitorDisconnectedMsg {
  static constexpr std::string_view kName =
      "helios::window::MonitorDisconnectedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  MonitorId index = 0;
};

/**
 * @brief Returns the string name of a monitor event.
 * @param event Monitor event
 * @return String name of the event
 */
[[nodiscard]] constexpr std::string_view ToString(MonitorEvent event) noexcept {
  switch (event) {
    using enum MonitorEvent;
    case kConnected:
      return "Connected";
    case kDisconnected:
      return "Disconnected";
  }
  return "unknown";
}

/**
 * @brief Outputs a monitor event to an output stream.
 * @param os Output stream
 * @param event Monitor event
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MonitorEvent event) {
  return os << "MonitorEvent::" << ToString(event);
}

/**
 * @brief Formats a video mode using an output iterator.
 * @param out Output iterator to write the formatted string to
 * @param mode Video mode
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const VideoMode& mode) {
  return std::format_to(out,
                        "VideoMode{{width={}, height={}, refresh_rate={}}}",
                        mode.width, mode.height, mode.refresh_rate);
}

/**
 * @brief Formats a video mode as a string.
 * @param mode Video mode
 * @return Formatted video mode string
 */
[[nodiscard]] inline std::string ToString(const VideoMode& mode) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), mode);
  return result;
}

/**
 * @brief Formats a video mode as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param mode VideoMode
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const VideoMode& mode) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), mode);
  return result;
}

/**
 * @brief Outputs a video mode to an output stream.
 * @param os Output stream
 * @param mode Video mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const VideoMode& mode) {
  ToString(std::ostreambuf_iterator<char>(os), mode);
  return os;
}

/**
 * @brief Formats a monitor using an output iterator.
 * @param out Output iterator to write the formatted string to
 * @param monitor Monitor snapshot
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Monitor& monitor) {
  out = std::format_to(out, "Monitor{{");
  out = std::format_to(out, "name=\"{}\"", monitor.name);
  out = std::format_to(out, ", modes={}", monitor.modes.size());
  out = std::format_to(out, ", index={}", monitor.index);
  out = std::format_to(out, ", x={}, y={}", monitor.x, monitor.y);
  out = std::format_to(out, ", width={}, height={}", monitor.width,
                       monitor.height);
  out = std::format_to(out, ", work_x={}, work_y={}", monitor.work_x,
                       monitor.work_y);
  out = std::format_to(out, ", work_width={}, work_height={}",
                       monitor.work_width, monitor.work_height);
  out =
      std::format_to(out, ", physical_width_mm={}", monitor.physical_width_mm);
  out = std::format_to(out, ", physical_height_mm={}",
                       monitor.physical_height_mm);
  out = std::format_to(out, ", current=");
  out = ToString(out, monitor.current);
  out = std::format_to(out, ", primary={}", monitor.primary);
  out = std::format_to(out, "}}");
  return out;
}

/**
 * @brief Formats a monitor as a string.
 * @param monitor Monitor snapshot
 * @return Formatted monitor string
 */
[[nodiscard]] inline std::string ToString(const Monitor& monitor) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), monitor);
  return result;
}

/**
 * @brief Formats a monitor as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param monitor Monitor
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Monitor& monitor) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), monitor);
  return result;
}

/**
 * @brief Outputs a monitor to an output stream.
 * @param os Output stream
 * @param monitor Monitor snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Monitor& monitor) {
  ToString(std::ostreambuf_iterator<char>(os), monitor);
  return os;
}

/**
 * @brief Formats the connected monitor list using an output iterator.
 * @param out Output iterator to write the formatted string to
 * @param monitors Monitors resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Monitors& monitors) {
  out = std::format_to(out, "Monitors{{count={}", monitors.monitors.size());
  if (!monitors.monitors.empty()) {
    out = std::format_to(out, ", names=[");
    bool first = true;
    for (const Monitor& monitor : monitors.monitors) {
      if (!first) {
        out = std::format_to(out, ", ");
      }
      first = false;
      out = std::format_to(out, "\"{}\"", monitor.name);
    }
    out = std::format_to(out, "]");
  }
  return std::format_to(out, "}}");
}

/**
 * @brief Formats the connected monitor list as a string.
 * @param monitors Monitors resource
 * @return Formatted monitors string
 */
[[nodiscard]] inline std::string ToString(const Monitors& monitors) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), monitors);
  return result;
}

/**
 * @brief Formats the connected monitor list as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param monitors Monitors
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Monitors& monitors) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), monitors);
  return result;
}

/**
 * @brief Outputs the connected monitor list to an output stream.
 * @param os Output stream
 * @param monitors Monitors resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Monitors& monitors) {
  ToString(std::ostreambuf_iterator<char>(os), monitors);
  return os;
}

/**
 * @brief Formats `MonitorConnectedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MonitorConnectedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MonitorConnectedMsg msg) {
  return std::format_to(out, "MonitorConnectedMsg{{index={}}}", msg.index);
}

/**
 * @brief Formats a `MonitorConnectedMsg` message as a string.
 * @param msg `MonitorConnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MonitorConnectedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MonitorConnectedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MonitorConnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(MonitorConnectedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MonitorConnectedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MonitorConnectedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MonitorConnectedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MonitorDisconnectedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MonitorDisconnectedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MonitorDisconnectedMsg msg) {
  return std::format_to(out, "MonitorConnectedMsg{{index={}}}", msg.index);
}

/**
 * @brief Formats a `MonitorDisconnectedMsg` message as a string.
 * @param msg `MonitorDisconnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MonitorDisconnectedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MonitorDisconnectedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MonitorDisconnectedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(MonitorDisconnectedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MonitorDisconnectedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MonitorDisconnectedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MonitorDisconnectedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::window

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::window::MonitorEvent> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::MonitorEvent event,
                               format_context& ctx) {
    return format_to(ctx.out(), "MonitorEvent::{}",
                     helios::window::ToString(event));
  }
};

template <>
struct formatter<helios::window::VideoMode> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::VideoMode& mode,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), mode);
  }
};

template <>
struct formatter<helios::window::Monitor> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Monitor& monitor,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), monitor);
  }
};

template <>
struct formatter<helios::window::Monitors> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Monitors& monitors,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), monitors);
  }
};

template <>
struct formatter<helios::window::MonitorConnectedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MonitorConnectedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MonitorDisconnectedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MonitorDisconnectedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
