#pragma once

#include <helios/app/builtin/app_exit.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world_view.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/window/params.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::glfw {

/// @brief Destroys native windows whose `Window` requested close.
struct DestroyClosedWindows {
  static constexpr std::string_view kName =
      "helios::glfw::DestroyClosedWindows";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native,
                  ecs::Res<const window::Settings> settings,
                  window::Windows windows, ecs::WorldView world_view,
                  ecs::Commands commands,
                  ecs::MessageWriter<window::ClosedMsg> closed,
                  ecs::MessageWriter<app::AppExit> app_exit) const;
};

}  // namespace helios::glfw
