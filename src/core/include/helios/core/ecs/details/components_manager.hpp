#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/container/sparse_set.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/logger.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Base class for type-erased component storage.
 * @details Provides a common interface for storing different component types
 * in a heterogeneous container. Each derived ComponentStorage<T> manages
 * components of a specific type T using a sparse set for efficient operations.
 *
 * @note Not thread-safe.
 * All operations should be performed from the main thread.
 */
class ComponentStorageBase {
public:
  virtual ~ComponentStorageBase() = default;

  /**
   * @brief Clears all components from storage.
   * @details Removes all component instances and resets internal state.
   */
  virtual void Clear() noexcept = 0;

  /**
   * @brief Removes component for the specified entity.
   * @details Removes the component associated with the entity.
   * @warning Triggers assertion if entity doesn't have the component.
   * @param entity Entity to remove component from
   */
  virtual void Remove(Entity entity) = 0;

  /**
   * @brief Attempts to remove component for the specified entity.
   * @details Removes the component if it exists, otherwise does nothing.
   * @param entity Entity to remove component from
   * @return True if component was removed, false if entity didn't have the component
   */
  virtual bool TryRemove(Entity entity) = 0;

  /**
   * @brief Checks if entity has a component in this storage.
   * @details Performs fast lookup to determine component presence.
   * @param entity Entity to check
   * @return True if entity has a component in this storage, false otherwise
   */
  [[nodiscard]] virtual bool Contains(Entity entity) const = 0;

  /**
   * @brief Gets the number of components in storage.
   * @details Returns count of component instances currently stored.
   * @return Number of components in storage
   */
  [[nodiscard]] virtual size_t Size() const noexcept = 0;

  /**
   * @brief Gets type information for the component type stored.
   * @details Returns compile-time component metadata including size, alignment, etc.
   * @return ComponentTypeInfo for the stored component type
   */
  [[nodiscard]] virtual ComponentTypeInfo GetTypeInfo() const noexcept = 0;
};

/**
 * @brief Type-specific component storage using sparse set.
 * @details Manages components of type T using a sparse set data structure for
 * O(1) insertion, removal, and lookup operations. Components are stored in
 * dense arrays for cache-friendly iteration.
 *
 * Memory layout:
 * - Sparse array: Entity index -> Dense index mapping
 * - Dense array: Packed component instances
 * - Reverse mapping: Dense index -> Entity index
 *
 * @tparam T Component type to store
 * @note Not thread-safe.
 * All operations should be performed from the main thread.
 */
template <ComponentTrait T>
class ComponentStorage final : public ComponentStorageBase {
public:
  using SparseSetType = container::SparseSet<T, Entity::IndexType>;
  using Iterator = typename SparseSetType::iterator;
  using ConstIterator = typename SparseSetType::const_iterator;

  ComponentStorage() = default;
  ComponentStorage(const ComponentStorage&) = default;
  ComponentStorage(ComponentStorage&&) noexcept = default;
  ~ComponentStorage() override = default;

  ComponentStorage& operator=(const ComponentStorage&) = default;
  ComponentStorage& operator=(ComponentStorage&&) noexcept = default;

  /**
   * @brief Clears all components from this storage.
   * @details Removes all component instances and resets sparse set state.
   * Time complexity: O(sparse_capacity).
   */
  void Clear() noexcept override { storage_.Clear(); }

  /**
   * @brief Constructs component in-place for the specified entity.
   * @details Creates a new component by forwarding arguments to T's constructor.
   * If entity already has this component, it will be replaced.
   * Time complexity: O(1) amortized.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Args Argument types to forward to T's constructor
   * @param entity Entity to add component to
   * @param args Arguments to forward to component constructor
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Entity entity, Args&&... args);

  /**
   * @brief Inserts component for the specified entity (copy version).
   * @details Adds a copy of the component to the entity.
   * If entity already has this component, it will be replaced.
   * Time complexity: O(1) amortized.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add component to
   * @param component Component to copy
   */
  void Insert(Entity entity, const T& component);

