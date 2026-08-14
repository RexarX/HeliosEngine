#pragma once

#include <helios/ecs/resource/param.hpp>
#include <helios/input/params.hpp>
#include <helios/input/resources.hpp>

#include <string_view>

namespace helios::input {

/// @brief Clears per-frame edge state and mouse deltas at the start of a frame.
struct ClearInputState {
  static constexpr std::string_view kName = "helios::input::ClearInputState";

  /**
   * @brief Clears just-pressed/released edges and mouse delta/scroll.
   * @param state Mutable keyboard, mouse, and gamepad resources
   */
  void operator()(State state) const;
};

/// @brief Applies keyboard messages into the `Keyboard` resource.
struct UpdateKeyboardState {
  static constexpr std::string_view kName =
      "helios::input::UpdateKeyboardState";

  /**
   * @brief Drains keyboard messages into aggregated keyboard state.
   * @param keyboard Keyboard resource
   * @param messages Keyboard key and text readers
   */
  void operator()(ecs::Res<Keyboard> keyboard, KeyboardMessages messages) const;
};

/// @brief Applies mouse messages into the `Mouse` resource.
struct UpdateMouseState {
  static constexpr std::string_view kName = "helios::input::UpdateMouseState";

  /**
   * @brief Drains mouse messages into aggregated mouse state.
   * @param mouse Mouse resource
   * @param messages Mouse button, cursor, motion, and wheel readers
   */
  void operator()(ecs::Res<Mouse> mouse, MouseMessages messages) const;
};

/// @brief Applies gamepad messages into the `Gamepads` resource.
struct UpdateGamepadState {
  static constexpr std::string_view kName = "helios::input::UpdateGamepadState";

  /**
   * @brief Drains gamepad messages into aggregated gamepad state.
   * @details Axis messages are stored as raw backend samples. Filtered stick
   * and trigger values are written to `Gamepad::axes` using `Settings`.
   * @param gamepads Gamepads resource
   * @param settings Stick / trigger filters and auto-calibration
   * @param messages Gamepad connection, button, and axis readers
   */
  void operator()(ecs::Res<Gamepads> gamepads,
                  ecs::Res<const Settings> settings,
                  GamepadMessages messages) const;
};

}  // namespace helios::input
