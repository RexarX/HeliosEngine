#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>

#include <string_view>
#endif
#include <helios/input/gamepad.hpp>
#include <helios/input/params.hpp>
#include <helios/input/settings.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies gamepad messages into the `Gamepads` resource.
struct UpdateGamepadState {
  static constexpr std::string_view kName = "helios::input::UpdateGamepadState";

  /**
   * @brief Drains gamepad messages into aggregated gamepad state.
   * @details Axis messages are stored as raw backend samples. Filtered stick
   * and trigger values are written to `Gamepad::axes` using `Settings`. Remap,
   * power, sensor, and touchpad messages update extra device fields.
   * @param gamepads Gamepads resource
   * @param settings Stick / trigger filters and auto-calibration
   * @param messages Gamepad connection, button, axis, and extra readers
   */
  void operator()(ecs::Res<Gamepads> gamepads,
                  ecs::Res<const Settings> settings,
                  GamepadMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
