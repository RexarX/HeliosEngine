#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/event_dispatcher.hpp>

#include <string_view>
#endif
#include <helios/sdl3/input/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief Retains the SDL gamepad subsystem and registers input event handlers.
struct Init {
  static constexpr std::string_view kName = "helios::sdl3::input::Init";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<EventDispatcher> dispatcher) const;
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
