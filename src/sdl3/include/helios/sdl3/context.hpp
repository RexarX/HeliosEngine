#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;

}

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

/// @brief SDL3 runtime state shared by window and input backends.
struct Context {
  static constexpr std::string_view kName = "helios::sdl3::Context";

  ecs::World* world = nullptr;
  void (*frame_pump)(void* user_data) = nullptr;
  void* frame_pump_user_data = nullptr;
  bool in_event_poll = false;
  /// @brief True while a nested `FramePumpOrder` is running from an event
  /// handler.
  bool in_nested_pump = false;
};

}  // namespace helios::sdl3
#endif  // HELIOS_MODULE_CONSUMER_SHIM
