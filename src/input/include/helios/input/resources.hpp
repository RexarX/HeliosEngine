#pragma once

#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace helios::input {

/// @brief Global input behavior settings.
/// @details Stick and trigger `AxisFilter`s apply to `Gamepad::axes`. Axis
/// messages stay raw. `auto_calibrate` captures rest center on the first
/// near-rest sample and recenters after `rest_frames` at rest.
struct Settings {
  static constexpr std::string_view kName = "helios::input::Settings";
  static constexpr uint16_t kDefaultRestFrames = 60;

  AxisFilter stick{.deadzone = AxisFilter::kDefaultDeadzone,
                   .livezone = AxisFilter::kDefaultLivezone,
                   .rescale = true};
  AxisFilter trigger{.deadzone = AxisFilter::kDefaultTriggerDeadzone,
                     .livezone = AxisFilter::kDefaultLivezone,
                     .rescale = true};
  uint16_t rest_frames = kDefaultRestFrames;
  bool auto_calibrate = true;
  bool raw_mouse_motion = false;
};

using KeyboardInput = ButtonInput<Key>;

/// @brief Aggregated keyboard state for the main window / app.
struct Keyboard {
  static constexpr std::string_view kName = "helios::input::Keyboard";

  KeyboardInput keys;
  Modifiers modifiers = Modifiers::kNone;
};

using MouseButtonInput = ButtonInput<MouseButton>;

/// @brief Aggregated mouse state for the main window / app.
struct Mouse {
  static constexpr std::string_view kName = "helios::input::Mouse";

  MouseButtonInput buttons;
  double position_x = 0.0;
  double position_y = 0.0;
  double delta_x = 0.0;
  double delta_y = 0.0;
  double scroll_x = 0.0;
  double scroll_y = 0.0;

  /**
   * @brief Returns the cursor position.
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

  /**
   * @brief Returns the per-frame scroll delta.
   * @return Scroll x and y
   */
  [[nodiscard]] constexpr auto GetScroll() const noexcept
      -> std::pair<double, double> {
    return {scroll_x, scroll_y};
  }
};

/// @brief Fixed gamepad slot table (GLFW joystick range 0..15).
/// @details `filters` is rest-center bookkeeping for `UpdateGamepadState`.
struct Gamepads {
  static constexpr std::string_view kName = "helios::input::Gamepads";
  static constexpr size_t kSlotCount = 16;

  std::array<Gamepad, kSlotCount> pads = {};
  std::array<GamepadAxisFilter, kSlotCount> filters = {};

  /**
   * @brief Looks up a mutable gamepad slot by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr Gamepad* TryGet(int32_t id) noexcept;

  /**
   * @brief Looks up a const gamepad slot by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Gamepad* TryGet(int32_t id) const noexcept;

  /**
   * @brief Looks up mutable axis-filter state by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the filter slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr GamepadAxisFilter* TryGetFilter(int32_t id) noexcept;

  /**
   * @brief Looks up const axis-filter state by id.
   * @param id Gamepad / joystick id
   * @return Pointer to the filter slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const GamepadAxisFilter* TryGetFilter(
      int32_t id) const noexcept;
};

constexpr Gamepad* Gamepads::TryGet(int32_t id) noexcept {
  if (id < 0 || static_cast<size_t>(id) >= pads.size()) {
    return nullptr;
  }
  return &pads[static_cast<size_t>(id)];
}

constexpr const Gamepad* Gamepads::TryGet(int32_t id) const noexcept {
  if (id < 0 || static_cast<size_t>(id) >= pads.size()) {
    return nullptr;
  }
  return &pads[static_cast<size_t>(id)];
}

constexpr GamepadAxisFilter* Gamepads::TryGetFilter(int32_t id) noexcept {
  if (id < 0 || static_cast<size_t>(id) >= filters.size()) {
    return nullptr;
  }
  return &filters[static_cast<size_t>(id)];
}

constexpr const GamepadAxisFilter* Gamepads::TryGetFilter(
    int32_t id) const noexcept {
  if (id < 0 || static_cast<size_t>(id) >= filters.size()) {
    return nullptr;
  }
  return &filters[static_cast<size_t>(id)];
}

/// @brief Fixed unmapped-joystick slot table (same `0..15` ids as `Gamepads`).
/// @details A physical device occupies either a gamepad slot or a joystick
/// slot, never both.
struct Joysticks {
  static constexpr std::string_view kName = "helios::input::Joysticks";
  static constexpr size_t kSlotCount = Gamepads::kSlotCount;

  std::array<Joystick, kSlotCount> sticks = {};

  /**
   * @brief Looks up a mutable joystick slot by id.
   * @param id Joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr Joystick* TryGet(int32_t id) noexcept;

  /**
   * @brief Looks up a const joystick slot by id.
   * @param id Joystick id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Joystick* TryGet(int32_t id) const noexcept;
};

constexpr Joystick* Joysticks::TryGet(int32_t id) noexcept {
  if (id < 0 || static_cast<size_t>(id) >= sticks.size()) {
    return nullptr;
  }
  return &sticks[static_cast<size_t>(id)];
}

constexpr const Joystick* Joysticks::TryGet(int32_t id) const noexcept {
  if (id < 0 || static_cast<size_t>(id) >= sticks.size()) {
    return nullptr;
  }
  return &sticks[static_cast<size_t>(id)];
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
  [[nodiscard]] constexpr Pen* TryGet(int32_t id) noexcept;

  /**
   * @brief Looks up a const pen slot by id.
   * @param id Pen slot id
   * @return Pointer to the slot, or `nullptr` when out of range
   */
  [[nodiscard]] constexpr const Pen* TryGet(int32_t id) const noexcept;
};

