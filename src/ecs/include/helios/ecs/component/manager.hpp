#pragma once

#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/container/typed_buffer.hpp>
#include <helios/container/typed_buffer_array.hpp>
#include <helios/ecs/component/archetype.hpp>
#include <helios/ecs/component/archetype_id.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/sparse_storage.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/utils/common_traits.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <deque>
#include <functional>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

namespace helios::ecs {

/**
 * @brief Runtime metadata for a registered component type.
 * @details Stores runtime information required for type-erased component
 * storage and migration between archetypes.
 */
struct ComponentMetadata {
  ComponentTypeIndex type_index;
  std::string_view name;
  size_t size = 0;
  size_t alignment = 0;
  ComponentStorageType storage_type = ComponentStorageType::kArchetype;
  bool is_tag = false;

  /// @brief Type-erased function to initialize a column with the correct
  /// concrete type.
  using InitColumnFn = void (*)(container::TypedBufferArray<>&);

  /// @brief Type-erased function to move one element from a source column row
  /// into the back of a target column.
  using MoveColumnElementFn = void (*)(container::TypedBufferArray<>& dst,
                                       container::TypedBufferArray<>& src,
                                       size_t src_row);

  /// @brief Type-erased function to push a default-constructed element onto a
  /// column.
  using DefaultPushFn = void (*)(container::TypedBufferArray<>&);

  InitColumnFn init_column = nullptr;
  MoveColumnElementFn move_column_element = nullptr;
  DefaultPushFn default_push = nullptr;

  /**
   * @brief Creates metadata for a component type.
   * @tparam T Component type
   * @return Populated metadata
   */
  template <ComponentTrait T>
  [[nodiscard]] static consteval ComponentMetadata From() noexcept;
};

template <ComponentTrait T>
consteval ComponentMetadata ComponentMetadata::From() noexcept {
  using Decayed = std::remove_cvref_t<T>;

  ComponentMetadata meta{};
  meta.type_index = ComponentTypeIndex::From<Decayed>();
  meta.name = ComponentNameOf<Decayed>();
  meta.size = sizeof(Decayed);
  meta.alignment = alignof(Decayed);
  meta.storage_type = ComponentStorageTypeOf<Decayed>();
  meta.is_tag = TagComponentTrait<Decayed>;

  meta.init_column = [](container::TypedBufferArray<>& col) {
    col.template ChangeType<Decayed>();
  };

  if constexpr (std::move_constructible<Decayed>) {
    meta.move_column_element = [](container::TypedBufferArray<>& dst,
                                  container::TypedBufferArray<>& src,
                                  size_t src_row) {
      auto& val = src.template At<Decayed>(src_row);
      dst.template PushBack<Decayed>(std::move(val));
    };
  }

  if constexpr (std::default_initializable<Decayed>) {
    meta.default_push = [](container::TypedBufferArray<>& col) {
      col.template EmplaceBack<Decayed>();
    };
  }

  return meta;
}

/**
 * @brief Central manager for all component storage (archetype and sparse-set).
 * @details Maintains:
 * - A registry of component metadata (type-erased info per component type)
 * - An archetype graph for entities with archetype-stored components
 * - Sparse-set storages for sparse-set-stored components
 * - Entity-to-archetype mapping for O(1) archetype lookup by entity
 *
 * Archetype-stored components live in dense SoA columns inside `Archetype`
 * instances. Adding/removing archetype components causes entity migration
 * between archetypes.
 *
 * Sparse-set-stored components live in independent `SparseComponentStorage<T>`
 * instances, stored inside `SparseStorageEntry` values within a `MultiTypeMap`.
 *
 * Archetypes are owned by a `std::deque<Archetype>` which provides
 * pointer/reference stability on `push_back`, allowing all maps and edge caches
 * to safely hold `std::reference_wrapper<Archetype>` without invalidation
 * concerns.
 *
 * @note Not thread-safe.
 */
class ComponentManager {
public:
  using size_type = size_t;

  ComponentManager() = default;
  ComponentManager(const ComponentManager&) = delete;
  ComponentManager(ComponentManager&&) noexcept = default;
  ~ComponentManager() = default;

  ComponentManager& operator=(const ComponentManager&) = delete;
  ComponentManager& operator=(ComponentManager&&) noexcept = default;

  /// @brief Clears all component data, archetypes, and metadata.
  void Clear();

  /// @brief Clears all component data and archetypes but retains metadata
  /// registrations.
  void ClearData() noexcept;

  /**
   * @brief Registers a component type so the manager knows its metadata.
   * @details Registration is optional for compile-time-typed APIs (they
   * auto-register). Explicitly registering is useful for pre-warming or
   * validation.
   * @tparam T Component type
   */
  template <ComponentTrait T>
  void Register();

  /**
   * @brief Registers multiple component types.
   * @tparam Ts Component types
   */
  template <ComponentTrait... Ts>
    requires(sizeof...(Ts) > 1)
  void RegisterAll() {
    (Register<Ts>(), ...);
  }

  /**
   * @brief Initializes an entity in the component manager with an empty
   * archetype.
   * @details Must be called after entity creation but before any component
   * operations.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is already tracked.
   * @param entity Entity to initialize
   */
  void InitEntity(Entity entity);

  /**
   * @brief Removes all component data for an entity.
   * @details Removes the entity from its archetype and from all sparse-set
   * storages.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @param entity Entity to remove
   */
  void RemoveEntity(Entity entity);

  /**
   * @brief Tries to remove all component data for an entity.
   * @details Removes the entity from its archetype and from all sparse-set
   * storages.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to remove
   * @return True if removed, false if entity was not tracked
   */
  bool TryRemoveEntity(Entity entity);

  /**
   * @brief Removes all archetype and sparse-set components from an entity.
   * @details The entity is moved to the empty archetype.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @param entity Entity to clear
   */
  void Clear(Entity entity);

  /**
   * @brief Adds multiple archetype-stored components to an entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * - Any component type is not part of this archetype.
   * @tparam Ts Component types, must be unique and at least 1 type
   * @param entity Entity
   * @param components Component values
   */
  template <ArchetypeComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void AddArchetypeComponents(Entity entity, Ts&&... components);

  /**
   * @brief Adds multiple archetype-stored components to an entity.
   * @details Components are added in a single migration to minimize overhead.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam Ts Component types, must have archetype storage, be unique and at
   * least 1 type
   * @param entity Entity
   * @param components Component values
   */
  template <ArchetypeComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  inline auto TryAddArchetypeComponents(Entity entity, Ts&&... components)
      -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Emplaces an archetype-stored component (construct in-place).
   * @details Replaces if already present, otherwise migrates.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type (must have archetype storage)
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments forwarded to `T`'s constructor
   */
  template <ArchetypeComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceArchetypeComponent(Entity entity, Args&&... args);

  /**
   * @brief Tries to emplace an archetype-stored component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type (must have archetype storage)
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments forwarded to `T`'s constructor
   * @return True if emplaced, false if entity already had the component
   */
  template <ArchetypeComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplaceArchetypeComponent(Entity entity, Args&&... args);

