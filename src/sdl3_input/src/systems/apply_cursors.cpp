#include <pch.hpp>

#include <helios/sdl3/input/systems/apply_cursors.hpp>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/mouse.hpp>
#include <helios/sdl3/input/details/input_map.hpp>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace helios::sdl3::input {

namespace {

[[nodiscard]] SDL_Cursor*& FindOrInsertCustom(CursorCache& cache,
                                              ecs::Entity entity) {
  const auto it = std::ranges::find_if(
      cache.custom, [entity](const CursorCache::CustomEntry& entry) {
        return entry.entity == entity;
      });
  if (it != cache.custom.end()) {
    return it->cursor;
  }

  cache.custom.push_back({.entity = entity, .cursor = nullptr});
  return cache.custom.back().cursor;
}

void EraseCustom(CursorCache& cache, ecs::Entity entity) {
  const auto it = std::ranges::find_if(
      cache.custom, [entity](const CursorCache::CustomEntry& entry) {
        return entry.entity == entity;
      });
  if (it == cache.custom.end()) [[unlikely]] {
    return;
  }

  if (it->cursor != nullptr) [[likely]] {
    SDL_DestroyCursor(it->cursor);
  }

  cache.custom.erase(it);
}

[[nodiscard]] SDL_Cursor* EnsureStandardCursor(CursorCache& cache,
                                               helios::input::CursorIcon icon) {
  const auto index = static_cast<size_t>(icon);
  if (index >= cache.standard.size()) [[unlikely]] {
    return nullptr;
  }

  if (cache.standard[index] == nullptr) {
    cache.standard[index] = SDL_CreateSystemCursor(SdlFromCursorIcon(icon));
  }

  return cache.standard[index];
}

[[nodiscard]] SDL_Cursor* CreateCustomCursor(
    const helios::input::CursorImage& image) {
  if (image.width == 0 || image.height == 0 ||
      image.rgba.size() <
          static_cast<size_t>(image.width) * image.height * 4U) {
    return nullptr;
  }

  SDL_Surface* surface =
      SDL_CreateSurface(static_cast<int>(image.width),
                        static_cast<int>(image.height), SDL_PIXELFORMAT_RGBA32);
  if (surface == nullptr || surface->pixels == nullptr) [[unlikely]] {
    if (surface != nullptr) {
      SDL_DestroySurface(surface);
    }
    return nullptr;
  }

  const size_t row_bytes = static_cast<size_t>(image.width) * 4U;
  auto* dst = static_cast<uint8_t*>(surface->pixels);
  for (uint32_t row = 0; row < image.height; ++row) {
    std::memcpy(dst + (static_cast<size_t>(surface->pitch) * row),
                image.rgba.data() + (row * row_bytes), row_bytes);
  }

  SDL_Cursor* cursor =
      SDL_CreateColorCursor(surface, image.hotspot_x, image.hotspot_y);
  SDL_DestroySurface(surface);
  return cursor;
}

}  // namespace

void DestroyCursorCache(CursorCache& cache) {
  for (SDL_Cursor*& cursor : cache.standard) {
    if (cursor != nullptr) [[likely]] {
      SDL_DestroyCursor(cursor);
      cursor = nullptr;
    }
  }

  for (CursorCache::CustomEntry& entry : cache.custom) {
    if (entry.cursor != nullptr) [[likely]] {
      SDL_DestroyCursor(entry.cursor);
      entry.cursor = nullptr;
    }
  }
  cache.custom.clear();
}

void ApplyCursors::operator()(
    ecs::Res<const Context> context,
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
    ecs::OptRes<const window::WindowMap> windows,
#endif
    ecs::Res<CursorCache> cache,
    ecs::Query<helios::input::Cursor&> cursors) const {
  if (!context->input_enabled) [[unlikely]] {
    return;
  }

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
  if (!windows.has_value()) [[unlikely]] {
    return;
  }
#endif

  for (auto&& [entity, cursor] : cursors.WithEntity()) {
    if (!cursor.dirty) {
      continue;
    }

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
    const window::WindowMap::Entry* entry = (*windows)->TryGet(entity);
    if (entry == nullptr || entry->window == nullptr) [[unlikely]] {
      continue;
    }
#endif

    if (cursor.custom.has_value()) {
      SDL_Cursor* created = CreateCustomCursor(*cursor.custom);
      if (created == nullptr) [[unlikely]] {
        cursor.dirty = false;
        continue;
      }

      SDL_Cursor*& custom = FindOrInsertCustom(*cache, entity);
      if (custom != nullptr) [[likely]] {
        SDL_DestroyCursor(custom);
      }
      custom = created;
      SDL_SetCursor(custom);
    } else {
      EraseCustom(*cache, entity);
      SDL_Cursor* standard = EnsureStandardCursor(*cache, cursor.icon);
      SDL_SetCursor(standard);
    }

    cursor.dirty = false;
  }
}

}  // namespace helios::sdl3::input
