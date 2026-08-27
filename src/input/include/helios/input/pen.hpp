#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#endif
#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>
#include <helios/input/ids.hpp>
#include <helios/input/keyboard.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Pen barrel buttons (SDL buttons 1–5). Tip contact is not a button.
enum class PenButton : uint8_t {
  kBarrel1 = 0,
  kBarrel2,
  kBarrel3,
  kBarrel4,
  kBarrel5,
  kCount,
};

/// @brief Pen axes (SDL order and ranges).
enum class PenAxis : uint8_t {
  kPressure = 0,
  kXTilt,
  kYTilt,
  kDistance,
  kRotation,
  kSlider,
  kTangentialPressure,
  kCount,
};

/// @brief Whether the pen touches a display or an external tablet.
enum class PenDeviceType : uint8_t {
  kUnknown = 0,
  kDirect,
  kIndirect,
};

using PenButtonInput = ButtonInput<PenButton>;

/// @brief Snapshot of one pen / stylus slot.
struct Pen {
  /// @brief Clears identity, buttons, axes, position, and tip state.
  void Reset() noexcept;

  double position_x = 0.0;
  double position_y = 0.0;
  double delta_x = 0.0;
  double delta_y = 0.0;
  Axis<PenAxis> axes;
  PenButtonInput buttons;
  std::optional<PenId> id;
  PenDeviceType device_type = PenDeviceType::kUnknown;
  bool in_proximity = false;
  bool down = false;
  bool eraser = false;
  bool has_position = false;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {position_x, position_y};
  }

  /**
   * @brief Returns the per-frame motion delta.
   * @return Delta x and y
   */
  [[nodiscard]] constexpr auto GetDelta() const noexcept
      -> std::pair<double, double> {
    return {delta_x, delta_y};
  }
};

inline void Pen::Reset() noexcept {
  position_x = 0.0;
  position_y = 0.0;
  delta_x = 0.0;
  delta_y = 0.0;
  axes.Clear();
  buttons.Reset();
  id.reset();
  device_type = PenDeviceType::kUnknown;
  in_proximity = false;
  down = false;
  eraser = false;
  has_position = false;
}

/// @brief Fixed pen slot table (independent of gamepad / joystick ids).
struct Pens {
  static constexpr std::string_view kName = "helios::input::Pens";
  static constexpr size_t kSlotCount = 8;

  std::array<Pen, kSlotCount> pens = {};

  /**
   * @brief Looks up a mutable pen slot by id.
   * @param id Pen slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr Pen* TryGet(PenId id) noexcept;

  /**
   * @brief Looks up a const pen slot by id.
   * @param id Pen slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Pen* TryGet(PenId id) const noexcept;
};

constexpr Pen* Pens::TryGet(PenId id) noexcept {
  if (static_cast<size_t>(id) >= pens.size()) {
    return nullptr;
  }
  return &pens[static_cast<size_t>(id)];
}

constexpr const Pen* Pens::TryGet(PenId id) const noexcept {
  if (static_cast<size_t>(id) >= pens.size()) {
    return nullptr;
  }
  return &pens[static_cast<size_t>(id)];
}

/// @brief Pen proximity in / out notification for a window entity.
struct PenProximityMsg {
  static constexpr std::string_view kName = "helios::input::PenProximityMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  PenId id = 0;
  PenDeviceType device_type = PenDeviceType::kUnknown;
  bool in_proximity = false;
};

/// @brief Pen tip down / up event for a window entity.
struct PenTouchMsg {
  static constexpr std::string_view kName = "helios::input::PenTouchMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  PenId id = 0;
  bool down = false;
  bool eraser = false;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/// @brief Pen barrel-button press / release event for a window entity.
struct PenButtonInputMsg {
  static constexpr std::string_view kName = "helios::input::PenButtonInputMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  PenId id = 0;
  PenButton button = PenButton::kBarrel1;
  ButtonState state = ButtonState::kReleased;
};

/// @brief Absolute pen position change for a window entity.
struct PenMovedMsg {
  static constexpr std::string_view kName = "helios::input::PenMovedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  PenId id = 0;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/// @brief Pen axis value change for a window entity.
struct PenAxisChangedMsg {
  static constexpr std::string_view kName = "helios::input::PenAxisChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  double x = 0.0;
  double y = 0.0;
  float value = 0.0F;
  PenId id = 0;
  PenAxis axis = PenAxis::kPressure;

  /**
   * @brief Returns the pen position.
   * @return Window-relative x and y
   */
  [[nodiscard]] constexpr auto GetPosition() const noexcept
      -> std::pair<double, double> {
    return {x, y};
  }
};