  /**
   * @brief Removes multiple archetype-stored components from an entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam Ts Component types, must have archetype storage, be unique and at
   * least 1 type
   * @param entity Entity
   */
  template <ArchetypeComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void RemoveArchetypeComponents(Entity entity);

  /**
   * @brief Tries to remove multiple archetype-stored components.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam Ts Component types, must have archetype storage, be unique and at
   * least 1 type
   * @param entity Entity
   * @return Array of bools indicating whether each component was removed if
   * more than one type passed, or a single bool if only one type passed
   */
  template <ArchetypeComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryRemoveArchetypeComponents(Entity entity)
      -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Adds a sparse-set-stored component to an entity.
   * @details Replaces if already present.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type (must have sparse-set storage)
   * @param entity Entity
   * @param component Component value
   */
  template <SparseComponentTrait T>
  void AddSparseComponent(Entity entity, T&& component);

  /**
   * @brief Tries to add a sparse-set-stored component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type (must have sparse-set storage)
   * @param entity Entity
   * @param component Component value
   * @return True if added, false if entity already had the component
   */
  template <SparseComponentTrait T>
  bool TryAddSparseComponent(Entity entity, T&& component);

  /**
   * @brief Emplaces a sparse-set-stored component.
   * @details Replaces if already present.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type (must have sparse-set storage)
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments forwarded to `T`'s constructor
   */
  template <SparseComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void EmplaceSparseComponent(Entity entity, Args&&... args);

  /**
   * @brief Tries to emplace a sparse-set-stored component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments forwarded to `T`'s constructor
   * @return True if emplaced, false if entity already had the component
   */
  template <SparseComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplaceSparseComponent(Entity entity, Args&&... args);

  /**
   * @brief Removes a sparse-set-stored component from an entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * - Sparse storage for the component type does not exist.
   * - Entity does not have the component.
   * @tparam T Component type (must have sparse-set storage)
   * @param entity Entity
   */
  template <SparseComponentTrait T>
  void RemoveSparseComponent(Entity entity);

  /**
   * @brief Tries to remove a sparse-set-stored component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type (must have sparse-set storage)
   * @param entity Entity
   * @return True if removed, false if entity didn't have the component or
   * storage doesn't exist
   */
  template <SparseComponentTrait T>
  bool TryRemoveSparseComponent(Entity entity);

  /**
   * @brief Adds multiple components to an entity.
   * @details Archetype components are batched into a single migration.
   * Sparse components are added individually after the archetype
   * migration.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam Ts Component types, must be unique and at least 1 type
   * @param entity Entity
   * @param components Component values
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void Add(Entity entity, Ts&&... components);

  /**
   * @brief Tries to add multiple components.
   * @details Archetype components are batched into a single migration.
   * Sparse components are added individually after the archetype
   * migration. Results are returned in the same order as the input types.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam Ts Component types, must be unique and at least 1 type
   * @param entity Entity
   * @param components Component values
   * @return Array of bools indicating whether each component was added if
   * multiple components were added, or a single bool if only one component was
   * added
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryAdd(Entity entity, Ts&&... components)
      -> std::conditional_t<sizeof...(Ts) == 1, bool,
                            std::array<bool, sizeof...(Ts)>>;

  /**
   * @brief Emplaces a component (construct in-place), dispatching by storage
   * type.
   * @details Replaces if already present.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments forwarded to `T`'s constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Entity entity, Args&&... args);

  /**
   * @brief Tries to emplace a component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam T Component type
   * @tparam Args Constructor argument types
   * @param entity Entity
   * @param args Arguments forwarded to `T`'s constructor
   * @return True if emplaced, false if entity already had the component
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplace(Entity entity, Args&&... args);

  /**
   * @brief Removes multiple components from an entity.
   * @details Archetype components are removed in a single migration.
   * Sparse components are removed individually.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * - Entity does not have any of the components.
   * @tparam Ts Component types, must be unique and at least 1 type
   * @param entity Entity
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  void Remove(Entity entity);

  /**
   * @brief Tries to remove multiple components.
   * @details Archetype components are removed in a single migration.
   * Sparse components are removed individually.
   * Results are returned in the same order as the input types.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @tparam Ts Component types, must be unique and at least 1 type
   * @param entity Entity
   * @return Array of bools indicating whether each component was removed if
   * more than one type was passed, or a single bool if only one type was
   * passed
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  auto TryRemove(Entity entity)
      -> std::conditional_t<sizeof...(Ts) == 1, bool,
                            std::array<bool, sizeof...(Ts)>>;

  /**
   * @brief Gets a mutable reference to a component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked (for archetype-stored components).
   * - Sparse storage does not exist for the component type (for sparse-stored
   * components).
   * - Entity does not have the component.
   * @tparam T Component type
   * @param entity Entity
   * @return Mutable reference to component
   */
  template <ComponentTrait T>
  [[nodiscard]] T& Get(Entity entity);

  /**
   * @brief Gets a const reference to a component.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked (for archetype-stored components).
   * - Sparse storage does not exist for the component type (for sparse-stored
   * components).
   * - Entity does not have the component.
   * @tparam T Component type
   * @param entity Entity
   * @return Const reference to component
   */
  template <ComponentTrait T>
  [[nodiscard]] const T& Get(Entity entity) const;

  /**
   * @brief Tries to get a mutable pointer to a component.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type
   * @param entity Entity
   * @return Pointer to component or `nullptr` if not found
   */
  template <ComponentTrait T>
  [[nodiscard]] T* TryGet(Entity entity);

  /**
   * @brief Tries to get a const pointer to a component.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type
   * @param entity Entity
   * @return Const pointer to component or `nullptr` if not found
   */
  template <ComponentTrait T>
  [[nodiscard]] const T* TryGet(Entity entity) const;

  /**
   * @brief Checks if a component type is registered.
   * @tparam T Component type
   * @return True if registered
   */
  template <ComponentTrait T>
  [[nodiscard]] bool Registered() const noexcept;

  /**
   * @brief Checks if an entity is tracked by the component manager.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to check
   * @return True if the entity has been initialized
   */
  [[nodiscard]] bool Tracked(Entity entity) const noexcept;

  /**
   * @brief Checks if an entity has a component.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type
   * @param entity Entity
   * @return True if entity has the component
   */
  template <ComponentTrait T>
  [[nodiscard]] bool Has(Entity entity) const;

  /**
   * @brief Checks if an entity has multiple components.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Ts Component types
   * @param entity Entity
   * @return Array of bools indicating whether the entity has each component
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 1)
  [[nodiscard]] auto Has(Entity entity) const
      -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Gets the archetype that the entity currently belongs to.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @param entity Entity
   * @return Reference to the entity's archetype
   */
  [[nodiscard]] Archetype& EntityArchetype(Entity entity);

