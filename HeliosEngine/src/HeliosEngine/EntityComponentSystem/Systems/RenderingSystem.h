#pragma once

#include "Renderer/GraphicsContext.h"
#include "Renderer/ResourceManager.h"

#include <entt/entt.hpp>

namespace Helios
{
  class RenderingSystem
  {
  public:
    RenderingSystem();
    RenderingSystem(const RenderingSystem&);
    ~RenderingSystem() = default;

    void OnUpdate(entt::registry& registry);
    void Draw();

    RenderingSystem& operator=(const RenderingSystem&);

    inline std::unique_ptr<ResourceManager>& GetResourceManager() { return m_ResourceManager; }

  private:
    void FillRenderQueue(entt::registry& registry);

  private:
    std::shared_ptr<GraphicsContext> m_GraphicsContext = nullptr;
    std::unique_ptr<ResourceManager> m_ResourceManager = nullptr;

    RenderQueue m_RenderQueue;
  };
}