  /**
   * @brief Inserts component for the specified entity (move version).
   * @details Moves the component to the entity.
   * If entity already has this component, it will be replaced.
   * Time complexity: O(1) amortized.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to add component to
   * @param component Component to move
   */
  void Insert(Entity entity, T&& component);

  /**
   * @brief Removes component from the specified entity.
   * @details Removes the component using swap-and-pop for dense packing.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid or doesn't have the component.
   * @param entity Entity to remove component from
   */
  void Remove(Entity entity) override;

  /**
   * @brief Attempts to remove component from the specified entity.
   * @details Removes the component if it exists, otherwise does nothing.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove component from
   * @return True if component was removed, false if entity didn't have the component
   */
  bool TryRemove(Entity entity) override;

  /**
   * @brief Gets mutable reference to component for the specified entity.
   * @details Returns direct reference to the stored component.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid or doesn't have the component.
   * @param entity Entity to get component from
   * @return Mutable reference to the component
   */
  [[nodiscard]] T& Get(Entity entity);

  /**
   * @brief Gets const reference to component for the specified entity.
   * @details Returns direct const reference to the stored component.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid or doesn't have the component.
   * @param entity Entity to get component from
   * @return Const reference to the component
   */
  [[nodiscard]] const T& Get(Entity entity) const;

  /**
   * @brief Attempts to get mutable pointer to component for the specified entity.
   * @details Returns pointer to component if it exists, nullptr otherwise.
   * Time complexity: O(1).
   * @param entity Entity to get component from
   * @return Pointer to component, or nullptr if entity doesn't have the component
   */
  [[nodiscard]] T* TryGet(Entity entity);

  /**
   * @brief Attempts to get const pointer to component for the specified entity.
   * @details Returns const pointer to component if it exists, nullptr otherwise.
   * Time complexity: O(1).
   * @param entity Entity to get component from
   * @return Const pointer to component, or nullptr if entity doesn't have the component
   */
  [[nodiscard]] const T* TryGet(Entity entity) const;

  /**
   * @brief Checks if entity has a component in this storage.
   * @details Performs comprehensive validation including entity validity and component presence.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to check
   * @return True if entity has this component, false otherwise
   */
  [[nodiscard]] bool Contains(Entity entity) const override;

  /**
   * @brief Gets the number of components stored.
   * @details Returns count of component instances currently in storage.
   * Time complexity: O(1).
   * @return Number of components in storage
   */
  [[nodiscard]] size_t Size() const noexcept override { return storage_.Size(); }

  /**
   * @brief Gets compile-time type information for component T.
   * @details Returns metadata including type ID, size, alignment, and optimization flags.
   * @return ComponentTypeInfo for type T
   */
  [[nodiscard]] constexpr ComponentTypeInfo GetTypeInfo() const noexcept override {
    return ComponentTypeInfo::Create<T>();
  }

  /**
   * @brief Gets mutable span over all stored components.
   * @details Provides direct access to densely packed component array for efficient iteration.
   * Components are stored in insertion order (with removal gaps filled by later insertions).
   * Time complexity: O(1).
   * @return Span over all components in storage
   */
  [[nodiscard]] std::span<T> Data() noexcept { return storage_.Data(); }

  /**
   * @brief Gets const span over all stored components.
   * @details Provides direct read-only access to densely packed component array.
   * Components are stored in insertion order (with removal gaps filled by later insertions).
   * Time complexity: O(1).
   * @return Const span over all components in storage
   */
  [[nodiscard]] std::span<const T> Data() const noexcept { return storage_.Data(); }

  [[nodiscard]] Iterator begin() noexcept { return storage_.begin(); }
  [[nodiscard]] ConstIterator begin() const noexcept { return storage_.begin(); }
  [[nodiscard]] ConstIterator cbegin() const noexcept { return storage_.cbegin(); }

