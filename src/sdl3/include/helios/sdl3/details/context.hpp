#pragma once

#include <helios/ecs/world.hpp>

#include <string_view>

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