  /**
   * @brief Gets the archetype that the entity currently belongs to (const).
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not tracked.
   * @param entity Entity
   * @return Const reference to the entity's archetype
   */
  [[nodiscard]] const Archetype& EntityArchetype(Entity entity) const;

  /**
   * @brief Gets the sparse-set storage for a component type, creating it if
   * necessary.
   * @tparam T Component type (must have sparse-set storage)
   * @return Mutable reference to the sparse-set storage
   */
  template <SparseComponentTrait T>
  [[nodiscard]] auto SparseStorage() -> SparseComponentStorage<T>& {
    return EnsureSparseStorage<T>();
  }

  /**
   * @brief Gets the sparse-set storage for a component type (const).
   * @warning Triggers assertion if the storage for the component type does not
   * exist.
   * @tparam T Component type
   * @return Const reference to the sparse-set storage
   */
  template <SparseComponentTrait T>
  [[nodiscard]] auto SparseStorage() const -> const SparseComponentStorage<T>&;

  /**
   * @brief Gets metadata for a component type.
   * @warning Triggers assertion if not registered.
   * @tparam T Component type
   * @return Const reference to metadata
   */
  template <ComponentTrait T>
  [[nodiscard]] const ComponentMetadata& Metadata() const noexcept;

  /**
   * @brief Gets metadata for a component type by its runtime type index.
   * @param type_index Component type index
   * @return Pointer to metadata, or `nullptr` if not registered
   */
  [[nodiscard]] const ComponentMetadata* MetadataByIndex(
      ComponentTypeIndex type_index) const noexcept;

  struct SparseStorageEntry;

  [[nodiscard]] const SparseStorageEntry* SparseEntry(
      ComponentTypeIndex type_index) const noexcept {
    return sparse_storages_.TryGet(type_index);
  }

  /**
   * @brief Gets the number of entities currently tracked by the component
   * manager.
   * @return Number of tracked entities
   */
  [[nodiscard]] size_type TrackedEntityCount() const noexcept {
    return entity_archetype_.size();
  }

  /**
   * @brief Gets the number of archetypes currently in the manager.
   * @return Number of archetypes
   */
  [[nodiscard]] size_type ArchetypeCount() const noexcept {
    return archetype_list_.size();
  }

  /**
   * @brief Gets all archetypes currently in the manager.
   * @details Useful for query iteration. Callers can filter archetypes by their
   * ArchetypeId.
   * @return Span of reference wrappers to all archetypes
   */
  [[nodiscard]] auto Archetypes() noexcept
      -> std::span<std::reference_wrapper<Archetype>> {
    return archetype_list_;
  }

  /**
   * @brief Gets all archetypes (const).
   * @return Span of const reference wrappers to all archetypes
   */
  [[nodiscard]] auto Archetypes() const noexcept
      -> std::span<const std::reference_wrapper<Archetype>> {
    return archetype_list_;
  }

  /**
   * @brief Gets the current structural version of the component manager.
   * @details Incremented on every archetype creation or entity migration.
   * @return Structural version counter
   */
  [[nodiscard]] size_type StructuralVersion() const noexcept {
    return structural_version_;
  }

  /**
   * @brief Type-erased entry for sparse-set component storage.
   * @details Stores a `SparseComponentStorage<T>` inside a `TypedBuffer` along
   * with type-erased function pointers for operations that need to be performed
   * without knowing the concrete type (e.g., clearing all storages, removing an
   * entity from all storages).
   */
  struct SparseStorageEntry {
    using ClearFn = void (*)(container::TypedBuffer<>& storage);
    using TryRemoveFn = bool (*)(container::TypedBuffer<>& storage,
                                 Entity entity);
    using ContainsFn = bool (*)(const container::TypedBuffer<>& storage,
                                Entity entity);
    using SizeFn = size_t (*)(const container::TypedBuffer<>& storage);

    container::TypedBuffer<> storage;
    ClearFn clear_fn = nullptr;
    TryRemoveFn try_remove_fn = nullptr;
    ContainsFn contains_fn = nullptr;
    SizeFn size_fn = nullptr;

    constexpr SparseStorageEntry() = default;
    SparseStorageEntry(const SparseStorageEntry&) = delete;
    constexpr SparseStorageEntry(SparseStorageEntry&&) noexcept = default;
    constexpr ~SparseStorageEntry() = default;

    SparseStorageEntry& operator=(const SparseStorageEntry&) = delete;
    constexpr SparseStorageEntry& operator=(SparseStorageEntry&&) noexcept =
        default;

    /**
     * @brief Creates a `SparseStorageEntry` for a specific component type `T`.
     * @tparam T Component type
     * @return Initialized `SparseStorageEntry` with a
     * `SparseComponentStorage<T>` and appropriate function pointers
     */
    template <ComponentTrait T>
    [[nodiscard]] static SparseStorageEntry From() {
      using StorageT = SparseComponentStorage<T>;

      SparseStorageEntry entry;
      entry.storage.template Set<StorageT>();

      entry.clear_fn = [](container::TypedBuffer<>& buf) {
        buf.template Value<StorageT>().Clear();
      };

      entry.try_remove_fn = [](container::TypedBuffer<>& buf,
                               Entity entity) -> bool {
        return buf.template Value<StorageT>().TryRemove(entity);
      };

      entry.contains_fn = [](const container::TypedBuffer<>& buf,
                             Entity entity) -> bool {
        return buf.template Value<StorageT>().Contains(entity);
      };

      entry.size_fn = [](const container::TypedBuffer<>& buf) -> size_t {
        return buf.template Value<StorageT>().Size();
      };

      return entry;
    }

    /// @brief Clears the underlying storage.
    void Clear() noexcept {
      if (clear_fn != nullptr) [[unlikely]] {
        clear_fn(storage);
      }
    }

    /**
     * @brief Tries to remove the component for the given entity.
     * @warning Triggers assertion if entity is invalid.
     * @param entity Entity
     * @return True if removed, false otherwise
     */
    bool TryRemove(Entity entity) {
      HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
      if (try_remove_fn == nullptr) [[unlikely]] {
        return false;
      }
      return try_remove_fn(storage, entity);
    }

    /**
     * @brief Checks if the component for the given entity exists.
     * @warning Triggers assertion if entity is invalid.
     * @param entity Entity
     * @return True if entity has the component, false otherwise
     */
    [[nodiscard]] bool Contains(Entity entity) const noexcept {
      HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
      if (contains_fn == nullptr) [[unlikely]] {
        return false;
      }
      return contains_fn(storage, entity);
    }

    /**
     * @brief Gets the number of stored components.
     * @return Number of components
     */
    [[nodiscard]] size_t Size() const noexcept {
      if (size_fn == nullptr) [[unlikely]] {
        return 0;
      }
      return size_fn(storage);
    }

