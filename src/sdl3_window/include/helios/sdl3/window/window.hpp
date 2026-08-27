#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/sdl3/window/event_handlers.hpp>
#include <helios/sdl3/window/native_handle.hpp>
#include <helios/sdl3/window/plugin.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/sdl3/window/systems/apply.hpp>
#include <helios/sdl3/window/systems/create.hpp>
#include <helios/sdl3/window/systems/destroy.hpp>
#include <helios/sdl3/window/systems/init.hpp>
#include <helios/sdl3/window/systems/poll.hpp>
#include <helios/sdl3/window/systems/shutdown.hpp>
#include <helios/sdl3/window/window_map.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