  [[nodiscard]] Iterator end() noexcept { return storage_.end(); }
  [[nodiscard]] ConstIterator end() const noexcept { return storage_.end(); }
  [[nodiscard]] ConstIterator cend() const noexcept { return storage_.cend(); }

private:
  SparseSetType storage_;  ///< Underlying sparse set storing components
};

template <ComponentTrait T>
template <typename... Args>
  requires std::constructible_from<T, Args...>
inline void ComponentStorage<T>::Emplace(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Failed to emplace component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  storage_.Emplace(entity.Index(), std::forward<Args>(args)...);
}

template <ComponentTrait T>
inline void ComponentStorage<T>::Insert(Entity entity, const T& component) {
  HELIOS_ASSERT(entity.Valid(), "Failed to insert component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  storage_.Insert(entity.Index(), component);
}

template <ComponentTrait T>
inline void ComponentStorage<T>::Insert(Entity entity, T&& component) {
  HELIOS_ASSERT(entity.Valid(), "Failed to insert component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  storage_.Insert(entity.Index(), std::move(component));
}

template <ComponentTrait T>
inline void ComponentStorage<T>::Remove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  HELIOS_ASSERT(storage_.Contains(entity.Index()),
                "Failed to remove component '{}': Entity with index '{}' does not have this component!",
                ComponentNameOf<T>(), entity.Index());
  storage_.Remove(entity.Index());
}

template <ComponentTrait T>
inline bool ComponentStorage<T>::TryRemove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try remove component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  if (!storage_.Contains(entity.Index())) {
    return false;
  }

  storage_.Remove(entity.Index());
  return true;
}

template <ComponentTrait T>
inline bool ComponentStorage<T>::Contains(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(),
                "Failed to check if '{}' component storage contains entity: Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  return storage_.Contains(entity.Index());
}

template <ComponentTrait T>
inline T& ComponentStorage<T>::Get(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to get component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  return storage_.Get(entity.Index());
}

template <ComponentTrait T>
inline const T& ComponentStorage<T>::Get(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to get component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  return storage_.Get(entity.Index());
}

template <ComponentTrait T>
inline T* ComponentStorage<T>::TryGet(Entity entity) {
  if (!entity.Valid()) {
    return nullptr;
  }
  return storage_.TryGet(entity.Index());
}

template <ComponentTrait T>
inline const T* ComponentStorage<T>::TryGet(Entity entity) const {
  if (!entity.Valid()) {
    return nullptr;
  }
  return storage_.TryGet(entity.Index());
}

/**
 * @brief Manager for all component storages in the ECS world.
 * @details Maintains a registry of type-erased component storages, providing
 * a unified interface for component operations across all types. Each component
 * type gets its own ComponentStorage<T> instance for type-safe operations.
 *
 * Key features:
 * - Type-erased storage management for heterogeneous component types
 * - Lazy storage creation - storages are created only when first component is added
 * - Efficient component lookup and manipulation
 * - Bulk operations for entity lifecycle management
 *
 * @note Not thread-safe. All operations should be performed from the main thread.
 */
class Components {
public:
  Components() = default;
  Components(const Components&) = delete;
  Components(Components&&) = default;
  ~Components() = default;

  Components& operator=(const Components&) = delete;
  Components& operator=(Components&&) = default;

  /**
   * @brief Clears all component storages.
   * @details Removes all components of all types and resets the manager state.
   * Time complexity: O(1).
   */
  void Clear() noexcept { storages_.clear(); }

  /**
   * @brief Removes all components from the specified entity.
   * @details Iterates through all component storages and removes entity's components.
   * Used when destroying entities or clearing their component sets.
   * Time complexity: O(S) where S is the number of component types.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove all components from
   */
  void RemoveAllComponents(Entity entity);

  /**
   * @brief Adds component to entity (move version).
   * @details Moves the component into storage. Creates storage if needed.
   * If entity already has this component type, it will be replaced.
   * Time complexity: O(1) amortized.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type to add
   * @param entity Entity to add component to
   * @param component Component to move
   */
  template <ComponentTrait T>
  void AddComponent(Entity entity, T&& component);

