#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/builtin_messages.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/component/bundle.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/manager.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/entity/manager.hpp>
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/manager.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/utils/common_traits.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <string>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief The World class manages entities with their components and systems.
 * @details All modifications to the world (adding/removing entities or
 * components) should be done via command buffers to defer changes until the
 * next update.
 * @note Partially thread safe.
 */
class World {
public:
  World() { AddBuiltinMessages(); }
  World(const World&) = delete;
  World(World&&) noexcept = default;
  ~World() = default;

  World& operator=(const World&) = delete;
  World& operator=(World&&) noexcept = default;

  /**
   * @brief Flushes buffer of pending operations.
   * @note Not thread-safe.
   */
  void Update();

  /**
   * @brief Flushes entity reservations and executes pending commands.
   * @details Does not advance the message lifecycle. Call `Update()` to also
   * swap message buffers and clear aged automatic messages.
   * @note Not thread-safe.
   */
  void Flush();

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
   * @brief Creates new entity.
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
  [[nodiscard]] Entity ReserveEntity() {
    return entity_manager_.ReserveEntity();
  }

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
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to destroy
   */
  void TryDestroyEntity(Entity entity);

  /**
   * @brief Destroys entities and removes them from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Any entity is invalid.
   * - Any entity does not exist in the world.
   * @tparam R Range type containing `Entity` elements
   * @param entities Entities to destroy
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void DestroyEntities(const R& entities);

  /**
   * @brief Tries to destroy entities if they exist in the world.
   * @note Not thread-safe.
   * @warning Triggers assertion only if any entity is invalid (non-existing
   * entities are skipped).
   * @tparam R Range type containing `Entity` elements
   * @param entities Entities to destroy
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  void TryDestroyEntities(const R& entities);

  /**
   * @brief Creates a query over entities matching the specified component
   * criteria.
   * @details Component filters (`With<>`, `Without<>`) and access types
   * (`T&`, `const T&`, `T*`, ...) are all specified as template arguments.
   * @note Not thread-safe.
   * @tparam Args Component access types and optional With/Without filters
   * @tparam Allocator Allocator type for internal query storage
   * @param alloc Allocator instance
   * @return Query object over entities matching the specified criteria
   *
   * @code
   * // Default allocator — no argument needed
   * auto query = world.Query<Transform&, const Velocity&,
   *                          const Gravity*, With<Player>,
   *                          Without<Dead>>();
   *
   * for (auto&& [transform, velocity, gravity] : query) {
   *   transform.position += velocity.direction * velocity.speed;
   *   if (gravity) {
   *     transform.position.y += gravity->force;
   *   }
   * }
   * @endcode
   */
  template <QueryArg... Args,
            typename Allocator = std::allocator<ComponentTypeIndex>>
  [[nodiscard]] auto Query(Allocator alloc = {}) noexcept
      -> BasicQuery<World, Allocator, Args...> {
    return BasicQuery<World, Allocator, Args...>(component_manager_,
                                                 std::move(alloc));
  }

  /**
   * @brief Creates a query using a PMR memory resource.
   * @details Equivalent to the allocator overload but accepts a
   * `pmr::memory_resource*` directly, constructing a
   * `pmr::polymorphic_allocator` internally.
   *
   * @note Not thread-safe.
   * @tparam Args Component access types and optional With/Without filters
   * @param resource Memory resource for internal query storage
   * @return Query object over entities matching the specified criteria
   *
   * @code
   * auto query = world.Query<Transform&, const Velocity&>(&resource);
   *
   * for (auto&& [transform, velocity] : query) {
   *   transform.position += velocity.direction * velocity.speed;
   * }
   * @endcode
   */
  template <QueryArg... Args>
  [[nodiscard]] auto Query(std::pmr::memory_resource* resource) noexcept
      -> BasicQuery<World, std::pmr::polymorphic_allocator<>, Args...> {
    return BasicQuery<World, std::pmr::polymorphic_allocator<>, Args...>(
        component_manager_, resource);
  }

  template <QueryArg... Args>
  auto Query(std::nullptr_t)
      -> BasicQuery<World, std::pmr::polymorphic_allocator<>, Args...> = delete;

