#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
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
#include <utility>
#endif

HELIOS_MODULE_EXPORT
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

}  // namespace helios::window

HELIOS_MODULE_EXPORT
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

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
