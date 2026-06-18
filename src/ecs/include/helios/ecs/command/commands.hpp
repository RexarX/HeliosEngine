#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/builtin_commands.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/command/entity_buffer.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/command/world_buffer.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/world.hpp>

#include <functional>
#include <memory_resource>
#include <ranges>

namespace helios::ecs {

/**
 * @brief Thin wrapper over command queue and world for deferred ECS operations.
 * @details Provides a convenient interface for spawning, despawning, and
 * manipulating entities and world state through command buffers.
 * @note Not thread-safe.
 */
class Commands {
public:
  constexpr Commands(PmrCmdQueue& queue, World& world,
                     std::pmr::memory_resource* resource =
                         std::pmr::get_default_resource()) noexcept
      : queue_(queue), world_(world), resource_(resource) {}

  Commands(const Commands&) = delete;
  Commands(Commands&&) = delete;
  constexpr ~Commands() = default;

  Commands& operator=(const Commands&) = delete;
  Commands& operator=(Commands&&) = delete;

  /**
   * @brief Spawns a new entity and returns a command buffer for it.
   * @details Reserves a new entity ID and returns a command buffer to schedule
   * operations on it.
   * @return Command buffer for the newly reserved entity
   */
  PmrEntityCmdBuffer Spawn();

  /**
   * @brief Enqueues a command to despawn an entity.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to despawn
   */
  void Despawn(Entity entity) {
    queue_.get().Enqueue(DestroyEntityCmd(entity));
  }

  /**
   * @brief Returns a command buffer for an existing entity.
   * @warning Triggers assertion if entity is invalid or does not exist in the
   * world.
   * @param entity Entity to get command buffer for
   * @return Command buffer for the entity
   */
  [[nodiscard]] constexpr PmrEntityCmdBuffer Entity(Entity entity);

  /**
   * @brief Returns a command buffer for world-level operations.
   * @return Command buffer for world operations
   */
  [[nodiscard]] constexpr PmrWorldCmdBuffer World() {
    return {queue_.get(), resource_};
  }

  /**
   * @brief Enqueues a command.
   * @tparam Cmd Command type, must satisfy `CommandTrait`
   * @param cmd Command to enqueue
   */
  template <CommandTrait Cmd>
  void Enqueue(Cmd&& cmd) {
    queue_.get().Enqueue(std::forward<Cmd>(cmd));
  }

  /**
   * @brief Enqueues multiple commands in bulk.
   * @tparam R Range type
   * @param range Range of commands to enqueue
   */
  template <std::ranges::input_range R>
    requires CommandTrait<std::ranges::range_value_t<R>>
  void EnqueueBulk(R&& range) {
    queue_.get().EnqueueBulk(std::forward<R>(range));
  }

private:
  std::reference_wrapper<PmrCmdQueue> queue_;  ///< Command queue reference
  std::reference_wrapper<class World> world_;  ///< World reference
  std::pmr::memory_resource* resource_;        ///< Memory resource for buffers
};

inline PmrEntityCmdBuffer Commands::Spawn() {
  auto entity = world_.get().ReserveEntity();
  return {entity, queue_.get(), resource_};
}

constexpr PmrEntityCmdBuffer Commands::Entity(::helios::ecs::Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is not valid!", entity);
  HELIOS_ASSERT(world_.get().Exists(entity), "World does not own entity '{}'!",
                entity);
  return {entity, queue_.get(), resource_};
}

}  // namespace helios::ecs
