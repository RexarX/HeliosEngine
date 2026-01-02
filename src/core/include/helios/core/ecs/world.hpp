#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/archetype.hpp>
#include <helios/core/ecs/details/command_queue.hpp>
#include <helios/core/ecs/details/components_manager.hpp>
#include <helios/core/ecs/details/entities_manager.hpp>
#include <helios/core/ecs/details/event_manager.hpp>
#include <helios/core/ecs/details/event_queue.hpp>
#include <helios/core/ecs/details/resources_manager.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/event_reader.hpp>
#include <helios/core/ecs/event_writer.hpp>
#include <helios/core/ecs/events/builtin_events.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::ecs {

// Forward declaration of World class for concept definition
class World;

/**
 * @brief Concept that constrains a type to be the World type (with any cv-qualifiers).
 * @details Used to ensure template parameters are World types for Query and QueryBuilder.
 * @tparam T Type to check
 */
template <typename T>
concept WorldType = std::same_as<std::remove_cvref_t<T>, World>;

/**
 * @brief The World class manages entities with their components and systems.
 * @note Partially thread safe.
 * All modifications to the world (adding/removing entities or components) should be done
 * via command buffers to defer changes until the next update.
 */
class World {
public:
  World() = default;
  World(const World&) = delete;
  World(World&&) noexcept = default;
  ~World() = default;

  World& operator=(const World&) = delete;
  World& operator=(World&&) noexcept = default;

  /**
   * @brief Flushes buffer of pending operations.
   * @note This method should be called once per frame after all systems have been updated.
   * Not thread-safe.
   */
  void Update();

  /**
   * @brief Clears the world, removing all data.
   * @note Not thread-safe.
   */
  void Clear();

  /**
   * @brief Clears all entities and components from the world.
   * @note Not thread-safe.
   */
  void ClearEntities() noexcept;

  /**
   * @brief Clears all event queues without removing event registration.
   * @details Events can still be written/read after calling this method.
   * To completely reset the event system including registration, use Clear().
   * @note Not thread-safe.
   */
  void ClearAllEventQueues() noexcept { event_manager_.ClearAllQueues(); }

  /**
   * @brief Merges events from another event queue into the main queue.
   * @note Not thread-safe.
   * @param other Event queue to merge from
   */
  void MergeEventQueue(details::EventQueue& other) { event_manager_.Merge(other); }

  /**
   * @brief Merges commands from a command range into the main queue.
   * @note Not thread-safe.
   * @tparam R Range type containing unique_ptr<Command> elements
   * @param commands Range of commands to merge from
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, std::unique_ptr<Command>>
  void MergeCommands(R&& commands);

  /**
   * @brief Creates new entity.
   * @details If EntitySpawnedEvent is registered, emits the event automatically.
   * @note Not thread-safe.
   * @return Newly created entity.
   */
  [[nodiscard]] Entity CreateEntity();

  /**
   * @brief Reserves an entity ID for deferred creation.
   * @details The actual entity creation is deferred until `Update()` is called.
   * @note Thread-safe.
   * @return Reserved entity ID
   */
  [[nodiscard]] Entity ReserveEntity() { return entities_.ReserveEntity(); }

  /**
   * @brief Destroys entity and removes it from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @param entity Entity to destroy
   */
  void DestroyEntity(Entity entity);

  /**
   * @brief Tries to destroy entity if it exists in the world.
   * @note Not thread-safe.
   * @warning Triggers assertion only if entity is invalid (does not assert when entity does not exist in the world).
   * @param entity Entity to destroy
   */
  void TryDestroyEntity(Entity entity);

