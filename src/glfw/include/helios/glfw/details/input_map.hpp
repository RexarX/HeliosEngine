#pragma once

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#ifndef HELIOS_BUILDING_MODULE
#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>

#include <GLFW/glfw3.h>

#include <optional>
#endif

namespace helios::glfw {

/**
 * @brief Maps a GLFW key code to a contiguous `input::Key`.
 * @param glfw_key GLFW key code (`GLFW_KEY_*`)
 * @return Matching key, or `Key::kUnknown` when unmapped
 */
[[nodiscard]] constexpr input::Key KeyFromGlfw(int glfw_key) noexcept {
  switch (glfw_key) {
    case GLFW_KEY_SPACE:
      return input::Key::kSpace;
    case GLFW_KEY_APOSTROPHE:
      return input::Key::kApostrophe;
    case GLFW_KEY_COMMA:
      return input::Key::kComma;
    case GLFW_KEY_MINUS:
      return input::Key::kMinus;
    case GLFW_KEY_PERIOD:
      return input::Key::kPeriod;
    case GLFW_KEY_SLASH:
      return input::Key::kSlash;
    case GLFW_KEY_0:
      return input::Key::kDigit0;
    case GLFW_KEY_1:
      return input::Key::kDigit1;
    case GLFW_KEY_2:
      return input::Key::kDigit2;
    case GLFW_KEY_3:
      return input::Key::kDigit3;
    case GLFW_KEY_4:
      return input::Key::kDigit4;
    case GLFW_KEY_5:
      return input::Key::kDigit5;
    case GLFW_KEY_6:
      return input::Key::kDigit6;
    case GLFW_KEY_7:
      return input::Key::kDigit7;
    case GLFW_KEY_8:
      return input::Key::kDigit8;
    case GLFW_KEY_9:
      return input::Key::kDigit9;
    case GLFW_KEY_SEMICOLON:
      return input::Key::kSemicolon;
    case GLFW_KEY_EQUAL:
      return input::Key::kEqual;
    case GLFW_KEY_A:
      return input::Key::kA;
    case GLFW_KEY_B:
      return input::Key::kB;
    case GLFW_KEY_C:
      return input::Key::kC;
    case GLFW_KEY_D:
      return input::Key::kD;
    case GLFW_KEY_E:
      return input::Key::kE;
    case GLFW_KEY_F:
      return input::Key::kF;
    case GLFW_KEY_G:
      return input::Key::kG;
    case GLFW_KEY_H:
      return input::Key::kH;
    case GLFW_KEY_I:
      return input::Key::kI;
    case GLFW_KEY_J:
      return input::Key::kJ;
    case GLFW_KEY_K:
      return input::Key::kK;
    case GLFW_KEY_L:
      return input::Key::kL;
    case GLFW_KEY_M:
      return input::Key::kM;
    case GLFW_KEY_N:
      return input::Key::kN;
    case GLFW_KEY_O:
      return input::Key::kO;
    case GLFW_KEY_P:
      return input::Key::kP;
    case GLFW_KEY_Q:
      return input::Key::kQ;
    case GLFW_KEY_R:
      return input::Key::kR;
    case GLFW_KEY_S:
      return input::Key::kS;
    case GLFW_KEY_T:
      return input::Key::kT;
    case GLFW_KEY_U:
      return input::Key::kU;
    case GLFW_KEY_V:
      return input::Key::kV;
    case GLFW_KEY_W:
      return input::Key::kW;
    case GLFW_KEY_X:
      return input::Key::kX;
    case GLFW_KEY_Y:
      return input::Key::kY;
    case GLFW_KEY_Z:
      return input::Key::kZ;
    case GLFW_KEY_LEFT_BRACKET:
      return input::Key::kLeftBracket;
    case GLFW_KEY_BACKSLASH:
      return input::Key::kBackslash;
    case GLFW_KEY_RIGHT_BRACKET:
      return input::Key::kRightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
      return input::Key::kGraveAccent;
    case GLFW_KEY_WORLD_1:
      return input::Key::kWorld1;
    case GLFW_KEY_WORLD_2:
      return input::Key::kWorld2;
    case GLFW_KEY_ESCAPE:
      return input::Key::kEscape;
    case GLFW_KEY_ENTER:
      return input::Key::kEnter;
    case GLFW_KEY_TAB:
      return input::Key::kTab;
    case GLFW_KEY_BACKSPACE:
      return input::Key::kBackspace;
    case GLFW_KEY_INSERT:
      return input::Key::kInsert;
    case GLFW_KEY_DELETE:
      return input::Key::kDelete;
    case GLFW_KEY_RIGHT:
      return input::Key::kRight;
    case GLFW_KEY_LEFT:
      return input::Key::kLeft;
    case GLFW_KEY_DOWN:
      return input::Key::kDown;
    case GLFW_KEY_UP:
      return input::Key::kUp;
    case GLFW_KEY_PAGE_UP:
      return input::Key::kPageUp;
    case GLFW_KEY_PAGE_DOWN:
      return input::Key::kPageDown;
    case GLFW_KEY_HOME:
      return input::Key::kHome;
    case GLFW_KEY_END:
      return input::Key::kEnd;
    case GLFW_KEY_CAPS_LOCK:
      return input::Key::kCapsLock;
    case GLFW_KEY_SCROLL_LOCK:
      return input::Key::kScrollLock;
    case GLFW_KEY_NUM_LOCK:
      return input::Key::kNumLock;
    case GLFW_KEY_PRINT_SCREEN:
      return input::Key::kPrintScreen;
    case GLFW_KEY_PAUSE:
      return input::Key::kPause;
    case GLFW_KEY_F1:
      return input::Key::kF1;
    case GLFW_KEY_F2:
      return input::Key::kF2;
    case GLFW_KEY_F3:
      return input::Key::kF3;
    case GLFW_KEY_F4:
      return input::Key::kF4;
    case GLFW_KEY_F5:
      return input::Key::kF5;
    case GLFW_KEY_F6:
      return input::Key::kF6;
    case GLFW_KEY_F7:
      return input::Key::kF7;
    case GLFW_KEY_F8:
      return input::Key::kF8;
    case GLFW_KEY_F9:
      return input::Key::kF9;
    case GLFW_KEY_F10:
      return input::Key::kF10;
    case GLFW_KEY_F11:
      return input::Key::kF11;
    case GLFW_KEY_F12:
      return input::Key::kF12;
    case GLFW_KEY_F13:
      return input::Key::kF13;
    case GLFW_KEY_F14:
      return input::Key::kF14;
    case GLFW_KEY_F15:
      return input::Key::kF15;
    case GLFW_KEY_F16:
      return input::Key::kF16;
    case GLFW_KEY_F17:
      return input::Key::kF17;
    case GLFW_KEY_F18:
      return input::Key::kF18;
    case GLFW_KEY_F19:
      return input::Key::kF19;
    case GLFW_KEY_F20:
      return input::Key::kF20;
    case GLFW_KEY_F21:
      return input::Key::kF21;
    case GLFW_KEY_F22:
      return input::Key::kF22;
    case GLFW_KEY_F23:
      return input::Key::kF23;
    case GLFW_KEY_F24:
      return input::Key::kF24;
    case GLFW_KEY_F25:
      return input::Key::kF25;
    case GLFW_KEY_KP_0:
      return input::Key::kNumPad0;
    case GLFW_KEY_KP_1:
      return input::Key::kNumPad1;
    case GLFW_KEY_KP_2:
      return input::Key::kNumPad2;
    case GLFW_KEY_KP_3:
      return input::Key::kNumPad3;
    case GLFW_KEY_KP_4:
      return input::Key::kNumPad4;
    case GLFW_KEY_KP_5:
      return input::Key::kNumPad5;
    case GLFW_KEY_KP_6:
      return input::Key::kNumPad6;
    case GLFW_KEY_KP_7:
      return input::Key::kNumPad7;
    case GLFW_KEY_KP_8:
      return input::Key::kNumPad8;
    case GLFW_KEY_KP_9:
      return input::Key::kNumPad9;
    case GLFW_KEY_KP_DECIMAL:
      return input::Key::kNumPadDecimal;
    case GLFW_KEY_KP_DIVIDE:
      return input::Key::kNumPadDivide;
    case GLFW_KEY_KP_MULTIPLY:
      return input::Key::kNumPadMultiply;
    case GLFW_KEY_KP_SUBTRACT:
      return input::Key::kNumPadSubtract;
    case GLFW_KEY_KP_ADD:
      return input::Key::kNumPadAdd;
    case GLFW_KEY_KP_ENTER:
      return input::Key::kNumPadEnter;
    case GLFW_KEY_KP_EQUAL:
      return input::Key::kNumPadEqual;
    case GLFW_KEY_LEFT_SHIFT:
      return input::Key::kLeftShift;
    case GLFW_KEY_LEFT_CONTROL:
      return input::Key::kLeftControl;
    case GLFW_KEY_LEFT_ALT:
      return input::Key::kLeftAlt;
    case GLFW_KEY_LEFT_SUPER:
      return input::Key::kLeftSuper;
    case GLFW_KEY_RIGHT_SHIFT:
      return input::Key::kRightShift;
    case GLFW_KEY_RIGHT_CONTROL:
      return input::Key::kRightControl;
    case GLFW_KEY_RIGHT_ALT:
      return input::Key::kRightAlt;
    case GLFW_KEY_RIGHT_SUPER:
      return input::Key::kRightSuper;
    case GLFW_KEY_MENU:
      return input::Key::kMenu;
    default:
      return input::Key::kUnknown;
  }
}

/**
 * @brief Maps an `input::Key` back to a GLFW key code.
 * @param key Contiguous keyboard key
 * @return GLFW key code, or `GLFW_KEY_UNKNOWN` when unmapped
 */
[[nodiscard]] constexpr int GlfwFromKey(input::Key key) noexcept {
  switch (key) {
    using enum input::Key;
    case kSpace:
      return GLFW_KEY_SPACE;
    case kApostrophe:
      return GLFW_KEY_APOSTROPHE;
    case kComma:
      return GLFW_KEY_COMMA;
    case kMinus:
      return GLFW_KEY_MINUS;
    case kPeriod:
      return GLFW_KEY_PERIOD;
    case kSlash:
      return GLFW_KEY_SLASH;
    case kDigit0:
      return GLFW_KEY_0;
    case kDigit1:
      return GLFW_KEY_1;
    case kDigit2:
      return GLFW_KEY_2;
    case kDigit3:
      return GLFW_KEY_3;
    case kDigit4:
      return GLFW_KEY_4;
    case kDigit5:
      return GLFW_KEY_5;
    case kDigit6:
      return GLFW_KEY_6;
    case kDigit7:
      return GLFW_KEY_7;
    case kDigit8:
      return GLFW_KEY_8;
    case kDigit9:
      return GLFW_KEY_9;
    case kSemicolon:
      return GLFW_KEY_SEMICOLON;
    case kEqual:
      return GLFW_KEY_EQUAL;
    case kA:
      return GLFW_KEY_A;
    case kB:
      return GLFW_KEY_B;
    case kC:
      return GLFW_KEY_C;
    case kD:
      return GLFW_KEY_D;
    case kE:
      return GLFW_KEY_E;
    case kF:
      return GLFW_KEY_F;
    case kG:
      return GLFW_KEY_G;
    case kH:
      return GLFW_KEY_H;
    case kI:
      return GLFW_KEY_I;
    case kJ:
      return GLFW_KEY_J;
    case kK:
      return GLFW_KEY_K;
    case kL:
      return GLFW_KEY_L;
    case kM:
      return GLFW_KEY_M;
    case kN:
      return GLFW_KEY_N;
    case kO:
      return GLFW_KEY_O;
    case kP:
      return GLFW_KEY_P;
    case kQ:
      return GLFW_KEY_Q;
    case kR:
      return GLFW_KEY_R;
    case kS:
      return GLFW_KEY_S;
    case kT:
      return GLFW_KEY_T;
    case kU:
      return GLFW_KEY_U;
    case kV:
      return GLFW_KEY_V;
    case kW:
      return GLFW_KEY_W;
    case kX:
      return GLFW_KEY_X;
    case kY:
      return GLFW_KEY_Y;
    case kZ:
      return GLFW_KEY_Z;
    case kLeftBracket:
      return GLFW_KEY_LEFT_BRACKET;
    case kBackslash:
      return GLFW_KEY_BACKSLASH;
    case kRightBracket:
      return GLFW_KEY_RIGHT_BRACKET;
    case kGraveAccent:
      return GLFW_KEY_GRAVE_ACCENT;
    case kWorld1:
      return GLFW_KEY_WORLD_1;
    case kWorld2:
      return GLFW_KEY_WORLD_2;
    case kEscape:
      return GLFW_KEY_ESCAPE;
    case kEnter:
      return GLFW_KEY_ENTER;
    case kTab:
      return GLFW_KEY_TAB;
    case kBackspace:
      return GLFW_KEY_BACKSPACE;
    case kInsert:
      return GLFW_KEY_INSERT;
    case kDelete:
      return GLFW_KEY_DELETE;
    case kRight:
      return GLFW_KEY_RIGHT;
    case kLeft:
      return GLFW_KEY_LEFT;
    case kDown:
      return GLFW_KEY_DOWN;
    case kUp:
      return GLFW_KEY_UP;
    case kPageUp:
      return GLFW_KEY_PAGE_UP;
    case kPageDown:
      return GLFW_KEY_PAGE_DOWN;
    case kHome:
      return GLFW_KEY_HOME;
    case kEnd:
      return GLFW_KEY_END;
    case kCapsLock:
      return GLFW_KEY_CAPS_LOCK;
    case kScrollLock:
      return GLFW_KEY_SCROLL_LOCK;
    case kNumLock:
      return GLFW_KEY_NUM_LOCK;
    case kPrintScreen:
      return GLFW_KEY_PRINT_SCREEN;
    case kPause:
      return GLFW_KEY_PAUSE;
    case kF1:
      return GLFW_KEY_F1;
    case kF2:
      return GLFW_KEY_F2;
    case kF3:
      return GLFW_KEY_F3;
    case kF4:
      return GLFW_KEY_F4;
    case kF5:
      return GLFW_KEY_F5;
    case kF6:
      return GLFW_KEY_F6;
    case kF7:
      return GLFW_KEY_F7;
    case kF8:
      return GLFW_KEY_F8;
    case kF9:
      return GLFW_KEY_F9;
    case kF10:
      return GLFW_KEY_F10;
    case kF11:
      return GLFW_KEY_F11;
    case kF12:
      return GLFW_KEY_F12;
    case kF13:
      return GLFW_KEY_F13;
    case kF14:
      return GLFW_KEY_F14;
    case kF15:
      return GLFW_KEY_F15;
    case kF16:
      return GLFW_KEY_F16;
    case kF17:
      return GLFW_KEY_F17;
    case kF18:
      return GLFW_KEY_F18;
    case kF19:
      return GLFW_KEY_F19;
    case kF20:
      return GLFW_KEY_F20;
    case kF21:
      return GLFW_KEY_F21;
    case kF22:
      return GLFW_KEY_F22;
    case kF23:
      return GLFW_KEY_F23;
    case kF24:
      return GLFW_KEY_F24;
    case kF25:
      return GLFW_KEY_F25;
    case kNumPad0:
      return GLFW_KEY_KP_0;
    case kNumPad1:
      return GLFW_KEY_KP_1;
    case kNumPad2:
      return GLFW_KEY_KP_2;
    case kNumPad3:
      return GLFW_KEY_KP_3;
    case kNumPad4:
      return GLFW_KEY_KP_4;
    case kNumPad5:
      return GLFW_KEY_KP_5;
    case kNumPad6:
      return GLFW_KEY_KP_6;
    case kNumPad7:
      return GLFW_KEY_KP_7;
    case kNumPad8:
      return GLFW_KEY_KP_8;
    case kNumPad9:
      return GLFW_KEY_KP_9;
    case kNumPadDecimal:
      return GLFW_KEY_KP_DECIMAL;
    case kNumPadDivide:
      return GLFW_KEY_KP_DIVIDE;
    case kNumPadMultiply:
      return GLFW_KEY_KP_MULTIPLY;
    case kNumPadSubtract:
      return GLFW_KEY_KP_SUBTRACT;
    case kNumPadAdd:
      return GLFW_KEY_KP_ADD;
    case kNumPadEnter:
      return GLFW_KEY_KP_ENTER;
    case kNumPadEqual:
      return GLFW_KEY_KP_EQUAL;
    case kLeftShift:
      return GLFW_KEY_LEFT_SHIFT;
    case kLeftControl:
      return GLFW_KEY_LEFT_CONTROL;
    case kLeftAlt:
      return GLFW_KEY_LEFT_ALT;
    case kLeftSuper:
      return GLFW_KEY_LEFT_SUPER;
    case kRightShift:
      return GLFW_KEY_RIGHT_SHIFT;
    case kRightControl:
      return GLFW_KEY_RIGHT_CONTROL;
    case kRightAlt:
      return GLFW_KEY_RIGHT_ALT;
    case kRightSuper:
      return GLFW_KEY_RIGHT_SUPER;
    case kMenu:
      return GLFW_KEY_MENU;
    case kUnknown:
    case kCount:
      return GLFW_KEY_UNKNOWN;
  }
  return GLFW_KEY_UNKNOWN;
}

/**
 * @brief Maps a GLFW mouse button index to `input::MouseButton`.
 * @param button GLFW mouse button (`GLFW_MOUSE_BUTTON_*`)
 * @return Matching button, or empty when out of range
 */
[[nodiscard]] constexpr auto MouseButtonFromGlfw(int button) noexcept
    -> std::optional<input::MouseButton> {
  if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_8) {
    return std::nullopt;
  }
  return static_cast<input::MouseButton>(button);
}

