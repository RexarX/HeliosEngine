#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>
#include <helios/sdl3/input/details/input_state.hpp>

#include <string_view>

namespace helios::sdl3::input {

/// @brief Retains the SDL gamepad subsystem and registers input event handlers.
struct Init {
  static constexpr std::string_view kName = "helios::sdl3::input::Init";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<EventDispatcher> dispatcher) const;
};

}  // namespace helios::sdl3::input
