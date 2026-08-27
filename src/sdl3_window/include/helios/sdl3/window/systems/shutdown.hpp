#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/context.hpp>

#include <string_view>
#endif
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/window_map.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Destroys native SDL windows and releases the video subsystem.
struct Shutdown {
  static constexpr std::string_view kName = "helios::sdl3::window::Shutdown";

  void operator()(ecs::Res<Context> context, ecs::Res<NativeWindows> native,
                  ecs::Res<WindowMap> window_map,
                  ecs::Res<sdl3::Context> sdl_context) const;
};

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