  /**
   * @brief Destroys entities and removes them from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Any entity is invalid.
   * - Any entity does not exist in the world.
   * @tparam R Range type containing Entity elements
   * @param entities Entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void DestroyEntities(const R& entities);

  /**
   * @brief Tries to destroy entities if they exist in the world.
   * @note Not thread-safe.
   * @warning Triggers assertion only if any entity is invalid (non-existing entities are skipped).
   * @tparam R Range type containing Entity elements
   * @param entities Entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void TryDestroyEntities(const R& entities);

  /**
   * @brief Adds component to the entity.
   * @details If entity already has component of provided type then it will be replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to add
   * @param entity Entity to add component to
   * @param component Component to add
   */
  template <ComponentTrait T>
  void AddComponent(Entity entity, T&& component);

  /**
   * @brief Adds components to the entity.
   * @details If entity already has component of provided type then it will be replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to add
   * @param entity Entity to add components to
   * @param components Components to add
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  void AddComponents(Entity entity, Ts&&... components);

  /**
   * @brief Tries to remove component from the entity.
   * @details Returns false if component is not on the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to remove
   * @param entity Entity to remove component from
   * @return True if component was removed, false otherwise
   */
  template <ComponentTrait T>
  bool TryAddComponent(Entity entity, T&& component);

  /**
   * @brief Tries to add components to the entity if they don't exist.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to add
   * @param entity Entity to add components to
   * @param components Components to add
   * @return Array of bools indicating whether each component was added (true if added, false otherwise)
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  auto TryAddComponents(Entity entity, Ts&&... components) -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Emplaces component for the entity.
   * @details Constructs component in-place. If entity already has component of provided type then it will be replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to emplace
   * @tparam Args Argument types to forward to T's constructor
   * @param entity Entity to emplace component to
   * @param args Arguments to forward to component constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceComponent(Entity entity, Args&&... args);

  /**
   * @brief Tries to emplace component for the entity.
   * @details Constructs component in-place. Returns false if component is already on the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to emplace
   * @tparam Args Argument types to forward to T's constructor
   * @param entity Entity to emplace component to
   * @param args Arguments to forward to component constructor
   * @return True if component was emplaced, false otherwise
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplaceComponent(Entity entity, Args&&... args);

  /**
   * @brief Removes component from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to remove
   * @param entity Entity to remove component from
   */
  template <ComponentTrait T>
  void RemoveComponent(Entity entity);

  /**
   * @brief Removes components from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to remove
   * @param entity Entity to remove components from
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  void RemoveComponents(Entity entity);

  /**
   * @brief Tries to removes component from the entity if it exists.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to remove
   * @param entity Entity to remove component from
   * @return True if component will be removed, false otherwise
   */
  template <ComponentTrait T>
  bool TryRemoveComponent(Entity entity);

  /**
   * @brief Gets component from the entity (const version).
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * - Entity does not have component.
   * @tparam T Component type to get
   * @param entity Entity to get component from
   * @return Const reference to component
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  auto TryRemoveComponents(Entity entity) -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Removes all components from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @param entity Entity to remove all components from
   */
  void ClearComponents(Entity entity);

  /**
   * @brief Inserts a resource into the world.
   * @details Replaces existing resource if present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ResourceTrait T>
  void InsertResource(T&& resource) {
    resources_.Insert(std::forward<T>(resource));
  }

  /**
   * @brief Tries to insert a resource if not present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @param resource Resource to insert
   * @return True if inserted, false if resource already exists
   */
  template <ResourceTrait T>
  bool TryInsertResource(T&& resource) {
    return resources_.TryInsert(std::forward<T>(resource));
  }

  /**
   * @brief Emplaces a resource in-place.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to resource constructor
   */
  template <ResourceTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceResource(Args&&... args) {
    resources_.Emplace<T>(std::forward<Args>(args)...);
  }

  /**
   * @brief Tries to emplace a resource if not present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to resource constructor
   * @return True if emplaced, false if resource already exists
   */
  template <ResourceTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplaceResource(Args&&... args) {
    return resources_.TryEmplace<T>(std::forward<Args>(args)...);
  }

  /**
   * @brief Removes a resource from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion if resource does not exist.
   * @tparam T Resource type
   */
  template <ResourceTrait T>
  void RemoveResource();

