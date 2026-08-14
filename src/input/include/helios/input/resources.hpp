#pragma once

#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

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

using KeyboardInput = ButtonInput<Key>;

/// @brief Aggregated keyboard state for the main window / app.
struct Keyboard {
  static constexpr std::string_view kName = "helios::input::Keyboard";

  KeyboardInput keys;
  Modifiers modifiers = Modifiers::kNone;
};

using MouseButtonInput = ButtonInput<MouseButton>;

/// @brief Aggregated mouse state for the main window / app.
struct Mouse {
  static constexpr std::string_view kName = "helios::input::Mouse";

  MouseButtonInput buttons;
  double position_x = 0.0;
  double position_y = 0.0;
  double delta_x = 0.0;
  double delta_y = 0.0;
  double scroll_x = 0.0;
  double scroll_y = 0.0;
};

/// @brief Fixed gamepad slot table (GLFW joystick range 0..15).
/// @details `filters` is rest-center bookkeeping for `UpdateGamepadState`.
struct Gamepads {
  static constexpr std::string_view kName = "helios::input::Gamepads";
  static constexpr size_t kSlotCount = 16;

  std::array<Gamepad, kSlotCount> pads = {};
  std::array<GamepadAxisFilter, kSlotCount> filters = {};

  /**
   * @brief Looks up a mutable gamepad slot by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr Gamepad* TryGet(int32_t id) noexcept;

  /**
   * @brief Looks up a const gamepad slot by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Gamepad* TryGet(int32_t id) const noexcept;

  /**
   * @brief Looks up mutable axis-filter state by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the filter slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr GamepadAxisFilter* TryGetFilter(int32_t id) noexcept;

  /**
   * @brief Looks up const axis-filter state by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the filter slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const GamepadAxisFilter* TryGetFilter(
      int32_t id) const noexcept;
};

constexpr Gamepad* Gamepads::TryGet(int32_t id) noexcept {
  if (id < 0 || static_cast<size_t>(id) >= pads.size()) {
    return nullptr;
  }
  return &pads[static_cast<size_t>(id)];
}

constexpr const Gamepad* Gamepads::TryGet(int32_t id) const noexcept {
  if (id < 0 || static_cast<size_t>(id) >= pads.size()) {
    return nullptr;
  }
  return &pads[static_cast<size_t>(id)];
}

constexpr GamepadAxisFilter* Gamepads::TryGetFilter(int32_t id) noexcept {
  if (id < 0 || static_cast<size_t>(id) >= filters.size()) {
    return nullptr;
  }
  return &filters[static_cast<size_t>(id)];
}

constexpr const GamepadAxisFilter* Gamepads::TryGetFilter(
    int32_t id) const noexcept {
  if (id < 0 || static_cast<size_t>(id) >= filters.size()) {
    return nullptr;
  }
  return &filters[static_cast<size_t>(id)];
}

}  // namespace helios::input
