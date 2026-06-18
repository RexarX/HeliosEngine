#pragma once

#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/container/typed_buffer_array.hpp>
#include <helios/ecs/component/archetype_id.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

namespace helios::ecs {

/**
 * @brief Stores entities and their component data in a Structure-of-Arrays
 * layout for a specific archetype.
 * @details An archetype represents a unique combination of component types.
 * All entities sharing the same set of archetype-storage components are stored
 * together for cache-friendly iteration.
 *
 * Internally maintains:
 * - A dense entity list for fast iteration
 * - One `TypedBufferArray` column per component type, indexed by
 * `ComponentTypeIndex`
 * - An entity-to-row mapping for O(1) lookup by entity index
 *
 * Removal uses swap-and-pop to maintain dense packing without leaving holes.
 *
 * Component data migration between archetypes is handled externally by
 * `ComponentManager`, which uses typed function pointers from
 * `ComponentMetadata` to avoid virtual calls and raw byte manipulation of
 * non-trivial types.
 *
 * @note Not thread-safe.
 */
class Archetype {
public:
  using size_type = size_t;
  using RowIndex = uint32_t;

  static constexpr RowIndex kInvalidRow = std::numeric_limits<RowIndex>::max();

  /**
   * @brief Constructs an archetype for the given archetype id.
   * @details Initializes one column per component type in the id.
   * Columns are created untyped; they will be typed on first insert via
   * `ComponentManager`.
   * @param id The archetype id defining which component types this archetype
   * stores
   */
  explicit Archetype(ArchetypeId id);
  Archetype(const Archetype&) = delete;
  Archetype(Archetype&&) noexcept = default;
  ~Archetype() = default;

  Archetype& operator=(const Archetype&) = delete;
  Archetype& operator=(Archetype&&) noexcept = default;

  /// @brief Removes all entities and component data from this archetype.
  void Clear();

  /**
   * @brief Allocates a new row for an entity without initializing component
   * data.
   * @details Appends the entity to the entity list and returns the row index.
   * The caller is responsible for pushing component data into each column
   * afterwards. This is the primary entry point used by `ComponentManager`
   * during entity migration.
   * @warning Triggers assertion if entity is invalid or already present.
   * @param entity Entity to allocate a row for
   * @return Row index of the newly allocated row
   */
  RowIndex AllocateRow(Entity entity);

  /**
   * @brief Adds an entity with multiple components.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity already exists in this archetype.
   * - Any component type is not part of this archetype.
   * @tparam Ts Component types, must be unique and at least 1 type
   * @param entity Entity to add
   * @param components Component values
   * @return Row index of the new entity
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  RowIndex Add(Entity entity, Ts&&... components);

  /**
   * @brief Removes an entity from this archetype using swap-and-pop.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not present in this archetype.
   * @param entity Entity to remove
   * @return The entity that was swapped into the removed row, or invalid entity
   * if the removed entity was last
   */
  Entity Remove(Entity entity);

  /**
   * @brief Sets (overwrites) a component value for an existing entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not present in this archetype.
   * - Component type is not part of this archetype.
   * @tparam T Component type
   * @param entity Entity to modify
   * @param component New component value
   */
  template <ComponentTrait T>
  void Set(Entity entity, T&& component);

  /**
   * @brief Emplaces (overwrites) a component for an existing entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not present in this archetype.
   * - Component type is not part of this archetype.
   * @tparam T Component type
   * @tparam Args Constructor argument types
   * @param entity Entity to modify
   * @param args Arguments forwarded to component constructor
   */
  template <ComponentTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Entity entity, Args&&... args);

  /**
   * @brief Gets a mutable reference to a component for an entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not present in this archetype.
   * - Component type is not part of this archetype.
   * @tparam T Component type
   * @param entity Entity to query
   * @return Mutable reference to component
   */
  template <ComponentTrait T>
  [[nodiscard]] T& Get(Entity entity);

  /**
   * @brief Gets a const reference to a component for an entity.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - Entity is not present in this archetype.
   * - Component type is not part of this archetype.
   * @tparam T Component type
   * @param entity Entity to query
   * @return Const reference to component
   */
  template <ComponentTrait T>
  [[nodiscard]] const T& Get(Entity entity) const;

  /**
   * @brief Tries to get a mutable pointer to a component for an entity.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type
   * @param entity Entity to query
   * @return Pointer to component or `nullptr` if entity or type not found
   */
  template <ComponentTrait T>
  [[nodiscard]] T* TryGet(Entity entity);

  /**
   * @brief Tries to get a const pointer to a component for an entity.
   * @warning Triggers assertion if entity is invalid.
   * @tparam T Component type
   * @param entity Entity to query
   * @return Const pointer to component or `nullptr` if entity or type not found
   */
  template <ComponentTrait T>
  [[nodiscard]] const T* TryGet(Entity entity) const;