  /**
   * @brief Tries to remove a resource.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @return True if removed, false if resource didn't exist
   */
  template <ResourceTrait T>
  bool TryRemoveResource() {
    return resources_.TryRemove<T>();
  }

  /**
   * @brief Gets reference to a resource.
   * @note Not thread-safe.
   * @warning Triggers assertion if resource does not exist.
   * @tparam T Resource type
   * @return Reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] T& WriteResource();

  /**
   * @brief Gets const reference to a resource.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   * @return Const reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] const T& ReadResource() const;

  /**
   * @brief Tries to get mutable pointer to a resource.
   * @note Thread-safe for read operations.
   * @tparam T Resource type
   * @return Pointer to resource, or nullptr if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] T* TryWriteResource() {
    return resources_.TryGet<T>();
  }

  /**
   * @brief Tries to get const pointer to a resource.
   * @note Thread-safe for read operations.
   * @tparam T Resource type
   * @return Const pointer to resource, or nullptr if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] const T* TryReadResource() const {
    return resources_.TryGet<T>();
  }

  /**
   * @brief Registers an event type for use.
   * @details Events must be registered before they can be written or read.
   * Built-in events (EntitySpawnedEvent, EntityDestroyedEvent) are registered with is_builtin flag.
   * @note Not thread-safe.
   * Should be called during initialization.
   * @tparam T Event type
   */
  template <EventTrait T>
  void AddEvent();

  /**
   * @brief Registers multiple event types for use.
   * @note Not thread-safe.
   * Should be called during initialization.
   * @tparam Events Event types to register
   */
  template <EventTrait... Events>
    requires(sizeof...(Events) > 1)
  void AddEvents() {
    (AddEvent<Events>(), ...);
  }

  /**
   * @brief Manually clears events of a specific type from both queues.
   * @details Should only be used for manually-managed events (auto_clear = false).
   * Built-in events cannot be manually cleared.
   * @note Not thread-safe.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   */
  template <EventTrait T>
  void ClearEvents();

  /**
   * @brief Gets an event reader for type T.
   * @details Provides a type-safe, ergonomic API for reading events with
   * support for iteration, filtering, and searching.
   * Event must be registered via AddEvent<T>() first.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   * @return EventReader for type T
   */
  template <EventTrait T>
  [[nodiscard]] auto ReadEvents() const noexcept -> EventReader<T>;

  /**
   * @brief Gets an event writer for type T.
   * @details Provides a type-safe, ergonomic API for writing events with
   * support for bulk operations and in-place construction.
   * Event must be registered via AddEvent<T>() first.
   * @note Not thread-safe.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   * @return EventWriter for type T
   */
  template <EventTrait T>
  [[nodiscard]] auto WriteEvents() -> EventWriter<T>;

  /**
   * @brief Checks if entity exists in the world.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to check
   * @return True if entity exists, false otherwise
   */
  [[nodiscard]] bool Exists(Entity entity) const;

  /**
   * @brief Checks if entity has component.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to check
   * @param entity Entity to check
   * @return Array of bools indicating whether entity has each component
   */
  template <ComponentTrait T>
  [[nodiscard]] bool HasComponent(Entity entity) const;

  /**
   * @brief Checks if entity has components.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Ts Components types
   * @param entity Entity to check
   * @return Array of bools indicating whether entity has each component (true if entity has component,
   * false otherwise)
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  [[nodiscard]] auto HasComponents(Entity entity) const -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Checks if a resource exists.
   * @note Thread-safe for read operations.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ResourceTrait T>
  [[nodiscard]] bool HasResource() const {
    return resources_.Has<T>();
  }

  /**
   * @brief Checks if a event registered.
   * @note Thread-safe for read operations.
   * @tparam T Event type
   * @return True if event exists, false otherwise
   */
  template <ResourceTrait T>
  [[nodiscard]] bool HasEvent() const {
    return event_manager_.IsRegistered<T>();
  }