  /**
   * @brief Creates a read-only query over entities matching the specified
   * component criteria.
   * @details All component accesses must be const-qualified or be values (being
   * copied) — `T&` and `T*` are rejected at compile time.
   * @note Not thread-safe.
   * @tparam Args Component access types and optional With/Without filters
   * @tparam Allocator Allocator type for internal query storage
   * @param alloc Allocator instance
   * @return Query object over entities matching the specified criteria
   *
   * @code
   * // Default allocator — no argument needed
   * auto query = world.ReadOnlyQuery<Health, const Status*,
   *                                  With<Player>, Without<Dead>>();
   *
   * for (auto&& [health, status] : query) {
   *   if (status) {
   *     helios::log::Trace("health: {}, status: {}", health, *status);
   *   }
   * }
   * @endcode
   */
  template <QueryArg... Args,
            typename Allocator = std::allocator<ComponentTypeIndex>>
    requires details::ValidWorldComponentAccessFromTuple<
        const World,
        typename details::QueryArgSplit<Args...>::Components>::kValue
  [[nodiscard]] auto ReadOnlyQuery(Allocator alloc = {}) const noexcept
      -> BasicQuery<const World, Allocator, Args...> {
    return BasicQuery<const World, Allocator, Args...>(component_manager_,
                                                       std::move(alloc));
  }

  /**
   * @brief Creates a read-only query using a PMR memory resource.
   * @details All component accesses must be const-qualified — see the allocator
   * overload for details.
   *
   * @note Not thread-safe.
   * @tparam Args Component access types and optional With/Without filters
   * @param resource Memory resource for internal query storage
   * @return Query object over entities matching the specified criteria
   *
   * @code
   * auto query = world.ReadOnlyQuery<Health, const Status*>(&resource);
   *
   * for (auto&& [health, status] : query) {
   *   if (status) {
   *     helios::log::Trace("health: {}, status: {}", health, *status);
   *   }
   * }
   * @endcode
   */
  template <QueryArg... Args>
    requires details::ValidWorldComponentAccessFromTuple<
        const World,
        typename details::QueryArgSplit<Args...>::Components>::kValue
  [[nodiscard]] auto ReadOnlyQuery(
      std::pmr::memory_resource* resource) const noexcept
      -> BasicQuery<const World, std::pmr::polymorphic_allocator<>, Args...> {
    return BasicQuery<const World, std::pmr::polymorphic_allocator<>, Args...>(
        component_manager_, resource);
  }

  template <QueryArg... Args>
    requires details::ValidWorldComponentAccessFromTuple<
                 const World,
                 typename details::QueryArgSplit<Args...>::Components>::kValue
  auto ReadOnlyQuery(std::nullptr_t) const
      -> BasicQuery<const World, std::pmr::polymorphic_allocator<>, Args...> =
          delete;

  /**
   * @brief Adds components to the entity.
   * @details If entity already has component of provided type then it will be
   * replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to add
   * @param entity Entity to add components to
   * @param components Components to add
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void AddComponents(Entity entity, Ts&&... components);

  /**
   * @brief Tries to add components to the entity if they don't exist.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to add
   * @param entity Entity to add components to
   * @param components Components to add
   * @return Array of bools indicating whether each component was added (true if
   * added, false otherwise) if more than one component was added, otherwise
   * returns a single bool indicating whether the component was added.
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryAddComponents(Entity entity, Ts&&... components)
      -> std::conditional_t<sizeof...(Ts) == 1, bool,
                            std::array<bool, sizeof...(Ts)>>;

  /**
   * @brief Adds all components in a bundle to the entity.
   * @details Nested bundles are flattened in declaration order. Existing
   * components are replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion if the entity is invalid or not owned by this
   * world.
   * @tparam B Component bundle type
   * @param entity Entity to add components to
   * @param bundle Bundle whose components to add
   */
  template <ComponentBundleTrait B>
  void AddBundle(Entity entity, B&& bundle);

  /**
   * @brief Tries to add every component in a bundle that does not exist.
   * @details Nested bundles are flattened in declaration order. Existing
   * components are preserved.
   * @note Not thread-safe.
   * @warning Triggers assertion if the entity is invalid or not owned by this
   * world.
   * @tparam B Component bundle type
   * @param entity Entity to add components to
   * @param bundle Bundle whose components to add
   * @return Whether each flattened component was added
   */
  template <ComponentBundleTrait B>
  auto TryAddBundle(Entity entity, B&& bundle)
      -> details::ComponentBundleResult<B>;

