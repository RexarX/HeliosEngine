#pragma once

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/input/components.hpp>
#include <helios/input/params.hpp>
#include <helios/input/resources.hpp>
#include <helios/window/components.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

struct GLFWcursor;

namespace helios::glfw {

/// @brief Cached previous GLFW gamepad slot state for edge detection.
struct GamepadSlotCache {
  std::string name;
  std::array<float, static_cast<size_t>(input::GamepadAxis::kCount)> axes = {};
  std::array<unsigned char, static_cast<size_t>(input::GamepadButton::kCount)>
      buttons = {};
  bool connected = false;
};

/// @brief Previous gamepad snapshots for joysticks `0..GLFW_JOYSTICK_LAST`.
struct GamepadCache {
  static constexpr std::string_view kName = "helios::glfw::GamepadCache";
  static constexpr size_t kSlotCount = 16;

  std::array<GamepadSlotCache, kSlotCount> slots = {};
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

/// @brief Polls GLFW gamepads and emits connection / button / axis messages.
/// @details Hot-plug is detected with `glfwJoystickPresent` each frame rather
/// than `glfwSetJoystickCallback`, so it stays consistent with per-frame
/// `glfwGetGamepadState` polling. GLFW joystick state is not delivered as OS
/// events. Newly connected mapped gamepads emit a full axis snapshot (and any
/// currently pressed buttons) so rest-center calibration can see the first
/// sample. Unmapped joysticks (wheels, pedals, HOTAS) are ignored.
struct PollGamepads {
  static constexpr std::string_view kName = "helios::glfw::PollGamepads";

  void operator()(ecs::Res<const Context> context,
                  input::GamepadWriters writers,
                  ecs::Res<GamepadCache> cache) const;
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
