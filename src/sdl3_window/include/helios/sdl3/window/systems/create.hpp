#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/query/params.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/window/components.hpp>
#include <helios/window/params.hpp>

#include <string_view>
#endif
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/window_map.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Creates native SDL windows for new `::helios::window::Window`
/// entities.
struct CreateNativeWindows {
  static constexpr std::string_view kName =
      "helios::sdl3::window::CreateNativeWindows";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native,
                  ecs::Res<WindowMap> window_map,
                  ecs::Res<const sdl3::Context> sdl_context,
                  ecs::Query<::helios::window::Window&,
                             ecs::Without<::helios::window::CreationFailed>>
                      windows,
                  ::helios::window::CreationWriters writers) const;
};

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