  /**
   * @brief Emplaces component for the entity.
   * @details Constructs component in-place. If entity already has component of
   * provided type then it will be replaced.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to emplace
   * @tparam Args Argument types to forward to `T`'s constructor
   * @param entity Entity to emplace component to
   * @param args Arguments to forward to component constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceComponent(Entity entity, Args&&... args);

  /**
   * @brief Tries to emplace component for the entity.
   * @details Constructs component in-place. Returns false if component is
   * already on the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to emplace
   * @tparam Args Argument types to forward to `T`'s constructor
   * @param entity Entity to emplace component to
   * @param args Arguments to forward to component constructor
   * @return True if component was emplaced, false otherwise
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplaceComponent(Entity entity, Args&&... args);

  /**
   * @brief Removes components from the entity.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * - Entity does not have any of the components.
   * @tparam Ts Components types to remove
   * @param entity Entity to remove components from
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void RemoveComponents(Entity entity);

  /**
   * @brief Tries to remove components from the entity if they exist.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types to remove
   * @param entity Entity to remove components from
   * @return Array of bools indicating whether each component was removed if
   * more than one component was passed, otherwise returns a single bool
   * indicating whether the component was removed.
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryRemoveComponents(Entity entity)
      -> std::conditional_t<sizeof...(Ts) == 1, bool,
                            std::array<bool, sizeof...(Ts)>>;

  /**
   * @brief Removes all components in a bundle from the entity.
   * @details Nested bundle types are flattened in declaration order.
   * @note Not thread-safe.
   * @warning Triggers assertion if the entity is invalid, not owned by this
   * world, or does not have every flattened component.
   * @tparam B Component bundle type
   * @param entity Entity to remove components from
   */
  template <ComponentBundleTrait B>
  void RemoveBundle(Entity entity);

  /**
   * @brief Tries to remove every component in a bundle that exists.
   * @details Nested bundle types are flattened in declaration order.
   * @note Not thread-safe.
   * @warning Triggers assertion if the entity is invalid or not owned by this
   * world.
   * @tparam B Component bundle type
   * @param entity Entity to remove components from
   * @return Whether each flattened component was removed
   */
  template <ComponentBundleTrait B>
  auto TryRemoveBundle(Entity entity) -> details::ComponentBundleResult<B>;

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
   * @brief Gets a mutable reference to a component.
   * @note Not thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * - Entity does not have the component.
   * @tparam T Component type
   * @param entity Entity to get component from
   * @return Mutable reference to component
   */
  template <ComponentTrait T>
  [[nodiscard]] T& WriteComponent(Entity entity);

  /**
   * @brief Gets a const reference to a component.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * - Entity does not have the component.
   * @tparam T Component type
   * @param entity Entity to get component from
   * @return Const reference to component
   */
  template <ComponentTrait T>
  [[nodiscard]] const T& ReadComponent(Entity entity) const;

  /**
   * @brief Tries to get a mutable pointer to a component.
   * @note Not thread-safe.
   * @tparam T Component type
   * @param entity Entity to get component from
   * @return Pointer to component, or `nullptr` if not found
   */
  template <ComponentTrait T>
  [[nodiscard]] T* TryWriteComponent(Entity entity);

  /**
   * @brief Tries to get a const pointer to a component.
   * @note Thread-safe for read operations.
   * @tparam T Component type
   * @param entity Entity to get component from
   * @return Const pointer to component, or `nullptr` if not found
   */
  template <ComponentTrait T>
  [[nodiscard]] const T* TryReadComponent(Entity entity) const;