    /**
     * @brief Gets the typed storage.
     * @warning Triggers assertion if the storage does not contain the expected
     * type.
     * @tparam T Component type
     * @return Mutable reference to the typed storage
     */
    template <ComponentTrait T>
    [[nodiscard]] auto As() noexcept -> SparseComponentStorage<T>& {
      return storage.template Value<SparseComponentStorage<T>>();
    }

    /**
     * @brief Gets the typed storage (const).
     * @warning Triggers assertion if the storage does not contain the expected
     * type.
     * @tparam T Component type
     * @return Const reference to the typed storage
     */
    template <ComponentTrait T>
    [[nodiscard]] auto As() const noexcept -> const SparseComponentStorage<T>& {
      return storage.template Value<SparseComponentStorage<T>>();
    }
  };

private:
  /// @brief Sparse-set storages keyed by component `TypeIndex`.
  using SparseStorageMap = container::MultiTypeMap<SparseStorageEntry>;

  /**
   * @brief Stores per-archetype edge caches for fast archetype transitions.
   * @details When a component is added or removed from an entity, the edge
   * graph allows O(1) lookup of the target archetype without recomputing the
   * archetype id. The archetype itself is owned by `archetype_storage_` (a
   * `std::deque`); the record only holds a reference to it.
   */
  struct ArchetypeRecord {
    std::reference_wrapper<Archetype> archetype;

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
    using EdgeMap =
        std::flat_map<ComponentTypeIndex, std::reference_wrapper<Archetype>>;
#else
    using EdgeMap = boost::container::flat_map<
        ComponentTypeIndex, std::reference_wrapper<Archetype>, std::less<>>;
#endif

    EdgeMap add_edges;     ///< Cache: component added -> target archetype.
    EdgeMap remove_edges;  ///< Cache: component removed -> target archetype.
  };

  // Strategy: a consteval lambda fills a compile-time std::array by iterating
  // over a bool[] predicate result. A second alias unpacks that array back into
  // a variadic index_sequence via std::make_index_sequence + another lambda.
  // The whole filter compiles as two flat alias instantiations — no recursion.
  template <template <typename> typename Pred, typename... Ts>
  static consteval auto FilteredIndicesArray() noexcept;

  // Convenience predicates.
  template <typename T>
  struct IsArchetype : std::bool_constant<ArchetypeComponentTrait<T>> {};
  template <typename T>
  struct IsSparse : std::bool_constant<SparseComponentTrait<T>> {};

  /// Mixed-storage dispatch
  template <size_t I, typename... Ts>
  using TypeAt =
      std::remove_cvref_t<std::tuple_element_t<I, std::tuple<Ts...>>>;

  template <size_t I, typename... Ts>
  using FwdTypeAt = std::tuple_element_t<I, std::tuple<Ts...>>;

  // Unpack a constexpr std::array<size_t, N> into std::index_sequence<arr[0],
  // arr[1], ...>
  template <auto Arr, typename Seq>
  struct ArrayToIndexSequence;

  template <auto Arr, size_t... Js>
  struct ArrayToIndexSequence<Arr, std::index_sequence<Js...>> {
    using type = std::index_sequence<Arr[Js]...>;
  };

  template <template <typename> typename Pred, typename... Ts>
  using FilteredIndices = typename ArrayToIndexSequence<
      FilteredIndicesArray<Pred, Ts...>(),
      std::make_index_sequence<FilteredIndicesArray<Pred, Ts...>().size()>>::
      type;

  // Batch-dispatch archetype components through the correct single/multi
  // overload.
  template <typename... Ts, size_t... ArchIs>
  void DispatchAddArchetype(Entity entity, std::tuple<Ts...>& args,
                            std::index_sequence<ArchIs...> /*seq*/);

  template <typename... Ts, size_t... ArchIs>
  auto DispatchTryAddArchetype(Entity entity, std::tuple<Ts...>& args,
                               std::index_sequence<ArchIs...> /*seq*/)
      -> std::array<bool, sizeof...(ArchIs)>;

  template <typename... Ts, size_t... ArchIs>
  void DispatchRemoveArchetype(Entity entity,
                               std::index_sequence<ArchIs...> /*seq*/);

  template <typename... Ts, size_t... ArchIs>
  auto DispatchTryRemoveArchetype(Entity entity,
                                  std::index_sequence<ArchIs...> /*seq*/)
      -> std::array<bool, sizeof...(ArchIs)>;

  template <ComponentTrait T>
  void EnsureRegistered();

  template <SparseComponentTrait T>
  SparseComponentStorage<T>& EnsureSparseStorage();

  Archetype& GetOrCreateArchetype(const ArchetypeId& id);

  void InitArchetypeColumns(Archetype& archetype);

  void MigrateEntity(Entity entity, Archetype& src, Archetype& dst);

  Archetype* TryGetAddEdge(Archetype& from, ComponentTypeIndex type);
  void SetAddEdge(Archetype& from, ComponentTypeIndex type, Archetype& to) {
    GetRecord(from).add_edges.emplace(type, std::ref(to));
  }

  Archetype* TryGetRemoveEdge(Archetype& from, ComponentTypeIndex type);
  void SetRemoveEdge(Archetype& from, ComponentTypeIndex type, Archetype& to) {
    GetRecord(from).remove_edges.emplace(type, std::ref(to));
  }

  ArchetypeRecord& GetRecord(Archetype& archetype);
  const ArchetypeRecord& GetRecord(const Archetype& archetype) const;

  /// Metadata per component type.
#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  using MetadataMap = std::flat_map<ComponentTypeIndex, ComponentMetadata>;
#else
  using MetadataMap =
      boost::container::flat_map<ComponentTypeIndex, ComponentMetadata,
                                 std::less<>>;
#endif
  MetadataMap metadata_;

  /// @brief Owns all archetypes. `std::deque` guarantees pointer/reference
  /// stability on `push_back`.
  std::deque<Archetype> archetype_storage_;

  /// Archetype lookup. Key = ArchetypeId hash. Value = ArchetypeRecord
  /// (references into `archetype_storage_`).
  std::unordered_map<size_t, ArchetypeRecord> archetype_map_;

  /// Flat list of all archetype references for fast iteration (query support).
  std::vector<std::reference_wrapper<Archetype>> archetype_list_;

  /// Maps entity index -> archetype the entity is in.
  std::unordered_map<Entity::IndexType, std::reference_wrapper<Archetype>>
      entity_archetype_;

  SparseStorageMap sparse_storages_;

  /// Empty archetype (entities with no archetype-stored components live here).
  Archetype* empty_archetype_ = nullptr;

