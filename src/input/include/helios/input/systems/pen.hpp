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
#include <helios/input/pen.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/// @brief Applies pen messages into the `Pens` resource.
struct UpdatePenState {
  static constexpr std::string_view kName = "helios::input::UpdatePenState";

  /**
   * @brief Drains pen messages into aggregated pen state.
   * @details Motion and axis samples are applied before tip/button events so
   * a down event in the same frame does not invert the motion delta. The first
   * position sample after proximity-in sets the cursor without contributing a
   * delta from the default origin.
   * @param pens Pens resource
   * @param messages Pen proximity, touch, button, motion, and axis readers
   */
  void operator()(ecs::Res<Pens> pens, PenMessages messages) const;
};

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
