#include "ScriptSystem.h"

#include "EntityComponentSystem/Components.h"

namespace Helios
{
  void ScriptSystem::OnUpdate(entt::registry& registry, Timestep deltaTime)
  {
    registry.view<Script>().each([deltaTime](entt::entity entity, Script& script) {
      script.OnUpdate(deltaTime);
    });
  }

  void ScriptSystem::OnEvent(entt::registry& registry, Event& event)
  {
    registry.view<Script>().each([&event](entt::entity entity, Script& script) {
      script.OnEvent(event);
    });
  }
}