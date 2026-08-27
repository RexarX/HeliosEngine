#pragma once

#ifndef HELIOS_BUILDING_MODULE
#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pen.h>
#include <SDL3/SDL_scancode.h>

#include <optional>
#include <utility>
#endif

namespace helios::sdl3::input {

constexpr float kAxisMax = 32767.0F;

/**
 * @brief Maps an SDL scancode to a contiguous `helios::input::Key`.
 * @param scancode SDL physical key code (`SDL_SCANCODE_*`)
 * @return Matching key, or `Key::kUnknown` when unmapped
 */
[[nodiscard]] constexpr helios::input::Key KeyFromSdl(
    SDL_Scancode scancode) noexcept {
  switch (scancode) {
    case SDL_SCANCODE_SPACE:
      return helios::input::Key::kSpace;
    case SDL_SCANCODE_APOSTROPHE:
      return helios::input::Key::kApostrophe;
    case SDL_SCANCODE_COMMA:
      return helios::input::Key::kComma;
    case SDL_SCANCODE_MINUS:
      return helios::input::Key::kMinus;
    case SDL_SCANCODE_PERIOD:
      return helios::input::Key::kPeriod;
    case SDL_SCANCODE_SLASH:
      return helios::input::Key::kSlash;
    case SDL_SCANCODE_0:
      return helios::input::Key::kDigit0;
    case SDL_SCANCODE_1:
      return helios::input::Key::kDigit1;
    case SDL_SCANCODE_2:
      return helios::input::Key::kDigit2;
    case SDL_SCANCODE_3:
      return helios::input::Key::kDigit3;
    case SDL_SCANCODE_4:
      return helios::input::Key::kDigit4;
    case SDL_SCANCODE_5:
      return helios::input::Key::kDigit5;
    case SDL_SCANCODE_6:
      return helios::input::Key::kDigit6;
    case SDL_SCANCODE_7:
      return helios::input::Key::kDigit7;
    case SDL_SCANCODE_8:
      return helios::input::Key::kDigit8;
    case SDL_SCANCODE_9:
      return helios::input::Key::kDigit9;
    case SDL_SCANCODE_SEMICOLON:
      return helios::input::Key::kSemicolon;
    case SDL_SCANCODE_EQUALS:
      return helios::input::Key::kEqual;
    case SDL_SCANCODE_A:
      return helios::input::Key::kA;
    case SDL_SCANCODE_B:
      return helios::input::Key::kB;
    case SDL_SCANCODE_C:
      return helios::input::Key::kC;
    case SDL_SCANCODE_D:
      return helios::input::Key::kD;
    case SDL_SCANCODE_E:
      return helios::input::Key::kE;
    case SDL_SCANCODE_F:
      return helios::input::Key::kF;
    case SDL_SCANCODE_G:
      return helios::input::Key::kG;
    case SDL_SCANCODE_H:
      return helios::input::Key::kH;
    case SDL_SCANCODE_I:
      return helios::input::Key::kI;
    case SDL_SCANCODE_J:
      return helios::input::Key::kJ;
    case SDL_SCANCODE_K:
      return helios::input::Key::kK;
    case SDL_SCANCODE_L:
      return helios::input::Key::kL;
    case SDL_SCANCODE_M:
      return helios::input::Key::kM;
    case SDL_SCANCODE_N:
      return helios::input::Key::kN;
    case SDL_SCANCODE_O:
      return helios::input::Key::kO;
    case SDL_SCANCODE_P:
      return helios::input::Key::kP;
    case SDL_SCANCODE_Q:
      return helios::input::Key::kQ;
    case SDL_SCANCODE_R:
      return helios::input::Key::kR;
    case SDL_SCANCODE_S:
      return helios::input::Key::kS;
    case SDL_SCANCODE_T:
      return helios::input::Key::kT;
    case SDL_SCANCODE_U:
      return helios::input::Key::kU;
    case SDL_SCANCODE_V:
      return helios::input::Key::kV;
    case SDL_SCANCODE_W:
      return helios::input::Key::kW;
    case SDL_SCANCODE_X:
      return helios::input::Key::kX;
    case SDL_SCANCODE_Y:
      return helios::input::Key::kY;
    case SDL_SCANCODE_Z:
      return helios::input::Key::kZ;
    case SDL_SCANCODE_LEFTBRACKET:
      return helios::input::Key::kLeftBracket;
    case SDL_SCANCODE_BACKSLASH:
      return helios::input::Key::kBackslash;
    case SDL_SCANCODE_RIGHTBRACKET:
      return helios::input::Key::kRightBracket;
    case SDL_SCANCODE_GRAVE:
      return helios::input::Key::kGraveAccent;
    case SDL_SCANCODE_INTERNATIONAL1:
      return helios::input::Key::kWorld1;
    case SDL_SCANCODE_INTERNATIONAL2:
      return helios::input::Key::kWorld2;
    case SDL_SCANCODE_ESCAPE:
      return helios::input::Key::kEscape;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_RETURN2:
      return helios::input::Key::kEnter;
    case SDL_SCANCODE_KP_ENTER:
      return helios::input::Key::kNumPadEnter;
    case SDL_SCANCODE_TAB:
      return helios::input::Key::kTab;
    case SDL_SCANCODE_BACKSPACE:
      return helios::input::Key::kBackspace;
    case SDL_SCANCODE_INSERT:
      return helios::input::Key::kInsert;
    case SDL_SCANCODE_DELETE:
      return helios::input::Key::kDelete;
    case SDL_SCANCODE_RIGHT:
      return helios::input::Key::kRight;
    case SDL_SCANCODE_LEFT:
      return helios::input::Key::kLeft;
    case SDL_SCANCODE_DOWN:
      return helios::input::Key::kDown;
    case SDL_SCANCODE_UP:
      return helios::input::Key::kUp;
    case SDL_SCANCODE_PAGEUP:
      return helios::input::Key::kPageUp;
    case SDL_SCANCODE_PAGEDOWN:
      return helios::input::Key::kPageDown;
    case SDL_SCANCODE_HOME:
      return helios::input::Key::kHome;
    case SDL_SCANCODE_END:
      return helios::input::Key::kEnd;
    case SDL_SCANCODE_CAPSLOCK:
      return helios::input::Key::kCapsLock;
    case SDL_SCANCODE_SCROLLLOCK:
      return helios::input::Key::kScrollLock;
    case SDL_SCANCODE_NUMLOCKCLEAR:
      return helios::input::Key::kNumLock;
    case SDL_SCANCODE_PRINTSCREEN:
      return helios::input::Key::kPrintScreen;
    case SDL_SCANCODE_PAUSE:
      return helios::input::Key::kPause;
    case SDL_SCANCODE_F1:
      return helios::input::Key::kF1;
    case SDL_SCANCODE_F2:
      return helios::input::Key::kF2;
    case SDL_SCANCODE_F3:
      return helios::input::Key::kF3;
    case SDL_SCANCODE_F4:
      return helios::input::Key::kF4;
    case SDL_SCANCODE_F5:
      return helios::input::Key::kF5;
    case SDL_SCANCODE_F6:
      return helios::input::Key::kF6;
    case SDL_SCANCODE_F7:
      return helios::input::Key::kF7;
    case SDL_SCANCODE_F8:
      return helios::input::Key::kF8;
    case SDL_SCANCODE_F9:
      return helios::input::Key::kF9;
    case SDL_SCANCODE_F10:
      return helios::input::Key::kF10;
    case SDL_SCANCODE_F11:
      return helios::input::Key::kF11;
    case SDL_SCANCODE_F12:
      return helios::input::Key::kF12;
    case SDL_SCANCODE_F13:
      return helios::input::Key::kF13;
    case SDL_SCANCODE_F14:
      return helios::input::Key::kF14;
    case SDL_SCANCODE_F15:
      return helios::input::Key::kF15;
    case SDL_SCANCODE_F16:
      return helios::input::Key::kF16;
    case SDL_SCANCODE_F17:
      return helios::input::Key::kF17;
    case SDL_SCANCODE_F18:
      return helios::input::Key::kF18;
    case SDL_SCANCODE_F19:
      return helios::input::Key::kF19;
    case SDL_SCANCODE_F20:
      return helios::input::Key::kF20;
    case SDL_SCANCODE_F21:
      return helios::input::Key::kF21;
    case SDL_SCANCODE_F22:
      return helios::input::Key::kF22;
    case SDL_SCANCODE_F23:
      return helios::input::Key::kF23;
    case SDL_SCANCODE_F24:
      return helios::input::Key::kF24;
    case SDL_SCANCODE_KP_0:
      return helios::input::Key::kNumPad0;
    case SDL_SCANCODE_KP_1:
      return helios::input::Key::kNumPad1;
    case SDL_SCANCODE_KP_2:
      return helios::input::Key::kNumPad2;
    case SDL_SCANCODE_KP_3:
      return helios::input::Key::kNumPad3;
    case SDL_SCANCODE_KP_4:
      return helios::input::Key::kNumPad4;
    case SDL_SCANCODE_KP_5:
      return helios::input::Key::kNumPad5;
    case SDL_SCANCODE_KP_6:
      return helios::input::Key::kNumPad6;
    case SDL_SCANCODE_KP_7:
      return helios::input::Key::kNumPad7;
    case SDL_SCANCODE_KP_8:
      return helios::input::Key::kNumPad8;
    case SDL_SCANCODE_KP_9:
      return helios::input::Key::kNumPad9;
    case SDL_SCANCODE_KP_PERIOD:
      return helios::input::Key::kNumPadDecimal;
    case SDL_SCANCODE_KP_DIVIDE:
      return helios::input::Key::kNumPadDivide;
    case SDL_SCANCODE_KP_MULTIPLY:
      return helios::input::Key::kNumPadMultiply;
    case SDL_SCANCODE_KP_MINUS:
      return helios::input::Key::kNumPadSubtract;
    case SDL_SCANCODE_KP_PLUS:
      return helios::input::Key::kNumPadAdd;
    case SDL_SCANCODE_KP_EQUALS:
      return helios::input::Key::kNumPadEqual;
    case SDL_SCANCODE_LSHIFT:
      return helios::input::Key::kLeftShift;
    case SDL_SCANCODE_LCTRL:
      return helios::input::Key::kLeftControl;
    case SDL_SCANCODE_LALT:
      return helios::input::Key::kLeftAlt;
    case SDL_SCANCODE_LGUI:
      return helios::input::Key::kLeftSuper;
    case SDL_SCANCODE_RSHIFT:
      return helios::input::Key::kRightShift;
    case SDL_SCANCODE_RCTRL:
      return helios::input::Key::kRightControl;
    case SDL_SCANCODE_RALT:
      return helios::input::Key::kRightAlt;
    case SDL_SCANCODE_RGUI:
      return helios::input::Key::kRightSuper;
    case SDL_SCANCODE_MENU:
    case SDL_SCANCODE_APPLICATION:
      return helios::input::Key::kMenu;
    default:
      return helios::input::Key::kUnknown;
  }
}

/**
 * @brief Maps an `helios::input::Key` back to an SDL scancode.
 * @param key Contiguous keyboard key
 * @return SDL scancode, or `SDL_SCANCODE_UNKNOWN` when unmapped
 */
[[nodiscard]] constexpr SDL_Scancode SdlFromKey(
    helios::input::Key key) noexcept {
  switch (key) {
    using enum helios::input::Key;
    case kSpace:
      return SDL_SCANCODE_SPACE;
    case kApostrophe:
      return SDL_SCANCODE_APOSTROPHE;
    case kComma:
      return SDL_SCANCODE_COMMA;
    case kMinus:
      return SDL_SCANCODE_MINUS;
    case kPeriod:
      return SDL_SCANCODE_PERIOD;
    case kSlash:
      return SDL_SCANCODE_SLASH;
    case kDigit0:
      return SDL_SCANCODE_0;
    case kDigit1:
      return SDL_SCANCODE_1;
    case kDigit2:
      return SDL_SCANCODE_2;
    case kDigit3:
      return SDL_SCANCODE_3;
    case kDigit4:
      return SDL_SCANCODE_4;
    case kDigit5:
      return SDL_SCANCODE_5;
    case kDigit6:
      return SDL_SCANCODE_6;
    case kDigit7:
      return SDL_SCANCODE_7;
    case kDigit8:
      return SDL_SCANCODE_8;
    case kDigit9:
      return SDL_SCANCODE_9;
    case kSemicolon:
      return SDL_SCANCODE_SEMICOLON;
    case kEqual:
      return SDL_SCANCODE_EQUALS;
    case kA:
      return SDL_SCANCODE_A;
    case kB:
      return SDL_SCANCODE_B;
    case kC:
      return SDL_SCANCODE_C;
    case kD:
      return SDL_SCANCODE_D;
    case kE:
      return SDL_SCANCODE_E;
    case kF:
      return SDL_SCANCODE_F;
    case kG:
      return SDL_SCANCODE_G;
    case kH:
      return SDL_SCANCODE_H;
    case kI:
      return SDL_SCANCODE_I;
    case kJ:
      return SDL_SCANCODE_J;
    case kK:
      return SDL_SCANCODE_K;
    case kL:
      return SDL_SCANCODE_L;
    case kM:
      return SDL_SCANCODE_M;
    case kN:
      return SDL_SCANCODE_N;
    case kO:
      return SDL_SCANCODE_O;
    case kP:
      return SDL_SCANCODE_P;
    case kQ:
      return SDL_SCANCODE_Q;
    case kR:
      return SDL_SCANCODE_R;
    case kS:
      return SDL_SCANCODE_S;
    case kT:
      return SDL_SCANCODE_T;
    case kU:
      return SDL_SCANCODE_U;
    case kV:
      return SDL_SCANCODE_V;
    case kW:
      return SDL_SCANCODE_W;
    case kX:
      return SDL_SCANCODE_X;
    case kY:
      return SDL_SCANCODE_Y;
    case kZ:
      return SDL_SCANCODE_Z;
    case kLeftBracket:
      return SDL_SCANCODE_LEFTBRACKET;
    case kBackslash:
      return SDL_SCANCODE_BACKSLASH;
    case kRightBracket:
      return SDL_SCANCODE_RIGHTBRACKET;
    case kGraveAccent:
      return SDL_SCANCODE_GRAVE;
    case kWorld1:
      return SDL_SCANCODE_INTERNATIONAL1;
    case kWorld2:
      return SDL_SCANCODE_INTERNATIONAL2;
    case kEscape:
      return SDL_SCANCODE_ESCAPE;
    case kEnter:
      return SDL_SCANCODE_RETURN;
    case kTab:
      return SDL_SCANCODE_TAB;
    case kBackspace:
      return SDL_SCANCODE_BACKSPACE;
    case kInsert:
      return SDL_SCANCODE_INSERT;
    case kDelete:
      return SDL_SCANCODE_DELETE;
    case kRight:
      return SDL_SCANCODE_RIGHT;
    case kLeft:
      return SDL_SCANCODE_LEFT;
    case kDown:
      return SDL_SCANCODE_DOWN;
    case kUp:
      return SDL_SCANCODE_UP;
    case kPageUp:
      return SDL_SCANCODE_PAGEUP;
    case kPageDown:
      return SDL_SCANCODE_PAGEDOWN;
    case kHome:
      return SDL_SCANCODE_HOME;
    case kEnd:
      return SDL_SCANCODE_END;
    case kCapsLock:
      return SDL_SCANCODE_CAPSLOCK;
    case kScrollLock:
      return SDL_SCANCODE_SCROLLLOCK;
    case kNumLock:
      return SDL_SCANCODE_NUMLOCKCLEAR;
    case kPrintScreen:
      return SDL_SCANCODE_PRINTSCREEN;
    case kPause:
      return SDL_SCANCODE_PAUSE;
    case kF1:
      return SDL_SCANCODE_F1;
    case kF2:
      return SDL_SCANCODE_F2;
    case kF3:
      return SDL_SCANCODE_F3;
    case kF4:
      return SDL_SCANCODE_F4;
    case kF5:
      return SDL_SCANCODE_F5;
    case kF6:
      return SDL_SCANCODE_F6;
    case kF7:
      return SDL_SCANCODE_F7;
    case kF8:
      return SDL_SCANCODE_F8;
    case kF9:
      return SDL_SCANCODE_F9;
    case kF10:
      return SDL_SCANCODE_F10;
    case kF11:
      return SDL_SCANCODE_F11;
    case kF12:
      return SDL_SCANCODE_F12;
    case kF13:
      return SDL_SCANCODE_F13;
    case kF14:
      return SDL_SCANCODE_F14;
    case kF15:
      return SDL_SCANCODE_F15;
    case kF16:
      return SDL_SCANCODE_F16;
    case kF17:
      return SDL_SCANCODE_F17;
    case kF18:
      return SDL_SCANCODE_F18;
    case kF19:
      return SDL_SCANCODE_F19;
    case kF20:
      return SDL_SCANCODE_F20;
    case kF21:
      return SDL_SCANCODE_F21;
    case kF22:
      return SDL_SCANCODE_F22;
    case kF23:
      return SDL_SCANCODE_F23;
    case kF24:
      return SDL_SCANCODE_F24;
    case kF25:
      return SDL_SCANCODE_UNKNOWN;
    case kNumPad0:
      return SDL_SCANCODE_KP_0;
    case kNumPad1:
      return SDL_SCANCODE_KP_1;
    case kNumPad2:
      return SDL_SCANCODE_KP_2;
    case kNumPad3:
      return SDL_SCANCODE_KP_3;
    case kNumPad4:
      return SDL_SCANCODE_KP_4;
    case kNumPad5:
      return SDL_SCANCODE_KP_5;
    case kNumPad6:
      return SDL_SCANCODE_KP_6;
    case kNumPad7:
      return SDL_SCANCODE_KP_7;
    case kNumPad8:
      return SDL_SCANCODE_KP_8;
    case kNumPad9:
      return SDL_SCANCODE_KP_9;
    case kNumPadDecimal:
      return SDL_SCANCODE_KP_PERIOD;
    case kNumPadDivide:
      return SDL_SCANCODE_KP_DIVIDE;
    case kNumPadMultiply:
      return SDL_SCANCODE_KP_MULTIPLY;
    case kNumPadSubtract:
      return SDL_SCANCODE_KP_MINUS;
    case kNumPadAdd:
      return SDL_SCANCODE_KP_PLUS;
    case kNumPadEnter:
      return SDL_SCANCODE_KP_ENTER;
    case kNumPadEqual:
      return SDL_SCANCODE_KP_EQUALS;
    case kLeftShift:
      return SDL_SCANCODE_LSHIFT;
    case kLeftControl:
      return SDL_SCANCODE_LCTRL;
    case kLeftAlt:
      return SDL_SCANCODE_LALT;
    case kLeftSuper:
      return SDL_SCANCODE_LGUI;
    case kRightShift:
      return SDL_SCANCODE_RSHIFT;
    case kRightControl:
      return SDL_SCANCODE_RCTRL;
    case kRightAlt:
      return SDL_SCANCODE_RALT;
    case kRightSuper:
      return SDL_SCANCODE_RGUI;
    case kMenu:
      return SDL_SCANCODE_MENU;
    case kUnknown:
    case kCount:
      return SDL_SCANCODE_UNKNOWN;
  }
  return SDL_SCANCODE_UNKNOWN;
}

/**
 * @brief Maps an SDL mouse button index to `helios::input::MouseButton`.
 * @param button SDL mouse button (`SDL_BUTTON_*`)
 * @return Matching button, or empty when out of range
 */
[[nodiscard]] constexpr auto MouseButtonFromSdl(Uint8 button) noexcept
    -> std::optional<helios::input::MouseButton> {
  switch (button) {
    case SDL_BUTTON_LEFT:
      return helios::input::MouseButton::kLeft;
    case SDL_BUTTON_RIGHT:
      return helios::input::MouseButton::kRight;
    case SDL_BUTTON_MIDDLE:
      return helios::input::MouseButton::kMiddle;
    case SDL_BUTTON_X1:
      return helios::input::MouseButton::kExtra1;
    case SDL_BUTTON_X2:
      return helios::input::MouseButton::kExtra2;
    default:
      if (button >= SDL_BUTTON_LEFT && button <= SDL_BUTTON_X2 + 3) {
        const auto index = static_cast<size_t>(button - SDL_BUTTON_LEFT);
        if (index < std::to_underlying(helios::input::MouseButton::kCount)) {
          return static_cast<helios::input::MouseButton>(index);
        }
      }
      return std::nullopt;
  }
}

/**
 * @brief Maps SDL modifier bits to `helios::input::Modifiers`.
 * @param mods SDL key modifier mask (`SDL_KMOD_*`)
 * @return Combined modifier flags
 */
[[nodiscard]] constexpr helios::input::Modifiers ModifiersFromSdl(
    SDL_Keymod mods) noexcept {
  auto result = helios::input::Modifiers::kNone;
  if ((mods & SDL_KMOD_SHIFT) != 0) {
    result = result | helios::input::Modifiers::kShift;
  }
  if ((mods & SDL_KMOD_CTRL) != 0) {
    result = result | helios::input::Modifiers::kControl;
  }
  if ((mods & SDL_KMOD_ALT) != 0) {
    result = result | helios::input::Modifiers::kAlt;
  }
  if ((mods & SDL_KMOD_GUI) != 0) {
    result = result | helios::input::Modifiers::kSuper;
  }
  if ((mods & SDL_KMOD_CAPS) != 0) {
    result = result | helios::input::Modifiers::kCapsLock;
  }
  if ((mods & SDL_KMOD_NUM) != 0) {
    result = result | helios::input::Modifiers::kNumLock;
  }
  return result;
}

/**
 * @brief Maps SDL keyboard/mouse press state to `helios::input::ButtonState`.
 * @param down True when the button/key is pressed
 * @param repeat True when the key event is a repeat
 * @return Matching button state
 */
[[nodiscard]] constexpr helios::input::ButtonState ButtonStateFromSdl(
    bool down, bool repeat) noexcept {
  if (!down) {
    return helios::input::ButtonState::kReleased;
  }
  if (repeat) {
    return helios::input::ButtonState::kRepeat;
  }
  return helios::input::ButtonState::kPressed;
}

/**
 * @brief Maps an SDL gamepad button to `helios::input::GamepadButton`.
 * @param button SDL gamepad button (`SDL_GAMEPAD_BUTTON_*`)
 * @return Matching gamepad button, or empty when unmapped
 */
[[nodiscard]] constexpr std::optional<helios::input::GamepadButton>
GamepadButtonFromSdl(SDL_GamepadButton button) noexcept {
  switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH:
      return helios::input::GamepadButton::kA;
    case SDL_GAMEPAD_BUTTON_EAST:
      return helios::input::GamepadButton::kB;
    case SDL_GAMEPAD_BUTTON_WEST:
      return helios::input::GamepadButton::kX;
    case SDL_GAMEPAD_BUTTON_NORTH:
      return helios::input::GamepadButton::kY;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
      return helios::input::GamepadButton::kLeftBumper;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
      return helios::input::GamepadButton::kRightBumper;
    case SDL_GAMEPAD_BUTTON_BACK:
      return helios::input::GamepadButton::kBack;
    case SDL_GAMEPAD_BUTTON_START:
      return helios::input::GamepadButton::kStart;
    case SDL_GAMEPAD_BUTTON_GUIDE:
      return helios::input::GamepadButton::kGuide;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
      return helios::input::GamepadButton::kLeftThumb;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
      return helios::input::GamepadButton::kRightThumb;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
      return helios::input::GamepadButton::kDpadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
      return helios::input::GamepadButton::kDpadRight;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
      return helios::input::GamepadButton::kDpadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
      return helios::input::GamepadButton::kDpadLeft;
    case SDL_GAMEPAD_BUTTON_MISC1:
      return helios::input::GamepadButton::kMisc1;
    case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
      return helios::input::GamepadButton::kRightPaddle1;
    case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
      return helios::input::GamepadButton::kLeftPaddle1;
    case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
      return helios::input::GamepadButton::kRightPaddle2;
    case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
      return helios::input::GamepadButton::kLeftPaddle2;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
      return helios::input::GamepadButton::kTouchpad;
    case SDL_GAMEPAD_BUTTON_MISC2:
      return helios::input::GamepadButton::kMisc2;
    case SDL_GAMEPAD_BUTTON_MISC3:
      return helios::input::GamepadButton::kMisc3;
    case SDL_GAMEPAD_BUTTON_MISC4:
      return helios::input::GamepadButton::kMisc4;
    case SDL_GAMEPAD_BUTTON_MISC5:
      return helios::input::GamepadButton::kMisc5;
    case SDL_GAMEPAD_BUTTON_MISC6:
      return helios::input::GamepadButton::kMisc6;
    case SDL_GAMEPAD_BUTTON_INVALID:
    default:
      return std::nullopt;
  }
}

