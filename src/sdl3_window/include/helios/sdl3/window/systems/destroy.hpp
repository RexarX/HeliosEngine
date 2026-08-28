#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/app/builtin/app_exit.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world_view.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/settings.hpp>

#include <string_view>
#endif
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/window_map.hpp>

HELIOS_MODULE_EXPORT
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
#endif  // HELIOS_MODULE_CONSUMER_SHIM
