#pragma once

#include <helios/input/mouse.hpp>

#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

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
 * @param cursor Cursor component
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Cursor& cursor, It out) {
  out = std::format_to(out, "Cursor{{icon={}", ToString(cursor.icon));
  out = std::format_to(out, ", dirty={}", cursor.dirty);
  if (cursor.custom.has_value()) {
    out = std::format_to(out, ", custom=");
    out = ToString(*cursor.custom, out);
  }
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a cursor component as a string.
 * @param cursor Cursor component
 * @return Formatted cursor string
 */
[[nodiscard]] inline std::string ToString(const Cursor& cursor) {
  std::string result;
  result.reserve(128);
  ToString(cursor, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a cursor component to an output stream.
 * @param os Output stream
 * @param cursor Cursor component
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Cursor& cursor) {
  ToString(cursor, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::Cursor> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Cursor& cursor, format_context& ctx) {
    return helios::input::ToString(cursor, ctx.out());
  }
};

}  // namespace std
