#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif
#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/entity/entity.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT struct SDL_Window;
HELIOS_MODULE_EXPORT using SDL_WindowID = uint32_t;

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Maps window entities to native SDL window handles.
struct WindowMap {
  static constexpr std::string_view kName = "helios::sdl3::window::WindowMap";

  struct Entry {
    SDL_Window* window = nullptr;
    ecs::Entity entity;
    SDL_WindowID window_id = 0;
  };

  std::vector<Entry> entries;

  /**
   * @brief Inserts a mapping for a window entity.
   * @param entity Window entity
   * @param window Native SDL window
   * @param window_id SDL window identifier
   * @warning The entity must not already be registered
   */
  constexpr void Insert(ecs::Entity entity, SDL_Window* window,
                        SDL_WindowID window_id);

  /**
   * @brief Removes the mapping for a window entity.
   * @param entity Window entity
   * @return True when an entry was removed
   */
  constexpr bool Erase(ecs::Entity entity);

  /**
   * @brief Finds an entry by window entity.
   * @param entity Window entity
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr Entry* TryGet(ecs::Entity entity) noexcept;

  /**
   * @brief Finds an entry by window entity.
   * @param entity Window entity
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr const Entry* TryGet(
      ecs::Entity entity) const noexcept;

  /**
   * @brief Finds an entry by SDL window id.
   * @param window_id SDL window identifier
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr Entry* TryGetByWindowId(
      SDL_WindowID window_id) noexcept;

  /**
   * @brief Finds an entry by SDL window id.
   * @param window_id SDL window identifier
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr const Entry* TryGetByWindowId(
      SDL_WindowID window_id) const noexcept;

  /**
   * @brief Finds an entry by native window pointer.
   * @param window Native SDL window
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr Entry* TryGetByWindow(SDL_Window* window) noexcept;

  /**
   * @brief Finds an entry by native window pointer.
   * @param window Native SDL window
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr const Entry* TryGetByWindow(
      SDL_Window* window) const noexcept;

  /**
   * @brief Tests whether a mapping exists for an entity.
   * @param entity Window entity
   * @return True when registered
   */
  [[nodiscard]] constexpr bool Contains(ecs::Entity entity) const noexcept {
    return TryGet(entity) != nullptr;
  }
};

[[nodiscard]] constexpr auto FindWindowMapEntry(
    std::vector<WindowMap::Entry>& entries, ecs::Entity entity) {
  return std::lower_bound(entries.begin(), entries.end(), entity,
                          [](const WindowMap::Entry& entry, ecs::Entity value) {
                            return entry.entity < value;
                          });
}

[[nodiscard]] constexpr auto FindWindowMapEntry(
    const std::vector<WindowMap::Entry>& entries, ecs::Entity entity) {
  return std::lower_bound(entries.begin(), entries.end(), entity,
                          [](const WindowMap::Entry& entry, ecs::Entity value) {
                            return entry.entity < value;
                          });
}

constexpr void WindowMap::Insert(ecs::Entity entity, SDL_Window* window,
                                 SDL_WindowID window_id) {
  const auto it = FindWindowMapEntry(entries, entity);
  HELIOS_ASSERT(it == entries.end() || it->entity != entity,
                "SDL window entry already exists for entity '{}'!", entity);
  entries.insert(
      it, Entry{.window = window, .entity = entity, .window_id = window_id});
}

constexpr bool WindowMap::Erase(ecs::Entity entity) {
  const auto it = FindWindowMapEntry(entries, entity);
  if (it == entries.end() || it->entity != entity) {
    return false;
  }
  entries.erase(it);
  return true;
}

constexpr auto WindowMap::TryGet(ecs::Entity entity) noexcept -> Entry* {
  const auto it = FindWindowMapEntry(entries, entity);
  if (it == entries.end() || it->entity != entity) {
    return nullptr;
  }
  return &*it;
}

constexpr auto WindowMap::TryGet(ecs::Entity entity) const noexcept
    -> const Entry* {
  const auto it = FindWindowMapEntry(entries, entity);
  if (it == entries.end() || it->entity != entity) {
    return nullptr;
  }
  return &*it;
}

constexpr auto WindowMap::TryGetByWindowId(SDL_WindowID window_id) noexcept
    -> Entry* {
  for (auto& entry : entries) {
    if (entry.window_id == window_id) {
      return &entry;
    }
  }
  return nullptr;
}

constexpr auto WindowMap::TryGetByWindowId(
    SDL_WindowID window_id) const noexcept -> const Entry* {
  for (const auto& entry : entries) {
    if (entry.window_id == window_id) {
      return &entry;
    }
  }
  return nullptr;
}

constexpr auto WindowMap::TryGetByWindow(SDL_Window* window) noexcept
    -> Entry* {
  if (window == nullptr) [[unlikely]] {
    return nullptr;
  }

  for (auto& entry : entries) {
    if (entry.window == window) {
      return &entry;
    }
  }
  return nullptr;
}

constexpr auto WindowMap::TryGetByWindow(SDL_Window* window) const noexcept
    -> const Entry* {
  if (window == nullptr) [[unlikely]] {
    return nullptr;
  }

  for (const auto& entry : entries) {
    if (entry.window == window) {
      return &entry;
    }
  }
  return nullptr;
}

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
