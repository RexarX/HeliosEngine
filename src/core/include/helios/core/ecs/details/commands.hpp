#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/command.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Command that executes a function with World reference.
 * @details Wraps arbitrary functions for deferred execution during
 * World::Update(). The function must be invocable with a World& parameter.
 * @tparam F Function type that accepts World&
 */
template <typename F>
  requires std::invocable<F, World&>
class FunctionCmd final : public Command {
public:
  /**
   * @brief Constructs function command.
   * @tparam G Type of function to wrap
   * @param func Function to execute, must accept World& parameter
   */
  template <typename G>
    requires std::invocable<G, World&> && std::constructible_from<F, G&&>
  explicit constexpr FunctionCmd(G&& func) noexcept(std::is_nothrow_constructible_v<F, G&&>)
      : func_(std::forward<G>(func)) {}

  constexpr FunctionCmd(const FunctionCmd&) noexcept(std::is_nothrow_copy_constructible_v<F>) = default;
  constexpr FunctionCmd(FunctionCmd&&) noexcept(std::is_nothrow_move_constructible_v<F>) = default;
  constexpr ~FunctionCmd() noexcept(std::is_nothrow_destructible_v<F>) override = default;

  constexpr FunctionCmd& operator=(const FunctionCmd&) noexcept(std::is_nothrow_copy_assignable_v<F>) = default;
  constexpr FunctionCmd& operator=(FunctionCmd&&) noexcept(std::is_nothrow_move_assignable_v<F>) = default;

  /**
   * @brief Executes the wrapped function.
   * @param world World reference to pass to the function
   */
  void Execute(World& world) override { func_(world); }

private:
  F func_;  ///< Stored function to execute
};

/**
 * @brief Command to destroy a single entity.
 * @details Removes entity and all its components from the world during
 * execution. Will assert if the world does not own (entity index/generation
 * mismatch) the entity.
 */
class DestroyEntityCmd final : public Command {
public:
  /**
   * @brief Constructs destroy entity command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to destroy
   */
  explicit constexpr DestroyEntityCmd(Entity entity) noexcept;
  constexpr DestroyEntityCmd(const DestroyEntityCmd&) noexcept = default;
  constexpr DestroyEntityCmd(DestroyEntityCmd&&) noexcept = default;
  constexpr ~DestroyEntityCmd() noexcept override = default;

  constexpr DestroyEntityCmd& operator=(const DestroyEntityCmd&) noexcept = default;
  constexpr DestroyEntityCmd& operator=(DestroyEntityCmd&&) noexcept = default;

  /**
   * @brief Executes entity destruction.
   * @param world World to remove entity from
   */
  void Execute(World& world) override { world.DestroyEntity(entity_); }

private:
  Entity entity_;  ///< Entity to destroy
};

constexpr DestroyEntityCmd::DestroyEntityCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct destroy entity command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to destroy multiple entities.
 * @details Efficiently destroys multiple entities in a single operation.
 * Will assert if any of the entities do not exist in the world (when world
 * implementation updated).
 */
class DestroyEntitiesCmd final : public Command {
public:
  /**
   * @brief Constructs destroy entities command from range.
   * @warning Triggers assertion if any entity is invalid.
   * @tparam R Range type containing Entity elements
   * @param entities Range of entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  explicit constexpr DestroyEntitiesCmd(const R& entities);
  constexpr DestroyEntitiesCmd(const DestroyEntitiesCmd&) noexcept = default;
  constexpr DestroyEntitiesCmd(DestroyEntitiesCmd&&) noexcept = default;
  constexpr ~DestroyEntitiesCmd() noexcept override = default;

  constexpr DestroyEntitiesCmd& operator=(const DestroyEntitiesCmd&) noexcept = default;
  constexpr DestroyEntitiesCmd& operator=(DestroyEntitiesCmd&&) noexcept = default;

  /**
   * @brief Executes entities destruction.
   * @param world World to remove entities from
   */
  void Execute(World& world) override { world.DestroyEntities(entities_); }

private:
  std::vector<Entity> entities_;  ///< Entities to destroy
};

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
constexpr DestroyEntitiesCmd::DestroyEntitiesCmd(const R& entities)
    : entities_(std::ranges::begin(entities), std::ranges::end(entities)) {
  for (const Entity& entity : entities_) {
    HELIOS_ASSERT(entity.Valid(), "Failed to construct destroy entities command: Entity with index '{}' is invalid!",
                  entity.Index());
  }
}

