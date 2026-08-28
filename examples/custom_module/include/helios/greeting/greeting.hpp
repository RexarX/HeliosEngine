#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.greeting;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/core/core.hpp>

#include <string>
#endif

HELIOS_MODULE_EXPORT
namespace helios::greeting {

/// @brief Formats a greeting for the given name.
[[nodiscard]] std::string Format(helios::CStringView name);

}  // namespace helios::greeting
#endif  // HELIOS_MODULE_CONSUMER_SHIM
