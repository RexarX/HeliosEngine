#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/details/commands.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <concepts>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace helios::ecs {

/**
 * @brief Command buffer to record world operations for deferred execution.
 * @details Provides a convenient interface for recording operations that will be executed
 * during World::Update(). All operations are queued locally and flushed to SystemLocalStorage
 * on destruction or explicit Flush() call.
 *
 * Supports custom allocators for internal storage, enabling use of frame allocators
 * for zero-overhead temporary allocations within systems.
 *
 * @note Not thread-safe. Created per system.
 * @tparam Allocator Allocator type for internal command pointer storage
 *
 * @example
 * @code
 * void MySystem(app::SystemContext& ctx) {
 *   auto cmd = ctx.Commands();  // Uses frame allocator by default
 *   cmd.Destroy(entity);
 *   cmd.ClearEvents<MyEvent>();
 *   // Commands are flushed automatically when cmd goes out of scope
 * }
 * @endcode
 */
template <typename Allocator = std::allocator<std::unique_ptr<Command>>>
class WorldCmdBuffer {
public:
  using allocator_type = Allocator;

  /**
   * @brief Constructs a WorldCmdBuffer for recording commands.
   * @param local_storage Reference to system local storage for command execution
   * @param alloc Allocator instance for internal storage
   */
  explicit WorldCmdBuffer(details::SystemLocalStorage& local_storage, Allocator alloc = Allocator{}) noexcept
      : local_storage_(local_storage), commands_(alloc), alloc_(alloc) {}

  WorldCmdBuffer(const WorldCmdBuffer&) = delete;
  WorldCmdBuffer(WorldCmdBuffer&&) noexcept = default;

  /**
   * @brief Destructor that automatically flushes pending commands.
   */
  ~WorldCmdBuffer() noexcept { Flush(); }

  WorldCmdBuffer& operator=(const WorldCmdBuffer&) = delete;
  WorldCmdBuffer& operator=(WorldCmdBuffer&&) noexcept = default;

  /**
   * @brief Flushes all pending commands to the system local storage.
   * @details Moves all locally buffered commands to SystemLocalStorage for later execution.
   * Called automatically by destructor. Safe to call multiple times.
   */
  void Flush() noexcept;

  /**
   * @brief Pushes a custom command function to be executed on the world.
   * @tparam F Callable type that accepts World&
   * @param func Function to execute on the world
   */
  template <typename F>
    requires std::invocable<F, World&>
  void Push(F&& func) {
    commands_.push_back(std::make_unique<details::FunctionCmd<F>>(std::forward<F>(func)));
  }

  /**
   * @brief Enqueues destruction of a single entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not owned by world during command execution.
   * @param entity Entity to destroy
   */
  void Destroy(Entity entity);

  /**
   * @brief Enqueues destruction of entities.
   * @warning Triggers assertion in next cases:
   * - Any entity is invalid.
   * - Any entity is not owned by world during command execution.
   * @tparam R Range type containing Entity elements
   * @param entities Entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void Destroy(const R& entities);

  /**
   * @brief Enqueues try-destruction of a single entity.
   * @details Skips non-existing entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not owned by world during command execution.
   * @param entity Entity to try destroy
   */
  void TryDestroy(Entity entity);

  /**
   * @brief Enqueues try-destruction of entities.
   * @details Skips non-existing entities.
   * @warning Triggers assertion in next cases:
   * - Any entity is invalid.
   * - Any entity is not owned by world during command execution.
   * @tparam R Range type containing Entity elements
   * @param entities Entities to try destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void TryDestroy(const R& entities);

  /**
   * @brief Queues a command to clear all events of a specific type.
   * @tparam T Event type to clear
   */
  template <EventTrait T>
  void ClearEvents() {
    commands_.push_back(std::make_unique<details::ClearEventsCmd<T>>());
  }

  /**
   * @brief Queues a command to clear all event queues without removing registration.
   * @details Events can still be written/read after this command executes.
   */
  void ClearEvents() { commands_.push_back(std::make_unique<details::ClearAllEventsCmd>()); }

  /**
   * @brief Checks if there are no pending commands.
   * @return True if no commands are buffered, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return commands_.empty(); }

  /**
   * @brief Gets the number of pending commands.
   * @return Number of commands in the buffer
   */
  [[nodiscard]] size_t Size() const noexcept { return commands_.size(); }

  /**
   * @brief Gets the allocator used by this buffer.
   * @return Copy of the allocator
   */
  [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

private:
  details::SystemLocalStorage& local_storage_;                 ///< Reference to system local storage
  std::vector<std::unique_ptr<Command>, Allocator> commands_;  ///< Local command buffer
  [[no_unique_address]] Allocator alloc_;                      ///< Allocator instance
};

template <typename Allocator>
inline void WorldCmdBuffer<Allocator>::Flush() noexcept {
  for (auto& cmd : commands_) {
    if (cmd != nullptr) [[likely]] {
      local_storage_.AddCommand(std::move(cmd));
    }
  }
  commands_.clear();
}

template <typename Allocator>
inline void WorldCmdBuffer<Allocator>::Destroy(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to destroy entity: Entity is not valid!");
  commands_.push_back(std::make_unique<details::DestroyEntityCmd>(entity));
}

template <typename Allocator>
template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void WorldCmdBuffer<Allocator>::Destroy(const R& entities) {
  HELIOS_ASSERT(std::ranges::all_of(entities, [](const auto& entity) { return entity.Valid(); }),
                "Failed to destroy entities: All entities must be valid!");
  commands_.push_back(std::make_unique<details::DestroyEntitiesCmd>(entities));
}

template <typename Allocator>
inline void WorldCmdBuffer<Allocator>::TryDestroy(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try destroy entity: Entity is not valid!");
  commands_.push_back(std::make_unique<details::TryDestroyEntityCmd>(entity));
}

template <typename Allocator>
template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void WorldCmdBuffer<Allocator>::TryDestroy(const R& entities) {
  HELIOS_ASSERT(std::ranges::all_of(entities, [](const auto& entity) { return entity.Valid(); }),
                "Failed to destroy entities: All entities must be valid!");
  commands_.push_back(std::make_unique<details::TryDestroyEntitiesCmd>(entities));
}

}  // namespace helios::ecs
