#pragma once

#include "Renderer/ResourceManager.h"

#include <entt/entt.hpp>

namespace Helios
{
  class GraphicsContext;
  class RenderQueue;

  class RenderingSystem
  {
  public:
    RenderingSystem();
    RenderingSystem(const RenderingSystem&);
    ~RenderingSystem() = default;

    void OnUpdate(entt::registry& registry);

    RenderingSystem& operator=(const RenderingSystem&);

    inline ResourceManager& GetResourceManager() { return *m_ResourceManager.get(); }

  private:
    void FillRenderQueue(entt::registry& registry, RenderQueue& renderQueue);

  private:
    std::shared_ptr<GraphicsContext> m_GraphicsContext = nullptr;
    std::unique_ptr<ResourceManager> m_ResourceManager = nullptr;
  };
}