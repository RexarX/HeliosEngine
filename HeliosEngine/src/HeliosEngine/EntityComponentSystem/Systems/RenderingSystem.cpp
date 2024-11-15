#include "RenderingSystem.h"

#include "Renderer/GraphicsContext.h"

namespace Helios {
  RenderingSystem::RenderingSystem()
    : m_GraphicsContext(GraphicsContext::Get()), m_ResourceManager(ResourceManager::Create()) {
  }

  RenderingSystem::RenderingSystem(const RenderingSystem& other)
    : m_GraphicsContext(other.m_GraphicsContext), m_ResourceManager(other.m_ResourceManager->Clone()) {
  }

  void RenderingSystem::OnUpdate(entt::registry& registry) {
    PROFILE_FUNCTION();

    FillRenderQueue(registry, m_RenderQueue);

    m_ResourceManager->UpdateResources(m_RenderQueue);
    m_GraphicsContext->Record(m_RenderQueue, *m_ResourceManager);

    m_RenderQueue.Clear();
  }

  RenderingSystem& RenderingSystem::operator=(const RenderingSystem& other) {
    if (this != &other) {
      m_GraphicsContext = other.m_GraphicsContext;
      m_ResourceManager = other.m_ResourceManager->Clone();
    }

    return *this;
  }

  void RenderingSystem::FillRenderQueue(entt::registry& registry, RenderQueue& renderQueue) {}
}