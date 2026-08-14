#pragma once

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace helios::input {

/// @brief Keyboard key press / release / repeat event for a window entity.
struct KeyboardInputMsg {
  static constexpr std::string_view kName = "helios::input::KeyboardInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  Key key = Key::kUnknown;
  ButtonState state = ButtonState::kReleased;
  Modifiers modifiers = Modifiers::kNone;
};

/// @brief Unicode text input event for a window entity.
struct TextInputMsg {
  static constexpr std::string_view kName = "helios::input::TextInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  uint32_t codepoint = 0;
};

/// @brief Mouse button press / release / repeat event for a window entity.
struct MouseButtonInputMsg {
  static constexpr std::string_view kName =
      "helios::input::MouseButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  MouseButton button = MouseButton::kLeft;
  ButtonState state = ButtonState::kReleased;
  Modifiers modifiers = Modifiers::kNone;
};

/// @brief Absolute cursor position change for a window entity.
struct CursorMovedMsg {
  static constexpr std::string_view kName = "helios::input::CursorMovedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
};

/// @brief Raw relative mouse motion for a window entity.
struct MouseMotionMsg {
  static constexpr std::string_view kName = "helios::input::MouseMotionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double delta_x = 0.0;
  double delta_y = 0.0;
};

/// @brief Mouse scroll / wheel event for a window entity.
struct MouseWheelMsg {
  static constexpr std::string_view kName = "helios::input::MouseWheelMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
};

/// @brief Gamepad connect / disconnect notification.
struct GamepadConnectionMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadConnectionMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  bool connected = false;
  std::string name;
};

/// @brief Gamepad button press / release event.
struct GamepadButtonInputMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  GamepadButton button = GamepadButton::kA;
  ButtonState state = ButtonState::kReleased;
};

/// @brief Gamepad axis value change.
/// @details `value` is the raw backend sample (GLFW gamepad range). Filtered
/// stick / trigger values are applied to `Gamepad::axes` by
/// `UpdateGamepadState`.
struct GamepadAxisChangedMsg {
  static constexpr std::string_view kName =
      "helios::input::GamepadAxisChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t id = -1;
  GamepadAxis axis = GamepadAxis::kLeftX;
  float value = 0.0F;
};

}  // namespace helios::input
