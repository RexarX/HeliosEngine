#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/details/event_queue.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Default initial capacity for the per-system frame allocator (64KB).
 */
inline constexpr size_t kDefaultFrameAllocatorCapacity = 64UZ * 1024UZ;

/**
 * @brief Local storage for system-specific data (commands, events, and temporary allocations).
 * @details Each system gets its own local storage during execution to avoid contention.
 * After system execution, the local storage is flushed to the appropriate global queues.
 *
 * The frame allocator is designed for temporary per-frame allocations within a system.
 * It uses a growable allocator internally to automatically expand if the initial capacity is exceeded.
 * The allocator should be reset at frame boundaries or after each system execution.
 *
 * @note Not thread-safe - each system has its own instance.
 */
class SystemLocalStorage {
public:
  using FrameAllocatorType = memory::GrowableAllocator<memory::FrameAllocator>;

  /**
   * @brief Constructs system local storage with default frame allocator capacity.
   */
  SystemLocalStorage() : frame_allocator_(kDefaultFrameAllocatorCapacity) {}

  /**
   * @brief Constructs system local storage with specified frame allocator capacity.
   * @param frame_allocator_capacity Initial capacity for the frame allocator in bytes
   */
  explicit SystemLocalStorage(size_t frame_allocator_capacity) : frame_allocator_(frame_allocator_capacity) {}
  SystemLocalStorage(const SystemLocalStorage&) = delete;
  SystemLocalStorage(SystemLocalStorage&&) noexcept = default;
  ~SystemLocalStorage() = default;

  SystemLocalStorage& operator=(const SystemLocalStorage&) = delete;
  SystemLocalStorage& operator=(SystemLocalStorage&&) noexcept = default;

  /**
   * @brief Clears all stored commands and events.
   * @note Does not reset the frame allocator. Call ResetFrameAllocator() separately.
   */
  void Clear();

  /**
   * @brief Clears commands, events, and resets the frame allocator.
   * @details Call this at frame boundaries to reclaim all temporary memory.
   */
  void ClearAll();

  /**
   * @brief Adds a command to the local command buffer.
   * @tparam T Command type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to command constructor
   */
  template <CommandTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceCommand(Args&&... args) {
    commands_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

  /**
   * @brief Adds a pre-constructed command to the local buffer.
   * @param command Unique pointer to command
   */
  void AddCommand(std::unique_ptr<Command>&& command);

  /**
   * @brief Reserves space for commands.
   * @param capacity Number of commands to reserve space for
   */
  void ReserveCommands(size_t capacity) { commands_.reserve(capacity); }

  /**
   * @brief Writes an event to the local event queue.
   * @tparam T Event type
   * @param event Event to store
   */
  template <EventTrait T>
  void WriteEvent(const T& event);

  /**
   * @brief Writes multiple events to the local event queue in bulk.
   * @tparam R Range of events
   * @param events Range of events to store
   */
  template <std::ranges::sized_range R>
    requires EventTrait<std::ranges::range_value_t<R>>
  void WriteEventBulk(const R& events);

  /**
   * @brief Resets the frame allocator, freeing all temporary allocations.
   * @details Call this at frame boundaries or after system execution to reclaim memory.
   * @warning All pointers obtained from the frame allocator become invalid after this call.
   * Do not store references or pointers to frame-allocated data beyond the current frame.
   */
  void ResetFrameAllocator() noexcept { frame_allocator_.Reset(); }

  /**
   * @brief Checks if both commands and events are empty.
   * @return True if no commands or events are stored, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return commands_.empty() && events_.Empty(); }

  /**
   * @brief Gets the number of commands in the buffer.
   * @return Count of commands
   */
  [[nodiscard]] size_t CommandCount() const noexcept { return commands_.size(); }

  /**
   * @brief Gets frame allocator statistics.
   * @return Allocator statistics with current usage information
   */
  [[nodiscard]] memory::AllocatorStats FrameAllocatorStats() const noexcept { return frame_allocator_.Stats(); }

  /**
   * @brief Gets total capacity of the frame allocator across all internal allocators.
   * @return Total capacity in bytes
   */
  [[nodiscard]] size_t FrameAllocatorCapacity() const noexcept { return frame_allocator_.TotalCapacity(); }

  /**
   * @brief Gets reference to the command buffer.
   * @return Reference to command vector
   */
  [[nodiscard]] auto GetCommands() noexcept -> std::vector<std::unique_ptr<Command>>& { return commands_; }

  /**
   * @brief Gets const reference to the command buffer.
   * @return Const reference to command vector
   */
  [[nodiscard]] auto GetCommands() const noexcept -> const std::vector<std::unique_ptr<Command>>& { return commands_; }

  /**
   * @brief Gets reference to the event queue.
   * @return Reference to event queue
   */
  [[nodiscard]] EventQueue& GetEventQueue() noexcept { return events_; }

  /**
   * @brief Gets const reference to the event queue.
   * @return Const reference to event queue
   */
  [[nodiscard]] const EventQueue& GetEventQueue() const noexcept { return events_; }

  /**
   * @brief Gets reference to the frame allocator.
   * @details Use this allocator for temporary per-frame allocations that don't need individual deallocation.
   * Memory is reclaimed when ResetFrameAllocator() is called.
   * @return Reference to the growable frame allocator
   */
  [[nodiscard]] FrameAllocatorType& FrameAllocator() noexcept { return frame_allocator_; }

  /**
   * @brief Gets const reference to the frame allocator.
   * @warning Data allocated with the frame allocator is only valid for the current frame.
   * @return Const reference to the growable frame allocator
   */
  [[nodiscard]] const FrameAllocatorType& FrameAllocator() const noexcept { return frame_allocator_; }

private:
  std::vector<std::unique_ptr<Command>> commands_;  ///< Local command buffer
  EventQueue events_;                               ///< Local event queue
  FrameAllocatorType frame_allocator_;              ///< Per-system frame allocator for temporary allocations
};

inline void SystemLocalStorage::AddCommand(std::unique_ptr<Command>&& command) {
  HELIOS_ASSERT(command != nullptr, "Failed to add command: command is null!");
  commands_.push_back(std::move(command));
}

inline void SystemLocalStorage::Clear() {
  commands_.clear();
  events_.Clear();
}

inline void SystemLocalStorage::ClearAll() {
  Clear();
  ResetFrameAllocator();
}

template <EventTrait T>
inline void SystemLocalStorage::WriteEvent(const T& event) {
  if (!events_.IsRegistered<T>()) {
    events_.Register<T>();
  }
  events_.Write(event);
}

template <std::ranges::sized_range R>
  requires EventTrait<std::ranges::range_value_t<R>>
inline void SystemLocalStorage::WriteEventBulk(const R& events) {
  using EventType = std::ranges::range_value_t<R>;
  if (!events_.IsRegistered<EventType>()) {
    events_.Register<EventType>();
  }
  events_.WriteBulk(events);
}

}  // namespace helios::ecs::details
