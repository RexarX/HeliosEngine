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
#endif
#include <helios/profile/common.hpp>
#include <helios/profile/profiler.hpp>

HELIOS_MODULE_EXPORT
namespace helios::profile {

/**
 * @brief Records a plot sample on all active backends
 * @param name The name of the plot channel
 * @param value The value to record
 */
inline void Plot(CStringView name, double value) noexcept {
  Profiler::Instance().Plot(name, value);
}

/**
 * @brief Configures a plot channel on all active backends
 * @param name The name of the plot channel
 * @param type The format of the plot channel
 * @param step Whether the plot should be step-wise
 * @param fill Whether the plot should be filled
 * @param color The color to associate with the plot channel
 */
inline void PlotConfig(CStringView name, PlotFormat type, bool step, bool fill,
                       uint32_t color) noexcept {
  Profiler::Instance().PlotConfig(name, type, step, fill, color);
}

}  // namespace helios::profile
#endif  // HELIOS_MODULE_CONSUMER_SHIM
