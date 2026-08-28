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
#include <helios/input/params.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Clears per-frame edge state and mouse deltas at the start of a frame.
struct ClearInputState {
  static constexpr std::string_view kName = "helios::input::ClearInputState";

  /**
   * @brief Clears just-pressed/released edges and mouse / touch deltas.
   * @param state Mutable keyboard, mouse, gamepad, joystick, pen, touch, and
   * sensor resources
   */
  void operator()(State state) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