  /**
   * @brief Constructs component in-place for entity.
   * @details Creates component by forwarding arguments to T's constructor.
   * Creates storage if needed. If entity already has this component type, it will be replaced.
   * Time complexity: O(1) amortized.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type to construct
   * @tparam Args Argument types for T's constructor
   * @param entity Entity to add component to
   * @param args Arguments to forward to component constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceComponent(Entity entity, Args&&... args);

  /**
   * @brief Removes component from entity.
   * @details Removes the specified component type from the entity.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid or doesn't have the component.
   * @tparam T Component type to remove
   * @param entity Entity to remove component from
   */
  template <ComponentTrait T>
  void RemoveComponent(Entity entity);

  /**
   * @brief Gets mutable reference to entity's component.
   * @details Returns direct reference to the component for modification.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid or doesn't have the component.
   * @tparam T Component type to get
   * @param entity Entity to get component from
   * @return Mutable reference to the component
   */
  template <ComponentTrait T>
  [[nodiscard]] T& GetComponent(Entity entity) {
    return GetStorage<T>().Get(entity);
  }

  /**
   * @brief Gets const reference to entity's component.
   * @details Returns direct const reference to the component for read-only access.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid or doesn't have the component.
   * @tparam T Component type to get
   * @param entity Entity to get component from
   * @return Const reference to the component
   */
  template <ComponentTrait T>
  [[nodiscard]] const T& GetComponent(Entity entity) const {
    return GetStorage<T>().Get(entity);
  }

  /**
   * @brief Attempts to get mutable pointer to entity's component.
   * @details Returns pointer to component if it exists, nullptr otherwise.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type to get
   * @param entity Entity to get component from
   * @return Pointer to component, or nullptr if entity doesn't have the component
   */
  template <ComponentTrait T>
  [[nodiscard]] T* TryGetComponent(Entity entity);

  /**
   * @brief Attempts to get const pointer to entity's component.
   * @details Returns const pointer to component if it exists, nullptr otherwise.
   * Time complexity: O(1).
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type to get
   * @param entity Entity to get component from
   * @return Const pointer to component, or nullptr if entity doesn't have the component
   */
  template <ComponentTrait T>
  [[nodiscard]] const T* TryGetComponent(Entity entity) const;

  /**
   * @brief Checks if entity has the specified component type.
   * @details Performs fast lookup to determine if entity has a component of type T.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type to check
   * @param entity Entity to check
   * @return True if entity has component of type T, false otherwise
   */
  template <ComponentTrait T>
  [[nodiscard]] bool HasComponent(Entity entity) const;

  /**
   * @brief Gets typed storage for component type T.
   * @details Returns reference to the ComponentStorage<T> for direct access.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if storage doesn't exist for type T.
   * @tparam T Component type
   * @return Reference to ComponentStorage<T>
   */
  template <ComponentTrait T>
  [[nodiscard]] ComponentStorage<T>& GetStorage();

  /**
   * @brief Gets const typed storage for component type T.
   * @details Returns const reference to the ComponentStorage<T> for read-only access.
   * Time complexity: O(1) average case.
   * @warning Triggers assertion if storage doesn't exist for type T.
   * @tparam T Component type
   * @return Const reference to ComponentStorage<T>
   */
  template <ComponentTrait T>
  [[nodiscard]] const ComponentStorage<T>& GetStorage() const;

  /**
   * @brief Gets all component types for the specified entity.
   * @details Scans all storages to build list of component types present on entity.
   * Used for archetype management and debugging.
   * Time complexity: O(S) where S is the number of component types.
   * @param entity Entity to get component types for
   * @return Vector of ComponentTypeInfo for all components on the entity
   */
  [[nodiscard]] std::vector<ComponentTypeInfo> GetComponentTypes(Entity entity) const;

private:
  /**
   * @brief Gets or creates storage for component type T.
   * @details Finds existing storage or creates new ComponentStorage<T> if needed.
   * Time complexity: O(1) average case.
   * @tparam T Component type
   * @return Reference to ComponentStorage<T> (existing or newly created)
   */
  template <ComponentTrait T>
  [[nodiscard]] ComponentStorage<T>& GetOrCreateStorage();

