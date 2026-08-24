#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/sdl3/window/details/window_map.hpp>

#include <string_view>

namespace helios::sdl3::window {

/// @brief Destroys native SDL windows and releases the video subsystem.
struct Shutdown {
  static constexpr std::string_view kName = "helios::sdl3::window::Shutdown";

  void operator()(ecs::Res<Context> context, ecs::Res<NativeWindows> native,
                  ecs::Res<WindowMap> window_map,
                  ecs::Res<sdl3::Context> sdl_context) const;
};

}  // namespace helios::sdl3::window
