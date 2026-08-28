#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>

#include <string_view>
#endif
#include <helios/sdl3/window/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Applies dirty window properties to native SDL windows.
struct ApplyChanges {
  static constexpr std::string_view kName =
      "helios::sdl3::window::ApplyChanges";

  void operator()(
      ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
      ::helios::window::Windows windows,
      ::helios::window::AppearanceWriters appearance,
      ecs::MessageWriter<::helios::window::PosChangedMsg> pos_changed) const;
};

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
