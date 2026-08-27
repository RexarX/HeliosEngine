#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>
#include <helios/input/components.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/plugin.hpp>
#include <helios/input/resources.hpp>
#include <helios/input/systems.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
