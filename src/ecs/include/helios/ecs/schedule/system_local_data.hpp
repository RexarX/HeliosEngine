#pragma once

#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/resource/manager.hpp>
#include <helios/ecs/world.hpp>
#include <helios/memory/arena_allocator.hpp>

#include <cstddef>
#include <memory>

namespace helios::ecs {

/// @brief Options for system local data.
struct SystemLocalDataOptions {
  static constexpr size_t kDefaultPreallocatedSize = 1024 * 1;  // 1 KB
  size_t preallocated_size = kDefaultPreallocatedSize;
};

/// @brief Local data for a system.
struct SystemLocalData {
  mem::ArenaAllocator allocator;  ///< local arena allocator

  PmrCmdQueue cmd_queue{&allocator};          ///< local command queue
  PmrMessageQueue message_queue{&allocator};  ///< local message queue

  /// local consumed messages registry
  PmrConsumedMessagesRegistry consumed_messages{&allocator};
  ResourceManager resource_manager;  ///< local resource manager

  /**
   * @brief Creates system local data from system local data options.
   * @param options System local data options
   * @return System local data
   */
  [[nodiscard]] static SystemLocalData From(
      SystemLocalDataOptions options = {}) {
    return SystemLocalData(options);
  }

  SystemLocalData() = default;
  explicit SystemLocalData(SystemLocalDataOptions options)
      : allocator(options.preallocated_size),
        cmd_queue(&allocator),
        message_queue(&allocator),
        consumed_messages(&allocator) {}

  SystemLocalData(const SystemLocalData&) = delete;
  SystemLocalData(SystemLocalData&& other) noexcept
      : allocator(std::move(other.allocator)),
        cmd_queue(&allocator),
        message_queue(&allocator),
        consumed_messages(&allocator),
        resource_manager(std::move(other.resource_manager)) {
    cmd_queue.Merge(std::move(other.cmd_queue));
    message_queue.Merge(std::move(other.message_queue));
    consumed_messages.MergeFrom(std::move(other.consumed_messages));
  }
  ~SystemLocalData() = default;

  SystemLocalData& operator=(SystemLocalData&& other) noexcept {
    if (this == &other) [[unlikely]] {
      return *this;
    }
    cmd_queue.Clear();
    message_queue.ClearAll();
    consumed_messages.Clear();
    resource_manager.Clear();

    allocator = std::move(other.allocator);

    cmd_queue.Merge(std::move(other.cmd_queue));
    message_queue.Merge(std::move(other.message_queue));
    consumed_messages.MergeFrom(std::move(other.consumed_messages));
    resource_manager = std::move(other.resource_manager);

    return *this;
  }
  SystemLocalData& operator=(const SystemLocalData&) = delete;

  /**
   * @brief Updates the system local data by executing commands and merging
   * messages.
   * @details Call after `World::Flush()` so reserved entities exist before
   * command execution.
   * @param world World to update
   */
  void Update(World& world) {
    HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::SystemLocalData::Update");

    ExecuteCommands(world);
    MergeMessages(world);
    ResetArena();
  }

  /// @brief Clears the system local data.
  void Clear() { ResetArena(); }

  /**
   * @brief Checks whether commands or messages are still pending application.
   * @return True if local commands, messages, or consumed-message bookkeeping
   * remain unapplied
   */
  [[nodiscard]] bool HasPendingWork() const noexcept {
    return !cmd_queue.Empty() || !consumed_messages.Empty() ||
           message_queue.MessageCount() > 0;
  }

  /**
   * @brief Executes all commands in the local command queue.
   * @param world World to execute commands on
   */
  void ExecuteCommands(World& world) { cmd_queue.ExecuteAll(world); }

  /**
   * @brief Merges messages from the local message queue into the world message
   * manager.
   * @param world World to merge messages into
   */
  void MergeMessages(World& world) {
    auto& message_manager = world.Messages();

    // Remove consumed messages and clear the consumed registry
    message_manager.Update(consumed_messages);
    consumed_messages.Clear();

    // Merge local messages and clear the local message queue
    message_manager.MergeLocalMessages(std::move(message_queue));
    message_queue.ClearAll();
  }

  /// @brief Resets the arena allocator and clears all local data.
  /// @details Polymorphic allocator types are move-constructible but not
  /// move-assignable, so fresh objects are constructed in place after
  /// destroying the stale ones whose internal storage was invalidated by
  /// the arena reset.
  void ResetArena() noexcept {
    resource_manager.Clear();
    allocator.Reset();

    Reconstruct(cmd_queue, &allocator);
    Reconstruct(message_queue, &allocator);
    Reconstruct(consumed_messages, &allocator);
  }

private:
  template <typename T, typename... Args>
  static void Reconstruct(T& obj, Args&&... args) {
    std::destroy_at(&obj);
    std::construct_at(&obj, std::forward<Args>(args)...);
  }
};

}  // namespace helios::ecs