  /**
   * @brief Checks if events of a specific type exist in event queue.
   * @note Thread-safe for read operations.
   * @tparam T Event type
   * @return True if event exists, false otherwise
   */
  template <ResourceTrait T>
  [[nodiscard]] bool HasEvents() const {
    return event_manager_.HasEvents<T>();
  }

  /**
   * @brief Gets the number of entities in the world.
   * @note Thread-safe for read operations.
   * @return Number of entities in the world
   */
  [[nodiscard]] size_t EntityCount() const noexcept { return entities_.Count(); }

private:
  /**
   * @brief Updates entity's archetype based on current components (full rebuild).
   * @details Used when multiple components are added/removed at once.
   * @param entity Entity to update
   */
  void UpdateEntityArchetype(Entity entity);

  /**
   * @brief Updates entity's archetype after adding a single component.
   * @details Uses edge graph for O(1) amortized lookup.
   * @tparam T Component type that was added
   * @param entity Entity to update
   */
  template <ComponentTrait T>
  void UpdateEntityArchetypeOnAdd(Entity entity);

  /**
   * @brief Updates entity's archetype after removing a single component.
   * @details Uses edge graph for O(1) amortized lookup.
   * @tparam T Component type that was removed
   * @param entity Entity to update
   */
  template <ComponentTrait T>
  void UpdateEntityArchetypeOnRemove(Entity entity);

  [[nodiscard]] details::Entities& GetEntities() noexcept { return entities_; }
  [[nodiscard]] const details::Entities& GetEntities() const noexcept { return entities_; }

  [[nodiscard]] details::Components& GetComponents() noexcept { return components_; }
  [[nodiscard]] const details::Components& GetComponents() const noexcept { return components_; }

  [[nodiscard]] details::Archetypes& GetArchetypes() noexcept { return archetypes_; }
  [[nodiscard]] const details::Archetypes& GetArchetypes() const noexcept { return archetypes_; }

  [[nodiscard]] details::Resources& GetResources() noexcept { return resources_; }
  [[nodiscard]] const details::Resources& GetResources() const noexcept { return resources_; }

  [[nodiscard]] details::CmdQueue& GetCmdQueue() noexcept { return command_queue_; }
  [[nodiscard]] const details::CmdQueue& GetCmdQueue() const noexcept { return command_queue_; }

  [[nodiscard]] details::EventManager& GetEventManager() noexcept { return event_manager_; }
  [[nodiscard]] const details::EventManager& GetEventManager() const noexcept { return event_manager_; }

  details::Entities entities_;
  details::Components components_;
  details::Archetypes archetypes_;
  details::Resources resources_;
  details::CmdQueue command_queue_;
  details::EventManager event_manager_;

  template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
    requires utils::UniqueTypes<Components...>
  friend class BasicQuery;

  template <WorldType WorldT, typename Allocator, ComponentTrait... Components>
    requires utils::UniqueTypes<Components...>
  friend class BasicQueryWithEntity;
};

inline void World::Update() {
  // First, flush reserved entities to ensure all reserved IDs are created.
  entities_.FlushReservedEntities();

  // Then, execute all commands in the command queue.
  if (!command_queue_.Empty()) [[likely]] {
    auto commands = command_queue_.DequeueAll();
    for (auto& command : commands) {
      if (command) [[likely]] {
        command->Execute(*this);
      }
    }
  }

  // Update event lifecycle - swap buffers and clear old events
  event_manager_.Update();
}

inline void World::Clear() {
  entities_.Clear();
  components_.Clear();
  archetypes_.Clear();
  resources_.Clear();
  command_queue_.Clear();
  event_manager_.Clear();
}

inline void World::ClearEntities() noexcept {
  entities_.Clear();
  components_.Clear();
  archetypes_.Clear();
}