/**
 * @brief Returns the string name of a pen button.
 * @param button Pen button
 * @return String name of the button
 */
[[nodiscard]] constexpr std::string_view ToString(PenButton button) noexcept {
  switch (button) {
    using enum PenButton;
    case kBarrel1:
      return "Barrel1";
    case kBarrel2:
      return "Barrel2";
    case kBarrel3:
      return "Barrel3";
    case kBarrel4:
      return "Barrel4";
    case kBarrel5:
      return "Barrel5";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a pen button to an output stream.
 * @param os Output stream
 * @param button Pen button
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PenButton button) {
  return os << "PenButton::" << ToString(button);
}

/**
 * @brief Returns the string name of a pen axis.
 * @param axis Pen axis
 * @return String name of the axis
 */
[[nodiscard]] constexpr std::string_view ToString(PenAxis axis) noexcept {
  switch (axis) {
    using enum PenAxis;
    case kPressure:
      return "Pressure";
    case kXTilt:
      return "XTilt";
    case kYTilt:
      return "YTilt";
    case kDistance:
      return "Distance";
    case kRotation:
      return "Rotation";
    case kSlider:
      return "Slider";
    case kTangentialPressure:
      return "TangentialPressure";
    case kCount:
      return "Count";
  }
  return "unknown";
}

/**
 * @brief Outputs a pen axis to an output stream.
 * @param os Output stream
 * @param axis Pen axis
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PenAxis axis) {
  return os << "PenAxis::" << ToString(axis);
}

/**
 * @brief Returns the string name of a pen device type.
 * @param type Pen device type
 * @return String name of the type
 */
[[nodiscard]] constexpr std::string_view ToString(PenDeviceType type) noexcept {
  switch (type) {
    using enum PenDeviceType;
    case kUnknown:
      return "Unknown";
    case kDirect:
      return "Direct";
    case kIndirect:
      return "Indirect";
  }
  return "unknown";
}

/**
 * @brief Outputs a pen device type to an output stream.
 * @param os Output stream
 * @param type Pen device type
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PenDeviceType type) {
  return os << "PenDeviceType::" << ToString(type);
}

/**
 * @brief Formats a pen snapshot using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param pen Pen snapshot
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Pen& pen) {
  out = std::format_to(
      out, "Pen{{position=({}, {}), delta=({}, {}), id=", pen.position_x,
      pen.position_y, pen.delta_x, pen.delta_y);
  if (pen.id.has_value()) {
    out = std::format_to(out, "{}", *pen.id);
  } else {
    out = std::format_to(out, "none");
  }
  return std::format_to(
      out, ", device_type={}, in_proximity={}, down={}, eraser={}}}",
      ToString(pen.device_type), pen.in_proximity, pen.down, pen.eraser);
}

/**
 * @brief Formats a pen snapshot as a string.
 * @param pen Pen snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Pen& pen) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), pen);
  return result;
}

/**
 * @brief Formats a pen snapshot as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param pen Pen snapshot
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Pen& pen) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), pen);
  return result;
}

/**
 * @brief Outputs a pen snapshot to an output stream.
 * @param os Output stream
 * @param pen Pen snapshot
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Pen& pen) {
  ToString(std::ostreambuf_iterator<char>(os), pen);
  return os;
}

/**
 * @brief Formats a pen slot table using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param pens Pens resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Pens& pens) {
  out = std::format_to(out, "Pens{{sticks = [");

  size_t cnt = 0;
  for (const auto& pen : pens.pens) {
    if (!pen.in_proximity) {
      continue;
    }

    if (cnt++ > 0) [[likely]] {
      out = std::format_to(out, ", ");
    }
    ToString(out, pen);
  };

  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a pen slot table as a string.
 * @param pens Pens resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Pens& pens) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), pens);
  return result;
}

/**
 * @brief Formats a pen slot table as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param pens Pens resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Pens& pens) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), pens);
  return result;
}

/**
 * @brief Outputs a pen slot table to an output stream.
 * @param os Output stream
 * @param pens Pens resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Pens& pens) {
  ToString(std::ostreambuf_iterator<char>(os), pens);
  return os;
}

/**
 * @brief Formats a pen proximity message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen proximity message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenProximityMsg& msg) {
  return std::format_to(
      out,
      "PenProximityMsg{{entity={}, id={}, device_type={}, in_proximity={}}}",
      msg.entity, msg.id, ToString(msg.device_type), msg.in_proximity);
}

/**
 * @brief Formats a pen proximity message as a string.
 * @param msg Pen proximity message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenProximityMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen proximity message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen proximity message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const PenProximityMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen proximity message to an output stream.
 * @param os Output stream
 * @param msg Pen proximity message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenProximityMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen touch message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen touch message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenTouchMsg& msg) {
  return std::format_to(
      out, "PenTouchMsg{{entity={}, x={}, y={}, id={}, down={}, eraser={}}}",
      msg.entity, msg.x, msg.y, msg.id, msg.down, msg.eraser);
}

/**
 * @brief Formats a pen touch message as a string.
 * @param msg Pen touch message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenTouchMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen touch message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen touch message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const PenTouchMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen touch message to an output stream.
 * @param os Output stream
 * @param msg Pen touch message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenTouchMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen button message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen button message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenButtonInputMsg& msg) {
  return std::format_to(out,
                        "PenButtonInputMsg{{entity={}, id={}, button={}, "
                        "state={}}}",
                        msg.entity, msg.id, ToString(msg.button),
                        ToString(msg.state));
}

/**
 * @brief Formats a pen button input message as a string.
 * @param msg Pen button message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenButtonInputMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen button input message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen button input message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const PenButtonInputMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen button message to an output stream.
 * @param os Output stream
 * @param msg Pen button message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const PenButtonInputMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen moved message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen moved message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenMovedMsg& msg) {
  return std::format_to(out, "PenMovedMsg{{entity={}, x={}, y={}, id={}}}",
                        msg.entity, msg.x, msg.y, msg.id);
}

/**
 * @brief Formats a pen moved message as a string.
 * @param msg Pen moved message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenMovedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen moved message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen moved message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const PenMovedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen moved message to an output stream.
 * @param os Output stream
 * @param msg Pen moved message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const PenMovedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats a pen axis message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Pen axis message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const PenAxisChangedMsg& msg) {
  return std::format_to(
      out,
      "PenAxisChangedMsg{{entity={}, x={}, y={}, value={}, id={}, axis={}}}",
      msg.entity, msg.x, msg.y, msg.value, msg.id, ToString(msg.axis));
}

/**
 * @brief Formats a pen axis message as a string.
 * @param msg Pen axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const PenAxisChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a pen axis message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Pen axis message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const PenAxisChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a pen axis message to an output stream.
 * @param os Output stream
 * @param msg Pen axis message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const PenAxisChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::input

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::input::PenButton> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::PenButton button,
                               format_context& ctx) {
    return format_to(ctx.out(), "PenButton::{}",
                     helios::input::ToString(button));
  }
};

template <>
struct formatter<helios::input::PenAxis> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::PenAxis axis,
                               format_context& ctx) {
    return format_to(ctx.out(), "PenAxis::{}", helios::input::ToString(axis));
  }
};

template <>
struct formatter<helios::input::PenDeviceType> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::input::PenDeviceType type,
                               format_context& ctx) {
    return format_to(ctx.out(), "PenDeviceType::{}",
                     helios::input::ToString(type));
  }
};

template <>
struct formatter<helios::input::Pen> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Pen& pen, format_context& ctx) {
    return helios::input::ToString(ctx.out(), pen);
  }
};

template <>
struct formatter<helios::input::Pens> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Pens& pens, format_context& ctx) {
    return helios::input::ToString(ctx.out(), pens);
  }
};

template <>
struct formatter<helios::input::PenProximityMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenProximityMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenTouchMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenTouchMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenButtonInputMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenButtonInputMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenMovedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenMovedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::input::PenAxisChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::PenAxisChangedMsg& msg,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
