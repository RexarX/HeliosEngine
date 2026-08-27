#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.log;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/log/config.hpp>
#include <helios/log/logger.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
