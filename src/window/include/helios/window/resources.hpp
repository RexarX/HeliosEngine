#pragma once

#include <helios/memory/temporary_storage.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace helios::window {

/// @brief Conditions under which closing a window requests application exit.
enum class ExitTrigger : uint8_t {
  kNone = 0,
  kPrimaryClosed = 1U << 0U,     ///< A Primary-tagged window was destroyed.
  kAllWindowsClosed = 1U << 1U,  ///< The last native window was destroyed.
  kAnyWindowClosed = 1U << 2U,   ///< Any single window was destroyed.
};

/**
 * @brief Combines exit triggers.
 * @param lhs Left-hand side trigger flags
 * @param rhs Right-hand side trigger flags
 * @return Combined trigger flags
 */
[[nodiscard]] constexpr ExitTrigger operator|(ExitTrigger lhs,
                                              ExitTrigger rhs) noexcept {
  return static_cast<ExitTrigger>(std::to_underlying(lhs) |
                                  std::to_underlying(rhs));
}

/**
 * @brief Tests whether an exit trigger flag is set.
 * @param triggers Combined exit trigger flags
 * @param trigger Flag to test
 * @return True if the flag is set, false otherwise
 */
[[nodiscard]] constexpr bool HasFlag(ExitTrigger triggers,
                                     ExitTrigger trigger) noexcept {
  return (std::to_underlying(triggers) & std::to_underlying(trigger)) != 0U;
}

/// @brief Never auto-exit when windows close.
inline constexpr auto kExitTriggersNone = ExitTrigger::kNone;

/// @brief Exit when the last window is destroyed.
inline constexpr auto kExitTriggersLastWindow = ExitTrigger::kAllWindowsClosed;

/// @brief Exit when the primary window is destroyed.
inline constexpr auto kExitTriggersPrimary = ExitTrigger::kPrimaryClosed;

/// @brief Exit when the primary or last window is destroyed.
inline constexpr auto kExitTriggersDefault =
    ExitTrigger::kPrimaryClosed | ExitTrigger::kAllWindowsClosed;

/**
 * @brief Determines whether a destroyed window should request application exit.
 * @param triggers Configured exit triggers
 * @param was_primary True if the destroyed window had the Primary component
 * @param is_last_native_window True if no native windows remain after destroy
 * @return True when an exit trigger matched
 */
[[nodiscard]] constexpr bool ShouldRequestExitOnClose(
    ExitTrigger triggers, bool was_primary,
    bool is_last_native_window) noexcept {
  if (HasFlag(triggers, ExitTrigger::kAnyWindowClosed)) {
    return true;
  }
  if (HasFlag(triggers, ExitTrigger::kPrimaryClosed) && was_primary) {
    return true;
  }
  if (HasFlag(triggers, ExitTrigger::kAllWindowsClosed) &&
      is_last_native_window) {
    return true;
  }
  return false;
}

/// @brief How the window backend waits for OS events.
enum class EventMode : uint8_t {
  kPoll = 0,  ///< `glfwPollEvents` — return immediately (games).
  kWaitTimeout =
      1,  ///< `glfwWaitEventsTimeout` — idle up to `event_wait_timeout`.
};

/// @brief Global window behavior settings.
struct Settings {
  static constexpr std::string_view kName = "helios::window::Settings";
  static constexpr double kDefaultEventWaitTimeout = 1.0 / 60.0;

  /// Seconds to block when `event_mode == kWaitTimeout`. Must be finite and
  /// greater than zero. Also the max gamepad poll interval while waiting.
  double event_wait_timeout = kDefaultEventWaitTimeout;
  /// Bitmask of conditions that request application exit on window close.
  ExitTrigger exit_triggers = kExitTriggersDefault;
  /// How `PollEvents` waits for OS events. Default is poll (running sim).
  EventMode event_mode = EventMode::kPoll;
};

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
  int32_t index = 0;
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

/// @brief Process-global clipboard text snapshot.
struct Clipboard {
  static constexpr std::string_view kName = "helios::window::Clipboard";

  std::string text;
  /// When true, the backend writes `text` to the OS clipboard.
  bool pending_write = false;
};

