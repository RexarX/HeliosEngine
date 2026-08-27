#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.glfw;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/entity/entity.hpp>
#include <helios/memory/ref_counted.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;

}

HELIOS_MODULE_EXPORT struct GLFWwindow;

HELIOS_MODULE_EXPORT
namespace helios::glfw {

/// @brief Per-window user data stored on `GLFWwindow`.
struct NativeUserData final : public mem::ArcFromThis<NativeUserData> {
  ecs::World* world = nullptr;
  ecs::Entity entity;
};

/// @brief GLFW initialization state.
struct Context {
  static constexpr std::string_view kName = "helios::glfw::Context";

  ecs::World* world = nullptr;
  void (*frame_pump)(void* user_data) = nullptr;
  void* frame_pump_user_data = nullptr;
  /// @brief Last Win32 clipboard sequence number observed by `SyncClipboard`.
  uint32_t clipboard_sequence = 0;
  bool initialized = false;
  bool in_event_poll = false;
  /// @brief True while a nested `FramePumpOrder` is running from a GLFW
  /// callback.
  bool in_nested_pump = false;
  /// @brief True when the input module messages are registered.
  bool input_enabled = false;
  /// @brief True when `clipboard_sequence` has been sampled at least once.
  bool clipboard_sequence_valid = false;
  /// @brief True when a non-Win32 backend should re-read the OS clipboard.
  bool clipboard_dirty = false;
};

/// @brief Native GLFW window entry for a window entity.
struct NativeEntry {
  GLFWwindow* window = nullptr;
  mem::Arc<NativeUserData> user_data;
  double last_cursor_x = 0.0;
  double last_cursor_y = 0.0;
  bool has_cursor = false;
};

/// @brief Sorted native `GLFWwindow` handles keyed by window entity.
struct NativeWindows {
  static constexpr std::string_view kName = "helios::glfw::NativeWindows";

  struct Entry {
    ecs::Entity entity;
    NativeEntry native;
  };

  std::vector<Entry> entries;

  /**
   * @brief Inserts a native entry for an entity.
   * @param entity Window entity
   * @param entry Native GLFW entry
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

void RegisterCallbacks(GLFWwindow& window, NativeUserData& user_data);

void RegisterMonitorCallback(ecs::World& world);

void UnregisterMonitorCallback();

void RegisterErrorCallback();

}  // namespace helios::glfw
#endif  // HELIOS_MODULE_CONSUMER_SHIM
