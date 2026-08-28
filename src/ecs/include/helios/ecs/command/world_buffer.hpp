#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstddef>
#include <memory_resource>
#endif
#include <helios/ecs/builtin_commands.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/resource/resource.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/**
 * @brief Command buffer for deferred `World` operations.
 * @details Collects commands and then pushes them into a queue.
 * Commands are enqueued in the order they were added, ensuring predictable
 * behavior.
 * @note Not thread-safe.
 */
class WorldCmdBuffer {
public:
  using size_type = CmdQueue::size_type;

  /**
   * @brief Constructs a world command buffer.
   * @param queue Queue to push commands into
   * @param resource Memory resource used for command storage
   */
  explicit constexpr WorldCmdBuffer(
      CmdQueue& queue,
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : commands_(resource), queue_(queue) {}

  WorldCmdBuffer(CmdQueue& queue, std::nullptr_t) = delete;

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
   * @brief Enqueues a command to be executed during the next `World::Flush()`.
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
   * @brief Returns the memory resource used for command storage.
   * @return Memory resource passed to the constructor, or the default resource
   */
  [[nodiscard]] constexpr std::pmr::memory_resource* GetMemoryResource()
      const noexcept {
    return commands_.GetMemoryResource();
  }

private:
  CmdQueue commands_;
  CmdQueue& queue_;
};

template <ResourceTrait T>
inline auto WorldCmdBuffer::InsertResource(this auto&& self, T&& resource)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(InsertResourceCmd<T>(std::forward<T>(resource)));
  return std::forward<decltype(self)>(self);
}

template <ResourceTrait T>
inline auto WorldCmdBuffer::TryInsertResource(this auto&& self, T&& resource)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryInsertResourceCmd<T>(std::forward<T>(resource)));
  return std::forward<decltype(self)>(self);
}

template <ResourceTrait T>
inline auto WorldCmdBuffer::RemoveResource(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(RemoveResourceCmd<T>());
  return std::forward<decltype(self)>(self);
}

template <ResourceTrait T>
inline auto WorldCmdBuffer::TryRemoveResource(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryRemoveResourceCmd<T>());
  return std::forward<decltype(self)>(self);
}

template <typename F>
  requires std::invocable<F, World&>
inline auto WorldCmdBuffer::DeferredUpdate(this auto&& self, F&& command)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(FunctionCmd(std::forward<F>(command)));
  return std::forward<decltype(self)>(self);
}

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
