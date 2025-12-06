#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/commands.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::ecs {

// Forward declaration
class World;

/**
 * @brief Command buffer to record entity operations for deferred execution.
 * @details Provides a convenient interface for recording operations on a specific entity
 * that will be executed during World::Update(). All operations are queued locally and
 * flushed to SystemLocalStorage on destruction or explicit Flush() call.
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
 *   auto cmd = ctx.EntityCommands(entity);  // Uses frame allocator by default
 *   cmd.AddComponent(Position{1.0F, 2.0F, 3.0F});
 *   cmd.RemoveComponent<Velocity>();
 *   // Commands are flushed automatically when cmd goes out of scope
 * }
 * @endcode
 */
template <typename Allocator = std::allocator<std::unique_ptr<Command>>>
class EntityCmdBuffer {
public:
  using allocator_type = Allocator;

  /**
   * @brief Creates command buffer for a new reserved entity.
   * @details Reserves an entity ID and creates a command buffer for operations on it.
   * The entity will be created when commands are flushed during World::Update().
   * @param world World to reserve entity in
   * @param local_storage Reference to system local storage
   * @param alloc Allocator instance for internal storage
   */
  EntityCmdBuffer(World& world, details::SystemLocalStorage& local_storage, Allocator alloc = Allocator{});

  /**
   * @brief Creates command buffer for entity operations on existing entity.
   * @details Creates command buffer for operations on the specified entity.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to operate on
   * @param local_storage Reference to system local storage
   * @param alloc Allocator instance for internal storage
   */
  EntityCmdBuffer(Entity entity, details::SystemLocalStorage& local_storage, Allocator alloc = Allocator{});
  EntityCmdBuffer(const EntityCmdBuffer&) = delete;
  EntityCmdBuffer(EntityCmdBuffer&&) noexcept = default;

  /**
   * @brief Destructor that automatically flushes pending commands.
   */
  ~EntityCmdBuffer() noexcept { Flush(); }

  EntityCmdBuffer& operator=(const EntityCmdBuffer&) = delete;
  EntityCmdBuffer& operator=(EntityCmdBuffer&&) noexcept = default;

  /**
   * @brief Flushes all pending commands to the system local storage.
   * @details Moves all locally buffered commands to SystemLocalStorage for later execution.
   * Called automatically by destructor. Safe to call multiple times.
   */
  void Flush() noexcept;

  /**
   * @brief Destroys entity and removes it from the world.
   * @details Queues command to destroy the entity and remove all its components.
   * @warning Triggers assertion during command execution if entity does not exist.
   * After execution of this command, the entity will become invalid.
   */
  void Destroy() { commands_.push_back(std::make_unique<details::DestroyEntityCmd>(entity_)); }

  /**
   * @brief Tries to destroy entity if it exists.
   * @details Queues command to destroy the entity only if it currently exists in the world.
   * If entity does not exist during command execution, the command is ignored.
   * After execution of this command, the entity will become invalid if it existed.
   */
  void TryDestroy() { commands_.push_back(std::make_unique<details::TryDestroyEntityCmd>(entity_)); }

  /**
   * @brief Adds component to the entity.
   * @details Queues command to add component to entity.
   * If entity already has this component type, it will be replaced during command execution.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam T Component type to add
   * @param component Component to add
   */
  template <ComponentTrait T>
  void AddComponent(T&& component) {
    commands_.push_back(
        std::make_unique<details::AddComponentCmd<std::remove_cvref_t<T>>>(entity_, std::forward<T>(component)));
  }

  /**
   * @brief Adds components to the entity.
   * @details Queues command to add all specified components to entity in a single operation.
   * If entity already has this component type, it will be replaced during command execution.
   * More efficient than multiple AddComponent calls.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam Ts Component types to add (must be unique)
   * @param components Components to add
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  void AddComponents(Ts&&... components) {
    commands_.push_back(std::make_unique<details::AddComponentsCmd<std::remove_cvref_t<Ts>...>>(
        entity_, std::forward<Ts>(components)...));
  }

  /**
   * @brief Tries to add a component to the entity.
   * @details Queues command that will add the component only if the entity does not
   * already have one during command execution.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam T Component type to add
   * @param component Component instance to add
   */
  template <ComponentTrait T>
  void TryAddComponent(T&& component) {
    commands_.push_back(
        std::make_unique<details::TryAddComponentCmd<std::remove_cvref_t<T>>>(entity_, std::forward<T>(component)));
  }

