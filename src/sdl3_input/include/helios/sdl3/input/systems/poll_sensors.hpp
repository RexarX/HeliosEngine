#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/input/params.hpp>
#include <helios/input/sensor.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#endif
#include <helios/sdl3/input/state.hpp>

HELIOS_MODULE_EXPORT struct SDL_Sensor;

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief Cached open SDL sensor mapped to a `Sensors` slot.
struct SensorSlotCache {
  std::string name;
  SDL_Sensor* sensor = nullptr;
  uint32_t instance_id = 0;
  helios::input::SensorType type = helios::input::SensorType::kUnknown;
  bool connected = false;
};

/// @brief Open standalone sensors keyed by slot `0..7`.
struct SensorCache {
  static constexpr std::string_view kName = "helios::sdl3::input::SensorCache";
  static constexpr size_t kSlotCount = 8;

  std::array<SensorSlotCache, kSlotCount> slots = {};
};

/**
 * @brief Closes all open SDL sensors and clears slot state.
 * @param cache Sensor cache to reset
 */
void DestroySensorCache(SensorCache& cache);

/// @brief Enumerates SDL sensors and emits connection / disconnect messages.
struct PollSensors {
  static constexpr std::string_view kName = "helios::sdl3::input::PollSensors";

  void operator()(ecs::Res<const Context> context,
                  helios::input::SensorWriters sensors,
                  ecs::Res<SensorCache> cache) const;
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
