#include "RenderingSystem.h"

#include "Renderer/GraphicsContext.h"

namespace Helios
{
  RenderingSystem::RenderingSystem()
    : m_GraphicsContext(GraphicsContext::Get()), m_ResourceManager(ResourceManager::Create())
  {
  }

  RenderingSystem::RenderingSystem(const RenderingSystem& other)
    : m_GraphicsContext(other.m_GraphicsContext),
    m_ResourceManager(other.m_ResourceManager->Clone())
  {
  }

  void RenderingSystem::OnUpdate(const entt::registry& registry)
  {
    RenderQueue renderQueue;
    FillRenderQueue(registry, renderQueue);

    m_ResourceManager->UpdateResources(renderQueue);
    m_GraphicsContext->Record(renderQueue, *m_ResourceManager);
  }

  RenderingSystem& RenderingSystem::operator=(const RenderingSystem& other)
  {
    if (this != &other) {
      m_GraphicsContext = other.m_GraphicsContext;
      m_ResourceManager = other.m_ResourceManager->Clone();
    }

    return *this;
  }

  void RenderingSystem::FillRenderQueue(const entt::registry& registry, RenderQueue& renderQueue)
  {
  }
}