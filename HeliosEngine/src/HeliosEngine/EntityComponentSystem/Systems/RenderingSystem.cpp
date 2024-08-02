#include "EntityComponentSystem/Systems/RenderingSystem.h"
#include "EntityComponentSystem/Systems/SystemImpl.h"

namespace Helios
{
  RenderingSystem::RenderingSystem()
    : m_GraphicsContext(GraphicsContext::Get()), m_ResourceManager(std::move(ResourceManager::Create()))
  {
  }

  RenderingSystem::RenderingSystem(const RenderingSystem& other)
    : m_GraphicsContext(other.m_GraphicsContext)
  {
  }

  void RenderingSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
    m_RenderQueue.Clear();
    CollectRenderables(ecs);

    m_ResourceManager->UpdateResources(m_RenderQueue);

    m_GraphicsContext->BeginFrame();
    m_GraphicsContext->Record(m_RenderQueue);
    m_GraphicsContext->EndFrame();
  }

  void RenderingSystem::OnEvent(ECSManager& ecs, Event& event)
  {
  }

  void RenderingSystem::CollectRenderables(ECSManager& ecs)
  {
    auto& entities = ecs.GetEntitiesWithComponents(
      GetRequiredComponents<Renderable>(ecs)
    );

    for (const auto entity : entities) {
      m_RenderQueue.AddRenderable(ecs.GetComponent<Renderable>(entity));
    }
  }
}