constexpr Pen* Pens::TryGet(int32_t id) noexcept {
  if (id < 0 || static_cast<size_t>(id) >= pens.size()) {
    return nullptr;
  }
  return &pens[static_cast<size_t>(id)];
}

constexpr const Pen* Pens::TryGet(int32_t id) const noexcept {
  if (id < 0 || static_cast<size_t>(id) >= pens.size()) {
    return nullptr;
  }
  return &pens[static_cast<size_t>(id)];
}

/// @brief Pending SDL_GameControllerDB mapping lines and files for backends.
struct GamepadMappings {
  static constexpr std::string_view kName = "helios::input::GamepadMappings";

  /**
   * @brief Queues a mapping database line.
   * @param mapping Single mapping string
   */
  void Add(std::string_view mapping);

  /**
   * @brief Queues a mapping database file path.
   * @param path File containing mapping lines
   */
  void AddFromFile(std::string_view path);

  /// @brief Clears queued mappings after a backend has consumed them.
  void ClearPending() noexcept;

  std::vector<std::string> pending_lines;
  std::vector<std::string> pending_files;
  bool dirty = false;
};

inline void GamepadMappings::Add(std::string_view mapping) {
  pending_lines.emplace_back(mapping);
  dirty = true;
}

inline void GamepadMappings::AddFromFile(std::string_view path) {
  pending_files.emplace_back(path);
  dirty = true;
}

inline void GamepadMappings::ClearPending() noexcept {
  pending_lines.clear();
  pending_files.clear();
  dirty = false;
}

/**
 * @brief Formats input settings using an output iterator.
 * @tparam It Output iterator type
 * @param settings Input settings
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Settings& settings, It out) {
  out = std::format_to(out, "Settings{{stick=");
  out = ToString(settings.stick, out);
  out = std::format_to(out, ", trigger=");
  out = ToString(settings.trigger, out);
  return std::format_to(
      out, ", rest_frames={}, auto_calibrate={}, raw_mouse_motion={}}}",
      settings.rest_frames, settings.auto_calibrate, settings.raw_mouse_motion);
}

/**
 * @brief Formats input settings as a string.
 * @param settings Input settings
 * @return Formatted settings string
 */
