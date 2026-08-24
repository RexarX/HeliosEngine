#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/builtin_commands.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/component/bundle.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/utils/common_traits.hpp>

#include <cstddef>
#include <memory_resource>
#include <utility>

namespace helios::ecs {

/**
 * @brief Command buffer for deferred entity operations.
 * @details Collects commands and then pushes them into a queue.
 * Commands are enqueued in the order they were added, ensuring predictable
 * behavior.
 * @note Not thread-safe.
 */
class EntityCmdBuffer {
public:
  using size_type = CmdQueue::size_type;

  /**
   * @brief Constructs an entity command buffer.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity associated with this buffer
   * @param queue Queue to push commands into
   * @param resource Memory resource used for command storage (default: default
   * memory resource)
   */
  constexpr EntityCmdBuffer(
      Entity entity, CmdQueue& queue,
      std::pmr::memory_resource* resource = std::pmr::get_default_resource());

  EntityCmdBuffer(Entity entity, CmdQueue& queue, std::nullptr_t) = delete;

  EntityCmdBuffer(const EntityCmdBuffer&) = delete;
  EntityCmdBuffer(EntityCmdBuffer&&) = delete;
  constexpr ~EntityCmdBuffer() { queue_.Merge(std::move(commands_)); }

  EntityCmdBuffer& operator=(const EntityCmdBuffer&) = delete;
  EntityCmdBuffer& operator=(EntityCmdBuffer&&) = delete;

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
   * @brief Enqueues a command to destroy entity and removes it from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   */
  auto Destroy(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try destroy entity if it exists in the world.
   * @note Not thread-safe.
   * @warning Triggers assertion if entity is invalid.
   */
  auto TryDestroy(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to add components to the entity.
   * @details If entity already has component of provided type then it will be
   * replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to add
   * @param components Components to add
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto AddComponents(this auto&& self, Ts&&... components)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to add a component bundle to the entity.
   * @details If the entity already has a represented component, it will be
   * replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam B Component bundle type
   * @param bundle Component bundle to add
   */
  template <ComponentBundleTrait B>
  auto AddBundle(this auto&& self, B&& bundle)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try to add components to the entity if they
   * don't exist.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to add
   * @param components Components to add
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryAddComponents(this auto&& self, Ts&&... components)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try to add missing components from a bundle.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam B Component bundle type
   * @param bundle Component bundle to add
   */
  template <ComponentBundleTrait B>
  auto TryAddBundle(this auto&& self, B&& bundle)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to remove components from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * - Entity does not have any of the components.
   * @tparam Ts Components types to remove
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto RemoveComponents(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to remove a component bundle from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * - Entity does not have every component represented by the bundle.
   * @tparam B Component bundle type
   */
  template <ComponentBundleTrait B>
  auto RemoveBundle(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try to remove components from the entity if
   * they exist.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to remove
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryRemoveComponents(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to try to remove present components represented
   * by a bundle.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam B Component bundle type
   */
  template <ComponentBundleTrait B>
  auto TryRemoveBundle(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Enqueues a command to remove all components from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   */
  auto ClearComponents(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Checks if the buffer is empty.
   * @return True if buffer is empty, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return commands_.Empty();
  }

  /**
   * @brief Gets the entity associated with this buffer.
   * @return Entity instance
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

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
  Entity entity_;
  CmdQueue commands_;
  CmdQueue& queue_;
};

constexpr EntityCmdBuffer::EntityCmdBuffer(Entity entity, CmdQueue& queue,
                                           std::pmr::memory_resource* resource)
    : entity_(entity), commands_(resource), queue_(queue) {
  HELIOS_ASSERT(entity_.Valid(), "Entity '{}' is not valid!", entity_);
}

inline auto EntityCmdBuffer::Destroy(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(DestroyEntityCmd(self.entity_));
  return std::forward<decltype(self)>(self);
}

inline auto EntityCmdBuffer::TryDestroy(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryDestroyEntityCmd(self.entity_));
  return std::forward<decltype(self)>(self);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto EntityCmdBuffer::AddComponents(this auto&& self, Ts&&... components)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(
      AddComponentsCmd(self.entity_, std::forward<Ts>(components)...));
  return std::forward<decltype(self)>(self);
}

template <ComponentBundleTrait B>
inline auto EntityCmdBuffer::AddBundle(this auto&& self, B&& bundle)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(AddBundleCmd(self.entity_, std::forward<B>(bundle)));
  return std::forward<decltype(self)>(self);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto EntityCmdBuffer::TryAddComponents(this auto&& self,
                                              Ts&&... components)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(
      TryAddComponentsCmd(self.entity_, std::forward<Ts>(components)...));
  return std::forward<decltype(self)>(self);
}

template <ComponentBundleTrait B>
inline auto EntityCmdBuffer::TryAddBundle(this auto&& self, B&& bundle)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(
      TryAddBundleCmd(self.entity_, std::forward<B>(bundle)));
  return std::forward<decltype(self)>(self);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto EntityCmdBuffer::RemoveComponents(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(RemoveComponentsCmd<Ts...>(self.entity_));
  return std::forward<decltype(self)>(self);
}

template <ComponentBundleTrait B>
inline auto EntityCmdBuffer::RemoveBundle(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(RemoveBundleCmd<B>(self.entity_));
  return std::forward<decltype(self)>(self);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto EntityCmdBuffer::TryRemoveComponents(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryRemoveComponentsCmd<Ts...>(self.entity_));
  return std::forward<decltype(self)>(self);
}

template <ComponentBundleTrait B>
inline auto EntityCmdBuffer::TryRemoveBundle(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(TryRemoveBundleCmd<B>(self.entity_));
  return std::forward<decltype(self)>(self);
}

inline auto EntityCmdBuffer::ClearComponents(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.commands_.Enqueue(ClearComponentsCmd(self.entity_));
  return std::forward<decltype(self)>(self);
}

}  // namespace helios::ecs
