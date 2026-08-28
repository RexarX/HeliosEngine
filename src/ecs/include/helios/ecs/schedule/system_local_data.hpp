#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/memory/arena_allocator.hpp>

#include <cstddef>
#include <memory>
#endif
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/queue.hpp>
#include <helios/ecs/resource/manager.hpp>
#include <helios/ecs/resource/resource.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;

/// @brief Options for system local data.
struct SystemLocalDataOptions {
  static constexpr size_t kDefaultPreallocatedSize = 1UZ << 10UZ;  // 1 KB
  size_t preallocated_size = kDefaultPreallocatedSize;
};

/// @brief Local data for a system.
struct SystemLocalData {
  mem::ArenaAllocator allocator;  ///< local arena allocator

  CmdQueue cmd_queue{&allocator};          ///< local command queue
  MessageQueue message_queue{&allocator};  ///< local message queue

  /// local consumed messages registry
  ConsumedMessagesRegistry consumed_messages{&allocator};
  /// Local resources (including message cursors). Uses the default PMR
  /// resource so entries survive `ResetArena()`, which reclaims the
  /// command / message bump arena.
  ResourceManager resource_manager;

  /**
   * @brief Creates system local data from system local data options.
   * @param options System local data options
   * @return System local data
   */
  [[nodiscard]] static SystemLocalData From(
      SystemLocalDataOptions options = {}) {
    return SystemLocalData(options);
  }

  SystemLocalData() { AddLocalArena(); }
  explicit SystemLocalData(SystemLocalDataOptions options)
      : allocator(options.preallocated_size),
        cmd_queue(&allocator),
        message_queue(&allocator),
        consumed_messages(&allocator) {
    AddLocalArena();
  }

  SystemLocalData(const SystemLocalData&) = delete;
  SystemLocalData(SystemLocalData&& other) noexcept;
  ~SystemLocalData() = default;

  SystemLocalData& operator=(const SystemLocalData&) = delete;
  SystemLocalData& operator=(SystemLocalData&& other) noexcept;

  /**
   * @brief Applies selected deferred local work.
   * @details Commands and messages share one arena — `ResetArena()` runs only
   * when `!HasPendingWork()` after the selected steps. Call `ExecuteCommands`
   * after `World::Flush()` so reserved entities exist.
   * @param world World to apply against
   * @param apply_commands Whether to execute the local command queue
   * @param merge_messages Whether to merge local messages / consumed registries
   */
  void Apply(World& world, bool apply_commands, bool merge_messages);

  /**
   * @brief Updates the system local data by executing commands and merging
   * messages.
   * @details Equivalent to `Apply(world, true, true)`. Call after
   * `World::Flush()` so reserved entities exist before command execution.
   * @param world World to update
   */
  void Update(World& world) {
    HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::SystemLocalData::Update");
    Apply(world, true, true);
  }

  /// @brief Clears the system local data.
  void Clear() {
    resource_manager.Clear();
    ResetArena();
  }

  /**
   * @brief Executes all commands in the local command queue.
   * @param world World to execute commands on
   */
  void ExecuteCommands(World& world) { cmd_queue.ExecuteAll(world); }

  /**
   * @brief Merges messages from the local message queue into the world message
   * manager.
   * @details Applies consumed-message removal, then merges this system's local
   * writes into the world current queue. Does **not** advance the
   * previous/current lifecycle — that happens via stage
   * `StageSettings::advance_messages` (`MessageManager::Update()`).
   * @param world World to merge messages into
   */
  void MergeMessages(World& world);

  /**
   * @brief Resets the arena allocator and clears all local data.
   * @details Polymorphic allocator types are move-constructible but not
   * move-assignable, so fresh objects are constructed in place after
   * destroying the stale ones whose internal storage was invalidated by
   * the arena reset.
   */
  void ResetArena() noexcept;

  /// @brief Inserts or replaces the system-local arena resource.
  void AddLocalArena() noexcept;

  /**
   * @brief Checks whether commands or messages are still pending application.
   * @return True if local commands, messages, or consumed-message bookkeeping
   * remain unapplied
   */
  [[nodiscard]] bool HasPendingWork() const noexcept {
    return !cmd_queue.Empty() || !consumed_messages.Empty() ||
           message_queue.MessageCount() > 0;
  }

private:
  /**
   * @brief Rebuilds moved-from PMR members against the source allocator.
   * @details The source keeps its arena (PMR containers bind the allocator
   * object address). Queues are reconstructed empty so the source destructor
   * does not walk storage that was merged into the destination.
   */
  void ReleaseMovedFrom() noexcept;

  template <typename T, typename... Args>
  static void Reconstruct(T& obj, Args&&... args) {
    std::destroy_at(&obj);
    std::construct_at(&obj, std::forward<Args>(args)...);
  }
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