  /**
   * @brief Inserts a resource into the world.
   * @details Replaces existing resource if present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ResourceTrait T>
  void InsertResources(T&& resource);

  /**
   * @brief Inserts resources into the world.
   * @details Replaces existing resources if present.
   * @note Not thread-safe.
   * @tparam Ts Resource types
   * @param resources Resources to insert
   */
  template <ResourceTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 1)
  void InsertResources(Ts&&... resources) {
    (InsertResources(std::forward<Ts>(resources)), ...);
  }

  /**
   * @brief Tries to insert resource if not present.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @param resource Resource to insert
   * @return True if inserted, false if resource already exists
   */
  template <ResourceTrait T>
  bool TryInsertResources(T&& resource);

  /**
   * @brief Tries to insert resources if not present.
   * @note Not thread-safe.
   * @tparam Ts Resource types
   * @param resources Resources to insert
   * @return Array of bools indicating whether each resource was inserted
   */
  template <ResourceTrait... Ts>
  auto TryInsertResources(Ts&&... resources)
      -> std::array<bool, sizeof...(Ts)> {
    return {TryInsertResources(std::forward<Ts>(resources))...};
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
  void EmplaceResource(Args&&... args);

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
  bool TryEmplaceResource(Args&&... args);

  /**
   * @brief Removes resources from the world.
   * @note Not thread-safe.
   * @warning Triggers assertion if resource does not exist.
   * @tparam Ts Resource types
   */
  template <ResourceTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void RemoveResources();

  /**
   * @brief Tries to remove resource.
   * @note Not thread-safe.
   * @tparam T Resource type
   * @return True if removed, false if resource didn't exist
   */
  template <ResourceTrait T>
  bool TryRemoveResources();

  /**
   * @brief Tries to remove resources.
   * @note Not thread-safe.
   * @tparam Ts Resource types
   * @return Array of bools indicating whether each resource was removed
   */
  template <ResourceTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 1)
  auto TryRemoveResources() -> std::array<bool, sizeof...(Ts)> {
    return {TryRemoveResources<Ts>()...};
  }

  /**
   * @brief Gets reference to a resource.
   * @note Not thread-safe.
   * @warning Triggers assertion if resource does not exist.
   * @tparam T Resource type
   * @return Reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] T& WriteResource() noexcept;

  /**
   * @brief Gets const reference to a resource.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   * @return Const reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] const T& ReadResource() const noexcept;

  /**
   * @brief Tries to get mutable pointer to a resource.
   * @note Thread-safe for read operations.
   * @tparam T Resource type
   * @return Pointer to resource, or `nullptr` if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] T* TryWriteResource() noexcept {
    return resources_.TryGet<T>();
  }

  /**
   * @brief Tries to get const pointer to a resource.
   * @note Thread-safe for read operations.
   * @tparam T Resource type
   * @return Const pointer to resource, or `nullptr` if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] const T* TryReadResource() const noexcept {
    return resources_.TryGet<T>();
  }

  /**
   * @brief Registers an message type for use.
   * @note Not thread-safe.
   * Should be called during initialization.
   * @tparam T Message type
   */
  template <AnyMessageTrait T>
  void AddMessage();

  /**
   * @brief Registers multiple message types for use.
   * @note Not thread-safe.
   * Should be called during initialization.
   * @tparam Ts Message types to add
   */
  template <AnyMessageTrait... Ts>
    requires(sizeof...(Ts) > 0)
  void AddMessages() {
    (AddMessage<Ts>(), ...);
  }

  /**
   * @brief Registers all built-in non-template message types.
   * @details This is called automatically during `World` construction.
   * @note Not thread-safe.
   */
  void AddBuiltinMessages();

  /**
   * @brief Clears all messages without removing registration.
   * @details Messages can still be written/read after calling this method.
   * To completely reset the message system including registration, use
   * `Clear()`.
   * @note Not thread-safe.
   */
  void ClearMessages() noexcept { messages_.ClearAllQueues(); }

  /**
   * @brief Manually clears messages of specific types from both queues.
   * @details Should only be used for manually-managed messages (auto_clear =
   * false).
   * @note Thread-safe if message is async, not thread-safe otherwise.
   * @warning Triggers assertion if message type is not added.
   * @tparam Ts Message types
   */
  template <AnyMessageTrait... Ts>
    requires(sizeof...(Ts) > 0)
  void ClearMessages();

  /**
   * @brief Gets a reader for messages of type `T` without consume support.
   * @note Thread-safe.
   * @warning Triggers assertion if message type is not added.
   * @tparam T Message type
   * @return Message reader for type `T`
   */
  template <MessageTrait T>
  [[nodiscard]] auto ReadMessages() const noexcept -> MessageReader<T>;

  /**
   * @brief Gets a consumable reader for messages of type `T` without consume
   * support.
   * @note Thread-safe.
   * @warning Triggers assertion if message type is not added.
   * @tparam T Consumable message type
   * @return Consumable message reader for type `T`
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] auto ReadConsumableMessages() noexcept
      -> ConsumableMessageReader<T>;

  /**
   * @brief Gets a writer for messages of type `T`.
   * @note Not thread-safe.
   * @warning Triggers assertion if message type is not added.
   * @tparam T Message type
   * @return Message writer for type `T`
   */
  template <MessageTrait T>
  [[nodiscard]] auto WriteMessages() noexcept -> BasicMessageWriter<T>;

  /**
   * @brief Gets a reader for messages of type `T`.
   * @note Thread-safe.
   * @warning Triggers assertion if message type is not added.
   * @tparam T Async message type
   * @return Async message reader for type `T`
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] auto ReadAsyncMessages() noexcept -> AsyncMessageReader<T>;

  /**
   * @brief Gets a writer for messages of type `T`.
   * @note Thread-safe.
   * @warning Triggers assertion if message type is not added.
   * @tparam T Async message type
   * @return Async message writer for type `T`
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] auto WriteAsyncMessages() noexcept -> AsyncMessageWriter<T>;

  /**
   * @brief Reserves capacity in the command queue to avoid reallocations.
   * @note Not thread-safe.
   * @param count Number of commands to reserve space for
   */
  void ReserveCommands(size_t count) { command_queue_.Reserve(count); }

  /**
   * @brief Enqueues a command to be executed during the next `Update()`.
   * @note Not thread-safe.
   * @tparam Cmd Command type, must satisfy `CommandTrait`
   * @param command Command to enqueue
   */
  template <CommandTrait Cmd>
  void EnqueueCommand(Cmd&& command) {
    command_queue_.Enqueue(std::forward<Cmd>(command));
  }

  /**
   * @brief Enqueues multiple commands to be executed during the next
   * `Update()`.
   * @note Not thread-safe.
   * @tparam R Range type containing command elements that must satisfy  and
   * `CommandTrait`
   * @param range Commands to enqueue
   */
  template <std::ranges::input_range R>
    requires CommandTrait<std::ranges::range_value_t<R>>
  void EnqueueCommandBulk(R&& range) {
    command_queue_.EnqueueBulk(std::forward<R>(range));
  }

  /**
   * @brief Checks if entity exists in the world.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to check
   * @return True if entity exists, false otherwise
   */
  [[nodiscard]] bool Exists(Entity entity) const noexcept;

  /**
   * @brief Checks if entity has component.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to check
   * @param entity Entity to check
   * @return True if entity has the component
   */
  template <ComponentTrait T>
  [[nodiscard]] bool HasComponent(Entity entity) const;

  /**
   * @brief Checks if entity has components.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Ts Components types
   * @param entity Entity to check
   * @return Array of bools indicating whether entity has each component (true
   * if entity has component, false otherwise)
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  [[nodiscard]] auto HasComponents(Entity entity) const
      -> std::array<bool, sizeof...(Ts)>;

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
   * @brief Checks if a message added.
   * @note Thread-safe for read operations.
   * @tparam T Message type
   * @return True if message exists, false otherwise
   */
  template <AnyMessageTrait T>
  [[nodiscard]] bool HasMessage() const noexcept {
    return messages_.IsRegistered<T>();
  }

  /**
   * @brief Checks if messages of a specific type exist in message queue.
   * @note Thread-safe for read operations.
   * @tparam T Message type
   * @return True if messages exist, false otherwise
   */
  template <AnyMessageTrait T>
  [[nodiscard]] bool HasMessages() const noexcept {
    return messages_.HasMessages<T>();
  }

  /**
   * @brief Checks if there are any pending commands in the command queue.
   * @note Thread-safe for read operations.
   * @return True if there are pending commands, false otherwise
   */
  [[nodiscard]] bool HasCommands() const noexcept {
    return !command_queue_.Empty();
  }

  /**
   * @brief Gets the number of entities in the world.
   * @note Thread-safe for read operations.
   * @return Number of entities in the world
   */
  [[nodiscard]] size_t EntityCount() const noexcept {
    return entity_manager_.Count();
  }

  /**
   * @brief Gets the number of resources in the world.
   * @note Thread-safe for read operations.
   * @return Number of resources in the world
   */
  [[nodiscard]] size_t ResourceCount() const noexcept {
    return resources_.Count();
  }

  /**
   * @brief Gets the number of pending commands in the command queue.
   * @note Thread-safe for read operations.
   * @return Number of pending commands in the command queue
   */
  [[nodiscard]] size_t CommandCount() const noexcept {
    return command_queue_.Size();
  }

  /**
   * @brief Gets the entity manager.
   * @note Thread-safe.
   * @return Reference to entity manager
   */
  [[nodiscard]] EntityManager& Entities() noexcept { return entity_manager_; }

  /**
   * @brief Gets the entity manager.
   * @note Thread-safe.
   * @return Const reference to entity manager
   */
  [[nodiscard]] const EntityManager& Entities() const noexcept {
    return entity_manager_;
  }

  /**
   * @brief Gets the component manager.
   * @note Thread-safe.
   * @return Mutable reference to component manager
   */
  [[nodiscard]] ComponentManager& Components() noexcept {
    return component_manager_;
  }

  /**
   * @brief Gets the component manager (const).
   * @note Thread-safe.
   * @return Const reference to component manager
   */
  [[nodiscard]] const ComponentManager& Components() const noexcept {
    return component_manager_;
  }

  /**
   * @brief Gets the resource manager.
   * @note Thread-safe.
   * @return Mutable reference to resource manager
   */
  [[nodiscard]] ResourceManager& Resources() noexcept { return resources_; }

  /**
   * @brief Gets the resource manager (const).
   * @note Thread-safe.
   * @return Const reference to resource manager
   */
  [[nodiscard]] const ResourceManager& Resources() const noexcept {
    return resources_;
  }

  /**
   * @brief Gets the message manager.
   * @note Thread-safe.
   * @return Mutable reference to message manager
   */
  [[nodiscard]] MessageManager& Messages() noexcept { return messages_; }

  /**
   * @brief Gets the message manager (const).
   * @note Thread-safe.
   * @return Const reference to message manager
   */
  [[nodiscard]] const MessageManager& Messages() const noexcept {
    return messages_;
  }

