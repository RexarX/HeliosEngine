#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/sdl3/input/cursor_cache.hpp>
#include <helios/sdl3/input/event_handlers.hpp>
#include <helios/sdl3/input/plugin.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/apply_cursors.hpp>
#include <helios/sdl3/input/systems/apply_raw_mouse.hpp>
#include <helios/sdl3/input/systems/init.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>
#include <helios/sdl3/input/systems/poll_sensors.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
