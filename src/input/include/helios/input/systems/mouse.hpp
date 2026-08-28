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
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies mouse messages into the `Mouse` resource.
struct UpdateMouseState {
  static constexpr std::string_view kName = "helios::input::UpdateMouseState";

  /**
   * @brief Drains mouse messages into aggregated mouse state.
   * @details Connection messages are ignored (`Mouse` stays process-global).
   * @param mouse Mouse resource
   * @param messages Mouse button, cursor, motion, wheel, and connection readers
   */
  void operator()(ecs::Res<Mouse> mouse, MouseMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
