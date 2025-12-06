#include <helios/core/ecs/world.hpp>

#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/entity.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace helios::ecs {

void World::UpdateEntityArchetype(Entity entity) {
  const auto component_infos = components_.GetComponentTypes(entity);

  // Use a stack buffer for small counts
  constexpr size_t kStackSize = 16;
  if (component_infos.size() <= kStackSize) {
    std::array<ComponentTypeId, kStackSize> type_ids = {};
    for (size_t i = 0; i < component_infos.size(); ++i) {
      type_ids[i] = component_infos[i].TypeId();
    }
    archetypes_.UpdateEntityArchetype(entity, std::span(type_ids.data(), component_infos.size()));
  } else {
    // Fallback for large counts
    std::vector<ComponentTypeId> type_ids;
    type_ids.reserve(component_infos.size());
    for (const auto& info : component_infos) {
      type_ids.push_back(info.TypeId());
    }
    archetypes_.UpdateEntityArchetype(entity, type_ids);
  }
}

}  // namespace helios::ecs
