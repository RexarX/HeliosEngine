#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/memory/temporary_storage.hpp>

#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#endif
#include <helios/input/mouse.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Per-window cursor presentation state.
struct Cursor {
  static constexpr std::string_view kName = "helios::input::Cursor";

  std::optional<CursorImage> custom;
  CursorIcon icon = CursorIcon::kDefault;
  bool dirty = false;

  /// @brief Clears any custom cursor image and marks the cursor dirty.
  constexpr void ClearCustom() noexcept;

  /**
   * @brief Sets the standard cursor icon and marks the cursor dirty.
   * @param value Cursor icon to apply
   */
  constexpr void SetIcon(CursorIcon value) noexcept;

  /**
   * @brief Sets a custom cursor image and marks the cursor dirty.
   * @param image Custom RGBA cursor image
   */
  constexpr void SetCustom(CursorImage image) noexcept;
};

constexpr void Cursor::SetIcon(CursorIcon value) noexcept {
  icon = value;
  dirty = true;
}

constexpr void Cursor::SetCustom(CursorImage image) noexcept {
  custom = std::move(image);
  dirty = true;
}

constexpr void Cursor::ClearCustom() noexcept {
  custom.reset();
  dirty = true;
}

/**
 * @brief Formats a cursor component using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param cursor Cursor component
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Cursor& cursor) {
  out = std::format_to(out, "Cursor{{icon={}", ToString(cursor.icon));
  out = std::format_to(out, ", dirty={}", cursor.dirty);
  if (cursor.custom.has_value()) {
    out = std::format_to(out, ", custom=");
    out = ToString(out, *cursor.custom);
  }
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a cursor component as a string.
 * @param cursor Cursor component
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Cursor& cursor) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), cursor);
  return result;
}

/**
 * @brief Formats a cursor component as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param cursor Cursor component
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Cursor& cursor) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), cursor);
  return result;
}

/**
 * @brief Outputs a cursor component to an output stream.
 * @param os Output stream
 * @param cursor Cursor component
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Cursor& cursor) {
  ToString(std::ostreambuf_iterator<char>(os), cursor);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::Cursor> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Cursor& cursor, format_context& ctx) {
    return helios::input::ToString(ctx.out(), cursor);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
