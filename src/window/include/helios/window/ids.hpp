#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstdint>
#endif

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Connected-monitor identifier (index into `Monitors::monitors`).
using MonitorId = uint32_t;

#ifdef HELIOS_PLATFORM_LINUX_X11
/// @brief X11 `Window` identifier stored in `XlibHandle`.
using NativeXWindowId = uint64_t;
#endif

}  // namespace helios::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