/**
 * @brief Maps an SDL gamepad axis to `helios::input::GamepadAxis`.
 * @param axis SDL gamepad axis (`SDL_GAMEPAD_AXIS_*`)
 * @return Matching gamepad axis, or `GamepadAxis::kLeftX` when out of range
 */
[[nodiscard]] constexpr helios::input::GamepadAxis GamepadAxisFromSdl(
    SDL_GamepadAxis axis) noexcept {
  switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX:
      return helios::input::GamepadAxis::kLeftX;
    case SDL_GAMEPAD_AXIS_LEFTY:
      return helios::input::GamepadAxis::kLeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX:
      return helios::input::GamepadAxis::kRightX;
    case SDL_GAMEPAD_AXIS_RIGHTY:
      return helios::input::GamepadAxis::kRightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
      return helios::input::GamepadAxis::kLeftTrigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
      return helios::input::GamepadAxis::kRightTrigger;
    default:
      if (axis >= 0 && axis < static_cast<SDL_GamepadAxis>(
                                  helios::input::GamepadAxis::kCount)) {
        return static_cast<helios::input::GamepadAxis>(axis);
      }
      return helios::input::GamepadAxis::kLeftX;
  }
}

/**
 * @brief Maps an `helios::input::CursorIcon` to an SDL standard cursor shape.
 * @param icon Cursor icon
 * @return SDL standard cursor shape (`SDL_SYSTEM_CURSOR_*`)
 */
