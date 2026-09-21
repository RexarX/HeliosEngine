#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.profile;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstdint>
#include <source_location>
#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::profile {

/// @brief Plot value format for timeline plots.
enum class PlotFormat : uint8_t {
  kNumber,
  kMemory,
  kPercentage,
  kWatts,
};

/**
 * @brief Immutable zone description for a single instrumentation site.
 * @details Constructed at each instrumentation entry. Empty @p
 * name means the backend should use @p loc.function_name().
 */
struct ZoneSpec {
  std::string_view name{};
  uint32_t color = 0;
  bool active = true;
  /// Optional stack-capture depth hint. Backends that do not support callstack
  /// capture may ignore this field.
  int callstack_depth = 0;
  std::source_location loc = std::source_location::current();
};

}  // namespace helios::profile
#endif  // HELIOS_MODULE_CONSUMER_SHIM