inline Entity World::CreateEntity() {
  Entity entity = entities_.CreateEntity();

  // Emit EntitySpawnedEvent if registered
  if (event_manager_.IsRegistered<EntitySpawnedEvent>()) {
    event_manager_.Write(EntitySpawnedEvent{entity});
  }

  return entity;
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, std::unique_ptr<Command>>
inline void World::MergeCommands(R&& commands) {
  if (commands.empty()) [[unlikely]] {
    return;
  }

  // Enqueue all commands from the range to the command queue
  command_queue_.EnqueueBulk(std::forward<R>(commands));
  commands.clear();
}

inline void World::DestroyEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to destroy entity: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to destroy entity: World does not own entity with index '{}'!", entity.Index());

  // Emit EntityDestroyedEvent if registered
  if (event_manager_.IsRegistered<EntityDestroyedEvent>()) {
    event_manager_.Write(EntityDestroyedEvent{entity});
  }

  components_.RemoveAllComponents(entity);
  archetypes_.RemoveEntity(entity);
  entities_.Destroy(entity);
}

inline void World::TryDestroyEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try destroy entity: Entity is invalid!");
  if (!Exists(entity)) {
    return;
  }

  // Emit EntityDestroyedEvent if registered
  if (event_manager_.IsRegistered<EntityDestroyedEvent>()) {
    event_manager_.Write(EntityDestroyedEvent{entity});
  }

  components_.RemoveAllComponents(entity);
  archetypes_.RemoveEntity(entity);
  entities_.Destroy(entity);
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void World::DestroyEntities(const R& entities_range) {
  const bool emit_events = event_manager_.IsRegistered<EntityDestroyedEvent>();

  for (const Entity& entity : entities_range) {
    HELIOS_ASSERT(entity.Valid(), "Failed to destroy entities: Entity is invalid!");
    HELIOS_ASSERT(Exists(entity), "Failed to destroy entities: World does not own entity with index '{}'!",
                  entity.Index());

    // Emit EntityDestroyedEvent if registered
    if (emit_events) {
      event_manager_.Write(EntityDestroyedEvent{entity});
    }

    components_.RemoveAllComponents(entity);
    archetypes_.RemoveEntity(entity);
  }
  entities_.Destroy(entities_range);
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void World::TryDestroyEntities(const R& entities_range) {
  const bool emit_events = event_manager_.IsRegistered<EntityDestroyedEvent>();

  for (const Entity& entity : entities_range) {
    HELIOS_ASSERT(entity.Valid(), "Failed to try destroy entities: Entity is invalid!");
    if (!Exists(entity)) [[unlikely]] {
      continue;
    }

    // Emit EntityDestroyedEvent if registered
    if (emit_events) {
      event_manager_.Write(EntityDestroyedEvent{entity});
    }

    components_.RemoveAllComponents(entity);
    archetypes_.RemoveEntity(entity);
  }
  entities_.Destroy(entities_range);
}

template <ComponentTrait T>
inline void World::AddComponent(Entity entity, T&& component) {
  HELIOS_ASSERT(entity.Valid(), "Failed to add component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to add component: World does not own entity with index '{}'!", entity.Index());

  components_.AddComponent(entity, std::forward<T>(component));
  UpdateEntityArchetypeOnAdd<std::remove_cvref_t<T>>(entity);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline void World::AddComponents(Entity entity, Ts&&... components) {
  HELIOS_ASSERT(entity.Valid(), "Failed to add components: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to add components: World does not own entity with index '{}'!", entity.Index());

  (components_.AddComponent(entity, std::forward<Ts>(components)), ...);
  UpdateEntityArchetype(entity);
}

template <ComponentTrait T>
inline bool World::TryAddComponent(Entity entity, T&& component) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try add component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to try add component: World does not own entity with index '{}'!",
                entity.Index());

  using ComponentType = std::decay_t<T>;
  if (components_.HasComponent<ComponentType>(entity)) {
    return false;
  }

  components_.AddComponent(entity, std::forward<T>(component));
  UpdateEntityArchetypeOnAdd<ComponentType>(entity);
  return true;
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline auto World::TryAddComponents(Entity entity, Ts&&... components) -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Failed to try add components: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to try add components: World does not own entity with index '{}'!",
                entity.Index());

  std::array<bool, sizeof...(Ts)> result = {!components_.HasComponent<std::decay_t<Ts>>(entity)...};
  bool any_added = false;

  size_t idx = 0;
  auto add_if_missing = [this, &result, &any_added, &idx, entity](auto&& component) {
    using ComponentType = std::decay_t<decltype(component)>;
    if (!components_.HasComponent<ComponentType>(entity)) {
      components_.AddComponent(entity, std::forward<decltype(component)>(component));
      result[idx] = true;
      any_added = true;
    } else {
      result[idx] = false;
    }
    ++idx;
  };

  (add_if_missing(std::forward<Ts>(components)), ...);

  if (any_added) {
    UpdateEntityArchetype(entity);
  }

  return result;
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void World::EmplaceComponent(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Failed to emplace component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to emplace component: World does not own entity with index '{}'!",
                entity.Index());

  components_.EmplaceComponent<T>(entity, std::forward<Args>(args)...);
  UpdateEntityArchetypeOnAdd<T>(entity);
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool World::TryEmplaceComponent(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try emplace component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to try emplace component: World does not own entity with index '{}'!",
                entity.Index());

  if (components_.HasComponent<T>(entity)) {
    return false;
  }

  components_.EmplaceComponent<T>(entity, std::forward<Args>(args)...);
  UpdateEntityArchetypeOnAdd<T>(entity);
  return true;
}