  /**
   * @brief Tries to add multiple components to the entity.
   * @details Queues command that attempts to add all provided components.
   * Any component types already present are ignored during command execution.
   * No per-component success flags are returned—inspection after World::Update() should be done via HasComponent().
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam Ts Component types to add (must be unique)
   * @param components Component instances to add
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  void TryAddComponents(Ts&&... components) {
    commands_.push_back(std::make_unique<details::TryAddComponentsCmd<std::remove_cvref_t<Ts>...>>(
        entity_, std::forward<Ts>(components)...));
  }

  /**
   * @brief Emplaces and adds component to the entity.
   * @details Queues command that will construct and add the component to the entity.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam T Component type to construct and add
   * @tparam Args Argument types for T's constructor
   * @param args Arguments perfectly forwarded to the component constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceComponent(Args&&... args) {
    commands_.push_back(std::make_unique<details::AddComponentCmd<T>>(entity_, T{std::forward<Args>(args)...}));
  }

  /**
   * @brief Tries to emplace and add component to the entity.
   * @details Queues command that will construct and add the component to the entity.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam T Component type to construct and add
   * @tparam Args Argument types for T's constructor
   * @param args Arguments to forward to component constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void TryEmplaceComponent(Args&&... args) {
    commands_.push_back(std::make_unique<details::TryAddComponentCmd<T>>(entity_, T{std::forward<Args>(args)...}));
  }

  /**
   * @brief Removes component from the entity.
   * @details Queues command to remove the specified component type from entity.
   * @warning Triggers assertion during command execution in next cases:
   * - Entity does not exist.
   * - Entity does not have the specified component.
   * @tparam T Component type to remove
   */
  template <ComponentTrait T>
  void RemoveComponent() {
    commands_.push_back(std::make_unique<details::RemoveComponentCmd<T>>(entity_));
  }

  /**
   * @brief Removes components from the entity.
   * @details Queues command to remove all specified component types from entity.
   * More efficient than multiple RemoveComponent calls.
   * @warning Triggers assertion during command execution in next cases:
   * - Entity does not exist.
   * - Entity lacks any of the specified components.
   * @tparam Ts Component types to remove (must be unique)
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  void RemoveComponents() {
    commands_.push_back(std::make_unique<details::RemoveComponentsCmd<Ts...>>(entity_));
  }

  /**
   * @brief Tries to remove a component from the entity.
   * @details Queues command that attempts to remove the specified component type.
   * If the entity lacks the component during command execution the command does nothing.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam T Component type to remove
   */
  template <ComponentTrait T>
  void TryRemoveComponent() {
    commands_.push_back(std::make_unique<details::TryRemoveComponentCmd<T>>(entity_));
  }

  /**
   * @brief Tries to remove multiple components from the entity (idempotent).
   * @details Queues a deferred command that attempts to remove all specified component types.
   * Any components not present are ignored silently.
   * @warning Triggers assertion during command execution if entity does not exist.
   * @tparam Ts Component types to remove (must be unique)
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  void TryRemoveComponents() {
    commands_.push_back(std::make_unique<details::TryRemoveComponentsCmd<Ts...>>(entity_));
  }

  /**
   * @brief Removes all components from the entity.
   * @details Queues command to remove all components from entity, leaving it with no components.
   * The entity itself will remain valid unless explicitly destroyed.
   * @warning Triggers assertion during command execution if entity does not exist.
   */
  void ClearComponents() { commands_.push_back(std::make_unique<details::ClearComponentsCmd>(entity_)); }

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
   * @brief Gets the entity this command buffer operates on.
   * @return Entity that this command buffer targets
   */
  [[nodiscard]] Entity GetEntity() const noexcept { return entity_; }

  /**
   * @brief Gets the allocator used by this buffer.
   * @return Copy of the allocator
   */
  [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

private:
  Entity entity_;                                              ///< Entity this command buffer operates on
  details::SystemLocalStorage& local_storage_;                 ///< Reference to system local storage
  std::vector<std::unique_ptr<Command>, Allocator> commands_;  ///< Local command buffer
  [[no_unique_address]] Allocator alloc_;                      ///< Allocator instance
};

template <typename Allocator>
inline EntityCmdBuffer<Allocator>::EntityCmdBuffer(World& world, details::SystemLocalStorage& local_storage,
                                                   Allocator alloc)
    : entity_(world.ReserveEntity()), local_storage_(local_storage), commands_(alloc), alloc_(alloc) {}

template <typename Allocator>
inline EntityCmdBuffer<Allocator>::EntityCmdBuffer(Entity entity, details::SystemLocalStorage& local_storage,
                                                   Allocator alloc)
    : entity_(entity), local_storage_(local_storage), commands_(alloc), alloc_(alloc) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct entity command buffer: Entity is invalid!");
  // Note: We don't check world.Exists(entity) here because this constructor is also used
  // for reserved entities (via ReserveEntity()), which don't exist in the world yet.
  // The entity will be created when commands are flushed during World::Update().
}

template <typename Allocator>
inline void EntityCmdBuffer<Allocator>::Flush() noexcept {
  for (auto& cmd : commands_) {
    if (cmd != nullptr) {
      local_storage_.AddCommand(std::move(cmd));
    }
  }
  commands_.clear();
}

}  // namespace helios::ecs
