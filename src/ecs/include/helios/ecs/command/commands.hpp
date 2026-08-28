#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <functional>
#include <memory_resource>
#include <ranges>
#endif
#include <helios/assert.hpp>
#include <helios/ecs/builtin_commands.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/command/entity_buffer.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/command/world_buffer.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/world.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class AccessPolicy;
class AccessPolicyBuilder;

/**
 * @brief Thin wrapper over command queue and world for deferred ECS operations.
 * @details Provides a convenient interface for spawning, despawning, and
 * manipulating entities and world state through command buffers.
 * @note Not thread-safe.
 */
class Commands {
public:
  /**
   * @brief Constructs a `Commands` object with a command queue and world
   * reference.
   * @param queue Reference to the command queue
   * @param world Reference to the world
   * @param resource Memory resource used for command storage (default: default
   * memory resource)
   */
  constexpr Commands(CmdQueue& queue, World& world,
                     std::pmr::memory_resource* resource =
                         std::pmr::get_default_resource()) noexcept
      : queue_(queue), world_(world), resource_(resource) {}

  Commands(CmdQueue&, World&, std::nullptr_t) = delete;

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
  EntityCmdBuffer Spawn();

  /**
   * @brief Enqueues a command to despawn an entity.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to despawn
   */
  void Despawn(Entity entity) { queue_.Enqueue(DestroyEntityCmd(entity)); }

  /**
   * @brief Returns a command buffer for an existing entity.
   * @warning Triggers assertion if entity is invalid or does not exist in the
   * world.
   * @param entity Entity to get command buffer for
   * @return Command buffer for the entity
   */
  [[nodiscard]] EntityCmdBuffer Entity(Entity entity);

  /**
   * @brief Returns a command buffer for world-level operations.
   * @return Command buffer for world operations
   */
  [[nodiscard]] WorldCmdBuffer World() {
    return WorldCmdBuffer(queue_, resource_);
  }

  /**
   * @brief Enqueues a command.
   * @tparam Cmd Command type, must satisfy `CommandTrait`
   * @param cmd Command to enqueue
   */
  template <CommandTrait Cmd>
  void Enqueue(Cmd&& cmd) {
    queue_.Enqueue(std::forward<Cmd>(cmd));
  }

  /**
   * @brief Enqueues multiple commands in bulk.
   * @tparam R Range type
   * @param range Range of commands to enqueue
   */
  template <std::ranges::input_range R>
    requires CommandTrait<std::ranges::range_value_t<R>>
  void EnqueueBulk(R&& range) {
    queue_.EnqueueBulk(std::forward<R>(range));
  }

private:
  CmdQueue& queue_;                            ///< Command queue reference
  std::reference_wrapper<class World> world_;  ///< World reference
  std::pmr::memory_resource* resource_;        ///< Memory resource for buffers
};

inline EntityCmdBuffer Commands::Spawn() {
  auto entity = world_.get().ReserveEntity();
  return {entity, queue_, resource_};
}

inline EntityCmdBuffer Commands::Entity(::helios::ecs::Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is not valid!", entity);
  HELIOS_ASSERT(world_.get().Exists(entity), "World does not own entity '{}'!",
                entity);
  return {entity, queue_, resource_};
}

template <>
struct SystemParamTraits<Commands> {
  static constexpr Commands Make(World& world, SystemLocalData& data,
                                 const AccessPolicy& /*policy*/) {
    return {data.cmd_queue, world, &data.allocator};
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
