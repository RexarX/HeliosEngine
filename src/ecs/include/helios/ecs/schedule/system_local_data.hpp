#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/resource/manager.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/world.hpp>
#include <helios/memory/arena_allocator.hpp>

#include <cstddef>
#include <functional>
#include <memory>

namespace helios::ecs {

/// @brief Options for system local data.
struct SystemLocalDataOptions {
  static constexpr size_t kDefaultPreallocatedSize = 1024 * 1;  // 1 KB
  size_t preallocated_size = kDefaultPreallocatedSize;
};

/**
 * @brief System-local arena allocator resource.
 * @details Inserted automatically into each system's `SystemLocalData`.
 * Request via `Local<LocalArena>` or `Local<const LocalArena>`.
 * Memory is reclaimed when the schedule applies deferred work or starts a new
 * run.
 */
struct LocalArena {
  /**
   * @brief Constructs a local arena referencing the given allocator.
   * @param allocator Per-system arena allocator
   */
  explicit LocalArena(mem::ArenaAllocator& allocator) noexcept
      : arena(allocator) {}

  LocalArena(const LocalArena&) noexcept = default;
  LocalArena(LocalArena&&) noexcept = default;
  ~LocalArena() noexcept = default;

  LocalArena& operator=(const LocalArena&) noexcept = default;
  LocalArena& operator=(LocalArena&&) noexcept = default;

  [[nodiscard]] mem::ArenaAllocator& operator*() const noexcept {
    return arena.get();
  }

  [[nodiscard]] mem::ArenaAllocator* operator->() const noexcept {
    return &arena.get();
  }

  /**
   * @brief Gets a pointer to the underlying arena allocator.
   * @return Pointer to the arena allocator
   */
  [[nodiscard]] mem::ArenaAllocator* GetPtr() const noexcept {
    return &arena.get();
  }

  std::reference_wrapper<mem::ArenaAllocator> arena;
};

static_assert(ResourceTrait<LocalArena>);

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

  SystemLocalData() { AddLocalArena(); }
  explicit SystemLocalData(SystemLocalDataOptions options)
      : allocator(options.preallocated_size),
        cmd_queue(&allocator),
        message_queue(&allocator),
        consumed_messages(&allocator) {
    AddLocalArena();
  }

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
    AddLocalArena();
    other.ReleaseMovedFrom();
  }

  ~SystemLocalData() = default;

  SystemLocalData& operator=(const SystemLocalData&) = delete;
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
    AddLocalArena();
    other.ReleaseMovedFrom();

    return *this;
  }

  /**
   * @brief Applies selected deferred local work.
   * @details Commands and messages share one arena — `ResetArena()` runs only
   * when `!HasPendingWork()` after the selected steps. Call `ExecuteCommands`
   * after `World::Flush()` so reserved entities exist.
   * @param world World to apply against
   * @param apply_commands Whether to execute the local command queue
   * @param merge_messages Whether to merge local messages / consumed registries
   */
  void Apply(World& world, bool apply_commands, bool merge_messages) {
    HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::SystemLocalData::Apply");

    if (apply_commands) {
      ExecuteCommands(world);
    }
    if (merge_messages) {
      MergeMessages(world);
    }
    if (!HasPendingWork()) {
      ResetArena();
    }
  }

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
   * @details Applies consumed-message removal, then merges this system's local
   * writes into the world current queue. Does **not** advance the
   * previous/current lifecycle — that happens via stage
   * `StageSettings::advance_messages` (`MessageManager::Update()`).
   * @param world World to merge messages into
   */
  void MergeMessages(World& world) {
    auto& message_manager = world.Messages();

    if (!consumed_messages.Empty()) {
      message_manager.ApplyConsumed(consumed_messages);
    }
    consumed_messages.Clear();

    message_manager.MergeLocalMessages(std::move(message_queue));
    message_queue.ClearAll();
  }

  /// @brief Resets the arena allocator and clears all local data.
  /// @details Polymorphic allocator types are move-constructible but not
  /// move-assignable, so fresh objects are constructed in place after
  /// destroying the stale ones whose internal storage was invalidated by
  /// the arena reset.
  void ResetArena() noexcept {
    std::destroy_at(&cmd_queue);
    std::destroy_at(&message_queue);
    std::destroy_at(&consumed_messages);

    allocator.Reset();

    std::construct_at(&cmd_queue, &allocator);
    std::construct_at(&message_queue, &allocator);
    std::construct_at(&consumed_messages, &allocator);
  }

  /// @brief Inserts or replaces the system-local arena resource.
  void AddLocalArena() noexcept {
    resource_manager.Insert(LocalArena{allocator});
  }

private:
  /// @brief Rebuilds moved-from PMR members against the empty source allocator.
  /// @details Must run before the destination destructor frees stolen arena
  /// blocks. MSVC iterator debugging stores proxy nodes in the arena; leaving
  /// them in the source queues UAF when the destination is destroyed first.
  void ReleaseMovedFrom() noexcept {
    Reconstruct(cmd_queue, &allocator);
    Reconstruct(message_queue, &allocator);
    Reconstruct(consumed_messages, &allocator);
    resource_manager.Clear();
  }

  template <typename T, typename... Args>
  static void Reconstruct(T& obj, Args&&... args) {
    std::destroy_at(&obj);
    std::construct_at(&obj, std::forward<Args>(args)...);
  }
};

}  // namespace helios::ecs
