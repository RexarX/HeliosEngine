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
#include <helios/input/params.hpp>
#include <helios/input/touch.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies touch messages into the `Touches` resource.
struct UpdateTouchState {
  static constexpr std::string_view kName = "helios::input::UpdateTouchState";

  /**
   * @brief Drains touch messages into aggregated finger state.
   * @details The first position sample after `kStarted` sets the cursor without
   * contributing a delta from the default origin. Ended / canceled fingers stay
   * visible until `ClearInputState` on the next frame.
   * @param touches Touches resource
   * @param messages Touch finger readers
   */
  void operator()(ecs::Res<Touches> touches, TouchMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
