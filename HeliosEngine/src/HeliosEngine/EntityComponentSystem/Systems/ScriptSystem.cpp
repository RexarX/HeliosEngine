#include "ScriptSystem.h"

#include "EntityComponentSystem/Components.h"

namespace Helios
{
  void ScriptSystem::OnUpdate(entt::registry& registry, Timestep deltaTime)
  {
    const auto& view = registry.view<Script>();
    for (entt::entity entity : view) {
      registry.get<Script>(entity).OnUpdate(deltaTime);
    }
  }

  void ScriptSystem::OnEvent(entt::registry& registry, Event& event)
  {
    const auto& view = registry.view<Script>();
    for (entt::entity entity : view) {
      registry.get<Script>(entity).OnEvent(event);
    }
  }
}