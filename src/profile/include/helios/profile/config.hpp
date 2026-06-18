#pragma once

#include <cstddef>

/// @brief Per-scope stack storage for backend zone state (bytes).
#ifndef HELIOS_PROFILE_ZONE_STORAGE_BYTES
#define HELIOS_PROFILE_ZONE_STORAGE_BYTES 256
#endif

namespace helios::profile {

inline constexpr size_t kZoneStorageBytes = HELIOS_PROFILE_ZONE_STORAGE_BYTES;

}  // namespace helios::profile