[[nodiscard]] constexpr SDL_SystemCursor SdlFromCursorIcon(
    helios::input::CursorIcon icon) noexcept {
  {
    switch (icon) {
      using enum helios::input::CursorIcon;
      case kDefault:
      case kArrow:
        return SDL_SYSTEM_CURSOR_DEFAULT;
      case kIBeam:
        return SDL_SYSTEM_CURSOR_TEXT;
      case kCrosshair:
        return SDL_SYSTEM_CURSOR_CROSSHAIR;
      case kPointingHand:
        return SDL_SYSTEM_CURSOR_POINTER;
      case kResizeEw:
        return SDL_SYSTEM_CURSOR_EW_RESIZE;
      case kResizeNs:
        return SDL_SYSTEM_CURSOR_NS_RESIZE;
      case kResizeNwse:
        return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
      case kResizeNesw:
        return SDL_SYSTEM_CURSOR_NESW_RESIZE;
      case kResizeAll:
        return SDL_SYSTEM_CURSOR_MOVE;
      case kNotAllowed:
        return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
      case kCount:
        return SDL_SYSTEM_CURSOR_DEFAULT;
    }
    return SDL_SYSTEM_CURSOR_DEFAULT;
  }
}

/**
 * @brief Normalizes an SDL stick axis sample to the GLFW gamepad range.
 * @param value Raw SDL axis sample
 * @return Value in `[-1, 1]`
 */