template <ComponentTrait T>
inline void World::RemoveComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to remove component: World does not own entity with index '{}'!",
                entity.Index());

  components_.RemoveComponent<T>(entity);
  UpdateEntityArchetypeOnRemove<T>(entity);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline void World::RemoveComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove components: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to remove components: World does not own entity with index '{}'!",
                entity.Index());

  (components_.RemoveComponent<Ts>(entity), ...);
  UpdateEntityArchetype(entity);
}

template <ComponentTrait T>
inline bool World::TryRemoveComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try remove component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to try remove component: World does not own entity with index '{}'!",
                entity.Index());

  const bool had_component = components_.HasComponent<T>(entity);
  if (had_component) {
    components_.RemoveComponent<T>(entity);
    UpdateEntityArchetypeOnRemove<T>(entity);
  }
  return had_component;
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline auto World::TryRemoveComponents(Entity entity) -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Failed to try remove components: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to try remove components: World does not own entity with index '{}'!",
                entity.Index());

  std::array<bool, sizeof...(Ts)> result = {};
  bool any_removed = false;

  size_t index = 0;
  auto try_remove_single = [this, &index, &result, &any_removed, entity](auto type_identity_tag) {
    using ComponentType = typename decltype(type_identity_tag)::type;
    if (components_.HasComponent<ComponentType>(entity)) {
      components_.RemoveComponent<ComponentType>(entity);
      result[index] = true;
      any_removed = true;
    } else {
      result[index] = false;
    }
    ++index;
  };

  (try_remove_single(std::type_identity<Ts>{}), ...);

  if (any_removed) {
    UpdateEntityArchetype(entity);
  }

  return result;
}

inline void World::ClearComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to clear components: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to clear components: World does not own entity with index '{}'!",
                entity.Index());

  components_.RemoveAllComponents(entity);
  archetypes_.RemoveEntity(entity);
}

template <ResourceTrait T>
inline void World::RemoveResource() {
  HELIOS_ASSERT(HasResource<T>(), "Failed to remove resource '{}': Resource does not exist!", ResourceNameOf<T>());
  resources_.Remove<T>();
}

template <ResourceTrait T>
inline T& World::WriteResource() {
  HELIOS_ASSERT(HasResource<T>(), "Failed to write resource '{}': Resource does not exist!", ResourceNameOf<T>());
  return resources_.Get<T>();
}

template <ResourceTrait T>
inline const T& World::ReadResource() const {
  HELIOS_ASSERT(HasResource<T>(), "Failed to read resource '{}': Resource does not exist!", ResourceNameOf<T>());
  return resources_.Get<T>();
}

