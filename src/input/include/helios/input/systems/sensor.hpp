#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>

#include <string_view>
#endif
#include <helios/input/params.hpp>
#include <helios/input/sensor.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies standalone sensor messages into the `Sensors` resource.
struct UpdateSensorState {
  static constexpr std::string_view kName = "helios::input::UpdateSensorState";

  /**
   * @brief Drains sensor connection and sample messages into aggregated state.
   * @param sensors Sensors resource
   * @param messages Sensor connection and sample readers
   */
  void operator()(ecs::Res<Sensors> sensors, SensorMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
