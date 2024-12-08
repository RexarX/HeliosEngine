#pragma once

#include "Renderer/PipelineManager.h"

#include <entt/entt.hpp>

namespace Helios {
  class GraphicsContext;
  class RenderQueue;

  class RenderingSystem {
  public:
    RenderingSystem();
    RenderingSystem(const RenderingSystem&);
    ~RenderingSystem() = default;

    void OnUpdate(entt::registry& registry);

    RenderingSystem& operator=(const RenderingSystem&);

    inline PipelineManager& GetPipelineManager() { return *m_PipelineManager.get(); }

  private:
    static void FillRenderQueue(entt::registry& registry, RenderQueue& renderQueue);

  private:
    std::shared_ptr<GraphicsContext> m_GraphicsContext = nullptr;
    std::unique_ptr<PipelineManager> m_PipelineManager = nullptr;

    RenderQueue m_RenderQueue;
  };
}