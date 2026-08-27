#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/window/clipboard.hpp>
#include <helios/window/ids.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/params.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/schedules.hpp>
#include <helios/window/settings.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
