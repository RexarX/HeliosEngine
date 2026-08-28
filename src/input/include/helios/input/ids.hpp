#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstdint>
#endif

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Backend keyboard instance identifier.
using KeyboardId = uint32_t;

/// @brief Backend mouse instance identifier.
using MouseId = uint32_t;

/// @brief Mapped gamepad slot identifier.
using GamepadId = uint32_t;

/// @brief Unmapped joystick slot identifier.
using JoystickId = uint32_t;

/// @brief Pen slot identifier.
using PenId = uint32_t;

/// @brief Touch-finger slot identifier.
using TouchId = uint32_t;

/// @brief Standalone sensor slot identifier.
using SensorId = uint32_t;

/// @brief Index into an IME candidate list.
using TextCandidateIndex = uint32_t;

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
