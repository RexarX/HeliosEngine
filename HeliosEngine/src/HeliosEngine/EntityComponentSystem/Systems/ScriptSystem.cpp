#include "EntityComponentSystem/Systems/ScriptSystem.h"
#include "EntityComponentSystem/Systems/SystemImpl.h"

#include "EntityComponentSystem/Components/Script.h"

namespace Helios
{
  void ScriptSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
    auto& entities = ecs.GetEntitiesWithComponents(
      GetRequiredComponents<Script>(ecs)
    );

    for (const auto entity : entities) {
      auto& scriptComponent = ecs.GetComponent<Script>(entity);
      scriptComponent.OnUpdate(deltaTime);
    }
  }

  void ScriptSystem::OnEvent(ECSManager& ecs, Event& event)
  {
    auto& entities = ecs.GetEntitiesWithComponents(
      GetRequiredComponents<Script>(ecs)
    );

    for (const auto entity : entities) {
      auto& scriptComponent = ecs.GetComponent<Script>(entity);
      scriptComponent.OnEvent(event);
    }
  }
}