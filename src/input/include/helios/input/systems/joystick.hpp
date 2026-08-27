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
#include <helios/input/joystick.hpp>
#include <helios/input/params.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies unmapped joystick messages into the `Joysticks` resource.
struct UpdateJoystickState {
  static constexpr std::string_view kName =
      "helios::input::UpdateJoystickState";

  /**
   * @brief Drains joystick messages into aggregated joystick state.
   * @param joysticks Joysticks resource
   * @param messages Joystick connection, button, axis, and hat readers
   */
  void operator()(ecs::Res<Joysticks> joysticks,
                  JoystickMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
