#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/systems/init.hpp>
#include <helios/sdl3/systems/pump_events.hpp>
#include <helios/sdl3/systems/shutdown.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
