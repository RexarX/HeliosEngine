#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/command.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Command queue for deferred ECS operations.
 * @details Provides a queue for commands that will be executed during World::Update().
 * Commands are executed in the order they were enqueued, ensuring predictable behavior.
 *
 * Features:
 * - Simple vector-based implementation for main thread usage
 * - Bulk operations for better performance
 * - Type-safe command emplacement
 *
 * @note Not thread-safe.
 * All operations must be performed on the main thread.
 */
class CmdQueue {
public:
  CmdQueue() = default;
  CmdQueue(const CmdQueue&) = delete;
  CmdQueue(CmdQueue&&) noexcept = default;
  ~CmdQueue() = default;

  CmdQueue& operator=(const CmdQueue&) = delete;
  CmdQueue& operator=(CmdQueue&&) noexcept = default;

  /**
   * @brief Clears all pending commands from the queue.
   * @details Removes all commands currently in the queue.
   */
  void Clear() noexcept { commands_.clear(); }

  /**
   * @brief Reserves capacity for commands.
   * @details Pre-allocates memory to avoid reallocations during enqueue operations.
   * @param capacity Number of commands to reserve space for
   */
  void Reserve(size_t capacity) { commands_.reserve(capacity); }

  /**
   * @brief Constructs and enqueues a command in-place.
   * @details Creates a command of type T by forwarding arguments to its constructor.
   * @tparam T Command type that derives from Command
   * @tparam Args Argument types for T's constructor
   * @param args Arguments to forward to T's constructor
   */
  template <CommandTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Args&&... args) {
    commands_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

  /**
   * @brief Enqueues a pre-constructed command.
   * @details Adds an already constructed command to the queue.
   * @warning Triggers assertion if command is nullptr.
   * @param command Unique pointer to command to enqueue
   */
  void Enqueue(std::unique_ptr<Command> command);

  /**
   * @brief Enqueues multiple commands in bulk.
   * @details Efficiently enqueues a range of commands in a single operation.
   * The range is consumed (moved from) during this operation.
   * @warning Triggers assertion if any command in range is nullptr.
   * @tparam R Range type containing unique_ptr<Command> elements
   * @param commands Range of commands to enqueue (will be moved from)
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, std::unique_ptr<Command>>
  void EnqueueBulk(R&& commands);

  /**
   * @brief Gets all commands and clears the queue.
   * @details Moves all commands out of the queue, leaving it empty.
   * @return Vector containing all commands that were in the queue
   */
  [[nodiscard]] auto DequeueAll() noexcept -> std::vector<std::unique_ptr<Command>> {
    return std::exchange(commands_, {});
  }

  /**
   * @brief Checks if the queue is empty.
   * @details Returns true if there are no commands in the queue.
   * @return True if queue is empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return commands_.empty(); }

  /**
   * @brief Gets the number of commands in the queue.
   * @details Returns the exact size of the queue.
   * @return Number of commands in queue
   */
  [[nodiscard]] size_t Size() const noexcept { return commands_.size(); }

private:
  std::vector<std::unique_ptr<Command>> commands_;  ///< Vector storing command pointers
};

inline void CmdQueue::Enqueue(std::unique_ptr<Command> command) {
  HELIOS_ASSERT(command != nullptr, "Failed to enqueue command: command is nullptr!");
  commands_.push_back(std::move(command));
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, std::unique_ptr<Command>>
inline void CmdQueue::EnqueueBulk(R&& commands) {
  if constexpr (std::ranges::sized_range<R>) {
    // Reserve space to avoid multiple reallocations
    commands_.reserve(commands_.size() + std::ranges::size(commands));
  }

  // Move all commands into the queue
  for (auto&& command : commands) {
    HELIOS_ASSERT(command != nullptr, "Failed to enqueue commands in bulk: one of the commands is nullptr!");
    commands_.push_back(std::move(command));
  }
}

}  // namespace helios::ecs::details
