#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>
#include <helios/sdl3/window/details/native_state.hpp>

#include <string_view>

namespace helios::sdl3::window {

/// @brief Syncs clipboard state after SDL events have been pumped.
struct PollEvents {
  static constexpr std::string_view kName = "helios::sdl3::window::PollEvents";

  void operator()(ecs::Res<Context> context, ecs::Res<NativeWindows> native,
                  ecs::Res<sdl3::Context> sdl_context) const;
};

}  // namespace helios::sdl3::window
