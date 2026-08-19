#pragma once

#include <helios/ecs/query/params.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/components.hpp>
#include <helios/sdl3/input/details/input_state.hpp>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/ecs/query/query.hpp>
#include <helios/sdl3/input/details/cursor_cache.hpp>
#include <helios/sdl3/window/details/window_map.hpp>
#endif

#include <string_view>

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