  /**
   * @brief Gets a typed span over an entire component column.
   * @tparam T Component type
   * @return Span of all component values of this type
   */
  template <ComponentTrait T>
  [[nodiscard]] auto ComponentColumn() -> std::span<T>;

  /**
   * @brief Gets a const typed span over an entire component column.
   * @tparam T Component type
   * @return Const span of all component values of this type
   */
  template <ComponentTrait T>
  [[nodiscard]] auto ComponentColumn() const -> std::span<const T>;

  /**
   * @brief Gets a raw column by component type index.
   * @warning Triggers assertion if type index not found.
   * @param index Component type index
   * @return Reference to the column
   */
  [[nodiscard]] container::TypedBufferArray<>& Column(
      ComponentTypeIndex index) noexcept;

  /**
   * @brief Gets a const raw column by component type index.
   * @warning Triggers assertion if type index not found.
   * @param index Component type index
   * @return Const reference to the column
   */
  [[nodiscard]] const container::TypedBufferArray<>& Column(
      ComponentTypeIndex index) const noexcept;

  /**
   * @brief Tries to get a raw column by component type index.
   * @param index Component type index
   * @return Pointer to column or `nullptr` if type index not found
   */
  [[nodiscard]] container::TypedBufferArray<>* TryColumn(
      ComponentTypeIndex index) noexcept;

  /**
   * @brief Tries to get a const raw column by component type index.
   * @param index Component type index
   * @return Const pointer to column or `nullptr` if type index not found
   */
  [[nodiscard]] const container::TypedBufferArray<>* TryColumn(
      ComponentTypeIndex index) const noexcept;

  /**
   * @brief Gets the row index for an entity.
   * @warning Triggers assertion if entity is not present.
   * @param entity Entity to look up
   * @return Row index
   */
  [[nodiscard]] RowIndex Row(Entity entity) const;

  /**
   * @brief Gets the entity at a given row.
   * @warning Triggers assertion if row is out of bounds.
   * @param row Row index
   * @return Entity at the given row
   */
  [[nodiscard]] Entity EntityAt(RowIndex row) const;

  /**
   * @brief Returns true if no entities are stored.
   * @return True if the archetype is empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return entities_.empty(); }

  /**
   * @brief Checks if the archetype contains the given entity.
   * @param entity Entity to check
   * @return True if the entity is stored in this archetype, false otherwise
   */
  [[nodiscard]] bool Contains(Entity entity) const noexcept;

  /**
   * @brief Checks if this archetype has a column for the given component type.
   * @tparam T Component type
   * @return True if the archetype stores this component type, false otherwise
   */
  template <ComponentTrait T>
  [[nodiscard]] bool HasColumn() const noexcept {
    return HasColumn(ComponentTypeIndex::From<T>());
  }

  /**
   * @brief Checks if this archetype has a column for the given component index.
   * @param index Component type index
   * @return True if the archetype stores this component type, false otherwise
   */
  [[nodiscard]] bool HasColumn(ComponentTypeIndex index) const noexcept {
    return column_map_.contains(index);
  }

  /**
   * @brief Gets the number of entities stored in this archetype.
   * @return Entity count
   */
  [[nodiscard]] size_type EntityCount() const noexcept {
    return entities_.size();
  }

  /**
   * @brief Gets the number of component columns in this archetype.
   * @return Column count
   */
  [[nodiscard]] size_type ColumnCount() const noexcept {
    return columns_.size();
  }

  /**
   * @brief Gets a span of all entities stored in this archetype.
   * @return Const span of entities
   */
  [[nodiscard]] auto Entities() const noexcept -> std::span<const Entity> {
    return entities_;
  }

  /**
   * @brief Gets the archetype id.
   * @return ArchetypeId of this archetype
   */
  [[nodiscard]] const ArchetypeId& Id() const noexcept { return id_; }

private:
#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  using ColumnMap = std::flat_map<ComponentTypeIndex, size_type>;
  using EntityRowMap = std::flat_map<Entity::IndexType, RowIndex>;
#else
  using ColumnMap =
      boost::container::flat_map<ComponentTypeIndex, size_type, std::less<>>;
  using EntityRowMap = boost::container::flat_map<Entity::IndexType, RowIndex>;
#endif

  [[nodiscard]] auto ColumnIndex(ComponentTypeIndex index) const noexcept
      -> std::optional<size_type>;

  ArchetypeId id_;

  std::vector<container::TypedBufferArray<>>
      columns_;           ///< One column per component type.
  ColumnMap column_map_;  ///< Maps ComponentTypeIndex -> column vector index.

  std::vector<Entity> entities_;  ///< Dense entity array, indexed by row.

  EntityRowMap
      entity_to_row_;  ///< Maps entity index -> row. Sparse by entity index.
};