/**
 * @brief Command to try destroy a single entity.
 * @details Will destroy the entity only if it exists in the world. Invalid
 * entity handle still asserts.
 */
class TryDestroyEntityCmd final : public Command {
public:
  /**
   * @brief Constructs try destroy entity command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to destroy
   */
  explicit constexpr TryDestroyEntityCmd(Entity entity) noexcept;
  constexpr TryDestroyEntityCmd(const TryDestroyEntityCmd&) noexcept = default;
  constexpr TryDestroyEntityCmd(TryDestroyEntityCmd&&) noexcept = default;
  constexpr ~TryDestroyEntityCmd() noexcept override = default;

  constexpr TryDestroyEntityCmd& operator=(const TryDestroyEntityCmd&) noexcept = default;
  constexpr TryDestroyEntityCmd& operator=(TryDestroyEntityCmd&&) noexcept = default;

  /**
   * @brief Executes entity destruction if it exists.
   * @param world World to remove entity from
   */
  void Execute(World& world) override { world.TryDestroyEntity(entity_); }

private:
  Entity entity_;
};

constexpr TryDestroyEntityCmd::TryDestroyEntityCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct try destroy entity command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to try destroy multiple entities.
 */
class TryDestroyEntitiesCmd final : public Command {
public:
  /**
   * @brief Constructs try destroy entities command from range.
   * @warning Triggers assertion if any entity is invalid.
   * @tparam R Range type containing Entity elements
   * @param entities Range of entities to destroy
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Entity>
  explicit constexpr TryDestroyEntitiesCmd(const R& entities);
  constexpr TryDestroyEntitiesCmd(const TryDestroyEntitiesCmd&) noexcept = default;
  constexpr TryDestroyEntitiesCmd(TryDestroyEntitiesCmd&&) noexcept = default;
  constexpr ~TryDestroyEntitiesCmd() noexcept override = default;

  constexpr TryDestroyEntitiesCmd& operator=(const TryDestroyEntitiesCmd&) noexcept = default;
  constexpr TryDestroyEntitiesCmd& operator=(TryDestroyEntitiesCmd&&) noexcept = default;

  /**
   * @brief Executes entities destruction if they exist.
   * @param world World to remove entities from
   */
  void Execute(World& world) override { world.TryDestroyEntities(entities_); }

private:
  std::vector<Entity> entities_;
};

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Entity>
constexpr TryDestroyEntitiesCmd::TryDestroyEntitiesCmd(const R& entities)
    : entities_(std::ranges::begin(entities), std::ranges::end(entities)) {
  for (const Entity& entity : entities_) {
    HELIOS_ASSERT(entity.Valid(),
                  "Failed to construct try destroy entities command: Entity with index '{}' is invalid!",
                  entity.Index());
  }
}

/**
 * @brief Command to add a component to an entity.
 * @details Adds component of type T to the specified entity during execution.
 * If entity already has the component, it will be replaced.
 * @tparam T Component type to add
 */
template <ComponentTrait T>
class AddComponentCmd final : public Command {
public:
  /**
   * @brief Constructs add component command with copy.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add component to
   * @param component Component to copy
   */
  constexpr AddComponentCmd(Entity entity, const T& component) noexcept(std::is_nothrow_copy_constructible_v<T>);

  /**
   * @brief Constructs add component command with move.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add component to
   * @param component Component to move
   */
  constexpr AddComponentCmd(Entity entity, T&& component) noexcept(std::is_nothrow_move_constructible_v<T>);
  constexpr AddComponentCmd(const AddComponentCmd&) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
  constexpr AddComponentCmd(AddComponentCmd&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
  constexpr ~AddComponentCmd() noexcept(std::is_nothrow_destructible_v<T>) override = default;

  constexpr AddComponentCmd& operator=(const AddComponentCmd&) noexcept(std::is_nothrow_copy_assignable_v<T>) = default;
  constexpr AddComponentCmd& operator=(AddComponentCmd&&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

  /**
   * @brief Executes component addition.
   * @param world World to add component in
   */
  void Execute(World& world) override { world.AddComponent(entity_, std::move(component_)); }

private:
  Entity entity_;  ///< Entity to add component to
  T component_;    ///< Component to add
};

template <ComponentTrait T>
constexpr AddComponentCmd<T>::AddComponentCmd(Entity entity,
                                              const T& component) noexcept(std::is_nothrow_copy_constructible_v<T>)
    : entity_(entity), component_(component) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct add component command: Entity with index '{}' is invalid!",
                entity.Index());
}

