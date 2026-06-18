#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/command/command.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/world.hpp>
#include <helios/utils/common_traits.hpp>

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::ecs {

/**
 * @brief Command that executes a function with `World` reference.
 * @details Wraps arbitrary functions for deferred execution during
 * `World::Update()`. The function must be invocable with a `World&` parameter.
 * @tparam F Function type that accepts `World&`
 */
template <typename F>
  requires std::invocable<F, World&>
class FunctionCmd {
public:
  /**
   * @brief Constructs function command from an lvalue callable.
   * @param func Function to copy, must accept `World&` parameter
   */
  explicit constexpr FunctionCmd(const F& func) noexcept(
      std::is_nothrow_copy_constructible_v<F>)
      : func_(func) {}

  /**
   * @brief Constructs function command from an rvalue callable.
   * @param func Function to move, must accept `World&` parameter
   */
  explicit constexpr FunctionCmd(F&& func) noexcept(
      std::is_nothrow_move_constructible_v<F>)
      : func_(std::move(func)) {}

  constexpr FunctionCmd(const FunctionCmd&) noexcept(
      std::is_nothrow_copy_constructible_v<F>) = default;
  constexpr FunctionCmd(FunctionCmd&&) noexcept(
      std::is_nothrow_move_constructible_v<F>) = default;
  constexpr ~FunctionCmd() noexcept(std::is_nothrow_destructible_v<F>) =
      default;

  constexpr FunctionCmd& operator=(const FunctionCmd&) noexcept(
      std::is_nothrow_copy_assignable_v<F>) = default;
  constexpr FunctionCmd& operator=(FunctionCmd&&) noexcept(
      std::is_nothrow_move_assignable_v<F>) = default;

  /**
   * @brief Executes the wrapped function.
   * @param world World reference to pass to the function
   */
  void Execute(World& world) noexcept(std::is_nothrow_invocable_v<F, World&>) {
    func_(world);
  }

private:
  F func_;  ///< Stored function to execute
};

template <typename G>
FunctionCmd(G&&) -> FunctionCmd<std::remove_cvref_t<G>>;

/**
 * @brief Command to destroy a single entity.
 * @details Removes entity and all its components from the world during
 * execution.
 * @warning Will trigger assertion if the world does not own (entity
 * index/generation mismatch) the entity.
 */
class DestroyEntityCmd {
public:
  /**
   * @brief Constructs destroy entity command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to destroy
   */
  explicit constexpr DestroyEntityCmd(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr DestroyEntityCmd(const DestroyEntityCmd&) noexcept = default;
  constexpr DestroyEntityCmd(DestroyEntityCmd&&) noexcept = default;
  constexpr ~DestroyEntityCmd() noexcept = default;

  constexpr DestroyEntityCmd& operator=(const DestroyEntityCmd&) noexcept =
      default;
  constexpr DestroyEntityCmd& operator=(DestroyEntityCmd&&) noexcept = default;

  /**
   * @brief Executes entity destruction.
   * @warning Triggers assertion if the world does not own the entity.
   * @param world World to remove entity from
   */
  void Execute(World& world) { world.DestroyEntity(entity_); }

private:
  Entity entity_;  ///< Entity to destroy
};

/**
 * @brief Command to destroy multiple entities.
 * @details Efficiently destroys multiple entities in a single operation.
 * @warning Will assertion if any of the entities do not exist in the world
 * (when world updated).
 * @tparam Alloc Allocator type
 */
template <typename Alloc = std::allocator<Entity>>
class DestroyEntitiesCmd {
public:
  /**
   * @brief Constructs destroy entities command from vector of entities.
   * @warning Triggers assertion if any entity is invalid.
   * @param entities Vector of entities to destroy
   */
  explicit constexpr DestroyEntitiesCmd(std::vector<Entity, Alloc> entities)
      : entities_(std::move(entities)) {
    HELIOS_ASSERT(std::ranges::all_of(
                      entities_, [](Entity entity) { return entity.Valid(); }),
                  "One or more entities are invalid!");
  }

  constexpr DestroyEntitiesCmd(const DestroyEntitiesCmd&) noexcept = default;
  constexpr DestroyEntitiesCmd(DestroyEntitiesCmd&&) noexcept = default;
  constexpr ~DestroyEntitiesCmd() noexcept = default;

  constexpr DestroyEntitiesCmd& operator=(const DestroyEntitiesCmd&) noexcept =
      default;
  constexpr DestroyEntitiesCmd& operator=(DestroyEntitiesCmd&&) noexcept =
      default;

  /**
   * @brief Executes entities destruction.
   * @warning Triggers assertion if any entity does not exist in the world.
   * @param world World to remove entities from
   */
  void Execute(World& world) { world.DestroyEntities(entities_); }

private:
  std::vector<Entity, Alloc> entities_;  ///< Entities to destroy
};

/**
 * @brief Command to try destroy a single entity.
 * @details Will destroy the entity only if it exists in the world.
 */
class TryDestroyEntityCmd {
public:
  /**
   * @brief Constructs try destroy entity command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to destroy
   */
  explicit constexpr TryDestroyEntityCmd(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr TryDestroyEntityCmd(const TryDestroyEntityCmd&) noexcept = default;
  constexpr TryDestroyEntityCmd(TryDestroyEntityCmd&&) noexcept = default;
  constexpr ~TryDestroyEntityCmd() noexcept = default;

  constexpr TryDestroyEntityCmd& operator=(
      const TryDestroyEntityCmd&) noexcept = default;
  constexpr TryDestroyEntityCmd& operator=(TryDestroyEntityCmd&&) noexcept =
      default;

  /**
   * @brief Executes entity destruction if it exists.
   * @param world World to remove entity from
   */
  void Execute(World& world) { world.TryDestroyEntity(entity_); }

private:
  Entity entity_;
};

/**
 * @brief Command to try destroy multiple entities.
 * @details Will destroy entities only if they exist in the world.
 * @tparam Alloc Allocator type
 */
template <typename Alloc = std::allocator<Entity>>
class TryDestroyEntitiesCmd {
public:
  /**
   * @brief Constructs try destroy entities command from vector of entities.
   * @warning Triggers assertion if any entity is invalid.
   * @param entities Vector of entities to destroy
   */
  explicit constexpr TryDestroyEntitiesCmd(std::vector<Entity, Alloc> entities)
      : entities_(std::move(entities)) {
    HELIOS_ASSERT(
        std::ranges::all_of(
            entities_, [](const Entity& entity) { return entity.Valid(); }),
        "One or more entities are invalid!");
  }

  constexpr TryDestroyEntitiesCmd(const TryDestroyEntitiesCmd&) noexcept =
      default;
  constexpr TryDestroyEntitiesCmd(TryDestroyEntitiesCmd&&) noexcept = default;
  constexpr ~TryDestroyEntitiesCmd() noexcept = default;

  constexpr TryDestroyEntitiesCmd& operator=(
      const TryDestroyEntitiesCmd&) noexcept = default;
  constexpr TryDestroyEntitiesCmd& operator=(TryDestroyEntitiesCmd&&) noexcept =
      default;

  /**
   * @brief Executes entities destruction if they exist.
   * @param world World to remove entities from
   */
  void Execute(World& world) { world.TryDestroyEntities(entities_); }

private:
  std::vector<Entity, Alloc> entities_;
};

/**
 * @brief Command to add multiple components to an entity.
 * @details Efficiently adds multiple components in a single operation.
 * If entity already has any of the components, they will be replaced.
 * @warning Will trigger assertion if the entity does not exist in the world
 * (when world updated).
 * @tparam Ts Component types to add (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
class AddComponentsCmd {
public:
  /**
   * @brief Constructs add components command.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Us Component types to add (must match `Ts`)
   * @param entity Entity to add components to
   * @param components Components to add
   */
  template <ComponentTrait... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) &&
                (std::same_as<Ts, std::remove_cvref_t<Us>> && ...)
  constexpr AddComponentsCmd(Entity entity, Us&&... components) noexcept(
      noexcept(std::tuple<Us...>(std::forward<Us>(components)...)))
      : entity_(entity), components_(std::forward<Us>(components)...) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr AddComponentsCmd(const AddComponentsCmd&) noexcept = default;
  constexpr AddComponentsCmd(AddComponentsCmd&&) noexcept = default;
  constexpr ~AddComponentsCmd() noexcept = default;

  constexpr AddComponentsCmd& operator=(const AddComponentsCmd&) noexcept =
      default;
  constexpr AddComponentsCmd& operator=(AddComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes components addition.
   * @warning Triggers assertion if the entity does not exist in the world.
   * @param world World to add components in
   */
  void Execute(World& world) {
    std::apply(
        [&world, this](auto&&... args) {
          world.AddComponents(entity_, std::forward<decltype(args)>(args)...);
        },
        std::move(components_));
  }

private:
  Entity entity_;                 ///< Entity to add components to
  std::tuple<Ts...> components_;  ///< Components to add
};

template <ComponentTrait... Us>
AddComponentsCmd(Entity, Us&&...)
    -> AddComponentsCmd<std::remove_cvref_t<Us>...>;

/**
 * @brief Command to try add multiple components (only missing ones).
 * @details Adds multiple components to the specified entity during execution
 * only if it doesn't already have them. If entity already has any of the
 * components, they will be left unchanged.
 * @warning Will trigger assertion if the entity does not exist in the world
 * (when world updated).
 * @tparam Ts Component types (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
class TryAddComponentsCmd {
public:
  /**
   * @brief Constructs try add components command.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Us Component types to add (must match `Ts`)
   * @param entity Entity to add components to
   * @param components Components to add
   */
  template <ComponentTrait... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) &&
                (std::same_as<Ts, std::remove_cvref_t<Us>> && ...)
  constexpr TryAddComponentsCmd(Entity entity, Us&&... components) noexcept
      : entity_(entity), components_(std::forward<Us>(components)...) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr TryAddComponentsCmd(const TryAddComponentsCmd&) noexcept = default;
  constexpr TryAddComponentsCmd(TryAddComponentsCmd&&) noexcept = default;
  constexpr ~TryAddComponentsCmd() noexcept = default;

  constexpr TryAddComponentsCmd& operator=(
      const TryAddComponentsCmd&) noexcept = default;
  constexpr TryAddComponentsCmd& operator=(TryAddComponentsCmd&&) noexcept =
      default;

  /**
   * @brief Executes components addition if entity is missing them.
   * @warning Triggers assertion if the entity does not exist in the world.
   * @param world World to add components in
   */
  void Execute(World& world) {
    std::apply(
        [&world, this](auto&&... args) {
          world.TryAddComponents(entity_,
                                 std::forward<decltype(args)>(args)...);
        },
        std::move(components_));
  }

private:
  Entity entity_;
  std::tuple<Ts...> components_;
};