  /// Structural version counter, incremented on archetype creation or entity
  /// migration.
  size_type structural_version_ = 0;
};

inline void ComponentManager::Clear() {
  entity_archetype_.clear();
  archetype_map_.clear();
  archetype_list_.clear();
  archetype_storage_.clear();
  sparse_storages_.ResetAll();
  metadata_.clear();
  empty_archetype_ = nullptr;
  structural_version_ = 0;
}

inline void ComponentManager::ClearData() noexcept {
  entity_archetype_.clear();
  archetype_map_.clear();
  archetype_list_.clear();
  archetype_storage_.clear();
  for (auto&& [type_index, entry] : sparse_storages_.Data()) {
    entry.Clear();
  }
  empty_archetype_ = nullptr;
  ++structural_version_;
}

template <ComponentTrait T>
inline void ComponentManager::Register() {
  constexpr auto type_index = ComponentTypeIndex::From<T>();
  if (metadata_.find(type_index) != metadata_.end()) {
    return;
  }
  metadata_.emplace(type_index, ComponentMetadata::From<T>());
}

inline void ComponentManager::InitEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(!Tracked(entity), "Entity '{}' already tracked!", entity);

  // Ensure empty archetype exists.
  if (empty_archetype_ == nullptr) {
    empty_archetype_ = &GetOrCreateArchetype(ArchetypeId{});
  }

  // Place entity in empty archetype (no components to add, just allocate a
  // row).
  empty_archetype_->AllocateRow(entity);
  entity_archetype_.emplace(entity.Index(), std::ref(*empty_archetype_));
}

inline void ComponentManager::RemoveEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);

  Archetype& arch = it->second.get();

  // Remove from archetype.
  Entity swapped = arch.Remove(entity);
  if (swapped.Valid()) {
    entity_archetype_.insert_or_assign(swapped.Index(), std::ref(arch));
  }

  entity_archetype_.erase(it);

  // Remove from all sparse storages.
  for (auto&& [type_index, entry] : sparse_storages_.Data()) {
    entry.TryRemove(entity);
  }
}

inline bool ComponentManager::TryRemoveEntity(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const auto it = entity_archetype_.find(entity.Index());
  if (it == entity_archetype_.end()) {
    return false;
  }

  Archetype& arch = it->second.get();
  Entity swapped = arch.Remove(entity);
  if (swapped.Valid()) {
    entity_archetype_.insert_or_assign(swapped.Index(), std::ref(arch));
  }
  entity_archetype_.erase(it);

  for (auto&& [type_index, entry] : sparse_storages_.Data()) {
    entry.TryRemove(entity);
  }
  return true;
}

inline void ComponentManager::Clear(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);

  Archetype& current = it->second.get();

  if (empty_archetype_ == nullptr) {
    empty_archetype_ = &GetOrCreateArchetype(ArchetypeId{});
  }

  if (&current != empty_archetype_) {
    Entity swapped = current.Remove(entity);
    if (swapped.Valid()) {
      entity_archetype_.insert_or_assign(swapped.Index(), std::ref(current));
    }
    empty_archetype_->AllocateRow(entity);
    entity_archetype_.insert_or_assign(entity.Index(),
                                       std::ref(*empty_archetype_));
    ++structural_version_;
  }

  // Remove from all sparse storages.
  for (auto&& [type_index, entry] : sparse_storages_.Data()) {
    entry.TryRemove(entity);
  }
}

template <ArchetypeComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void ComponentManager::AddArchetypeComponents(Entity entity,
                                                     Ts&&... components) {
  using AddedMask = std::array<bool, sizeof...(Ts)>;

  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  (EnsureRegistered<std::remove_cvref_t<Ts>>(), ...);

  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);

  Archetype& current = it->second.get();
  const AddedMask added = {(!current.HasColumn(
      ComponentTypeIndex::From<std::remove_cvref_t<Ts>>()))...};

  auto target_id = current.Id();
  [&]<size_t... Is>(std::index_sequence<Is...>) {
    ((added[Is]
          ? static_cast<void>(
                target_id = target_id.template With<std::remove_cvref_t<Ts>>())
          : void()),
     ...);
  }(std::make_index_sequence<sizeof...(Ts)>{});

  if (target_id != current.Id()) {
    Archetype& target = GetOrCreateArchetype(target_id);
    MigrateEntity(entity, current, target);

    auto args = std::forward_as_tuple(std::forward<Ts>(components)...);
    [&]<size_t... Is>(std::index_sequence<Is...>) {
      ((added[Is] ? target.Set(entity, std::forward<FwdTypeAt<Is, Ts...>>(
                                           std::get<Is>(args)))
                  : void()),
       ...);
    }(std::make_index_sequence<sizeof...(Ts)>{});

    ++structural_version_;
  } else {
    (current.Set(entity, std::forward<Ts>(components)), ...);
  }
}

template <ArchetypeComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto ComponentManager::TryAddArchetypeComponents(Entity entity,
                                                        Ts&&... components)
    -> std::array<bool, sizeof...(Ts)> {
  using AddedMask = std::array<bool, sizeof...(Ts)>;

  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  (EnsureRegistered<std::remove_cvref_t<Ts>>(), ...);

  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);

  Archetype& current = it->second.get();
  const AddedMask results = {(!current.HasColumn(
      ComponentTypeIndex::From<std::remove_cvref_t<Ts>>()))...};

  auto target_id = current.Id();
  [&]<size_t... Is>(std::index_sequence<Is...>) {
    ((results[Is]
          ? static_cast<void>(
                target_id = target_id.template With<std::remove_cvref_t<Ts>>())
          : void()),
     ...);
  }(std::make_index_sequence<sizeof...(Ts)>{});

  if (target_id != current.Id()) {
    Archetype& target = GetOrCreateArchetype(target_id);
    MigrateEntity(entity, current, target);

    auto args = std::forward_as_tuple(std::forward<Ts>(components)...);
    [&]<size_t... Is>(std::index_sequence<Is...>) {
      ((results[Is] ? target.Set(entity, std::forward<FwdTypeAt<Is, Ts...>>(
                                             std::get<Is>(args)))
                    : void()),
       ...);
    }(std::make_index_sequence<sizeof...(Ts)>{});

    ++structural_version_;
  }

  return results;
}

template <ArchetypeComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void ComponentManager::EmplaceArchetypeComponent(Entity entity,
                                                        Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  AddArchetypeComponents<T>(entity, T(std::forward<Args>(args)...));
}

template <ArchetypeComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool ComponentManager::TryEmplaceArchetypeComponent(Entity entity,
                                                           Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return TryAddArchetypeComponents<T>(entity, T(std::forward<Args>(args)...))
      .front();
}

template <ArchetypeComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void ComponentManager::RemoveArchetypeComponents(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);

  Archetype& current = it->second.get();
  auto target_id = current.Id();
  ((target_id = target_id.template Without<Ts>()), ...);

  if (target_id != current.Id()) {
    Archetype& target = GetOrCreateArchetype(target_id);
    MigrateEntity(entity, current, target);
    ++structural_version_;
  }
}

template <ArchetypeComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto ComponentManager::TryRemoveArchetypeComponents(Entity entity)
    -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);

  Archetype& current = it->second.get();
  std::array<bool, sizeof...(Ts)> results = {(current.HasColumn(
      ComponentTypeIndex::From<std::remove_cvref_t<Ts>>()))...};

  auto target_id = current.Id();
  ((target_id = target_id.template Without<Ts>()), ...);

  if (target_id != current.Id()) {
    Archetype& target = GetOrCreateArchetype(target_id);
    MigrateEntity(entity, current, target);
    ++structural_version_;
  }

  return results;
}