[[nodiscard]] constexpr float NormalizeStickAxis(Sint16 value) noexcept {
  return static_cast<float>(value) / kAxisMax;
}

/**
 * @brief Normalizes an SDL trigger axis sample to the GLFW gamepad range.
 * @param value Raw SDL trigger sample (`0` released to max pressed)
 * @return Value in `[-1, 1]` with rest at `-1`
 */
[[nodiscard]] constexpr float NormalizeTriggerAxis(Sint16 value) noexcept {
  const float normalized = static_cast<float>(value) / kAxisMax;
  return (normalized * 2.0F) - 1.0F;
}

/**
 * @brief Maps an SDL pen barrel button index to `helios::input::PenButton`.
 * @param button SDL pen button (`1`–`5`)
 * @return Matching button, or empty when out of range
 */
[[nodiscard]] constexpr auto PenButtonFromSdl(Uint8 button) noexcept
    -> std::optional<helios::input::PenButton> {
  if (button < 1 || button > 5) {
    return std::nullopt;
  }
  return static_cast<helios::input::PenButton>(button - 1);
}

/**
 * @brief Maps an SDL pen axis to `helios::input::PenAxis`.
 * @param axis SDL pen axis (`SDL_PEN_AXIS_*`)
 * @return Matching pen axis, or empty when out of range
 */
[[nodiscard]] constexpr auto PenAxisFromSdl(SDL_PenAxis axis) noexcept
    -> std::optional<helios::input::PenAxis> {
  if (axis < 0 || axis >= SDL_PEN_AXIS_COUNT) {
    return std::nullopt;
  }
  return static_cast<helios::input::PenAxis>(axis);
}

/**
 * @brief Maps an SDL pen device type to `helios::input::PenDeviceType`.
 * @param type SDL pen device type
 * @return Matching device type (`kUnknown` when invalid)
 */
[[nodiscard]] constexpr helios::input::PenDeviceType PenDeviceTypeFromSdl(
    SDL_PenDeviceType type) noexcept {
  switch (type) {
    case SDL_PEN_DEVICE_TYPE_DIRECT:
      return helios::input::PenDeviceType::kDirect;
    case SDL_PEN_DEVICE_TYPE_INDIRECT:
      return helios::input::PenDeviceType::kIndirect;
    case SDL_PEN_DEVICE_TYPE_UNKNOWN:
    case SDL_PEN_DEVICE_TYPE_INVALID:
    default:
      return helios::input::PenDeviceType::kUnknown;
  }
}

}  // namespace helios::sdl3::input