/**
 * @brief Formats exit triggers as a pipe-separated list and writes to an output
 * iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param triggers Combined exit trigger flags
 * @param with_prefix Whether to include an "ExitTrigger::" prefix for each flag
 * @return Updated output iterator after writing the formatted string
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ExitTrigger triggers, bool with_prefix = false) {
  const std::string_view kNoneStr = with_prefix ? "ExitTrigger::None" : "None";

  if (triggers == ExitTrigger::kNone) {
    return std::format_to(out, "{}", kNoneStr);
  }

  bool first = true;
  const auto append = [triggers, &out, &first, with_prefix](
                          std::string_view name, ExitTrigger flag) {
    if (!HasFlag(triggers, flag)) {
      return;
    }
    if (!first) {
      out = std::format_to(out, " | ");
    }
    first = false;
    out = with_prefix ? std::format_to(out, "ExitTrigger::{}", name)
                      : std::format_to(out, "{}", name);
  };

  append("PrimaryClosed", ExitTrigger::kPrimaryClosed);
  append("AllWindowsClosed", ExitTrigger::kAllWindowsClosed);
  append("AnyWindowClosed", ExitTrigger::kAnyWindowClosed);
  return out;
}

/**
 * @brief Formats exit triggers as a pipe-separated list of flag names.
 * @param triggers Combined exit trigger flags
 * @param with_prefix Whether to include an "ExitTrigger::" prefix for each flag
 * @return Formatted flag names
 */
[[nodiscard]] inline std::string ToString(ExitTrigger triggers,
                                          bool with_prefix = false) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), triggers, with_prefix);
  return result;
}

/**
 * @brief Formats exit triggers as a pipe-separated list of flag names using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param triggers Combined exit trigger flags
 * @param with_prefix Whether to include a prefix for each flag
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ExitTrigger triggers,
                                                   bool with_prefix = false) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), triggers, with_prefix);
  return result;
}

/**
 * @brief Outputs exit triggers to an output stream.
 * @param os Output stream
 * @param triggers Combined exit trigger flags
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ExitTrigger triggers) {
  ToString(std::ostreambuf_iterator<char>(os), triggers);
  return os;
}

/**
 * @brief Returns the string name of an event wait mode.
 * @param mode Event wait mode
 * @return String name of the mode
 */
[[nodiscard]] constexpr std::string_view ToString(EventMode mode) noexcept {
  switch (mode) {
    using enum EventMode;
    case kPoll:
      return "Poll";
    case kWaitTimeout:
      return "WaitTimeout";
  }
  return "unknown";
}

/**
 * @brief Outputs an event wait mode to an output stream.
 * @param os Output stream
 * @param mode Event wait mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, EventMode mode) {
  return os << "EventMode::" << ToString(mode);
}

/**
 * @brief Formats window settings using an output iterator.
 * @param out Output iterator to write the formatted string to
 * @param settings Window settings
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Settings& settings) {
  out = std::format_to(out, "Settings{{exit_triggers=");
  out = ToString(out, settings.exit_triggers);
  out = std::format_to(out, ", event_mode={}", ToString(settings.event_mode));
  return std::format_to(out, ", event_wait_timeout={}}}",
                        settings.event_wait_timeout);
}

/**
 * @brief Formats window settings as a string.
 * @param settings Window settings
 * @return Formatted settings string
 */
[[nodiscard]] inline std::string ToString(const Settings& settings) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), settings);
  return result;
}

/**
 * @brief Formats window settings as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Settings
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Settings& settings) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), settings);
  return result;
}

/**
 * @brief Outputs window settings to an output stream.
 * @param os Output stream
 * @param settings Window settings
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  ToString(std::ostreambuf_iterator<char>(os), settings);
  return os;
}

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
 * @brief Formats a clipboard snapshot using an output iterator.
 * @param out Output iterator to write the formatted string to
 * @param clipboard Clipboard resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Clipboard& clipboard) {
  return std::format_to(out, "Clipboard{{text=\"{}\", pending_write={}}}",
                        clipboard.text, clipboard.pending_write);
}

/**
 * @brief Formats a clipboard snapshot as a string.
 * @param clipboard Clipboard resource
 * @return Formatted clipboard string
 */
[[nodiscard]] inline std::string ToString(const Clipboard& clipboard) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), clipboard);
  return result;
}

/**
 * @brief Formats a clipboard snapshot as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param clipboard Clipboard
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Clipboard& clipboard) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), clipboard);
  return result;
}

/**
 * @brief Outputs a clipboard snapshot to an output stream.
 * @param os Output stream
 * @param clipboard Clipboard resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Clipboard& clipboard) {
  ToString(std::ostreambuf_iterator<char>(os), clipboard);
  return os;
}

}  // namespace helios::window

namespace std {

template <>
struct formatter<helios::window::ExitTrigger> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(helios::window::ExitTrigger triggers,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), triggers, /*with_prefix=*/true);
  }
};

template <>
struct formatter<helios::window::EventMode> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::EventMode mode,
                               format_context& ctx) {
    return format_to(ctx.out(), "EventMode::{}",
                     helios::window::ToString(mode));
  }
};

template <>
struct formatter<helios::window::Settings> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Settings& settings,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), settings);
  }
};

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
struct formatter<helios::window::Clipboard> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Clipboard& clipboard,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), clipboard);
  }
};

}  // namespace std