  /// Map from component type ID to type-erased storage
  std::unordered_map<ComponentTypeId, std::unique_ptr<ComponentStorageBase>> storages_;
};

inline void Components::RemoveAllComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove all components from entity: Entity with index '{}' is invalid!",
                entity.Index());
  for (auto& [_, storage] : storages_) {
    storage->TryRemove(entity);
  }
}

template <ComponentTrait T>
inline void Components::AddComponent(Entity entity, T&& component) {
  using ComponentType = std::decay_t<T>;
  HELIOS_ASSERT(entity.Valid(), "Failed to add component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<ComponentType>(), entity.Index());
  GetOrCreateStorage<ComponentType>().Insert(entity, std::forward<T>(component));
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void Components::EmplaceComponent(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Failed to emplace component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  GetOrCreateStorage<T>().Emplace(entity, std::forward<Args>(args)...);
}

template <ComponentTrait T>
inline void Components::RemoveComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to remove component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  if (const auto it = storages_.find(type_id); it != storages_.end()) {
    static_cast<ComponentStorage<T>&>(*it->second).Remove(entity);
  } else {
    HELIOS_ASSERT(false, "Failed to remove component '{}': Entity with index '{}' does not have this component!",
                  ComponentNameOf<T>(), entity.Index());
  }
}

template <ComponentTrait T>
inline T* Components::TryGetComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Failed to try get component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  if (const auto it = storages_.find(type_id); it != storages_.end()) {
    return static_cast<ComponentStorage<T>&>(*it->second).TryGet(entity);
  }
  return nullptr;
}

template <ComponentTrait T>
inline const T* Components::TryGetComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to try get component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  if (const auto it = storages_.find(type_id); it != storages_.end()) {
    return static_cast<const ComponentStorage<T>&>(*it->second).TryGet(entity);
  }
  return nullptr;
}

template <ComponentTrait T>
inline bool Components::HasComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity has component '{}': Entity with index '{}' is invalid!",
                ComponentNameOf<T>(), entity.Index());
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  if (const auto it = storages_.find(type_id); it != storages_.end()) {
    return static_cast<const ComponentStorage<T>&>(*it->second).Contains(entity);
  }
  return false;
}

template <ComponentTrait T>
inline ComponentStorage<T>& Components::GetStorage() {
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  const auto it = storages_.find(type_id);
  HELIOS_ASSERT(it != storages_.end(), "Failed to get storage: Component '{}' storage does not exist!",
                ComponentNameOf<T>());
  return static_cast<ComponentStorage<T>&>(*it->second);
}

template <ComponentTrait T>
inline const ComponentStorage<T>& Components::GetStorage() const {
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  const auto it = storages_.find(type_id);
  HELIOS_ASSERT(it != storages_.end(), "Failed to get storage: Component '{}' storage does not exist!",
                ComponentNameOf<T>());
  return static_cast<const ComponentStorage<T>&>(*it->second);
}

inline std::vector<ComponentTypeInfo> Components::GetComponentTypes(Entity entity) const {
  std::vector<ComponentTypeInfo> types;
  types.reserve(storages_.size());  // Reserve worst-case size

  for (const auto& [_, storage] : storages_) {
    if (storage->Contains(entity)) {
      types.push_back(storage->GetTypeInfo());
    }
  }

  types.shrink_to_fit();  // Reduce to actual size
  return types;
}

template <ComponentTrait T>
inline ComponentStorage<T>& Components::GetOrCreateStorage() {
  const ComponentTypeId type_id = ComponentTypeIdOf<T>();
  const auto [it, inserted] = storages_.try_emplace(type_id, nullptr);
  if (inserted) {
    it->second = std::make_unique<ComponentStorage<T>>();
  }
  return static_cast<ComponentStorage<T>&>(*it->second);
}

}  // namespace helios::ecs::details
