#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/entity/entity.hpp>
#include <helios/input/mouse.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>
#endif

HELIOS_MODULE_EXPORT struct SDL_Cursor;

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {

/// @brief Cached `SDL_Cursor*` objects for standard and custom cursors.
struct CursorCache {
  static constexpr std::string_view kName = "helios::sdl3::input::CursorCache";

  struct CustomEntry {
    ecs::Entity entity;
    SDL_Cursor* cursor = nullptr;
  };

  std::array<SDL_Cursor*,
             static_cast<size_t>(helios::input::CursorIcon::kCount)>
      standard = {};
  std::vector<CustomEntry> custom;
};

/**
 * @brief Destroys all cached SDL cursor objects.
 * @param cache Cursor cache to clear
 */
void DestroyCursorCache(CursorCache& cache);

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
