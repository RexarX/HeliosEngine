#pragma once

#include <helios/assert.hpp>
#include <helios/container/sparse_set.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

namespace helios::ecs {

/**
 * @brief Concrete sparse-set storage for a single component type
 * (non-polymorphic).
 * @details Wraps a `SparseSet<T, Entity::IndexType>` and provides
 * entity-oriented API. Not designed to be a base class.
 * @tparam T Component type
 */
template <ComponentTrait T>
class SparseComponentStorage {
public:
  constexpr SparseComponentStorage() = default;
  constexpr SparseComponentStorage(const SparseComponentStorage&) = default;
  constexpr SparseComponentStorage(SparseComponentStorage&&) noexcept = default;
  constexpr ~SparseComponentStorage() = default;

  constexpr SparseComponentStorage& operator=(const SparseComponentStorage&) =
      default;
  constexpr SparseComponentStorage& operator=(
      SparseComponentStorage&&) noexcept = default;

  /// @brief Removes all component instances.
  constexpr void Clear() noexcept { storage_.Clear(); }

  /**
   * @brief Removes the component for the given entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity does not have the component.
   * @param entity Entity
   */
  constexpr void Remove(Entity entity);

  /**
   * @brief Tries to remove the component for the given entity.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity
   * @return True if removed, false if entity did not have the component
   */
  constexpr bool TryRemove(Entity entity);

  /**
   * @brief Inserts or replaces a component for the given entity.
   * @warning Triggers assertion if entity is invalid.
   * @tparam U Component type, must be the same as `T`
   * @param entity Entity
   * @param component Component value
   */
  template <ComponentTrait U = T>
    requires std::same_as<std::remove_cvref_t<U>, T>
  constexpr void Set(Entity entity, U&& component);

  /**
   * @brief Tries to insert a component. Returns false if entity already has it.
   * @warning Triggers assertion if entity is invalid.
   * tparam U Component type, must be the same as `T`
   * @param entity Entity
   * @param component Component value
   * @return True if inserted
   */
  template <ComponentTrait U = T>
    requires std::same_as<std::remove_cvref_t<U>, T>
  constexpr bool TrySet(Entity entity, U&& component);

  /**
   * @brief Constructs a component in-place for the given entity.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr void Emplace(Entity entity, Args&&... args);

  /**
   * @brief Tries to emplace a component. Returns false if entity already has
   * it.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments
   * @return True if emplaced
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr bool TryEmplace(Entity entity, Args&&... args);

  /**
   * @brief Gets a mutable reference to the component for the entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity does not have the component.
   * @param entity Entity
   * @return Mutable reference to component
   */
  [[nodiscard]] constexpr T& Get(Entity entity) noexcept;

  /**
   * @brief Gets a const reference to the component for the entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid
   * - Entity does not have the component
   * @param entity Entity
   * @return Const reference to component
   */
  [[nodiscard]] constexpr const T& Get(Entity entity) const noexcept;

  /**
   * @brief Tries to get a mutable pointer to the component for the entity.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity
   * @return Pointer to component or `nullptr` if entity does not have the
   * component
   */
  [[nodiscard]] constexpr T* TryGet(Entity entity) noexcept;

  /**
   * @brief Tries to get a const pointer to the component for the entity.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity
   * @return Const pointer to component or `nullptr` if entity does not have the
   * component
   */
  [[nodiscard]] constexpr const T* TryGet(Entity entity) const noexcept;

  /**
   * @brief Checks if the entity has the component.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity
   * @return True if entity has the component, false otherwise
   */
  [[nodiscard]] constexpr bool Contains(Entity entity) const noexcept;

  /**
   * @brief Gets the number of stored components.
   * @return Number of components
   */
  [[nodiscard]] constexpr size_t Size() const noexcept {
    return storage_.Size();
  }

  /**
   * @brief Gets a span over all dense component data.
   * @return Span of components
   */
  [[nodiscard]] constexpr auto Data() noexcept -> std::span<T> {
    return storage_.Data();
  }

  /**
   * @brief Gets a span over all dense component data (const).
   * @return Span of const components
   */
  [[nodiscard]] constexpr auto Data() const noexcept -> std::span<const T> {
    return storage_.Data();
  }

private:
  using SparseSetType = container::SparseSet<T, Entity::IndexType>;

  SparseSetType storage_;
};

template <ComponentTrait T>
constexpr void SparseComponentStorage<T>::Remove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(storage_.Contains(entity.Index()),
                "Entity '{}' does not have sparse component '{}'!", entity,
                ComponentNameOf<T>());
  storage_.Remove(entity.Index());
}

template <ComponentTrait T>
constexpr bool SparseComponentStorage<T>::TryRemove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (!storage_.Contains(entity.Index())) {
    return false;
  }
  storage_.Remove(entity.Index());
  return true;
}

template <ComponentTrait T>
template <ComponentTrait U>
  requires std::same_as<std::remove_cvref_t<U>, T>
constexpr void SparseComponentStorage<T>::Set(Entity entity, U&& component) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (storage_.Contains(entity.Index())) {
    storage_.Get(entity.Index()) = std::forward<U>(component);
  } else {
    storage_.Insert(entity.Index(), std::forward<U>(component));
  }
}

template <ComponentTrait T>
template <ComponentTrait U>
  requires std::same_as<std::remove_cvref_t<U>, T>
constexpr bool SparseComponentStorage<T>::TrySet(Entity entity, U&& component) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (storage_.Contains(entity.Index())) {
    return false;
  }
  storage_.Insert(entity.Index(), std::forward<U>(component));
  return true;
}

template <ComponentTrait T>
template <typename... Args>
  requires std::constructible_from<T, Args...>
constexpr void SparseComponentStorage<T>::Emplace(Entity entity,
                                                  Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (storage_.Contains(entity.Index())) {
    auto& slot = storage_.Get(entity.Index());
    std::destroy_at(&slot);
    std::construct_at(&slot, std::forward<Args>(args)...);
  } else {
    storage_.Emplace(entity.Index(), std::forward<Args>(args)...);
  }
}

template <ComponentTrait T>
template <typename... Args>
  requires std::constructible_from<T, Args...>
constexpr bool SparseComponentStorage<T>::TryEmplace(Entity entity,
                                                     Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  if (storage_.Contains(entity.Index())) {
    return false;
  }
  storage_.Emplace(entity.Index(), std::forward<Args>(args)...);
  return true;
}

template <ComponentTrait T>
constexpr T& SparseComponentStorage<T>::Get(Entity entity) noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(storage_.Contains(entity.Index()),
                "Entity '{}' does not have sparse component '{}'!", entity,
                ComponentNameOf<T>());
  return storage_.Get(entity.Index());
}

template <ComponentTrait T>
constexpr const T& SparseComponentStorage<T>::Get(
    Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(storage_.Contains(entity.Index()),
                "Entity '{}' does not have sparse component '{}'!", entity,
                ComponentNameOf<T>());
  return storage_.Get(entity.Index());
}

template <ComponentTrait T>
constexpr T* SparseComponentStorage<T>::TryGet(Entity entity) noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return storage_.TryGet(entity.Index());
}

template <ComponentTrait T>
constexpr const T* SparseComponentStorage<T>::TryGet(
    Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return storage_.TryGet(entity.Index());
}

template <ComponentTrait T>
constexpr bool SparseComponentStorage<T>::Contains(
    Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return storage_.Contains(entity.Index());
}

}  // namespace helios::ecs
