#pragma once

#include <helios/memory/temporary_storage.hpp>

#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace helios::input {

/// @brief Contiguous keyboard key identifiers (backend mapping is separate).
enum class Key : uint8_t {
  kUnknown = 0,
  kSpace,
  kApostrophe,
  kComma,
  kMinus,
  kPeriod,
  kSlash,
  kDigit0,
  kDigit1,
  kDigit2,
  kDigit3,
  kDigit4,
  kDigit5,
  kDigit6,
  kDigit7,
  kDigit8,
  kDigit9,
  kSemicolon,
  kEqual,
  kA,
  kB,
  kC,
  kD,
  kE,
  kF,
  kG,
  kH,
  kI,
  kJ,
  kK,
  kL,
  kM,
  kN,
  kO,
  kP,
  kQ,
  kR,
  kS,
  kT,
  kU,
  kV,
  kW,
  kX,
  kY,
  kZ,
  kLeftBracket,
  kBackslash,
  kRightBracket,
  kGraveAccent,
  kWorld1,
  kWorld2,
  kEscape,
  kEnter,
  kTab,
  kBackspace,
  kInsert,
  kDelete,
  kRight,
  kLeft,
  kDown,
  kUp,
  kPageUp,
  kPageDown,
  kHome,
  kEnd,
  kCapsLock,
  kScrollLock,
  kNumLock,
  kPrintScreen,
  kPause,
  kF1,
  kF2,
  kF3,
  kF4,
  kF5,
  kF6,
  kF7,
  kF8,
  kF9,
  kF10,
  kF11,
  kF12,
  kF13,
  kF14,
  kF15,
  kF16,
  kF17,
  kF18,
  kF19,
  kF20,
  kF21,
  kF22,
  kF23,
  kF24,
  kF25,
  kNumPad0,
  kNumPad1,
  kNumPad2,
  kNumPad3,
  kNumPad4,
  kNumPad5,
  kNumPad6,
  kNumPad7,
  kNumPad8,
  kNumPad9,
  kNumPadDecimal,
  kNumPadDivide,
  kNumPadMultiply,
  kNumPadSubtract,
  kNumPadAdd,
  kNumPadEnter,
  kNumPadEqual,
  kLeftShift,
  kLeftControl,
  kLeftAlt,
  kLeftSuper,
  kRightShift,
  kRightControl,
  kRightAlt,
  kRightSuper,
  kMenu,
  kCount,
};

/// @brief Bitmask of keyboard modifier keys and lock states.
enum class Modifiers : uint8_t {
  kNone = 0,
  kShift = 1U << 0U,
  kControl = 1U << 1U,
  kAlt = 1U << 2U,
  kSuper = 1U << 3U,
  kCapsLock = 1U << 4U,
  kNumLock = 1U << 5U,
};

/// @brief Physical button / key action reported by an input backend.
enum class ButtonState : uint8_t {
  kReleased = 0,
  kPressed = 1,
  kRepeat = 2,
};

/**
 * @brief Returns the string name of a keyboard key.
 * @param key Keyboard key
 * @return String name of the key
 */
