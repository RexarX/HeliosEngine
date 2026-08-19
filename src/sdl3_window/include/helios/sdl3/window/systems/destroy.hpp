#pragma once

#include <helios/app/application.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world_view.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/sdl3/window/details/window_map.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/params.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::sdl3::window {

/// @brief Destroys native windows whose `Window` requested close.
struct DestroyClosedWindows {
  static constexpr std::string_view kName =
      "helios::sdl3::window::DestroyClosedWindows";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native,
                  ecs::Res<WindowMap> window_map,
                  ecs::Res<const ::helios::window::Settings> settings,
                  ::helios::window::Windows windows, ecs::WorldView world_view,
                  ecs::Commands commands,
                  ecs::MessageWriter<::helios::window::ClosedMsg> closed,
                  ecs::MessageWriter<app::AppExit> app_exit) const;
};

}  // namespace helios::sdl3::window