private:
  EntityManager entity_manager_;  ///< Entity manager that handles entity
                                  ///< creation, destruction, and validation.

  ComponentManager component_manager_;  ///< Component manager that handles
                                        ///< storage and access of components.

  ResourceManager resources_;  ///< Resource manager that handles storage and
                               ///< access of resources.

  MessageManager messages_;  ///< Message manager that handles registration and
                             ///< storage of messages.

  CmdQueue<> command_queue_;  ///< Command queue for deferred operations on the
                              ///< world, executed during `Flush()`.
};

inline void World::Update() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::World::Update");

  Flush();
  messages_.Update();
}

inline void World::Flush() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::World::Flush");

  entity_manager_.Flush(
      [this](Entity entity) { component_manager_.InitEntity(entity); });
  command_queue_.ExecuteAll(*this);
}

inline void World::Clear() {
  command_queue_.Clear();
  component_manager_.Clear();
  entity_manager_.Clear();
  resources_.Clear();
  messages_.Clear();
  AddBuiltinMessages();
}

inline void World::ClearEntities() noexcept {
  component_manager_.ClearData();
  entity_manager_.Clear();
}

inline Entity World::CreateEntity() {
  Entity entity = entity_manager_.Create();
  component_manager_.InitEntity(entity);
  messages_.Write(EntityAddedMsg(entity));
  return entity;
}

