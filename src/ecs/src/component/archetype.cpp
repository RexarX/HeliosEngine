#include <pch.hpp>

#include <helios/ecs/component/archetype.hpp>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>

#include <helios/assert.hpp>
#include <memory_resource>
#include <optional>
#include <utility>

namespace helios::ecs {

Archetype::Archetype(ArchetypeId id, std::pmr::memory_resource* resource)
    : id_(std::move(id)),
      column_map_(resource),
      entities_(resource),
      entity_to_row_(resource) {
  const auto types = id_.Types();
  columns_.reserve(types.size());
  for (size_type i = 0; i < types.size(); ++i) {
    columns_.emplace_back(resource);
    column_map_.Emplace(types[i], i);
  }
}

void Archetype::Clear() {
  for (auto& col : columns_) {
    col.Clear();
  }
  entities_.clear();
  entity_to_row_.Clear();
}

Entity Archetype::Remove(Entity entity) {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  HELIOS_ASSERT(Contains(entity), "Entity '{}' not present in archetype!",
                entity);

  const RowIndex row = entity_to_row_.At(entity.Index());
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
  entity_to_row_.Erase(entity.Index());

  for (auto& col : columns_) {
    if (!col.Empty()) {
      col.PopBack();
    }
  }

  return swapped_entity;
}

auto Archetype::ColumnIndex(ComponentTypeIndex index) const noexcept
    -> std::optional<size_type> {
  const auto it = column_map_.Find(index);
  if (it == column_map_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace helios::ecs
