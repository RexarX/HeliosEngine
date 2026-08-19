#pragma once

#include <helios/ecs/entity/entity.hpp>
#include <helios/input/mouse.hpp>

#include <SDL3/SDL_mouse.h>

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

struct SDL_Cursor;

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
