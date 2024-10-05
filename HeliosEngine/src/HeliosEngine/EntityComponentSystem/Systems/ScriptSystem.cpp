#include "ScriptSystem.h"

#include "EntityComponentSystem/Components.h"

namespace Helios
{
  void ScriptSystem::OnUpdate(entt::registry& registry, Timestep deltaTime)
  {
    registry.view<Script>().each([deltaTime](entt::entity entity, Script& script) {
      script.scriptable->OnUpdate(deltaTime);
    });
  }

  void ScriptSystem::OnEvent(entt::registry& registry, const Event& event)
  {
    registry.view<Script>().each([&event](entt::entity entity, Script& script) {
      script.scriptable->OnEvent(event);
    });
  }
}