inline void World::DestroyEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  component_manager_.RemoveEntity(entity);
  entity_manager_.Destroy(entity);
  messages_.Write(EntityDestroyedMsg(entity));
}

inline void World::TryDestroyEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (!entity_manager_.Validate(entity)) {
    return;
  }

  component_manager_.TryRemoveEntity(entity);
  entity_manager_.Destroy(entity);
  messages_.Write(EntityDestroyedMsg(entity));
}

template <std::ranges::input_range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void World::DestroyEntities(const R& entities) {
  for (const auto& entity : entities) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
    HELIOS_ASSERT(entity_manager_.Validate(entity),
                  "World does not own entity '{}'!", entity);
    DestroyEntity(entity);
  }
}

template <std::ranges::input_range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
inline void World::TryDestroyEntities(const R& entities) {
  for (const auto& entity : entities) {
    TryDestroyEntity(entity);
  }
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void World::AddComponents(Entity entity, Ts&&... components) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  AddMessages<ComponentAddedMsg<std::remove_cvref_t<Ts>>...>();
  component_manager_.Add(entity, std::forward<Ts>(components)...);
  (messages_.Write(ComponentAddedMsg<std::remove_cvref_t<Ts>>(entity)), ...);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto World::TryAddComponents(Entity entity, Ts&&... components)
    -> std::conditional_t<sizeof...(Ts) == 1, bool,
                          std::array<bool, sizeof...(Ts)>> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  AddMessages<ComponentAddedMsg<std::remove_cvref_t<Ts>>...>();

  auto added =
      component_manager_.TryAdd(entity, std::forward<Ts>(components)...);

  if constexpr (sizeof...(Ts) == 1) {
    if (added) {
      (messages_.Write(ComponentAddedMsg<std::remove_cvref_t<Ts>>(entity)),
       ...);
    }
  } else {
    [this, entity, &added]<size_t... Is>(std::index_sequence<Is...>) {
      ((added[Is] &&
        (messages_.Write(ComponentAddedMsg<std::remove_cvref_t<Ts>>(entity)),
         true)),
       ...);
    }(std::index_sequence_for<Ts...>{});
  }

  return added;
}

template <ComponentBundleTrait B>
inline void World::AddBundle(Entity entity, B&& bundle) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  details::ApplyComponentBundle(
      std::forward<B>(bundle), [this, entity](auto&&... components) {
        AddComponents(entity,
                      std::forward<decltype(components)>(components)...);
      });
}

