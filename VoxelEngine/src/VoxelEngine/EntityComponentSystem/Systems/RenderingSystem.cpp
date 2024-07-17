#include "EntityComponentSystem/Systems/RenderingSystem.h"

namespace Engine
{
  void RenderingSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
    m_GraphicsContext->BeginFrame();

    //logic

    m_GraphicsContext->EndFrame();
  }

  void RenderingSystem::OnEvent(ECSManager& ecs, Event& event)
  {
  }
}