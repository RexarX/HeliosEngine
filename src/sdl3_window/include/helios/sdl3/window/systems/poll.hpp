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

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Syncs clipboard state after SDL events have been pumped.
struct PollEvents {
  static constexpr std::string_view kName = "helios::sdl3::window::PollEvents";

  void operator()(ecs::Res<Context> context, ecs::Res<NativeWindows> native,
                  ecs::Res<sdl3::Context> sdl_context) const;
};

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