template <ComponentBundleTrait B>
inline auto World::TryAddBundle(Entity entity, B&& bundle)
    -> details::ComponentBundleResult<B> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  return details::ApplyComponentBundle(
      std::forward<B>(bundle), [this, entity](auto&&... components) {
        return TryAddComponents(
            entity, std::forward<decltype(components)>(components)...);
      });
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void World::EmplaceComponent(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  AddMessage<ComponentAddedMsg<T>>();
  component_manager_.template Emplace<T>(entity, std::forward<Args>(args)...);
  messages_.Write(ComponentAddedMsg<T>(entity));
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool World::TryEmplaceComponent(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  AddMessage<ComponentAddedMsg<T>>();
  const bool added = component_manager_.template TryEmplace<T>(
      entity, std::forward<Args>(args)...);
  if (added) {
    messages_.Write(ComponentAddedMsg<T>(entity));
  }
  return added;
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void World::RemoveComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  AddMessages<ComponentRemovedMsg<Ts>...>();
  component_manager_.template Remove<Ts...>(entity);
  (messages_.Write(ComponentRemovedMsg<Ts>(entity)), ...);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto World::TryRemoveComponents(Entity entity)
    -> std::conditional_t<sizeof...(Ts) == 1, bool,
                          std::array<bool, sizeof...(Ts)>> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  AddMessages<ComponentRemovedMsg<Ts>...>();
  auto removed = component_manager_.template TryRemove<Ts...>(entity);

  const auto results = [&removed]()->std::array<bool, sizeof...(Ts)> {
    if constexpr (sizeof...(Ts) == 1) {
      return {removed};
    } else {
      return removed;
    }
  }
  ();

  // Apply to each component type using index_sequence
  [this, entity, &results]<size_t... Is>(std::index_sequence<Is...>) {
    ((results[Is] ? messages_.Write(ComponentRemovedMsg<Ts>(entity)) : void()),
     ...);
  }(std::index_sequence_for<Ts...>{});

  return removed;
}

template <ComponentBundleTrait B>
inline void World::RemoveBundle(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  details::ApplyComponentBundleTypes<B>(
      [this, entity]<typename... Ts>() { RemoveComponents<Ts...>(entity); });
}

template <ComponentBundleTrait B>
inline auto World::TryRemoveBundle(Entity entity)
    -> details::ComponentBundleResult<B> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  return details::ApplyComponentBundleTypes<B>(
      [this, entity]<typename... Ts>() {
        return TryRemoveComponents<Ts...>(entity);
      });
}

inline void World::ClearComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);

  component_manager_.Clear(entity);
  messages_.Write(ComponentsClearedMsg(entity));
}

template <ComponentTrait T>
inline T& World::WriteComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);
  return component_manager_.template Get<T>(entity);
}

template <ComponentTrait T>
inline const T& World::ReadComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);
  return component_manager_.template Get<T>(entity);
}

template <ComponentTrait T>
inline T* World::TryWriteComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);
  return component_manager_.template TryGet<T>(entity);
}

template <ComponentTrait T>
inline const T* World::TryReadComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);
  return component_manager_.template TryGet<T>(entity);
}

template <ResourceTrait T>
inline void World::InsertResources(T&& resource) {
  AddMessage<ResourceInsertedMsg<T>>();

  resources_.Insert(std::forward<T>(resource));
  ResourceCallOnInsert(resources_.template Get<T>(), *this);
  messages_.Write(ResourceInsertedMsg<T>());
}

template <ResourceTrait T>
inline bool World::TryInsertResources(T&& resource) {
  AddMessage<ResourceInsertedMsg<T>>();

  const bool inserted = resources_.TryInsert(std::forward<T>(resource));
  if (inserted) {
    auto& inserted_resource = resources_.template Get<T>();
    ResourceCallOnInsert(inserted_resource, *this);
    messages_.Write(ResourceInsertedMsg<T>());
  }
  return inserted;
}

template <ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void World::EmplaceResource(Args&&... args) {
  AddMessage<ResourceInsertedMsg<T>>();

  resources_.template Emplace<T>(std::forward<Args>(args)...);
  auto& inserted_resource = resources_.template Get<T>();
  ResourceCallOnInsert(inserted_resource, *this);
  messages_.Write(ResourceInsertedMsg<T>());
}

template <ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool World::TryEmplaceResource(Args&&... args) {
  AddMessage<ResourceInsertedMsg<T>>();

  const bool emplaced =
      resources_.template TryEmplace<T>(std::forward<Args>(args)...);
  if (emplaced) {
    auto& inserted_resource = resources_.template Get<T>();
    ResourceCallOnInsert(inserted_resource, *this);
    messages_.Write(ResourceInsertedMsg<T>());
  }
  return emplaced;
}

template <ResourceTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void World::RemoveResources() {
#ifdef HELIOS_ENABLE_ASSERTS
  const auto has_resource = std::to_array({HasResource<Ts>()...});
  constexpr auto names = std::to_array({ResourceNameOf<Ts>()...});

  const bool all_has_resource =
      std::ranges::all_of(has_resource, std::identity{});
  if (!all_has_resource) [[unlikely]] {
    std::string missing;
    for (size_t i = 0; i < has_resource.size(); ++i) {
      if (has_resource[i]) {
        continue;
      }
      missing.append(names[i]);
      if (i < has_resource.size() - 1) {
        missing.append(", ");
      }
    }
    HELIOS_ASSERT(missing.empty(), "Resource(s) '{}' do not exist!", missing);
  }
#endif

  (AddMessage<ResourceRemovedMsg<Ts>>(), ...);
  (ResourceCallOnRemove(resources_.template Get<Ts>(), *this), ...);
  (resources_.template Remove<Ts>(), ...);
  (messages_.Write(ResourceRemovedMsg<Ts>()), ...);
}

