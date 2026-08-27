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
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT struct SDL_Window;

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Per-backend clipboard and initialization state.
struct Context {
  static constexpr std::string_view kName = "helios::sdl3::window::Context";

  /// @brief Last Win32 clipboard sequence number observed by `SyncClipboard`.
  uint32_t clipboard_sequence = 0;
  bool initialized = false;
  /// @brief True when `clipboard_sequence` has been sampled at least once.
  bool clipboard_sequence_valid = false;
  /// @brief True when a non-Win32 backend should re-read the OS clipboard.
  bool clipboard_dirty = false;
};

/// @brief Native SDL window entry for a window entity.
struct NativeEntry {
  SDL_Window* window = nullptr;
};

/// @brief Sorted native `SDL_Window` handles keyed by window entity.
struct NativeWindows {
  static constexpr std::string_view kName =
      "helios::sdl3::window::NativeWindows";

  struct Entry {
    ecs::Entity entity;
    NativeEntry native;
  };

  std::vector<Entry> entries;

  /**
   * @brief Inserts a native entry for an entity.
   * @param entity Window entity
   * @param entry Native SDL entry
   * @warning The entity must not already have an entry
   */
  constexpr void Insert(ecs::Entity entity, NativeEntry entry);

  /**
   * @brief Removes the native entry for an entity.
   * @param entity Window entity
   * @return True when an entry was removed
   */
  constexpr bool Erase(ecs::Entity entity);

  /**
   * @brief Finds a native entry for an entity.
   * @param entity Window entity
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr Entry* TryGet(ecs::Entity entity) noexcept;

  /**
   * @brief Finds a native entry for an entity.
   * @param entity Window entity
   * @return Pointer to the entry when found, otherwise `nullptr`
   */
  [[nodiscard]] constexpr const Entry* TryGet(
      ecs::Entity entity) const noexcept;

  /**
   * @brief Tests whether a native entry exists for an entity.
   * @param entity Window entity
   * @return True when an entry exists
   */
  [[nodiscard]] constexpr bool Contains(ecs::Entity entity) const noexcept {
    return TryGet(entity) != nullptr;
  }

  /**
   * @brief Tests whether no native entries are stored.
   * @return True when empty
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return entries.empty();
  }

  /**
   * @brief Returns the number of native entries.
   * @return Entry count
   */
  [[nodiscard]] constexpr size_t Size() const noexcept {
    return entries.size();
  }
};

[[nodiscard]] constexpr auto FindEntry(
    std::vector<NativeWindows::Entry>& entries, ecs::Entity entity) {
  return std::lower_bound(
      entries.begin(), entries.end(), entity,
      [](const NativeWindows::Entry& entry, ecs::Entity value) {
        return entry.entity < value;
      });
}

[[nodiscard]] constexpr auto FindEntry(
    const std::vector<NativeWindows::Entry>& entries, ecs::Entity entity) {
  return std::lower_bound(
      entries.begin(), entries.end(), entity,
      [](const NativeWindows::Entry& entry, ecs::Entity value) {
        return entry.entity < value;
      });
}

constexpr void NativeWindows::Insert(ecs::Entity entity, NativeEntry entry) {
  const auto it = FindEntry(entries, entity);
  HELIOS_ASSERT(it == entries.end() || it->entity != entity,
                "Native window entry already exists for entity '{}'!", entity);
  entries.insert(it, Entry{.entity = entity, .native = std::move(entry)});
}

constexpr bool NativeWindows::Erase(ecs::Entity entity) {
  const auto it = FindEntry(entries, entity);
  if (it == entries.end() || it->entity != entity) {
    return false;
  }
  entries.erase(it);
  return true;
}

constexpr auto NativeWindows::TryGet(ecs::Entity entity) noexcept -> Entry* {
  const auto it = FindEntry(entries, entity);
  if (it == entries.end() || it->entity != entity) {
    return nullptr;
  }
  return &*it;
}

constexpr auto NativeWindows::TryGet(ecs::Entity entity) const noexcept
    -> const Entry* {
  const auto it = FindEntry(entries, entity);
  if (it == entries.end() || it->entity != entity) {
    return nullptr;
  }
  return &*it;
}

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
