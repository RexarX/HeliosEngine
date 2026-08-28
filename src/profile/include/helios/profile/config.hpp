#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.profile;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <cstddef>

/// @brief Per-scope stack storage for backend zone state (bytes).
#ifndef HELIOS_PROFILE_ZONE_STORAGE_BYTES
#define HELIOS_PROFILE_ZONE_STORAGE_BYTES 256
#endif

HELIOS_MODULE_EXPORT
namespace helios::profile {

inline constexpr size_t kZoneStorageBytes = HELIOS_PROFILE_ZONE_STORAGE_BYTES;

}  // namespace helios::profile
#endif  // HELIOS_MODULE_CONSUMER_SHIM