inline Archetype::Archetype(ArchetypeId id) : id_(std::move(id)) {
  const auto types = id_.Types();
  columns_.resize(types.size());
  for (size_type i = 0; i < types.size(); ++i) {
    column_map_.emplace(types[i], i);
  }
}

inline void Archetype::Clear() {
  for (auto& col : columns_) {
    col.Clear();
  }
  entities_.clear();
  entity_to_row_.clear();
}

inline auto Archetype::AllocateRow(Entity entity) -> RowIndex {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(!Contains(entity), "Entity '{}' already present in archetype!",
                entity);

  auto row = static_cast<RowIndex>(entities_.size());
  entities_.push_back(entity);
  entity_to_row_.emplace(entity.Index(), row);
  return row;
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto Archetype::Add(Entity entity, Ts&&... components) -> RowIndex {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

#ifdef HELIOS_ENABLE_ASSERTS
  const std::array<bool, sizeof...(Ts)> has_column = {
      HasColumn<std::remove_cvref_t<Ts>>()...};

  constexpr std::array<std::string_view, sizeof...(Ts)> names = {
      ComponentNameOf<std::remove_cvref_t<Ts>>()...};

  const bool all_has_column = std::ranges::all_of(has_column, std::identity{});
  if (!all_has_column) {
    std::string missing;
    for (size_t i = 0; i < sizeof...(Ts); ++i) {
      if (has_column[i]) {
        continue;
      }
      if (!missing.empty()) {
        missing.append(", ");
      }
      missing.append(names[i]);
    }
    HELIOS_ASSERT(false, "Components '{}' are not part of this archetype!",
                  missing);
  }
#endif

  auto row = AllocateRow(entity);

  // Push each component into its respective column.
  (columns_[column_map_.at(ComponentTypeIndex::From<std::remove_cvref_t<Ts>>())]
       .PushBack(std::forward<Ts>(components)),
   ...);

  return row;
}

inline Entity Archetype::Remove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);

  const RowIndex row = entity_to_row_.at(entity.Index());
  const auto last_row = static_cast<RowIndex>(entities_.size() - 1);
  Entity swapped_entity;

  if (row != last_row) {
    swapped_entity = entities_[last_row];
    // Swap entity in dense list.
    entities_[row] = swapped_entity;
    entity_to_row_[swapped_entity.Index()] = row;

    // Swap component data in every column.
    for (auto& col : columns_) {
      if (!col.Empty()) {
        col.Swap(static_cast<size_type>(row), static_cast<size_type>(last_row));
      }
    }
  }

  // Pop back.
  entities_.pop_back();
  entity_to_row_.erase(entity.Index());

  for (auto& col : columns_) {
    if (!col.Empty()) {
      col.PopBack();
    }
  }

  return swapped_entity;
}

template <ComponentTrait T>
inline void Archetype::Set(Entity entity, T&& component) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);

  using DecayedT = std::remove_cvref_t<T>;
  constexpr auto type_index = ComponentTypeIndex::From<DecayedT>();

  HELIOS_ASSERT(HasColumn(type_index),
                "Component '{}' is not part of this archetype!",
                ComponentNameOf<DecayedT>());

  auto& col = columns_[column_map_.at(type_index)];
  const size_type row =
      static_cast<size_type>(entity_to_row_.at(entity.Index()));

  HELIOS_ASSERT(row <= col.Size(),
                "Inconsistent archetype state for component '{}': row '{}' is "
                "out of bounds for column size '{}'!",
                ComponentNameOf<DecayedT>(), row, col.Size());

  if (row == col.Size()) {
    col.PushBack(std::forward<T>(component));
    return;
  }

  col.template At<DecayedT>(row) = std::forward<T>(component);
}

template <ComponentTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void Archetype::Emplace(Entity entity, Args&&... args) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);

  using DecayedT = std::remove_cvref_t<T>;
  constexpr auto type_index = ComponentTypeIndex::From<DecayedT>();

  HELIOS_ASSERT(HasColumn(type_index),
                "Component '{}' is not part of this archetype!",
                ComponentNameOf<DecayedT>());

  auto& col = columns_[column_map_.at(type_index)];
  const size_type row =
      static_cast<size_type>(entity_to_row_.at(entity.Index()));

  HELIOS_ASSERT(row <= col.Size(),
                "Inconsistent archetype state for component '{}': row '{}' is "
                "out of bounds for column size '{}'!",
                ComponentNameOf<DecayedT>(), row, col.Size());

  if (row == col.Size()) {
    col.template EmplaceBack<DecayedT>(std::forward<Args>(args)...);
    return;
  }

  auto& slot = col.template At<DecayedT>(row);

  std::destroy_at(&slot);
  std::construct_at(&slot, std::forward<Args>(args)...);
}