template <SparseComponentTrait T>
inline void ComponentManager::AddSparseComponent(Entity entity, T&& component) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  auto& storage = EnsureSparseStorage<std::remove_cvref_t<T>>();
  storage.Set(entity, std::forward<T>(component));
}

template <SparseComponentTrait T>
inline bool ComponentManager::TryAddSparseComponent(Entity entity,
                                                    T&& component) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  auto& storage = EnsureSparseStorage<std::remove_cvref_t<T>>();
  return storage.TrySet(entity, std::forward<T>(component));
}

template <SparseComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void ComponentManager::EmplaceSparseComponent(Entity entity,
                                                     Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  auto& storage = EnsureSparseStorage<T>();
  storage.Emplace(entity, std::forward<Args>(args)...);
}

template <SparseComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool ComponentManager::TryEmplaceSparseComponent(Entity entity,
                                                        Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  auto& storage = EnsureSparseStorage<T>();
  return storage.TryEmplace(entity, std::forward<Args>(args)...);
}

template <SparseComponentTrait T>
inline void ComponentManager::RemoveSparseComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  auto* entry = sparse_storages_.template TryGet<T>();
  HELIOS_ASSERT(entry != nullptr,
                "Sparse storage for component '{}' does not exist!",
                ComponentNameOf<T>());
  entry->template As<T>().Remove(entity);
}

template <SparseComponentTrait T>
inline bool ComponentManager::TryRemoveSparseComponent(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  auto* entry = sparse_storages_.template TryGet<T>();
  if (entry == nullptr) {
    return false;
  }
  return entry->template As<T>().TryRemove(entity);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void ComponentManager::Add(Entity entity, Ts&&... components) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  constexpr bool kAllSparse =
      (... && SparseComponentTrait<std::remove_cvref_t<Ts>>);
  constexpr bool kAllArchetype =
      (... && ArchetypeComponentTrait<std::remove_cvref_t<Ts>>);

  if constexpr (kAllSparse) {
    (AddSparseComponent(entity, std::forward<Ts>(components)), ...);
  } else if constexpr (kAllArchetype) {
    AddArchetypeComponents(entity, std::forward<Ts>(components)...);
  } else {
    using ArchIs = FilteredIndices<IsArchetype, Ts...>;
    using SparseIs = FilteredIndices<IsSparse, Ts...>;
    auto args = std::forward_as_tuple(std::forward<Ts>(components)...);
    DispatchAddArchetype(entity, args, ArchIs{});
    [this, entity, &args]<size_t... Is>(std::index_sequence<Is...>) {
      (AddSparseComponent(
           entity, std::forward<FwdTypeAt<Is, Ts...>>(std::get<Is>(args))),
       ...);
    }(SparseIs{});
  }
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto ComponentManager::TryAdd(Entity entity, Ts&&... components)
    -> std::conditional_t<sizeof...(Ts) == 1, bool,
                          std::array<bool, sizeof...(Ts)>> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  constexpr bool kAllSparse =
      (... && SparseComponentTrait<std::remove_cvref_t<Ts>>);
  constexpr bool kAllArchetype =
      (... && ArchetypeComponentTrait<std::remove_cvref_t<Ts>>);

  if constexpr (kAllSparse) {
    auto results = std::to_array(
        {TryAddSparseComponent(entity, std::forward<Ts>(components))...});
    if constexpr (sizeof...(Ts) == 1) {
      return results.front();
    } else {
      return results;
    }
  } else if constexpr (kAllArchetype) {
    auto results =
        TryAddArchetypeComponents(entity, std::forward<Ts>(components)...);
    if constexpr (sizeof...(Ts) == 1) {
      return results.front();
    } else {
      return results;
    }
  } else {
    using ArchIs = FilteredIndices<IsArchetype, Ts...>;
    using SparseIs = FilteredIndices<IsSparse, Ts...>;
    auto args = std::forward_as_tuple(std::forward<Ts>(components)...);
    std::array<bool, sizeof...(Ts)> results = {};

    // Batch archetype: sub-results map back to original positions via ArchIs.
    auto arch_results = DispatchTryAddArchetype(entity, args, ArchIs{});
    [this, entity, &args, &results,
     &arch_results]<size_t... ArchPos, size_t... SubIs>(
        std::index_sequence<ArchPos...>, std::index_sequence<SubIs...>) {
      ((results[ArchPos] = arch_results[SubIs]), ...);
    }(ArchIs{}, std::make_index_sequence<arch_results.size()>{});

    // Sparse: one call each, result goes directly to the original position.
    [this, entity, &args, &results]<size_t... Is>(std::index_sequence<Is...>) {
      ((results[Is] = TryAddSparseComponent(
            entity, std::forward<FwdTypeAt<Is, Ts...>>(std::get<Is>(args)))),
       ...);
    }(SparseIs{});

    if constexpr (sizeof...(Ts) == 1) {
      return results.front();
    } else {
      return results;
    }
  }
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void ComponentManager::Emplace(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  if constexpr (SparseComponentTrait<T>) {
    EmplaceSparseComponent<T>(entity, std::forward<Args>(args)...);
  } else {
    EmplaceArchetypeComponent<T>(entity, std::forward<Args>(args)...);
  }
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool ComponentManager::TryEmplace(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  if constexpr (SparseComponentTrait<T>) {
    return TryEmplaceSparseComponent<T>(entity, std::forward<Args>(args)...);
  } else {
    return TryEmplaceArchetypeComponent<T>(entity, std::forward<Args>(args)...);
  }
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline void ComponentManager::Remove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  constexpr bool kAllSparse =
      (... && SparseComponentTrait<std::remove_cvref_t<Ts>>);
  constexpr bool kAllArchetype =
      (... && ArchetypeComponentTrait<std::remove_cvref_t<Ts>>);

  if constexpr (kAllSparse) {
    (RemoveSparseComponent<Ts>(entity), ...);
  } else if constexpr (kAllArchetype) {
    RemoveArchetypeComponents<Ts...>(entity);
  } else {
    using ArchIs = FilteredIndices<IsArchetype, Ts...>;
    using SparseIs = FilteredIndices<IsSparse, Ts...>;
    DispatchRemoveArchetype<Ts...>(entity, ArchIs{});
    [this, entity]<size_t... Is>(std::index_sequence<Is...>) {
      (RemoveSparseComponent<TypeAt<Is, Ts...>>(entity), ...);
    }(SparseIs{});
  }
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto ComponentManager::TryRemove(Entity entity)
    -> std::conditional_t<sizeof...(Ts) == 1, bool,
                          std::array<bool, sizeof...(Ts)>> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Tracked(entity), "Entity '{}' not tracked!", entity);

  constexpr bool kAllSparse =
      (... && SparseComponentTrait<std::remove_cvref_t<Ts>>);
  constexpr bool kAllArchetype =
      (... && ArchetypeComponentTrait<std::remove_cvref_t<Ts>>);

  if constexpr (kAllSparse) {
    auto results = std::to_array({TryRemoveSparseComponent<Ts>(entity)...});
    if constexpr (sizeof...(Ts) == 1) {
      return results.front();
    } else {
      return results;
    }
  } else if constexpr (kAllArchetype) {
    auto results = TryRemoveArchetypeComponents<Ts...>(entity);
    if constexpr (sizeof...(Ts) == 1) {
      return results.front();
    } else {
      return results;
    }
  } else {
    using ArchIs = FilteredIndices<IsArchetype, Ts...>;
    using SparseIs = FilteredIndices<IsSparse, Ts...>;
    std::array<bool, sizeof...(Ts)> results = {};

    auto arch_results = DispatchTryRemoveArchetype<Ts...>(entity, ArchIs{});
    [&results,
     &arch_results]<size_t... ArchPos>(std::index_sequence<ArchPos...>) {
      ((results[ArchPos] = arch_results[ArchPos]), ...);
    }(ArchIs{});

    [this, entity, &results]<size_t... Is>(std::index_sequence<Is...>) {
      ((results[Is] = TryRemoveSparseComponent<TypeAt<Is, Ts...>>(entity)),
       ...);
    }(SparseIs{});

    if constexpr (sizeof...(Ts) == 1) {
      return results.front();
    } else {
      return results;
    }
  }
}

template <ComponentTrait T>
inline T& ComponentManager::Get(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  T* component = nullptr;
  if constexpr (SparseComponentTrait<T>) {
    auto* entry = sparse_storages_.template TryGet<T>();
    HELIOS_ASSERT(entry != nullptr,
                  "Sparse storage for component '{}' does not exist!",
                  ComponentNameOf<T>());
    component = entry->template As<T>().TryGet(entity);
  } else {
    const auto it = entity_archetype_.find(entity.Index());
    HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                  entity);
    component = it->second.get().template TryGet<T>(entity);
  }

  HELIOS_ASSERT(component != nullptr,
                "Entity '{}' does not have component '{}'!", entity,
                ComponentNameOf<T>());
  return *component;
}