template <ResourceTrait T>
inline bool World::TryRemoveResources() {
  AddMessage<ResourceRemovedMsg<T>>();

  bool removed = false;

  if (T* resource = resources_.template TryGet<T>(); resource != nullptr) {
    ResourceCallOnRemove(*resource, *this);
    removed = resources_.template TryRemove<T>();
    messages_.Write(ResourceRemovedMsg<T>());
  }
  return removed;
}

template <ResourceTrait T>
inline T& World::WriteResource() noexcept {
  HELIOS_ASSERT(HasResource<T>(), "Resource of type '{}' does not exist!",
                ResourceNameOf<T>());
  return resources_.template Get<T>();
}

template <ResourceTrait T>
inline const T& World::ReadResource() const noexcept {
  HELIOS_ASSERT(HasResource<T>(), "Resource of type '{}' does not exist!",
                ResourceNameOf<T>());
  return resources_.template Get<T>();
}

template <AnyMessageTrait T>
inline void World::AddMessage() {
  if (!HasMessage<T>()) {
    messages_.template Register<T>();
  }
}

inline void World::AddBuiltinMessages() {
  AddMessage<EntityAddedMsg>();
  AddMessage<EntityDestroyedMsg>();
  AddMessage<ComponentsClearedMsg>();
}

template <AsyncMessageTrait T>
inline auto World::ReadAsyncMessages() noexcept -> AsyncMessageReader<T> {
  HELIOS_ASSERT(HasMessage<T>(), "Message of type '{}' is not registered!",
                MessageNameOf<T>());
  return AsyncMessageReader<T>(messages_);
}

template <AsyncMessageTrait T>
inline auto World::WriteAsyncMessages() noexcept -> AsyncMessageWriter<T> {
  HELIOS_ASSERT(HasMessage<T>(), "Message of type '{}' is not registered!",
                MessageNameOf<T>());
  return AsyncMessageWriter<T>(messages_);
}

template <AnyMessageTrait... Ts>
  requires(sizeof...(Ts) > 0)
inline void World::ClearMessages() {
#ifdef HELIOS_ENABLE_ASSERTS
  const auto has_message = std::to_array({HasMessage<Ts>()...});
  constexpr auto names = std::to_array({MessageNameOf<Ts>()...});

  const bool all_has_message =
      std::ranges::all_of(has_message, std::identity{});
  if (!all_has_message) [[unlikely]] {
    std::string missing;
    for (size_t i = 0; i < has_message.size(); ++i) {
      if (has_message[i]) {
        continue;
      }
      missing.append(names[i]);
      if (i < has_message.size() - 1) {
        missing.append(", ");
      }
    }
    HELIOS_ASSERT(missing.empty(), "Message(s) '{}' not registered!", missing);
  }
#endif

  (messages_.template ManualClear<Ts>(), ...);
}

template <MessageTrait T>
inline auto World::ReadMessages() const noexcept -> MessageReader<T> {
  HELIOS_ASSERT(HasMessage<T>(), "Message of type '{}' is not registered!",
                MessageNameOf<T>());
  return MessageReader<T>(messages_);
}

template <ConsumableMessageTrait T>
inline auto World::ReadConsumableMessages() noexcept
    -> ConsumableMessageReader<T> {
  HELIOS_ASSERT(HasMessage<T>(), "Message of type '{}' is not registered!",
                MessageNameOf<T>());
  return ConsumableMessageReader<T>(messages_);
}

template <MessageTrait T>
inline auto World::WriteMessages() noexcept -> BasicMessageWriter<T> {
  HELIOS_ASSERT(HasMessage<T>(), "Message of type '{}' is not registered!",
                MessageNameOf<T>());
  return BasicMessageWriter<T>(messages_.CurrentQueue());
}

inline bool World::Exists(Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return entity_manager_.Validate(entity);
}

template <ComponentTrait T>
inline bool World::HasComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);
  return component_manager_.template Has<T>(entity);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto World::HasComponents(Entity entity) const
    -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(entity_manager_.Validate(entity),
                "World does not own entity '{}'!", entity);
  return component_manager_.template Has<Ts...>(entity);
}

}  // namespace helios::ecs