/**
 * @brief Maps GLFW modifier bits to `input::Modifiers`.
 * @param mods GLFW modifier mask (`GLFW_MOD_*`)
 * @return Combined modifier flags
 */
[[nodiscard]] constexpr input::Modifiers ModifiersFromGlfw(int mods) noexcept {
  auto result = input::Modifiers::kNone;
  if ((mods & GLFW_MOD_SHIFT) != 0) {
    result = result | input::Modifiers::kShift;
  }
  if ((mods & GLFW_MOD_CONTROL) != 0) {
    result = result | input::Modifiers::kControl;
  }
  if ((mods & GLFW_MOD_ALT) != 0) {
    result = result | input::Modifiers::kAlt;
  }
  if ((mods & GLFW_MOD_SUPER) != 0) {
    result = result | input::Modifiers::kSuper;
  }
  if ((mods & GLFW_MOD_CAPS_LOCK) != 0) {
    result = result | input::Modifiers::kCapsLock;
  }
  if ((mods & GLFW_MOD_NUM_LOCK) != 0) {
    result = result | input::Modifiers::kNumLock;
  }
  return result;
}

/**
 * @brief Maps a GLFW key/button action to `input::ButtonState`.
 * @param action GLFW action (`GLFW_PRESS` / `GLFW_RELEASE` / `GLFW_REPEAT`)
 * @return Matching button state
 */
