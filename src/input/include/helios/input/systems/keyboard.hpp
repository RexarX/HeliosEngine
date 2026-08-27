#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>

#include <string_view>
#endif
#include <helios/input/keyboard.hpp>
#include <helios/input/params.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies keyboard messages into the `Keyboard` resource.
struct UpdateKeyboardState {
  static constexpr std::string_view kName =
      "helios::input::UpdateKeyboardState";

  /**
   * @brief Drains keyboard messages into aggregated keyboard state.
   * @details IME composition is applied from `TextEditingMsg` and cleared on
   * an empty preedit or any `TextInputMsg` commit. Connection messages are
   * ignored.
   * @param keyboard Keyboard resource
   * @param messages Keyboard key, text, IME, and connection readers
   */
  void operator()(ecs::Res<Keyboard> keyboard, KeyboardMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
