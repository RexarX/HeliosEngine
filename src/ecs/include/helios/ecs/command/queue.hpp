#pragma once

#include <helios/container/callable_buffer_array.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/details/profile.hpp>

#include <cstddef>
#include <memory_resource>
#include <ranges>

namespace helios::ecs {

class World;

/**
 * @brief Command queue for deferred ECS operations.
 * @details Provides a queue for commands that will be executed during
 * `World::Update()`.
 * Commands are executed in the order they were enqueued, ensuring predictable
 * behavior.
 * @note Not thread-safe.
 * @tparam Allocator Allocator type for command storage (default:
 * `std::allocator<std::byte>`)
 */
template <typename Allocator = std::allocator<std::byte>>
class CmdQueue {
private:
  using CommandStorage =
      container::CallableBufferArray<Allocator, void(World&)>;

public:
  using size_type = CommandStorage::size_type;
  using allocator_type = CommandStorage::allocator_type;

  /**
   * @brief Constructs a command queue with a custom allocator.
   * @param allocator Allocator instance
   */
  explicit constexpr CmdQueue(allocator_type allocator = allocator_type{})
      : commands_(std::move(allocator)) {}

  /**
   * @brief Constructs a command queue from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr CmdQueue(std::pmr::memory_resource* resource)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : CmdQueue(allocator_type{resource}) {}

  CmdQueue(std::nullptr_t) = delete;

  CmdQueue(const CmdQueue&) = delete;
  constexpr CmdQueue(CmdQueue&&) noexcept = default;
  ~CmdQueue() = default;

  CmdQueue& operator=(const CmdQueue&) = delete;
  CmdQueue& operator=(CmdQueue&&) noexcept = default;

  /**
   * @brief Clears all pending commands from the queue.
   * @details Removes all commands currently in the queue.
   */
  void Clear() noexcept { commands_.Clear(); }

  /**
   * @brief Reserves capacity for commands.
   * @details Pre-allocates memory to avoid reallocations during enqueue
   * operations.
   * @param capacity Number of commands to reserve space for
   */
  constexpr void Reserve(size_type capacity) { commands_.Reserve(capacity); }

  /**
   * @brief Merges another queue into this one by moving its commands.
   * @details Moves all commands from the other queue into this one.
   * @param other Queue to merge
   */
  constexpr void Merge(CmdQueue&& other) {
    commands_.Merge(std::move(other.commands_));
  }

  /**
   * @brief Enqueues a command by moving it into the queue.
   * @details Moves a command into the queue.
   * @tparam T Command type
   * @param command Command to enqueue
   */
  template <CommandTrait T>
  void Enqueue(T&& command);

  /**
   * @brief Enqueues multiple commands in bulk.
   * @details Moves a range of commands into the queue in a single operation.
   * @tparam R Range type
   * @param commands Range of commands to enqueue
   */
  template <std::ranges::input_range R>
    requires CommandTrait<std::ranges::range_value_t<R>>
  void EnqueueBulk(R&& commands);

  /**
   * @brief Executes all commands in the queue and clears the queue.
   * @param world Reference to the World instance
   */
  void ExecuteAll(World& world);

  /**
   * @brief Checks if the queue is empty.
   * @return True if queue is empty, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return commands_.Empty();
  }

  /**
   * @brief Gets the number of commands in the queue.
   * @return Number of commands in queue
   */
  [[nodiscard]] constexpr size_type Size() const noexcept {
    return commands_.Size();
  }

  /**
   * @brief Gets the allocator used by the command storage.
   * @return Allocator instance
   */
  [[nodiscard]] constexpr allocator_type GetAllocator() const {
    return commands_.GetAllocator();
  }

private:
  CommandStorage commands_;  ///< Container storing commands
};

template <typename Allocator>
template <CommandTrait T>
inline void CmdQueue<Allocator>::Enqueue(T&& command) {
  using DecayedT = std::remove_cvref_t<T>;
  commands_.template Push<&DecayedT::Execute>(std::forward<T>(command));
}

template <typename Allocator>
template <std::ranges::input_range R>
  requires CommandTrait<std::ranges::range_value_t<R>>
inline void CmdQueue<Allocator>::EnqueueBulk(R&& commands) {
  if constexpr (std::ranges::sized_range<R>) {
    commands_.Reserve(commands_.Size() + std::ranges::size(commands));
  }

  for (auto&& cmd : std::forward<R>(commands)) {
    Enqueue(std::forward<decltype(cmd)>(cmd));
  }
}

template <typename Allocator>
inline void CmdQueue<Allocator>::ExecuteAll(World& world) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::CmdQueue::ExecuteAll");
  HELIOS_ECS_PROFILE_ZONE_VALUE(commands_.Size());
  commands_.Invoke(world);
  Clear();
}

/**
 * @brief Command queue for deferred ECS operations that uses a polymorphic
 * allocator.
 * @details Provides a queue for commands that will be executed during
 * `World::Update()`.
 * Commands are executed in the order they were enqueued, ensuring predictable
 * behavior.
 * @note Not thread-safe.
 */
using PmrCmdQueue = CmdQueue<std::pmr::polymorphic_allocator<std::byte>>;

}  // namespace helios::ecs
