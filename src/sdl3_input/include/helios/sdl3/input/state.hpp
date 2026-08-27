#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief SDL3 input backend runtime flags.
struct Context {
  static constexpr std::string_view kName = "helios::sdl3::input::Context";

  bool input_enabled = false;
  bool handlers_registered = false;
  bool gamepad_subsystem_retained = false;
  bool sensor_subsystem_retained = false;
  bool raw_mouse_hint_set = false;
  bool last_raw_mouse_motion = false;
};

/// @brief Cached SDL pen instance mapped to a `Pens` slot.
struct PenSlotCache {
  uint32_t instance_id = 0;
  bool connected = false;
};

/// @brief Open pens keyed by slot `0..7`.
struct PenCache {
  static constexpr std::string_view kName = "helios::sdl3::input::PenCache";
  static constexpr size_t kSlotCount = 8;

  std::array<PenSlotCache, kSlotCount> slots = {};
};

/// @brief Cached SDL touch finger mapped to a `Touches` slot.
struct TouchSlotCache {
  uint64_t touch_id = 0;
  uint64_t finger_id = 0;
  bool connected = false;
};

/// @brief Open fingers keyed by slot `0..15`.
struct TouchCache {
  static constexpr std::string_view kName = "helios::sdl3::input::TouchCache";
  static constexpr size_t kSlotCount = 16;

  std::array<TouchSlotCache, kSlotCount> slots = {};
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