template <ComponentTrait T>
inline const T& ComponentManager::Get(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  const T* component = nullptr;
  if constexpr (SparseComponentTrait<T>) {
    auto* entry = sparse_storages_.template TryGet<T>();
    HELIOS_ASSERT(entry != nullptr,
                  "Sparse storage for component '{}' does not exist!",
                  ComponentNameOf<T>());
    component = entry->template As<T>().TryGet(entity);
  } else {
    const auto it = entity_archetype_.find(entity.Index());
    HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                  entity);
    component = it->second.get().template TryGet<T>(entity);
  }

  HELIOS_ASSERT(component != nullptr,
                "Entity '{}' does not have component '{}'!", entity,
                ComponentNameOf<T>());
  return *component;
}

template <ComponentTrait T>
inline T* ComponentManager::TryGet(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if constexpr (SparseComponentTrait<T>) {
    auto* entry = sparse_storages_.template TryGet<T>();
    if (entry == nullptr) {
      return nullptr;
    }
    return entry->template As<T>().TryGet(entity);
  } else {
    const auto it = entity_archetype_.find(entity.Index());
    if (it == entity_archetype_.end()) {
      return nullptr;
    }
    return it->second.get().template TryGet<T>(entity);
  }
}

template <ComponentTrait T>
inline const T* ComponentManager::TryGet(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if constexpr (SparseComponentTrait<T>) {
    const auto* entry = sparse_storages_.template TryGet<T>();
    if (entry == nullptr) {
      return nullptr;
    }
    return entry->template As<T>().TryGet(entity);
  } else {
    const auto it = entity_archetype_.find(entity.Index());
    if (it == entity_archetype_.end()) {
      return nullptr;
    }
    return it->second.get().template TryGet<T>(entity);
  }
}

template <ComponentTrait T>
inline bool ComponentManager::Registered() const noexcept {
  constexpr auto type_index = ComponentTypeIndex::From<T>();
  return metadata_.find(type_index) != metadata_.end();
}

inline bool ComponentManager::Tracked(Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return entity_archetype_.contains(entity.Index());
}

template <ComponentTrait T>
inline bool ComponentManager::Has(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if constexpr (SparseComponentTrait<T>) {
    const auto* entry = sparse_storages_.template TryGet<T>();
    if (entry == nullptr) {
      return false;
    }
    return entry->template As<T>().Contains(entity);
  } else {
    const auto it = entity_archetype_.find(entity.Index());
    if (it == entity_archetype_.end()) {
      return false;
    }
    return it->second.get().template HasColumn<T>();
  }
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 1)
inline auto ComponentManager::Has(Entity entity) const
    -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return {Has<Ts>(entity)...};
}

inline Archetype& ComponentManager::EntityArchetype(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);
  return it->second.get();
}

inline const Archetype& ComponentManager::EntityArchetype(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto it = entity_archetype_.find(entity.Index());
  HELIOS_ASSERT(it != entity_archetype_.end(), "Entity '{}' not tracked!",
                entity);
  return it->second.get();
}

template <SparseComponentTrait T>
inline auto ComponentManager::SparseStorage() const
    -> const SparseComponentStorage<T>& {
  const auto* entry = sparse_storages_.template TryGet<T>();
  HELIOS_ASSERT(entry != nullptr, "Sparse storage for '{}' does not exist!",
                ComponentNameOf<T>());
  return entry->template As<T>();
}

template <ComponentTrait T>
inline const ComponentMetadata& ComponentManager::Metadata() const noexcept {
  constexpr auto type_index = ComponentTypeIndex::From<T>();
  const auto it = metadata_.find(type_index);
  HELIOS_ASSERT(it != metadata_.end(), "Component '{}' is not registered!",
                ComponentNameOf<T>());
  return it->second;
}

inline const ComponentMetadata* ComponentManager::MetadataByIndex(
    ComponentTypeIndex type_index) const noexcept {
  const auto it = metadata_.find(type_index);
  if (it == metadata_.end()) {
    return nullptr;
  }
  return &it->second;
}

template <ComponentTrait T>
inline void ComponentManager::EnsureRegistered() {
  constexpr auto type_index = ComponentTypeIndex::From<T>();

  if (metadata_.find(type_index) == metadata_.end()) {
    metadata_.emplace(type_index, ComponentMetadata::From<T>());
  }
}

template <template <typename> typename Pred, typename... Ts>
consteval auto ComponentManager::FilteredIndicesArray() noexcept {
  // Predicate results as a bool array.
  constexpr std::array mask = {Pred<std::remove_cvref_t<Ts>>::value...};
  constexpr size_t count =
      (0 + ... + (Pred<std::remove_cvref_t<Ts>>::value ? 1 : 0));

  std::array<size_t, count> out = {};
  size_t j = 0;
  for (size_t i = 0; i < sizeof...(Ts); ++i) {
    if (mask[i]) {
      out[j++] = i;
    }
  }
  return out;
}