[[nodiscard]] constexpr std::string_view ToString(Key key) noexcept {
  switch (key) {
    using enum Key;
    case kUnknown:
      return "Unknown";
    case kSpace:
      return "Space";
    case kApostrophe:
      return "Apostrophe";
    case kComma:
      return "Comma";
    case kMinus:
      return "Minus";
    case kPeriod:
      return "Period";
    case kSlash:
      return "Slash";
    case kDigit0:
      return "Digit0";
    case kDigit1:
      return "Digit1";
    case kDigit2:
      return "Digit2";
    case kDigit3:
      return "Digit3";
    case kDigit4:
      return "Digit4";
    case kDigit5:
      return "Digit5";
    case kDigit6:
      return "Digit6";
    case kDigit7:
      return "Digit7";
    case kDigit8:
      return "Digit8";
    case kDigit9:
      return "Digit9";
    case kSemicolon:
      return "Semicolon";
    case kEqual:
      return "Equal";
    case kA:
      return "A";
    case kB:
      return "B";
    case kC:
      return "C";
    case kD:
      return "D";
    case kE:
      return "E";
    case kF:
      return "F";
    case kG:
      return "G";
    case kH:
      return "H";
    case kI:
      return "I";
    case kJ:
      return "J";
    case kK:
      return "K";
    case kL:
      return "L";
    case kM:
      return "M";
    case kN:
      return "N";
    case kO:
      return "O";
    case kP:
      return "P";
    case kQ:
      return "Q";
    case kR:
      return "R";
    case kS:
      return "S";
    case kT:
      return "T";
    case kU:
      return "U";
    case kV:
      return "V";
    case kW:
      return "W";
    case kX:
      return "X";
    case kY:
      return "Y";
    case kZ:
      return "Z";
    case kLeftBracket:
      return "LeftBracket";
    case kBackslash:
      return "Backslash";
    case kRightBracket:
      return "RightBracket";
    case kGraveAccent:
      return "GraveAccent";
    case kWorld1:
      return "World1";
    case kWorld2:
      return "World2";
    case kEscape:
      return "Escape";
    case kEnter:
      return "Enter";
    case kTab:
      return "Tab";
    case kBackspace:
      return "Backspace";
    case kInsert:
      return "Insert";
    case kDelete:
      return "Delete";
    case kRight:
      return "Right";
    case kLeft:
      return "Left";
    case kDown:
      return "Down";
    case kUp:
      return "Up";
    case kPageUp:
      return "PageUp";
    case kPageDown:
      return "PageDown";
    case kHome:
      return "Home";
    case kEnd:
      return "End";
    case kCapsLock:
      return "CapsLock";
    case kScrollLock:
      return "ScrollLock";
    case kNumLock:
      return "NumLock";
    case kPrintScreen:
      return "PrintScreen";
    case kPause:
      return "Pause";
    case kF1:
      return "F1";
    case kF2:
      return "F2";
    case kF3:
      return "F3";
    case kF4:
      return "F4";
    case kF5:
      return "F5";
    case kF6:
      return "F6";
    case kF7:
      return "F7";
    case kF8:
      return "F8";
    case kF9:
      return "F9";
    case kF10:
      return "F10";
    case kF11:
      return "F11";
    case kF12:
      return "F12";
    case kF13:
      return "F13";
    case kF14:
      return "F14";
    case kF15:
      return "F15";
    case kF16:
      return "F16";
    case kF17:
      return "F17";
    case kF18:
      return "F18";
    case kF19:
      return "F19";
    case kF20:
      return "F20";
    case kF21:
      return "F21";
    case kF22:
      return "F22";
    case kF23:
      return "F23";
    case kF24:
      return "F24";
    case kF25:
      return "F25";
    case kNumPad0:
      return "NumPad0";
    case kNumPad1:
      return "NumPad1";
    case kNumPad2:
      return "NumPad2";
    case kNumPad3:
      return "NumPad3";
    case kNumPad4:
      return "NumPad4";
    case kNumPad5:
      return "NumPad5";
    case kNumPad6:
      return "NumPad6";
    case kNumPad7:
      return "NumPad7";
    case kNumPad8:
      return "NumPad8";
    case kNumPad9:
      return "NumPad9";
    case kNumPadDecimal:
      return "NumPadDecimal";
    case kNumPadDivide:
      return "NumPadDivide";
    case kNumPadMultiply:
      return "NumPadMultiply";
    case kNumPadSubtract:
      return "NumPadSubtract";
    case kNumPadAdd:
      return "NumPadAdd";
    case kNumPadEnter:
      return "NumPadEnter";
    case kNumPadEqual:
      return "NumPadEqual";
    case kLeftShift:
      return "LeftShift";
    case kLeftControl:
      return "LeftControl";
    case kLeftAlt:
      return "LeftAlt";
    case kLeftSuper:
      return "LeftSuper";
    case kRightShift:
      return "RightShift";
    case kRightControl:
      return "RightControl";
    case kRightAlt:
      return "RightAlt";
    case kRightSuper:
      return "RightSuper";
    case kMenu:
      return "Menu";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a keyboard key to an output stream.
 * @param os Output stream
 * @param key Keyboard key
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, Key key) {
  return os << "Key::" << ToString(key);
}

/**
 * @brief Combines modifier flags.
 * @param lhs Left-hand side modifiers
 * @param rhs Right-hand side modifiers
 * @return Combined modifiers
 */
[[nodiscard]] constexpr Modifiers operator|(Modifiers lhs,
                                            Modifiers rhs) noexcept {
  return static_cast<Modifiers>(std::to_underlying(lhs) |
                                std::to_underlying(rhs));
}

/**
 * @brief Tests whether a modifier flag is set.
 * @param modifiers Combined modifier flags
 * @param flag Flag to test
 * @return True if the flag is set, false otherwise
 */
[[nodiscard]] constexpr bool HasFlag(Modifiers modifiers,
                                     Modifiers flag) noexcept {
  return (std::to_underlying(modifiers) & std::to_underlying(flag)) != 0U;
}

/**
 * @brief Formats modifiers as a pipe-separated list and writes to an output
 * iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param modifiers Combined modifier flags
 * @param with_prefix Whether to include a "Modifiers::" prefix for each flag
 * @return Updated output iterator after writing the formatted string
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, Modifiers modifiers, bool with_prefix = false) {
  const std::string_view kNoneStr = with_prefix ? "Modifiers::None" : "None";

  if (modifiers == Modifiers::kNone) {
    return std::format_to(out, "{}", kNoneStr);
  }

  bool first = true;
  const auto append = [modifiers, &out, &first, with_prefix](
                          std::string_view name, Modifiers flag) {
    if (!HasFlag(modifiers, flag)) {
      return;
    }
    if (!first) {
      out = std::format_to(out, " | ");
    }
    first = false;
    out = with_prefix ? std::format_to(out, "Modifiers::{}", name)
                      : std::format_to(out, "{}", name);
  };

  append("Shift", Modifiers::kShift);
  append("Control", Modifiers::kControl);
  append("Alt", Modifiers::kAlt);
  append("Super", Modifiers::kSuper);
  append("CapsLock", Modifiers::kCapsLock);
  append("NumLock", Modifiers::kNumLock);
  return out;
}

/**
 * @brief Formats modifiers as a pipe-separated list of flag names.
 * @param modifiers Combined modifier flags
 * @param with_prefix Whether to include a "Modifiers::" prefix for each flag
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(Modifiers modifiers,
                                          bool with_prefix = false) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), modifiers, with_prefix);
  return result;
}

/**
 * @brief Formats modifiers as a pipe-separated list of flag names using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param modifiers Combined modifier flags
 * @param with_prefix Whether to include a "Modifiers::" prefix for each flag
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(Modifiers modifiers,
                                                   bool with_prefix = false) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), modifiers, with_prefix);
  return result;
}

/**
 * @brief Outputs modifiers to an output stream.
 * @param os Output stream
 * @param modifiers Combined modifier flags
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, Modifiers modifiers) {
  ToString(std::ostreambuf_iterator<char>(os), modifiers);
  return os;
}

/**
 * @brief Returns the string name of a button state.
 * @param state Button state
 * @return String name of the state
 */
[[nodiscard]] constexpr std::string_view ToString(ButtonState state) noexcept {
  switch (state) {
    using enum ButtonState;
    case kReleased:
      return "Released";
    case kPressed:
      return "Pressed";
    case kRepeat:
      return "Repeat";
  }
  return "unknown";
}

/**
 * @brief Outputs a button state to an output stream.
 * @param os Output stream
 * @param state Button state
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ButtonState state) {
  return os << "ButtonState::" << ToString(state);
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::Key> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::Key key, format_context& ctx) {
    return format_to(ctx.out(), "Key::{}", helios::input::ToString(key));
  }
};

template <>
struct formatter<helios::input::Modifiers> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(helios::input::Modifiers modifiers, format_context& ctx) {
    return helios::input::ToString(ctx.out(), modifiers,
                                   /*with_prefix=*/true);
  }
};

template <>
struct formatter<helios::input::ButtonState> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::ButtonState state,
                               format_context& ctx) {
    return format_to(ctx.out(), "ButtonState::{}",
                     helios::input::ToString(state));
  }
};

}  // namespace std