template <ComponentTrait T>
inline T& Archetype::Get(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);

  constexpr auto type_index = ComponentTypeIndex::From<T>();
  HELIOS_ASSERT(HasColumn(type_index),
                "Component '{}' is not part of this archetype!",
                ComponentNameOf<T>());

  auto& col = columns_[column_map_.at(type_index)];
  const RowIndex row = entity_to_row_.at(entity.Index());
  return col.template At<T>(static_cast<size_type>(row));
}

template <ComponentTrait T>
inline const T& Archetype::Get(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);

  constexpr auto type_index = ComponentTypeIndex::From<T>();
  HELIOS_ASSERT(HasColumn(type_index),
                "Component '{}' is not part of this archetype!",
                ComponentNameOf<T>());

  const auto& col = columns_[column_map_.at(type_index)];
  const RowIndex row = entity_to_row_.at(entity.Index());
  return col.template At<T>(static_cast<size_type>(row));
}

template <ComponentTrait T>
inline T* Archetype::TryGet(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if (!Contains(entity)) {
    return nullptr;
  }

  constexpr auto type_index = ComponentTypeIndex::From<T>();
  const auto col_idx = ColumnIndex(type_index);
  if (!col_idx.has_value()) {
    return nullptr;
  }

  const size_type col_index = col_idx.value();
  const RowIndex row = entity_to_row_.at(entity.Index());
  return &columns_[col_index].template At<T>(static_cast<size_type>(row));
}

template <ComponentTrait T>
inline const T* Archetype::TryGet(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);

  if (!Contains(entity)) {
    return nullptr;
  }

  constexpr auto type_index = ComponentTypeIndex::From<T>();
  const auto col_idx = ColumnIndex(type_index);
  if (!col_idx.has_value()) {
    return nullptr;
  }

  const size_type col_index = col_idx.value();
  const RowIndex row = entity_to_row_.at(entity.Index());
  return &columns_[col_index].template At<T>(static_cast<size_type>(row));
}

template <ComponentTrait T>
inline auto Archetype::ComponentColumn() -> std::span<T> {
  constexpr auto type_index = ComponentTypeIndex::From<T>();

  const auto col_idx = ColumnIndex(type_index);
  if (!col_idx.has_value()) {
    return {};
  }

  auto& col = columns_[col_idx.value()];
  return col.template Data<T>();
}

template <ComponentTrait T>
inline auto Archetype::ComponentColumn() const -> std::span<const T> {
  constexpr auto type_index = ComponentTypeIndex::From<T>();

  const auto col_idx = ColumnIndex(type_index);
  if (!col_idx.has_value()) {
    return {};
  }

  const auto& col = columns_[col_idx.value()];
  return col.template Data<T>();
}

inline auto Archetype::Column(ComponentTypeIndex index) noexcept
    -> container::TypedBufferArray<>& {
  const auto col_idx = ColumnIndex(index);
  HELIOS_ASSERT(col_idx.has_value(),
                "Component index '{}' is not part of this archetype!",
                static_cast<size_t>(index));
  return columns_[col_idx.value()];
}

inline auto Archetype::Column(ComponentTypeIndex index) const noexcept
    -> const container::TypedBufferArray<>& {
  const auto col_idx = ColumnIndex(index);
  HELIOS_ASSERT(col_idx.has_value(),
                "Component index '{}' is not part of this archetype!",
                static_cast<size_t>(index));
  return columns_[col_idx.value()];
}

inline auto Archetype::TryColumn(ComponentTypeIndex index) noexcept
    -> container::TypedBufferArray<>* {
  const auto col_idx = ColumnIndex(index);
  if (!col_idx.has_value()) {
    return nullptr;
  }
  return &columns_[col_idx.value()];
}

inline auto Archetype::TryColumn(ComponentTypeIndex index) const noexcept
    -> const container::TypedBufferArray<>* {
  const auto col_idx = ColumnIndex(index);
  if (!col_idx.has_value()) {
    return nullptr;
  }
  return &columns_[col_idx.value()];
}

inline auto Archetype::Row(Entity entity) const -> RowIndex {
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);
  return entity_to_row_.at(entity.Index());
}

inline Entity Archetype::EntityAt(RowIndex row) const {
  HELIOS_ASSERT(static_cast<size_type>(row) < entities_.size(),
                "Row index '{}' out of bounds (size: {})!",
                static_cast<size_type>(row), entities_.size());
  return entities_[static_cast<size_type>(row)];
}

inline bool Archetype::Contains(Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  return entity_to_row_.contains(entity.Index());
}

inline auto Archetype::ColumnIndex(ComponentTypeIndex index) const noexcept
    -> std::optional<size_type> {
  const auto it = column_map_.find(index);
  if (it == column_map_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace helios::ecs
