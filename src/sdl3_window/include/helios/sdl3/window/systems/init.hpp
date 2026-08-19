#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::sdl3::window {

/// @brief Initializes SDL video and registers window event handlers.
struct Init {
  static constexpr std::string_view kName = "helios::sdl3::window::Init";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<::helios::window::Monitors> monitors,
                  ecs::Res<sdl3::EventDispatcher> dispatcher) const;
};

}  // namespace helios::sdl3::window
