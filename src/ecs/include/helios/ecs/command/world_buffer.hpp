#pragma once

#include <helios/ecs/builtin_commands.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/resource/resource.hpp>

#include <concepts>
#include <memory_resource>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief Command buffer for deferred `World` operations.
 * @details Collects commands and then pushes them into a queue.
 * Commands are enqueued in the order they were added, ensuring predictable
 * behavior.
 * @note Not thread-safe.
 * @tparam Allocator Allocator type for command storage (default:
 * `std::allocator<std::byte>`)
 */
template <typename Allocator = std::allocator<std::byte>>
class WorldCmdBuffer {
public:
  using size_type = CmdQueue<Allocator>::size_type;
  using allocator_type = CmdQueue<Allocator>::allocator_type;

  /**
   * @brief Constructs a world command buffer with a custom allocator.
   * @param queue Queue to push commands into
   * @param allocator Allocator instance
   */
  explicit constexpr WorldCmdBuffer(CmdQueue<Allocator>& queue,
                                    allocator_type allocator = allocator_type{})
      : commands_(allocator), queue_(queue) {}

  /**
   * @brief Constructs a world command buffer from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param queue Queue to push commands into
   * @param resource Memory resource used to construct allocator
   */
  constexpr WorldCmdBuffer(CmdQueue<Allocator>& queue,
                           std::pmr::memory_resource* resource)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : WorldCmdBuffer(queue, allocator_type{resource}) {}

  WorldCmdBuffer(CmdQueue<Allocator>& queue, std::nullptr_t) = delete;

  WorldCmdBuffer(const WorldCmdBuffer&) = delete;
  WorldCmdBuffer(WorldCmdBuffer&&) = delete;
  constexpr ~WorldCmdBuffer() { queue_.Merge(std::move(commands_)); }

  WorldCmdBuffer& operator=(const WorldCmdBuffer&) = delete;
  WorldCmdBuffer& operator=(WorldCmdBuffer&&) = delete;

  /**
   * @brief Clears all pending commands from the buffer.
   * @details Removes all commands currently in the buffer.
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
   * @brief Enqueues a command to insert a resource into the world.
   * @details Replaces existing resource if present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ResourceTrait T>
  auto InsertResource(this auto&& self, T&& resource)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try to insert a resource if not present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ResourceTrait T>
  auto TryInsertResource(this auto&& self, T&& resource)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to remove a resource from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion if resource does not exist.
   * @tparam T Resource type
   */
  template <ResourceTrait T>
  auto RemoveResource(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try to remove a resource.
   * @note Not thread-safe.
   * @tparam T Resource type
   */
  template <ResourceTrait T>
  auto TryRemoveResource(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to be executed during the next `World::Update()`.
   * @details Equivalent to enqueueing a `FunctionCmd` with the given callable.
   * @note Not thread-safe.
   * @tparam F Callable type, must have signature `void(World&)`
   * @param command Command to enqueue
   */
  template <typename F>
    requires std::invocable<F, World&>
  auto DeferredUpdate(this auto&& self, F&& command)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Checks if the buffer is empty.
   * @return True if buffer is empty, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return commands_.Empty();
  }

  /**
   * @brief Gets the number of commands in the buffer.
   * @return Number of commands in buffer
   */
  [[nodiscard]] constexpr size_type Size() const noexcept {
    return commands_.Size();
  }

  /**
   * @brief Gets the allocator used by the command storage.
   * @return Allocator instance
   */
  [[nodiscard]] constexpr allocator_type GetAllocator() const
      noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) {
    return commands_.GetAllocator();
  }

private:
  CmdQueue<Allocator> commands_;
  CmdQueue<Allocator>& queue_;
};

template <typename Allocator>
template <ResourceTrait T>
inline auto WorldCmdBuffer<Allocator>::InsertResource(this auto&& self,
                                                      T&& resource)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(InsertResourceCmd<T>(std::forward<T>(resource)));
  return std::forward<decltype(self)>(self);
}

template <typename Allocator>
template <ResourceTrait T>
inline auto WorldCmdBuffer<Allocator>::TryInsertResource(this auto&& self,
                                                         T&& resource)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryInsertResourceCmd<T>(std::forward<T>(resource)));
  return std::forward<decltype(self)>(self);
}

template <typename Allocator>
template <ResourceTrait T>
inline auto WorldCmdBuffer<Allocator>::RemoveResource(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(RemoveResourceCmd<T>());
  return std::forward<decltype(self)>(self);
}

template <typename Allocator>
template <ResourceTrait T>
inline auto WorldCmdBuffer<Allocator>::TryRemoveResource(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryRemoveResourceCmd<T>());
  return std::forward<decltype(self)>(self);
}

template <typename Allocator>
template <typename F>
  requires std::invocable<F, World&>
inline auto WorldCmdBuffer<Allocator>::DeferredUpdate(this auto&& self,
                                                      F&& command)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(FunctionCmd(std::forward<F>(command)));
  return std::forward<decltype(self)>(self);
}

/**
 * @brief Command buffer for deferred `World` operations that uses a
 * polymorphic allocator.
 * @details Collects commands and then pushes them into a queue.
 * Commands are enqueued in the order they were added, ensuring predictable
 * behavior.
 * @note Not thread-safe.
 * @tparam Allocator Allocator type for command storage (default:
 * `std::allocator<std::byte>`)
 */
using PmrWorldCmdBuffer =
    WorldCmdBuffer<std::pmr::polymorphic_allocator<std::byte>>;

}  // namespace helios::ecs
