#pragma once

#include <cstdint>
#include <source_location>
#include <string_view>

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
  std::string_view name;
  std::source_location loc;
  uint32_t color = 0;
  bool active = true;
  /// Optional stack-capture depth hint. Backends that do not support callstack
  /// capture may ignore this field.
  int callstack_depth = 0;
};

}  // namespace helios::profile
