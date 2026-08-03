#include <pch.hpp>

#include <helios/ecs/component/manager.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/component/archetype.hpp>
#include <helios/ecs/component/archetype_id.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/entity/entity.hpp>

#include <cstddef>
#include <functional>

namespace helios::ecs {

void ComponentManager::Clear() {
  entity_archetype_.clear();
  archetype_map_.clear();
  archetype_list_.clear();
  archetype_storage_.clear();
  sparse_storages_.ResetAll();
  metadata_.clear();
  empty_archetype_ = nullptr;
  structural_version_ = 0;
}

void ComponentManager::ClearData() noexcept {
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

void ComponentManager::RemoveEntity(Entity entity) {
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

bool ComponentManager::TryRemoveEntity(Entity entity) {
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

void ComponentManager::Clear(Entity entity) {
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

Archetype& ComponentManager::GetOrCreateArchetype(const ArchetypeId& id) {
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

void ComponentManager::InitArchetypeColumns(Archetype& archetype) {
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

void ComponentManager::MigrateEntity(Entity entity, Archetype& src,
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

Archetype* ComponentManager::TryGetAddEdge(Archetype& from,
                                           ComponentTypeIndex type) {
  auto& record = GetRecord(from);
  const auto it = record.add_edges.find(type);
  return (it != record.add_edges.end()) ? &it->second.get() : nullptr;
}

Archetype* ComponentManager::TryGetRemoveEdge(Archetype& from,
                                              ComponentTypeIndex type) {
  auto& record = GetRecord(from);
  const auto it = record.remove_edges.find(type);
  return (it != record.remove_edges.end()) ? &it->second.get() : nullptr;
}

auto ComponentManager::GetRecord(Archetype& archetype) -> ArchetypeRecord& {
  const size_t hash = archetype.Id().Hash();
  const auto it = archetype_map_.find(hash);
  HELIOS_ASSERT(it != archetype_map_.end(), "Archetype record not found!");
  return it->second;
}

auto ComponentManager::GetRecord(const Archetype& archetype) const
    -> const ArchetypeRecord& {
  const size_t hash = archetype.Id().Hash();
  const auto it = archetype_map_.find(hash);
  HELIOS_ASSERT(it != archetype_map_.end(), "Archetype record not found!");
  return it->second;
}

}  // namespace helios::ecs
