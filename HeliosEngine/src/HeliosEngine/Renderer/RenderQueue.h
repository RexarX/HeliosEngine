#pragma once

#include "EntityComponentSystem/Components/Renderable.h"

namespace Helios
{
  class RenderQueue
  {
  public:
    RenderQueue() = default;
    ~RenderQueue() = default;

    void Clear() { m_Renderables.clear(); }
    void AddRenderable(const Renderable& renderable) { m_Renderables.push_back(renderable); }

    inline const std::vector<Renderable>& GetRenderables() const { return m_Renderables; }

  private:
    std::vector<Renderable> m_Renderables;
  };
}