template <typename... Ts, size_t... ArchIs>
inline void ComponentManager::DispatchAddArchetype(
    Entity entity, std::tuple<Ts...>& args,
    std::index_sequence<ArchIs...> /*seq*/) {
  AddArchetypeComponents(entity, std::forward<FwdTypeAt<ArchIs, Ts...>>(
                                     std::get<ArchIs>(args))...);
}

template <typename... Ts, size_t... ArchIs>
inline auto ComponentManager::DispatchTryAddArchetype(
    Entity entity, std::tuple<Ts...>& args,
    std::index_sequence<ArchIs...> /*seq*/)
    -> std::array<bool, sizeof...(ArchIs)> {
  return TryAddArchetypeComponents(
      entity,
      std::forward<FwdTypeAt<ArchIs, Ts...>>(std::get<ArchIs>(args))...);
}

template <typename... Ts, size_t... ArchIs>
inline void ComponentManager::DispatchRemoveArchetype(
    Entity entity, std::index_sequence<ArchIs...> /*seq*/) {
  RemoveArchetypeComponents<TypeAt<ArchIs, Ts...>...>(entity);
}

template <typename... Ts, size_t... ArchIs>
inline auto ComponentManager::DispatchTryRemoveArchetype(
    Entity entity, std::index_sequence<ArchIs...> /*seq*/)
    -> std::array<bool, sizeof...(ArchIs)> {
  return TryRemoveArchetypeComponents<TypeAt<ArchIs, Ts...>...>(entity);
}

template <SparseComponentTrait T>
inline auto ComponentManager::EnsureSparseStorage()
    -> SparseComponentStorage<T>& {
  EnsureRegistered<T>();

  if (auto* entry = sparse_storages_.template TryGet<T>(); entry != nullptr) {
    return entry->template As<T>();
  }

  // Create and insert new entry keyed by the component type index.
  auto& new_entry = sparse_storages_.template Ensure<T>();
  new_entry = SparseStorageEntry::From<T>();
  return new_entry.template As<T>();
}

inline Archetype& ComponentManager::GetOrCreateArchetype(
    const ArchetypeId& id) {
  HELIOS_ECS_PROFILE_SCOPE_N(
      "helios::ecs::ComponentManager::GetOrCreateArchetype");
  HELIOS_ECS_PROFILE_ZONE_VALUE(id.Types().size());

  const size_t hash = id.Hash();
  if (const auto it = archetype_map_.find(hash); it != archetype_map_.end()) {
    return it->second.archetype.get();
  }

  // Create archetype in the stable deque storage.
  archetype_storage_.emplace_back(id);
  Archetype& archetype = archetype_storage_.back();

  // Create record referencing the archetype.
  archetype_map_.emplace(hash, ArchetypeRecord{.archetype = std::ref(archetype),
                                               .add_edges = {},
                                               .remove_edges = {}});

  // Initialize columns with type info from metadata.
  InitArchetypeColumns(archetype);
  archetype_list_.emplace_back(archetype);
  ++structural_version_;
  return archetype;
}

inline void ComponentManager::InitArchetypeColumns(Archetype& archetype) {
  for (const auto type_index : archetype.Id().Types()) {
    const auto meta_it = metadata_.find(type_index);
    if (meta_it == metadata_.end()) {
      continue;
    }

    auto* col = archetype.TryColumn(type_index);
    if (col != nullptr && meta_it->second.init_column != nullptr) {
      meta_it->second.init_column(*col);
    }
  }
}

inline void ComponentManager::MigrateEntity(Entity entity, Archetype& src,
                                            Archetype& dst) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::ComponentManager::MigrateEntity");
  HELIOS_ECS_PROFILE_ZONE_VALUE(src.EntityCount());
  HELIOS_ECS_PROFILE_ZONE_VALUE(dst.Id().Types().size());

  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if (&src == &dst) [[unlikely]] {
    return;
  }

  const auto src_row = src.Row(entity);

  // Phase 1: Allocate a row in the destination archetype for this entity.
  [[maybe_unused]] const auto dst_row = dst.AllocateRow(entity);

  // Phase 2: For each column in dst, either move data from src or push a
  // default placeholder.
  for (const auto type_index : dst.Id().Types()) {
    auto* dst_col = dst.TryColumn(type_index);
    if (dst_col == nullptr) {
      continue;
    }

    const auto meta_it = metadata_.find(type_index);
    HELIOS_ASSERT(meta_it != metadata_.end(),
                  "Component metadata not registered for a column in target "
                  "archetype!");

    if (src.HasColumn(type_index)) {
      // Shared column: move the element from src[src_row] into dst (appended
      // at back).
      auto* src_col = src.TryColumn(type_index);
      if (src_col != nullptr && !src_col->Empty() &&
          meta_it->second.move_column_element != nullptr) {
        meta_it->second.move_column_element(*dst_col, *src_col,
                                            static_cast<size_t>(src_row));
      }
    } else {
      // New column in dst that src doesn't have.
      // Push a default-constructed placeholder; the caller will overwrite it
      // via Set.
      if (meta_it->second.default_push != nullptr) {
        meta_it->second.default_push(*dst_col);
      }
    }
  }

  // Phase 3: Remove the entity from the source archetype using swap-and-pop.
  const Entity swapped = src.Remove(entity);

  // Phase 4: Update entity-to-archetype mapping.
  entity_archetype_.insert_or_assign(entity.Index(), std::ref(dst));
  if (swapped.Valid()) {
    entity_archetype_.insert_or_assign(swapped.Index(), std::ref(src));
  }

  ++structural_version_;
}

inline Archetype* ComponentManager::TryGetAddEdge(Archetype& from,
                                                  ComponentTypeIndex type) {
  auto& record = GetRecord(from);
  const auto it = record.add_edges.find(type);
  return (it != record.add_edges.end()) ? &it->second.get() : nullptr;
}

inline Archetype* ComponentManager::TryGetRemoveEdge(Archetype& from,
                                                     ComponentTypeIndex type) {
  auto& record = GetRecord(from);
  const auto it = record.remove_edges.find(type);
  return (it != record.remove_edges.end()) ? &it->second.get() : nullptr;
}

inline auto ComponentManager::GetRecord(Archetype& archetype)
    -> ArchetypeRecord& {
  const size_t hash = archetype.Id().Hash();
  const auto it = archetype_map_.find(hash);
  HELIOS_ASSERT(it != archetype_map_.end(), "Archetype record not found!");
  return it->second;
}

inline auto ComponentManager::GetRecord(const Archetype& archetype) const
    -> const ArchetypeRecord& {
  const size_t hash = archetype.Id().Hash();
  const auto it = archetype_map_.find(hash);
  HELIOS_ASSERT(it != archetype_map_.end(), "Archetype record not found!");
  return it->second;
}

}  // namespace helios::ecs
