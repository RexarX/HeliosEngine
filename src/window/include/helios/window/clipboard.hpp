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

#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Process-global clipboard text snapshot.
struct Clipboard {
  static constexpr std::string_view kName = "helios::window::Clipboard";

  std::string text;
  /// When true, the backend writes `text` to the OS clipboard.
  bool pending_write = false;
};

/// @brief Sent when the OS clipboard text changes.
struct ClipboardChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ClipboardChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string text;
};

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

/**
 * @brief Formats `ClipboardChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ClipboardChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const ClipboardChangedMsg& msg) {
  return std::format_to(out, "ClipboardChangedMsg{{text=\"{}\"}}", msg.text);
}

/**
 * @brief Formats a `ClipboardChangedMsg` message as a string.
 * @param msg `ClipboardChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const ClipboardChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ClipboardChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ClipboardChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const ClipboardChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ClipboardChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ClipboardChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const ClipboardChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::window

HELIOS_MODULE_EXPORT
namespace std {

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

template <>
struct formatter<helios::window::ClipboardChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ClipboardChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
