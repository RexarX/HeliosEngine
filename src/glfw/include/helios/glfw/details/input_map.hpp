#pragma once

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>

#include <optional>

namespace helios::glfw {

/**
 * @brief Maps a GLFW key code to a contiguous `input::Key`.
 * @param glfw_key GLFW key code (`GLFW_KEY_*`)
 * @return Matching key, or `Key::kUnknown` when unmapped
 */
[[nodiscard]] input::Key KeyFromGlfw(int glfw_key) noexcept;

/**
 * @brief Maps an `input::Key` back to a GLFW key code.
 * @param key Contiguous keyboard key
 * @return GLFW key code, or `GLFW_KEY_UNKNOWN` when unmapped
 */
[[nodiscard]] int GlfwFromKey(input::Key key) noexcept;

/**
 * @brief Maps a GLFW mouse button index to `input::MouseButton`.
 * @param button GLFW mouse button (`GLFW_MOUSE_BUTTON_*`)
 * @return Matching button, or empty when out of range
 */
[[nodiscard]] auto MouseButtonFromGlfw(int button) noexcept
    -> std::optional<input::MouseButton>;

/**
 * @brief Maps GLFW modifier bits to `input::Modifiers`.
 * @param mods GLFW modifier mask (`GLFW_MOD_*`)
 * @return Combined modifier flags
 */
[[nodiscard]] input::Modifiers ModifiersFromGlfw(int mods) noexcept;

/**
 * @brief Maps a GLFW key/button action to `input::ButtonState`.
 * @param action GLFW action (`GLFW_PRESS` / `GLFW_RELEASE` / `GLFW_REPEAT`)
 * @return Matching button state
 */
[[nodiscard]] input::ButtonState ButtonStateFromGlfw(int action) noexcept;

/**
 * @brief Maps a GLFW gamepad button index to `input::GamepadButton`.
 * @param button GLFW gamepad button (`GLFW_GAMEPAD_BUTTON_*`)
 * @return Matching gamepad button, or `GamepadButton::kA` when out of range
 */
[[nodiscard]] input::GamepadButton GamepadButtonFromGlfw(int button) noexcept;

/**
 * @brief Maps a GLFW gamepad axis index to `input::GamepadAxis`.
 * @param axis GLFW gamepad axis (`GLFW_GAMEPAD_AXIS_*`)
 * @return Matching gamepad axis, or `GamepadAxis::kLeftX` when out of range
 */
[[nodiscard]] input::GamepadAxis GamepadAxisFromGlfw(int axis) noexcept;

/**
 * @brief Maps an `input::CursorIcon` to a GLFW standard cursor shape.
 * @param icon Cursor icon
 * @return GLFW standard cursor shape (`GLFW_*_CURSOR`)
 */
[[nodiscard]] int GlfwFromCursorIcon(input::CursorIcon icon) noexcept;

}  // namespace helios::glfw

#endif  // HELIOS_MODULE_INPUT_AVAILABLE