[[nodiscard]] inline std::string ToString(const Settings& settings) {
  std::string result;
  result.reserve(160);
  ToString(settings, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs input settings to an output stream.
 * @param os Output stream
 * @param settings Input settings
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  ToString(settings, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats keyboard state using an output iterator.
 * @tparam It Output iterator type
 * @param keyboard Keyboard resource
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Keyboard& keyboard, It out) {
  out = std::format_to(out, "Keyboard{{modifiers=");
  out = ToString(keyboard.modifiers, out);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats keyboard state as a string.
 * @param keyboard Keyboard resource
 * @return Formatted keyboard string
 */
[[nodiscard]] inline std::string ToString(const Keyboard& keyboard) {
  std::string result;
  result.reserve(48);
  ToString(keyboard, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs keyboard state to an output stream.
 * @param os Output stream
 * @param keyboard Keyboard resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Keyboard& keyboard) {
  ToString(keyboard, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats mouse state using an output iterator.
 * @tparam It Output iterator type
 * @param mouse Mouse resource
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Mouse& mouse, It out) {
  return std::format_to(
      out, "Mouse{{position=({}, {}), delta=({}, {}), scroll=({}, {})}}",
      mouse.position_x, mouse.position_y, mouse.delta_x, mouse.delta_y,
      mouse.scroll_x, mouse.scroll_y);
}

/**
 * @brief Formats mouse state as a string.
 * @param mouse Mouse resource
 * @return Formatted mouse string
 */
[[nodiscard]] inline std::string ToString(const Mouse& mouse) {
  std::string result;
  result.reserve(96);
  ToString(mouse, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs mouse state to an output stream.
 * @param os Output stream
 * @param mouse Mouse resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Mouse& mouse) {
  ToString(mouse, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats the gamepad slot table using an output iterator.
 * @tparam It Output iterator type
 * @param gamepads Gamepads resource
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Gamepads& gamepads, It out) {
  size_t connected = 0;
  for (const Gamepad& pad : gamepads.pads) {
    if (pad.connected) {
      ++connected;
    }
  }
  return std::format_to(out, "Gamepads{{connected={}/{}}}", connected,
                        gamepads.pads.size());
}

/**
 * @brief Formats the gamepad slot table as a string.
 * @param gamepads Gamepads resource
 * @return Formatted gamepads string
 */
[[nodiscard]] inline std::string ToString(const Gamepads& gamepads) {
  std::string result;
  result.reserve(48);
  ToString(gamepads, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs the gamepad slot table to an output stream.
 * @param os Output stream
 * @param gamepads Gamepads resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Gamepads& gamepads) {
  ToString(gamepads, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats the joystick slot table using an output iterator.
 * @tparam It Output iterator type
 * @param joysticks Joysticks resource
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Joysticks& joysticks, It out) {
  size_t connected = 0;
  for (const Joystick& stick : joysticks.sticks) {
    if (stick.connected) {
      ++connected;
    }
  }
  return std::format_to(out, "Joysticks{{connected={}/{}}}", connected,
                        joysticks.sticks.size());
}

/**
 * @brief Formats the joystick slot table as a string.
 * @param joysticks Joysticks resource
 * @return Formatted joysticks string
 */
[[nodiscard]] inline std::string ToString(const Joysticks& joysticks) {
  std::string result;
  result.reserve(48);
  ToString(joysticks, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs the joystick slot table to an output stream.
 * @param os Output stream
 * @param joysticks Joysticks resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Joysticks& joysticks) {
  ToString(joysticks, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats the pen slot table using an output iterator.
 * @tparam It Output iterator type
 * @param pens Pens resource
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Pens& pens, It out) {
  size_t connected = 0;
  for (const Pen& pen : pens.pens) {
    if (pen.in_proximity) {
      ++connected;
    }
  }
  return std::format_to(out, "Pens{{in_proximity={}/{}}}", connected,
                        pens.pens.size());
}

/**
 * @brief Formats the pen slot table as a string.
 * @param pens Pens resource
 * @return Formatted pens string
 */
[[nodiscard]] inline std::string ToString(const Pens& pens) {
  std::string result;
  result.reserve(48);
  ToString(pens, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs the pen slot table to an output stream.
 * @param os Output stream
 * @param pens Pens resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Pens& pens) {
  ToString(pens, std::ostreambuf_iterator<char>(os));
  return os;
}

/**
 * @brief Formats pending gamepad mappings using an output iterator.
 * @tparam It Output iterator type
 * @param mappings Gamepad mappings resource
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const GamepadMappings& mappings, It out) {
  return std::format_to(
      out, "GamepadMappings{{pending_lines={}, pending_files={}, dirty={}}}",
      mappings.pending_lines.size(), mappings.pending_files.size(),
      mappings.dirty);
}

/**
 * @brief Formats pending gamepad mappings as a string.
 * @param mappings Gamepad mappings resource
 * @return Formatted mappings string
 */
[[nodiscard]] inline std::string ToString(const GamepadMappings& mappings) {
  std::string result;
  result.reserve(80);
  ToString(mappings, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs pending gamepad mappings to an output stream.
 * @param os Output stream
 * @param mappings Gamepad mappings resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadMappings& mappings) {
  ToString(mappings, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::input

namespace std {

template <>
struct formatter<helios::input::Settings> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Settings& settings,
                     format_context& ctx) {
    return helios::input::ToString(settings, ctx.out());
  }
};

template <>
struct formatter<helios::input::Keyboard> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Keyboard& keyboard,
                     format_context& ctx) {
    return helios::input::ToString(keyboard, ctx.out());
  }
};

template <>
struct formatter<helios::input::Mouse> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Mouse& mouse, format_context& ctx) {
    return helios::input::ToString(mouse, ctx.out());
  }
};

template <>
struct formatter<helios::input::Gamepads> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Gamepads& gamepads,
                     format_context& ctx) {
    return helios::input::ToString(gamepads, ctx.out());
  }
};

template <>
struct formatter<helios::input::Joysticks> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Joysticks& joysticks,
                     format_context& ctx) {
    return helios::input::ToString(joysticks, ctx.out());
  }
};

template <>
struct formatter<helios::input::Pens> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Pens& pens, format_context& ctx) {
    return helios::input::ToString(pens, ctx.out());
  }
};

template <>
struct formatter<helios::input::GamepadMappings> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadMappings& mappings,
                     format_context& ctx) {
    return helios::input::ToString(mappings, ctx.out());
  }
};

}  // namespace std