inline bool World::Exists(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity exists: Entity is invalid!");
  return entities_.IsValid(entity);
}

template <ComponentTrait T>
inline bool World::HasComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity has component: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to check if entity has component: World does not own entity with index '{}'!",
                entity.Index());
  return components_.HasComponent<T>(entity);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline auto World::HasComponents(Entity entity) const -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity has components: Entity is invalid!");
  HELIOS_ASSERT(Exists(entity), "Failed to check if entity has components: World does not own entity with index '{}'!",
                entity.Index());
  return {components_.HasComponent<Ts>(entity)...};
}

template <EventTrait T>
inline void World::AddEvent() {
  if (event_manager_.IsRegistered<T>()) [[unlikely]] {
    HELIOS_WARN("Event '{}' is already registered in the world!", EventNameOf<T>());
    return;
  }

  event_manager_.RegisterEvent<T>();
}

template <EventTrait T>
inline void World::ClearEvents() {
  HELIOS_ASSERT(event_manager_.IsRegistered<T>(), "Failed to clear events of type '{}': Event type is not registered!",
                EventNameOf<T>());

  event_manager_.ManualClear<T>();
}

template <EventTrait T>
inline auto World::ReadEvents() const noexcept -> EventReader<T> {
  HELIOS_ASSERT(event_manager_.IsRegistered<T>(),
                "Failed to get event reader for type '{}': Event type is not registered!", EventNameOf<T>());

  return EventReader<T>(event_manager_);
}

template <EventTrait T>
inline auto World::WriteEvents() -> EventWriter<T> {
  HELIOS_ASSERT(event_manager_.IsRegistered<T>(),
                "Failed to get event writer for type '{}': Event type is not registered!", EventNameOf<T>());

  return EventWriter<T>(event_manager_);
}

template <ComponentTrait T>
inline void World::UpdateEntityArchetypeOnAdd(Entity entity) {
  const auto component_infos = components_.GetComponentTypes(entity);

  // Use a stack buffer for small counts
  constexpr size_t kStackSize = 16;
  if (component_infos.size() <= kStackSize) {
    std::array<ComponentTypeId, kStackSize> type_ids = {};
    for (size_t i = 0; i < component_infos.size(); ++i) {
      type_ids[i] = component_infos[i].TypeId();
    }
    archetypes_.MoveEntityOnComponentAdd(entity, ComponentTypeIdOf<T>(),
                                         std::span(type_ids.data(), component_infos.size()));
  } else {
    // Fallback for large counts
    std::vector<ComponentTypeId> type_ids;
    type_ids.reserve(component_infos.size());
    for (const auto& info : component_infos) {
      type_ids.push_back(info.TypeId());
    }
    archetypes_.MoveEntityOnComponentAdd(entity, ComponentTypeIdOf<T>(), type_ids);
  }
}

template <ComponentTrait T>
inline void World::UpdateEntityArchetypeOnRemove(Entity entity) {
  const auto component_infos = components_.GetComponentTypes(entity);

  // Use a stack buffer for small counts
  constexpr size_t kStackSize = 16;
  if (component_infos.size() <= kStackSize) {
    std::array<ComponentTypeId, kStackSize> type_ids = {};
    for (size_t i = 0; i < component_infos.size(); ++i) {
      type_ids[i] = component_infos[i].TypeId();
    }
    archetypes_.MoveEntityOnComponentRemove(entity, ComponentTypeIdOf<T>(),
                                            std::span(type_ids.data(), component_infos.size()));
  } else {
    // Fallback for large counts
    std::vector<ComponentTypeId> type_ids;
    type_ids.reserve(component_infos.size());
    for (const auto& info : component_infos) {
      type_ids.push_back(info.TypeId());
    }
    archetypes_.MoveEntityOnComponentRemove(entity, ComponentTypeIdOf<T>(), type_ids);
  }
}

}  // namespace helios::ecs
