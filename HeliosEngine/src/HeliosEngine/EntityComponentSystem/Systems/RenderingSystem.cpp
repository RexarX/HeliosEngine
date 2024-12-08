#include "RenderingSystem.h"

#include "Renderer/GraphicsContext.h"

namespace Helios {
  RenderingSystem::RenderingSystem()
    : m_GraphicsContext(GraphicsContext::Get()), m_PipelineManager(PipelineManager::Create()) {
  }

  RenderingSystem::RenderingSystem(const RenderingSystem& other)
    : m_GraphicsContext(other.m_GraphicsContext), m_PipelineManager(other.m_PipelineManager->Clone()) {
  }

  void RenderingSystem::OnUpdate(entt::registry& registry) {
    PROFILE_FUNCTION();

    FillRenderQueue(registry, m_RenderQueue);

    m_PipelineManager->UpdateResources(m_RenderQueue);
    m_GraphicsContext->Record(m_RenderQueue, *m_PipelineManager);

    m_RenderQueue.Clear();
  }

  RenderingSystem& RenderingSystem::operator=(const RenderingSystem& other) {
    if (this != &other) {
      m_GraphicsContext = other.m_GraphicsContext;
      m_PipelineManager = other.m_PipelineManager->Clone();
    }

    return *this;
  }

  void RenderingSystem::FillRenderQueue(entt::registry& registry, RenderQueue& renderQueue) {}
}