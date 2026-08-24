#pragma once

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/params.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/input/components.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>
#include <helios/input/resources.hpp>
#include <helios/window/components.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

struct GLFWcursor;

namespace helios::glfw {

/// @brief Number of GLFW mapped gamepad buttons (`GLFW_GAMEPAD_BUTTON_LAST+1`).
inline constexpr size_t kGlfwMappedButtonCount = 15;
/// @brief Number of GLFW mapped gamepad axes (`GLFW_GAMEPAD_AXIS_LAST+1`).
inline constexpr size_t kGlfwMappedAxisCount = 6;

/// @brief Cached previous GLFW gamepad slot state for edge detection.
struct GamepadSlotCache {
  std::string name;
  std::array<float, kGlfwMappedAxisCount> axes = {};
  std::array<unsigned char, kGlfwMappedButtonCount> buttons = {};
  bool connected = false;
};

/// @brief Cached previous unmapped joystick slot state for edge detection.
struct JoystickSlotCache {
  std::string name;
  std::string guid;
  std::array<float, input::Joystick::kMaxAxes> axes = {};
  std::array<unsigned char, input::Joystick::kMaxButtons> buttons = {};
  std::array<unsigned char, input::Joystick::kMaxHats> hats = {};
  uint8_t axis_count = 0;
  uint8_t button_count = 0;
  uint8_t hat_count = 0;
  bool connected = false;
};

/// @brief Previous gamepad and joystick snapshots for ids `0..15`.
struct GamepadCache {
  static constexpr std::string_view kName = "helios::glfw::GamepadCache";
  static constexpr size_t kSlotCount = 16;

  std::array<GamepadSlotCache, kSlotCount> slots = {};
  std::array<JoystickSlotCache, kSlotCount> joysticks = {};
};

/// @brief Cached `GLFWcursor*` objects for standard and custom cursors.
struct CursorCache {
  static constexpr std::string_view kName = "helios::glfw::CursorCache";

  struct CustomEntry {
    ecs::Entity entity;
    GLFWcursor* cursor = nullptr;
  };

  std::array<GLFWcursor*, static_cast<size_t>(input::CursorIcon::kCount)>
      standard = {};
  std::vector<CustomEntry> custom;
};

/**
 * @brief Destroys all cached GLFW cursor objects.
 * @param cache Cursor cache to clear
 */
void DestroyCursorCache(CursorCache& cache);

/// @brief Applies queued `GamepadMappings` via `glfwUpdateGamepadMappings`.
struct ApplyGamepadMappings {
  static constexpr std::string_view kName =
      "helios::glfw::ApplyGamepadMappings";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<input::GamepadMappings> mappings) const;
};

/// @brief Polls GLFW gamepads and unmapped joysticks.
/// @details Hot-plug is detected with `glfwJoystickPresent` each frame.
/// Mapped devices emit gamepad messages; unmapped devices emit joystick
/// messages. A mapping change promotes or demotes the slot.
struct PollGamepads {
  static constexpr std::string_view kName = "helios::glfw::PollGamepads";

  void operator()(ecs::Res<const Context> context,
                  input::GamepadWriters gamepads, input::JoystickWriters sticks,
                  ecs::Res<GamepadCache> cache) const;
};

/// @brief GLFW has no rumble / LED / sensor APIs; consumes dirty flags only.
struct ApplyGamepadOutputs {
  static constexpr std::string_view kName = "helios::glfw::ApplyGamepadOutputs";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<input::Gamepads> gamepads) const;
};

/// @brief Applies dirty `input::Cursor` components to native GLFW windows.
struct ApplyCursors {
  static constexpr std::string_view kName = "helios::glfw::ApplyCursors";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native, ecs::Res<CursorCache> cache,
                  ecs::Query<window::Window&, input::Cursor&> cursors) const;
};

/// @brief Enables or disables GLFW raw mouse motion to match input settings.
struct ApplyRawMouseMotion {
  static constexpr std::string_view kName = "helios::glfw::ApplyRawMouseMotion";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native,
                  ecs::Res<const input::Settings> settings,
                  ecs::Query<const window::Window&> windows) const;
};

}  // namespace helios::glfw

#endif  // HELIOS_MODULE_INPUT_AVAILABLE
