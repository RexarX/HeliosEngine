#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/params.hpp>
#include <helios/input/settings.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#endif
#include <helios/sdl3/input/state.hpp>

HELIOS_MODULE_EXPORT struct SDL_Gamepad;
HELIOS_MODULE_EXPORT struct SDL_Joystick;

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief Cached previous SDL device slot state for edge detection.
struct GamepadSlotCache {
  std::string name;
  std::string guid;
  SDL_Gamepad* gamepad = nullptr;
  SDL_Joystick* joystick = nullptr;
  std::array<float, static_cast<size_t>(helios::input::GamepadAxis::kCount)>
      pad_axes = {};
  std::array<float, helios::input::Joystick::kMaxAxes> joy_axes = {};
  std::array<std::array<helios::input::GamepadTouchpadFinger,
                        helios::input::Gamepad::kMaxTouchpadFingers>,
             helios::input::Gamepad::kMaxTouchpads>
      touchpads = {};
  std::array<bool, static_cast<size_t>(helios::input::GamepadButton::kCount)>
      pad_buttons = {};
  std::array<bool, helios::input::Joystick::kMaxButtons> joy_buttons = {};
  std::array<uint8_t, helios::input::Joystick::kMaxHats> hats = {};
  uint32_t instance_id = 0;
  helios::input::GamepadPower power;
  uint8_t joy_axis_count = 0;
  uint8_t joy_button_count = 0;
  uint8_t joy_hat_count = 0;
  uint8_t touchpad_count = 0;
  bool connected = false;
  bool is_gamepad = false;
};

/// @brief Open devices and previous snapshots keyed by player slot `0..15`.
struct GamepadCache {
  static constexpr std::string_view kName = "helios::sdl3::input::GamepadCache";
  static constexpr size_t kSlotCount = 16;

  std::array<GamepadSlotCache, kSlotCount> slots = {};
  std::vector<uint32_t> pending_added;
  std::vector<uint32_t> pending_removed;
  std::vector<uint32_t> pending_remapped;
  bool enumerated = false;
};

/**
 * @brief Closes all open SDL devices and clears slot state.
 * @param cache Gamepad cache to reset
 */
void DestroyGamepadCache(GamepadCache& cache);

/// @brief Applies queued `GamepadMappings` via SDL mapping APIs.
struct ApplyGamepadMappings {
  static constexpr std::string_view kName =
      "helios::sdl3::input::ApplyGamepadMappings";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<helios::input::GamepadMappings> mappings) const;
};

/// @brief Polls open SDL gamepads and unmapped joysticks.
struct PollGamepads {
  static constexpr std::string_view kName = "helios::sdl3::input::PollGamepads";

  void operator()(ecs::Res<const Context> context,
                  helios::input::GamepadWriters gamepads,
                  helios::input::JoystickWriters sticks,
                  ecs::Res<GamepadCache> cache) const;
};

/// @brief Applies rumble, trigger rumble, LED, and sensor enables.
struct ApplyGamepadOutputs {
  static constexpr std::string_view kName =
      "helios::sdl3::input::ApplyGamepadOutputs";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<helios::input::Gamepads> gamepads,
                  ecs::Res<GamepadCache> cache) const;
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