[[nodiscard]] constexpr input::ButtonState ButtonStateFromGlfw(
    int action) noexcept {
  switch (action) {
    case GLFW_PRESS:
      return input::ButtonState::kPressed;
    case GLFW_RELEASE:
      return input::ButtonState::kReleased;
    case GLFW_REPEAT:
      return input::ButtonState::kRepeat;
    default:
      return input::ButtonState::kReleased;
  }
}

/**
 * @brief Maps a GLFW gamepad button index to `input::GamepadButton`.
 * @param button GLFW gamepad button (`GLFW_GAMEPAD_BUTTON_*`)
 * @return Matching gamepad button, or `GamepadButton::kA` when out of range
 */
[[nodiscard]] constexpr input::GamepadButton GamepadButtonFromGlfw(
    int button) noexcept {
  if (button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) {
    return input::GamepadButton::kA;
  }
  return static_cast<input::GamepadButton>(button);
}

/**
 * @brief Maps a GLFW gamepad axis index to `input::GamepadAxis`.
 * @param axis GLFW gamepad axis (`GLFW_GAMEPAD_AXIS_*`)
 * @return Matching gamepad axis, or `GamepadAxis::kLeftX` when out of range
 */
[[nodiscard]] constexpr input::GamepadAxis GamepadAxisFromGlfw(
    int axis) noexcept {
  if (axis < 0 || axis >= static_cast<int>(input::GamepadAxis::kCount)) {
    return input::GamepadAxis::kLeftX;
  }
  return static_cast<input::GamepadAxis>(axis);
}

