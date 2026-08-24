#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::sdl3 {

/// @brief Pumps the SDL event queue on the main thread.
struct PumpEvents {
  static constexpr std::string_view kName = "helios::sdl3::PumpEvents";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<EventDispatcher> dispatcher,
                  ecs::Res<const window::Settings> settings) const;
};

}  // namespace helios::sdl3