template <ComponentTrait... Us>
TryAddComponentsCmd(Entity, Us&&...)
    -> TryAddComponentsCmd<std::remove_cvref_t<Us>...>;

/**
 * @brief Command to remove multiple components from an entity.
 * @details Efficiently removes multiple components in a single operation.
 * @warning Will trigger assertion if the entity does not exist in the world
 * (when world updated).
 * @tparam Ts Component types to remove (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
class RemoveComponentsCmd {
public:
  /**
   * @brief Constructs remove components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove components from
   */
  explicit constexpr RemoveComponentsCmd(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }
  constexpr RemoveComponentsCmd(const RemoveComponentsCmd&) noexcept = default;
  constexpr RemoveComponentsCmd(RemoveComponentsCmd&&) noexcept = default;
  constexpr ~RemoveComponentsCmd() noexcept = default;

  constexpr RemoveComponentsCmd& operator=(
      const RemoveComponentsCmd&) noexcept = default;
  constexpr RemoveComponentsCmd& operator=(RemoveComponentsCmd&&) noexcept =
      default;

  /**
   * @brief Executes components removal.
   * @warning Triggers assertion if the entity does not exist in the world.
   * @param world World to remove components from
   */
  void Execute(World& world) { world.RemoveComponents<Ts...>(entity_); }