template <ComponentTrait T>
constexpr AddComponentCmd<T>::AddComponentCmd(Entity entity,
                                              T&& component) noexcept(std::is_nothrow_move_constructible_v<T>)
    : entity_(entity), component_(std::move(component)) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct add component command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to add multiple components to an entity.
 * @details Efficiently adds multiple components in a single operation.
 * @tparam Ts Component types to add (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
class AddComponentsCmd final : public Command {
public:
  /// @cond DOXYGEN_SKIP
  /**
   * @brief Constructs add components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add components to
   * @param components Components to add
   */
  explicit(sizeof...(Ts) == 0) constexpr AddComponentsCmd(Entity entity, Ts&&... components) noexcept
      : entity_(entity), components_(std::forward<Ts>(components)...) {
    HELIOS_ASSERT(entity.Valid(),
                  "Failed to construct add components command: Entity with "
                  "index '{}' is invalid!",
                  entity.Index());
  }
  /// @endcond

  constexpr AddComponentsCmd(const AddComponentsCmd&) noexcept = default;
  constexpr AddComponentsCmd(AddComponentsCmd&&) noexcept = default;
  constexpr ~AddComponentsCmd() noexcept override = default;

  constexpr AddComponentsCmd& operator=(const AddComponentsCmd&) noexcept = default;
  constexpr AddComponentsCmd& operator=(AddComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes components addition.
   * @param world World to add components in
   */
  void Execute(World& world) override;

private:
  Entity entity_;                 ///< Entity to add components to
  std::tuple<Ts...> components_;  ///< Components to add
};

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline void AddComponentsCmd<Ts...>::Execute(World& world) {
  std::apply([&world, this](auto&&... args) { world.AddComponents(entity_, std::forward<decltype(args)>(args)...); },
             std::move(components_));
}

/**
 * @brief Command to try add a component (only if missing).
 * @tparam T Component type
 */
template <ComponentTrait T>
class TryAddComponentCmd final : public Command {
public:
  /**
   * @brief Constructs try add component command with copy.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add component to
   * @param component Component to copy
   */
  constexpr TryAddComponentCmd(Entity entity, const T& component) noexcept(std::is_nothrow_copy_constructible_v<T>);

  /**
   * @brief Constructs try add component command with move.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add component to
   * @param component Component to move
   */
  constexpr TryAddComponentCmd(Entity entity, T&& component) noexcept(std::is_nothrow_move_constructible_v<T>);
  constexpr TryAddComponentCmd(const TryAddComponentCmd&) noexcept = default;
  constexpr TryAddComponentCmd(TryAddComponentCmd&&) noexcept = default;
  constexpr ~TryAddComponentCmd() noexcept override = default;

  constexpr TryAddComponentCmd& operator=(const TryAddComponentCmd&) noexcept = default;
  constexpr TryAddComponentCmd& operator=(TryAddComponentCmd&&) noexcept = default;

  /**
   * @brief Executes component addition if entity is missing it.
   * @param world World to add component in
   */
  void Execute(World& world) override { world.TryAddComponent(entity_, std::move(component_)); }

private:
  Entity entity_;
  T component_;
};

template <ComponentTrait T>
constexpr TryAddComponentCmd<T>::TryAddComponentCmd(Entity entity, const T& component) noexcept(
    std::is_nothrow_copy_constructible_v<T>)
    : entity_(entity), component_(component) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct try add component command: Entity with index '{}' is invalid!",
                entity.Index());
}

