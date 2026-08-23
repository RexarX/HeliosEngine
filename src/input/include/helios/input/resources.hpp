#pragma once

#include <helios/input/axis.hpp>
#include <helios/input/button_input.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <ostream>
#include <ranges>
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
 * @brief Formats a input settings using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param settings Input settings
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Settings& settings) {
  out = std::format_to(out, "Settings{{stick=");
  out = ToString(out, settings.stick);
  out = std::format_to(out, ", trigger=");
  out = ToString(out, settings.trigger);
  return std::format_to(
      out, ", rest_frames={}, auto_calibrate={}, raw_mouse_motion={}}}",
      settings.rest_frames, settings.auto_calibrate, settings.raw_mouse_motion);
}

/**
 * @brief Formats a input settings as a string.
 * @param settings Input settings
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Settings& settings) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), settings);
  return result;
}

/**
 * @brief Formats a input settings as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Input settings
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Settings& settings) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), settings);
  return result;
}

/**
 * @brief Outputs a input settings to an output stream.
 * @param os Output stream
 * @param settings Input settings
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  ToString(std::ostreambuf_iterator<char>(os), settings);
  return os;
}

/**
 * @brief Formats a keyboard state using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param keyboard Keyboard resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Keyboard& keyboard) {
  out = std::format_to(out, "Keyboard{{modifiers=");
  out = ToString(out, keyboard.modifiers);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a keyboard state as a string.
 * @param keyboard Keyboard resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Keyboard& keyboard) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), keyboard);
  return result;
}

/**
 * @brief Formats a keyboard state as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param settings Keyboard resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Keyboard& keyboard) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), keyboard);
  return result;
}

/**
 * @brief Outputs a keyboard state to an output stream.
 * @param os Output stream
 * @param keyboard Keyboard resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Keyboard& keyboard) {
  ToString(std::ostreambuf_iterator<char>(os), keyboard);
  return os;
}

/**
 * @brief Formats a mouse state using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param mouse Mouse resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Mouse& mouse) {
  return std::format_to(
      out, "Mouse{{position=({}, {}), delta=({}, {}), scroll=({}, {})}}",
      mouse.position_x, mouse.position_y, mouse.delta_x, mouse.delta_y,
      mouse.scroll_x, mouse.scroll_y);
}

/**
 * @brief Formats a mouse state as a string.
 * @param mouse Mouse resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Mouse& mouse) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), mouse);
  return result;
}

/**
 * @brief Formats a mouse state as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param mouse Mouse resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Mouse& mouse) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), mouse);
  return result;
}

/**
 * @brief Outputs a mouse state to an output stream.
 * @param os Output stream
 * @param mouse Mouse resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Mouse& mouse) {
  ToString(std::ostreambuf_iterator<char>(os), mouse);
  return os;
}

/**
 * @brief Formats a gamepad slot table using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param gamepads Gamepads resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Gamepads& gamepads) {
  out = std::format_to(out, "Gamepads{{pads = [");

  auto connected = std::views::filter(
      gamepads.pads, [](const Gamepad& pad) { return pad.connected; });
  for (const auto& [cnt, pad] : connected | std::views::enumerate) {
    if (cnt > 0) [[likely]] {
      out = std::format_to(out, ", ");
    }
    ToString(out, pad);
  };

  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a gamepad slot table as a string.
 * @param gamepads Gamepads resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Gamepads& gamepads) {
  std::string result;
  result.reserve(512);
  ToString(std::back_inserter(result), gamepads);
  return result;
}

/**
 * @brief Formats a gamepad slot table as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param gamepads Gamepads resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Gamepads& gamepads) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(512);
  ToString(std::back_inserter(result), gamepads);
  return result;
}

/**
 * @brief Outputs a gamepad slot table to an output stream.
 * @param os Output stream
 * @param gamepads Gamepads resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Gamepads& gamepads) {
  ToString(std::ostreambuf_iterator<char>(os), gamepads);
  return os;
}

/**
 * @brief Formats a joystick slot table using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param joysticks Joysticks resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Joysticks& joysticks) {
  out = std::format_to(out, "Joysticks{{sticks = [");

  auto connected = std::views::filter(
      joysticks.sticks, [](const Joystick& stick) { return stick.connected; });
  for (const auto& [cnt, stick] : connected | std::views::enumerate) {
    if (cnt > 0) [[likely]] {
      out = std::format_to(out, ", ");
    }
    ToString(out, stick);
  };

  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a joystick slot table as a string.
 * @param joysticks Joysticks resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Joysticks& joysticks) {
  std::string result;
  result.reserve(512);
  ToString(std::back_inserter(result), joysticks);
  return result;
}

/**
 * @brief Formats a joystick slot table as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param joysticks Joysticks resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Joysticks& joysticks) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(512);
  ToString(std::back_inserter(result), joysticks);
  return result;
}

/**
 * @brief Outputs a joystick slot table to an output stream.
 * @param os Output stream
 * @param joysticks Joysticks resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Joysticks& joysticks) {
  ToString(std::ostreambuf_iterator<char>(os), joysticks);
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

  auto connected = std::views::filter(
      pens.pens, [](const Pen& pen) { return pen.in_proximity; });
  for (const auto& [cnt, pen] : connected | std::views::enumerate) {
    if (cnt > 0) [[likely]] {
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
 * @brief Formats a pending gamepad mappings using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param mappings Gamepad mappings resource
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const GamepadMappings& mappings) {
  return std::format_to(
      out, "GamepadMappings{{pending_lines={}, pending_files={}, dirty={}}}",
      mappings.pending_lines.size(), mappings.pending_files.size(),
      mappings.dirty);
}

/**
 * @brief Formats a pending gamepad mappings as a string.
 * @param mappings Gamepad mappings resource
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const GamepadMappings& mappings) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), mappings);
  return result;
}

/**
 * @brief Formats a pending gamepad mappings as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param mappings Gamepad mappings resource
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const GamepadMappings& mappings) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), mappings);
  return result;
}

/**
 * @brief Outputs a pending gamepad mappings to an output stream.
 * @param os Output stream
 * @param mappings Gamepad mappings resource
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const GamepadMappings& mappings) {
  ToString(std::ostreambuf_iterator<char>(os), mappings);
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
    return helios::input::ToString(ctx.out(), settings);
  }
};

template <>
struct formatter<helios::input::Keyboard> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Keyboard& keyboard,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), keyboard);
  }
};

template <>
struct formatter<helios::input::Mouse> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Mouse& mouse, format_context& ctx) {
    return helios::input::ToString(ctx.out(), mouse);
  }
};

template <>
struct formatter<helios::input::Gamepads> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Gamepads& gamepads,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), gamepads);
  }
};

template <>
struct formatter<helios::input::Joysticks> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::Joysticks& joysticks,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), joysticks);
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
struct formatter<helios::input::GamepadMappings> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::input::GamepadMappings& mappings,
                     format_context& ctx) {
    return helios::input::ToString(ctx.out(), mappings);
  }
};

}  // namespace std