private:
  Entity entity_;  ///< Entity to remove components from
};

/**
 * @brief Command to try remove multiple components (only those present).
 * @details Removes multiple components from the specified entity during
 * execution only if they exist.
 * @warning Will trigger assertion if the entity does not exist in the world
 * (when world updated).
 * @tparam Ts Component types (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
class TryRemoveComponentsCmd {
public:
  /**
   * @brief Constructs try remove components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove components from
   */
  explicit constexpr TryRemoveComponentsCmd(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr TryRemoveComponentsCmd(const TryRemoveComponentsCmd&) noexcept =
      default;
  constexpr TryRemoveComponentsCmd(TryRemoveComponentsCmd&&) noexcept = default;
  constexpr ~TryRemoveComponentsCmd() noexcept = default;

  constexpr TryRemoveComponentsCmd& operator=(
      const TryRemoveComponentsCmd&) noexcept = default;
  constexpr TryRemoveComponentsCmd& operator=(
      TryRemoveComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes components removal if entity has them.
   * @warning Triggers assertion if the entity does not exist in the world.
   * @param world World to remove components from
   */
  void Execute(World& world) { world.TryRemoveComponents<Ts...>(entity_); }

private:
  Entity entity_;  ///< Entity to remove components from
};

/**
 * @brief Command to clear all components from an entity.
 * @details Removes all components from the specified entity during execution.
 * @warning Will trigger assertion if the entity does not exist in the world
 * (when world updated).
 */
class ClearComponentsCmd {
public:
  /**
   * @brief Constructs clear components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to clear components from
   */
  explicit constexpr ClearComponentsCmd(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ClearComponentsCmd(const ClearComponentsCmd&) noexcept = default;
  constexpr ClearComponentsCmd(ClearComponentsCmd&&) noexcept = default;
  constexpr ~ClearComponentsCmd() noexcept = default;

  constexpr ClearComponentsCmd& operator=(const ClearComponentsCmd&) noexcept =
      default;
  constexpr ClearComponentsCmd& operator=(ClearComponentsCmd&&) noexcept =
      default;

  /**
   * @brief Executes component clearing.
   * @warning Triggers assertion if the entity does not exist in the world.
   * @param world World to clear components in
   */
  void Execute(World& world) { world.ClearComponents(entity_); }

private:
  Entity entity_;  ///< Entity to clear components from
};

/**
 * @brief Command to insert a resource into the world.
 * @details Inserts resource of type `T` into the world during execution.
 * If resource already exists, it will be replaced.
 * @tparam T Resource type to insert
 */
template <ResourceTrait T>
class InsertResourceCmd {
public:
  /**
   * @brief Constructs insert resource command from an lvalue.
   * @param resource Resource to copy
   */
  explicit constexpr InsertResourceCmd(const T& resource) noexcept(
      std::is_nothrow_copy_constructible_v<T>)
      : resource_(resource) {}

  /**
   * @brief Constructs insert resource command from an rvalue.
   * @param resource Resource to move
   */
  explicit constexpr InsertResourceCmd(T&& resource) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : resource_(std::move(resource)) {}

  constexpr InsertResourceCmd(const InsertResourceCmd&) noexcept(
      std::is_nothrow_copy_constructible_v<T>) = default;
  constexpr InsertResourceCmd(InsertResourceCmd&&) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;
  constexpr ~InsertResourceCmd() noexcept(std::is_nothrow_destructible_v<T>) =
      default;

  constexpr InsertResourceCmd& operator=(const InsertResourceCmd&) noexcept(
      std::is_nothrow_copy_assignable_v<T>) = default;

  constexpr InsertResourceCmd& operator=(InsertResourceCmd&&) noexcept(
      std::is_nothrow_move_assignable_v<T>) = default;

  /**
   * @brief Executes resource insertion.
   * @param world World to insert resource into
   */
  void Execute(World& world) { world.InsertResources(std::move(resource_)); }

private:
  T resource_;  ///< Resource to insert
};

/**
 * @brief Command to try insert a resource (only if missing).
 * @details Inserts resource of type `T` into the world during execution only if
 * it doesn't already exist. If resource already exists, it will be left
 * unchanged.
 * @tparam T Resource type
 */
template <ResourceTrait T>
class TryInsertResourceCmd {
public:
  /**
   * @brief Constructs try insert resource command from an lvalue.
   * @param resource Resource to copy
   */
  explicit constexpr TryInsertResourceCmd(const T& resource) noexcept(
      std::is_nothrow_copy_constructible_v<T>)
      : resource_(resource) {}

  /**
   * @brief Constructs try insert resource command from an rvalue.
   * @param resource Resource to move
   */
  explicit constexpr TryInsertResourceCmd(T&& resource) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : resource_(std::move(resource)) {}

  constexpr TryInsertResourceCmd(const TryInsertResourceCmd&) noexcept =
      default;
  constexpr TryInsertResourceCmd(TryInsertResourceCmd&&) noexcept = default;
  constexpr ~TryInsertResourceCmd() noexcept = default;

  constexpr TryInsertResourceCmd& operator=(
      const TryInsertResourceCmd&) noexcept = default;
  constexpr TryInsertResourceCmd& operator=(TryInsertResourceCmd&&) noexcept =
      default;

  /**
   * @brief Executes resource insertion if it doesn't exist.
   * @param world World to insert resource into
   */
  void Execute(World& world) { world.TryInsertResources(std::move(resource_)); }

private:
  T resource_;  ///< Resource to insert
};

/**
 * @brief Command to remove resource from the world.
 * @details Removes resource from the world during execution.
 * @warning Will trigger assertion if the resource does not exist in the world
 * (when world updated).
 * @tparam T Resource type to remove
 */
template <ResourceTrait T>
struct RemoveResourceCmd {
  /**
   * @brief Executes resource removal.
   * @warning Triggers assertion if the resource does not exist in the world.
   * @param world World to remove resource from
   */
  static void Execute(World& world) { world.RemoveResources<T>(); }
};

/**
 * @brief Command to try remove resource (only if present).
 * @details Removes resource from the world during execution only if
 * it exists.
 * @tparam T Resource type to remove
 */
template <ResourceTrait T>
struct TryRemoveResourceCmd {
  /**
   * @brief Executes resource removal if it exists.
   * @param world World to remove resource from
   */
  static void Execute(World& world) { world.TryRemoveResources<T>(); }
};

/**
 * @brief Command to clear all messages of a specific type from the queue.
 * @details Clears all messages of type `T` from the message queue during
 * execution without removing registration.
 * @tparam T Message type to clear
 */
template <AnyMessageTrait T>
struct ClearMessagesCmd {
  /**
   * @brief Executes the command to clear messages of type `T`.
   * @param world Reference to the world
   */
  static void Execute(World& world) { world.ClearMessages<T>(); }
};

/**
 * @brief Command to clear all message queues without removing registration.
 * @details Clears all messages of all types from their respective queues during
 * execution.
 */
struct ClearAllMessagesCmd {
  /**
   * @brief Executes the command to clear all message queues.
   * @param world Reference to the world
   */
  static void Execute(World& world) { world.ClearMessages(); }
};

}  // namespace helios::ecs
