#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.profile;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/cstring_view.hpp>

#include <cstdint>
#include <string_view>
#endif
#include <helios/profile/profiler.hpp>

HELIOS_MODULE_EXPORT
namespace helios::profile {

/**
 * @brief Emits a timeline message on all active backends
 * @param text The message text to emit
 * @param color An optional color to associate with the message (default: 0)
 */
inline void Message(std::string_view text, uint32_t color = 0) noexcept {
  Profiler::Instance().Message(text, color);
}

/**
 * @brief Sets the name of the current thread on all active backends
 * @param name The name to set for the current thread
 */
inline void SetThreadName(CStringView name) noexcept {
  Profiler::Instance().SetThreadName(name);
}

}  // namespace helios::profile
#endif  // HELIOS_MODULE_CONSUMER_SHIM