/**
 * @brief Maps an `input::CursorIcon` to a GLFW standard cursor shape.
 * @param icon Cursor icon
 * @return GLFW standard cursor shape (`GLFW_*_CURSOR`)
 */
[[nodiscard]] constexpr int GlfwFromCursorIcon(
    input::CursorIcon icon) noexcept {
  switch (icon) {
    using enum input::CursorIcon;
    case kDefault:
    case kArrow:
      return GLFW_ARROW_CURSOR;
    case kIBeam:
      return GLFW_IBEAM_CURSOR;
    case kCrosshair:
      return GLFW_CROSSHAIR_CURSOR;
    case kPointingHand:
      return GLFW_POINTING_HAND_CURSOR;
    case kResizeEw:
      return GLFW_RESIZE_EW_CURSOR;
    case kResizeNs:
      return GLFW_RESIZE_NS_CURSOR;
    case kResizeNwse:
      return GLFW_RESIZE_NWSE_CURSOR;
    case kResizeNesw:
      return GLFW_RESIZE_NESW_CURSOR;
    case kResizeAll:
      return GLFW_RESIZE_ALL_CURSOR;
    case kNotAllowed:
      return GLFW_NOT_ALLOWED_CURSOR;
    case kCount:
      return GLFW_ARROW_CURSOR;
  }
  return GLFW_ARROW_CURSOR;
}

}  // namespace helios::glfw

#endif  // HELIOS_MODULE_INPUT_AVAILABLE
