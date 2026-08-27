#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/input/settings.hpp>

#include <string_view>
#endif
#include <helios/sdl3/input/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief Applies the raw-mouse-motion setting to SDL relative mouse scaling.
struct ApplyRawMouseMotion {
  static constexpr std::string_view kName =
      "helios::sdl3::input::ApplyRawMouseMotion";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<const helios::input::Settings> settings) const;
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
