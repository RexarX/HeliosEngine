#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/query/params.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/components.hpp>
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/window_map.hpp>
#endif

#include <string_view>
#endif
#include <helios/sdl3/input/cursor_cache.hpp>
#include <helios/sdl3/input/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief Applies dirty `helios::input::Cursor` components to native SDL
/// windows.
struct ApplyCursors {
  static constexpr std::string_view kName = "helios::sdl3::input::ApplyCursors";

  void operator()(ecs::Res<const Context> context,
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
                  ecs::OptRes<const window::WindowMap> windows,
#endif
                  ecs::Res<CursorCache> cache,
                  ecs::Query<helios::input::Cursor&> cursors) const;
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
