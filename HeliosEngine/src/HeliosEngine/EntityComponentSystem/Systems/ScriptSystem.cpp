#include "ScriptSystem.h"

#include "EntityComponentSystem/Components.h"

namespace Helios {
  void ScriptSystem::OnUpdate(entt::registry& registry, Timestep deltaTime) {
    PROFILE_FUNCTION();

    registry.view<Script>().each([deltaTime](entt::entity entity, const Script& script) {
      script.scriptable->OnUpdate(deltaTime);
    });
  }

  void ScriptSystem::OnEvent(entt::registry& registry, Event& event) {
    PROFILE_FUNCTION();

    registry.view<Script>().each([&event](entt::entity entity, const Script& script) {
      script.scriptable->OnEvent(event);
    });
  }
}