template <ComponentTrait T>
constexpr TryAddComponentCmd<T>::TryAddComponentCmd(Entity entity,
                                                    T&& component) noexcept(std::is_nothrow_move_constructible_v<T>)
    : entity_(entity), component_(std::move(component)) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct try add component command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to try add multiple components (only missing ones).
 * @tparam Ts Component types (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
class TryAddComponentsCmd final : public Command {
public:
  /// @cond DOXYGEN_SKIP
  /**
   * @brief Constructs try add components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add components to
   * @param components Components to add
   */
  explicit(sizeof...(Ts) == 0) constexpr TryAddComponentsCmd(Entity entity, Ts&&... components) noexcept;
  /// @endcond

  constexpr TryAddComponentsCmd(const TryAddComponentsCmd&) noexcept = default;
  constexpr TryAddComponentsCmd(TryAddComponentsCmd&&) noexcept = default;
  constexpr ~TryAddComponentsCmd() noexcept override = default;

  constexpr TryAddComponentsCmd& operator=(const TryAddComponentsCmd&) noexcept = default;
  constexpr TryAddComponentsCmd& operator=(TryAddComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes components addition if entity is missing them.
   * @param world World to add components in
   */
  void Execute(World& world) override;

private:
  Entity entity_;
  std::tuple<Ts...> components_;
};

/// @cond DOXYGEN_SKIP
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
constexpr TryAddComponentsCmd<Ts...>::TryAddComponentsCmd(Entity entity, Ts&&... components) noexcept
    : entity_(entity), components_(std::forward<Ts>(components)...) {
  HELIOS_ASSERT(entity.Valid(),
                "Failed to construct try add components command: Entity with "
                "index '{}' is invalid!",
                entity.Index());
}
/// @endcond

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline void TryAddComponentsCmd<Ts...>::Execute(World& world) {
  std::apply([&world, this](auto&&... args) { world.TryAddComponents(entity_, std::forward<decltype(args)>(args)...); },
             std::move(components_));
}

/**
 * @brief Command to remove a component from an entity.
 * @details Removes component of type T from the specified entity during execution.
 * @tparam T Component type to remove
 */
template <ComponentTrait T>
class RemoveComponentCmd final : public Command {
public:
  /**
   * @brief Constructs remove component command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove component from
   */
  explicit constexpr RemoveComponentCmd(Entity entity) noexcept;
  constexpr RemoveComponentCmd(const RemoveComponentCmd&) noexcept = default;
  constexpr RemoveComponentCmd(RemoveComponentCmd&&) noexcept = default;
  constexpr ~RemoveComponentCmd() noexcept override = default;

  constexpr RemoveComponentCmd& operator=(const RemoveComponentCmd&) noexcept = default;
  constexpr RemoveComponentCmd& operator=(RemoveComponentCmd&&) noexcept = default;

  /**
   * @brief Executes component removal.
   * @param world World to remove component from
   */
  void Execute(World& world) override { world.RemoveComponent<T>(entity_); }

private:
  Entity entity_;  ///< Entity to remove component from
};

template <ComponentTrait T>
constexpr RemoveComponentCmd<T>::RemoveComponentCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct remove component command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to remove multiple components from an entity.
 * @details Efficiently removes multiple components in a single operation.
 * @tparam Ts Component types to remove (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
class RemoveComponentsCmd final : public Command {
public:
  /**
   * @brief Constructs remove components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove components from
   */
  explicit constexpr RemoveComponentsCmd(Entity entity) noexcept;
  constexpr RemoveComponentsCmd(const RemoveComponentsCmd&) noexcept = default;
  constexpr RemoveComponentsCmd(RemoveComponentsCmd&&) noexcept = default;
  constexpr ~RemoveComponentsCmd() noexcept override = default;

  constexpr RemoveComponentsCmd& operator=(const RemoveComponentsCmd&) noexcept = default;
  constexpr RemoveComponentsCmd& operator=(RemoveComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes components removal.
   * @param world World to remove components from
   */
  void Execute(World& world) override { world.RemoveComponents<Ts...>(entity_); }

private:
  Entity entity_;  ///< Entity to remove components from
};

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
constexpr RemoveComponentsCmd<Ts...>::RemoveComponentsCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct remove components command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to try remove a single component (only if present).
 * @tparam T Component type
 */
template <ComponentTrait T>
class TryRemoveComponentCmd final : public Command {
public:
  /**
   * @brief Constructs try remove component command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove component from
   */
  explicit constexpr TryRemoveComponentCmd(Entity entity) noexcept;
  constexpr TryRemoveComponentCmd(const TryRemoveComponentCmd&) noexcept = default;
  constexpr TryRemoveComponentCmd(TryRemoveComponentCmd&&) noexcept = default;
  constexpr ~TryRemoveComponentCmd() noexcept override = default;

  constexpr TryRemoveComponentCmd& operator=(const TryRemoveComponentCmd&) noexcept = default;
  constexpr TryRemoveComponentCmd& operator=(TryRemoveComponentCmd&&) noexcept = default;

  /**
   * @brief Executes component removal if entity has it.
   * @param world World to remove component from
   */
  void Execute(World& world) override { world.TryRemoveComponent<T>(entity_); }

private:
  Entity entity_;  ///< Entity to remove component from
};

template <ComponentTrait T>
constexpr TryRemoveComponentCmd<T>::TryRemoveComponentCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct try remove component command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to try remove multiple components (only those present).
 * @tparam Ts Component types (must be unique)
 */
template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
class TryRemoveComponentsCmd final : public Command {
public:
  /**
   * @brief Constructs try remove components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove components from
   */
  explicit constexpr TryRemoveComponentsCmd(Entity entity) noexcept;
  constexpr TryRemoveComponentsCmd(const TryRemoveComponentsCmd&) noexcept = default;
  constexpr TryRemoveComponentsCmd(TryRemoveComponentsCmd&&) noexcept = default;
  constexpr ~TryRemoveComponentsCmd() noexcept override = default;

  constexpr TryRemoveComponentsCmd& operator=(const TryRemoveComponentsCmd&) noexcept = default;
  constexpr TryRemoveComponentsCmd& operator=(TryRemoveComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes components removal if entity has them.
   * @param world World to remove components from
   */
  void Execute(World& world) override { world.TryRemoveComponents<Ts...>(entity_); }

private:
  Entity entity_;  ///< Entity to remove components from
};

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
constexpr TryRemoveComponentsCmd<Ts...>::TryRemoveComponentsCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct try remove components command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to clear all components from an entity.
 * @details Removes all components from the specified entity during execution.
 */
class ClearComponentsCmd final : public Command {
public:
  /**
   * @brief Constructs clear components command.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to clear components from
   */
  explicit constexpr ClearComponentsCmd(Entity entity) noexcept;
  constexpr ClearComponentsCmd(const ClearComponentsCmd&) noexcept = default;
  constexpr ClearComponentsCmd(ClearComponentsCmd&&) noexcept = default;
  constexpr ~ClearComponentsCmd() noexcept override = default;

  constexpr ClearComponentsCmd& operator=(const ClearComponentsCmd&) noexcept = default;
  constexpr ClearComponentsCmd& operator=(ClearComponentsCmd&&) noexcept = default;

  /**
   * @brief Executes component clearing.
   * @param world World to clear components in
   */
  void Execute(World& world) override { world.ClearComponents(entity_); }

private:
  Entity entity_;  ///< Entity to clear components from
};

constexpr ClearComponentsCmd::ClearComponentsCmd(Entity entity) noexcept : entity_(entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to construct clear components command: Entity with index '{}' is invalid!",
                entity.Index());
}

/**
 * @brief Command to insert a resource into the world.
 * @details Adds resource of type T to the world during execution.
 * If world already has the resource, it will be replaced.
 * @tparam T Resource type to insert
 */
template <ResourceTrait T>
class InsertResourceCmd final : public Command {
public:
  /**
   * @brief Constructs insert resource command with copy.
   * @param resource Resource to copy
   */
  explicit constexpr InsertResourceCmd(const T& resource) noexcept(std::is_nothrow_copy_constructible_v<T>)
      : resource_(resource) {}

  /**
   * @brief Constructs insert resource command with move.
   * @param resource Resource to move
   */
  explicit constexpr InsertResourceCmd(T&& resource) noexcept(std::is_nothrow_move_constructible_v<T>)
      : resource_(std::move(resource)) {}

  constexpr InsertResourceCmd(const InsertResourceCmd&) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
  constexpr InsertResourceCmd(InsertResourceCmd&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
  constexpr ~InsertResourceCmd() noexcept(std::is_nothrow_destructible_v<T>) override = default;

  constexpr InsertResourceCmd& operator=(const InsertResourceCmd&) noexcept(std::is_nothrow_copy_assignable_v<T>) =
      default;

  constexpr InsertResourceCmd& operator=(InsertResourceCmd&&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

  /**
   * @brief Executes resource insertion.
   * @param world World to insert resource into
   */
  void Execute(World& world) override { world.InsertResource(std::move(resource_)); }

private:
  T resource_;  ///< Resource to insert
};

/**
 * @brief Command to try insert a resource (only if missing).
 * @tparam T Resource type
 */
template <ResourceTrait T>
class TryInsertResourceCmd final : public Command {
public:
  /**
   * @brief Constructs try insert resource command with copy.
   * @param resource Resource to copy
   */
  explicit constexpr TryInsertResourceCmd(const T& resource) noexcept(std::is_nothrow_copy_constructible_v<T>)
      : resource_(resource) {}

  /**
   * @brief Constructs try insert resource command with move.
   * @param resource Resource to move
   */
  explicit constexpr TryInsertResourceCmd(T&& resource) noexcept(std::is_nothrow_move_constructible_v<T>)
      : resource_(std::move(resource)) {}

  constexpr TryInsertResourceCmd(const TryInsertResourceCmd&) noexcept = default;
  constexpr TryInsertResourceCmd(TryInsertResourceCmd&&) noexcept = default;
  constexpr ~TryInsertResourceCmd() noexcept override = default;

  constexpr TryInsertResourceCmd& operator=(const TryInsertResourceCmd&) noexcept = default;
  constexpr TryInsertResourceCmd& operator=(TryInsertResourceCmd&&) noexcept = default;

  /**
   * @brief Executes resource insertion if it doesn't exist.
   * @param world World to insert resource into
   */
  void Execute(World& world) override { world.TryInsertResource(std::move(resource_)); }

private:
  T resource_;  ///< Resource to insert
};

/**
 * @brief Command to remove a resource from the world.
 * @details Removes resource of type T from the world during execution.
 * @tparam T Resource type to remove
 */
template <ResourceTrait T>
class RemoveResourceCmd final : public Command {
public:
  constexpr RemoveResourceCmd() noexcept = default;
  constexpr RemoveResourceCmd(const RemoveResourceCmd&) noexcept = default;
  constexpr RemoveResourceCmd(RemoveResourceCmd&&) noexcept = default;
  constexpr ~RemoveResourceCmd() noexcept override = default;

  constexpr RemoveResourceCmd& operator=(const RemoveResourceCmd&) noexcept = default;
  constexpr RemoveResourceCmd& operator=(RemoveResourceCmd&&) noexcept = default;

  /**
   * @brief Executes resource removal.
   * @param world World to remove resource from
   */
  void Execute(World& world) override { world.RemoveResource<T>(); }
};

/**
 * @brief Command to try remove a resource (only if present).
 * @tparam T Resource type
 */
template <ResourceTrait T>
class TryRemoveResourceCmd final : public Command {
public:
  constexpr TryRemoveResourceCmd() noexcept = default;
  constexpr TryRemoveResourceCmd(const TryRemoveResourceCmd&) noexcept = default;
  constexpr TryRemoveResourceCmd(TryRemoveResourceCmd&&) noexcept = default;
  constexpr ~TryRemoveResourceCmd() noexcept override = default;

  constexpr TryRemoveResourceCmd& operator=(const TryRemoveResourceCmd&) noexcept = default;
  constexpr TryRemoveResourceCmd& operator=(TryRemoveResourceCmd&&) noexcept = default;

  /**
   * @brief Executes resource removal if it exists.
   * @param world World to remove resource from
   */
  void Execute(World& world) override { world.TryRemoveResource<T>(); }
};

/**
 * @brief Command to clear all events of a specific type from the queue.
 * @tparam T Event type to clear
 */
template <EventTrait T>
class ClearEventsCmd final : public Command {
public:
  constexpr ClearEventsCmd() noexcept = default;
  constexpr ClearEventsCmd(const ClearEventsCmd&) noexcept = default;
  constexpr ClearEventsCmd(ClearEventsCmd&&) noexcept = default;
  constexpr ~ClearEventsCmd() noexcept override = default;

  constexpr ClearEventsCmd& operator=(const ClearEventsCmd&) noexcept = default;
  constexpr ClearEventsCmd& operator=(ClearEventsCmd&&) noexcept = default;

  /**
   * @brief Executes the command to clear events of type T.
   * @param world Reference to the world
   */
  void Execute(World& world) override { world.ClearEvents<T>(); }
};

/**
 * @brief Command to clear all event queues without removing registration.
 */
class ClearAllEventsCmd final : public Command {
public:
  constexpr ClearAllEventsCmd() noexcept = default;
  constexpr ClearAllEventsCmd(const ClearAllEventsCmd&) noexcept = default;
  constexpr ClearAllEventsCmd(ClearAllEventsCmd&&) noexcept = default;
  constexpr ~ClearAllEventsCmd() noexcept override = default;

  constexpr ClearAllEventsCmd& operator=(const ClearAllEventsCmd&) noexcept = default;
  constexpr ClearAllEventsCmd& operator=(ClearAllEventsCmd&&) noexcept = default;

  /**
   * @brief Executes the command to clear all event queues.
   * @param world Reference to the world
   */
  void Execute(World& world) override { world.ClearAllEventQueues(); }
};

}  // namespace helios::ecs::details
