#pragma once

#include <helios/input/mouse.hpp>

#include <optional>
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

}  // namespace helios::input
