#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/ids.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/plugin.hpp>
#include <helios/input/sensor.hpp>
#include <helios/input/settings.hpp>
#include <helios/input/systems/clear.hpp>
#include <helios/input/systems/gamepad.hpp>
#include <helios/input/systems/joystick.hpp>
#include <helios/input/systems/keyboard.hpp>
#include <helios/input/systems/mouse.hpp>
#include <helios/input/systems/pen.hpp>
#include <helios/input/systems/sensor.hpp>
#include <helios/input/systems/touch.hpp>
#include <helios/input/touch.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
