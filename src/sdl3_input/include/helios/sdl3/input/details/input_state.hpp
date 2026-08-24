#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace helios::sdl3::input {

/// @brief SDL3 input backend runtime flags.
struct Context {
  static constexpr std::string_view kName = "helios::sdl3::input::Context";

  bool input_enabled = false;
  bool handlers_registered = false;
  bool gamepad_subsystem_retained = false;
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

}  // namespace helios::sdl3::input
