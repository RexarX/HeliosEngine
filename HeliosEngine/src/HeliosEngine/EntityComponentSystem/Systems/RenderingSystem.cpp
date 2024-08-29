#include "RenderingSystem.h"

namespace Helios
{
  RenderingSystem::RenderingSystem()
    : m_GraphicsContext(GraphicsContext::Get()), m_ResourceManager(ResourceManager::Create())
  {
  }

  RenderingSystem::RenderingSystem(const RenderingSystem& other)
    : m_GraphicsContext(other.m_GraphicsContext),
    m_ResourceManager(other.m_ResourceManager->Clone()), m_RenderQueue(other.m_RenderQueue)
  {
  }

  void RenderingSystem::OnUpdate(entt::registry& registry)
  {
    m_RenderQueue.Clear();
    FillRenderQueue(registry);

    m_ResourceManager->UpdateResources(registry, m_RenderQueue);
  }

  void RenderingSystem::Draw()
  {
    m_GraphicsContext->Record(m_RenderQueue, *m_ResourceManager);
  }

  RenderingSystem& RenderingSystem::operator=(const RenderingSystem& other)
  {
    if (this != &other) {
      m_GraphicsContext = other.m_GraphicsContext;
      m_ResourceManager = other.m_ResourceManager->Clone();
      m_RenderQueue = other.m_RenderQueue;
    }

    return *this;
  }

  void RenderingSystem::FillRenderQueue(entt::registry& registry)
  {
  }
}