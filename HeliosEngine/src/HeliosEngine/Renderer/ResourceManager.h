#pragma once

#include "RenderQueue.h"

#include "EntityComponentSystem/Components.h"

namespace Helios
{
  class RenderQueue;

  class ResourceManager
  {
  public:
    ResourceManager() = default;
    virtual ~ResourceManager() = default;

    virtual void InitializeResources(const std::vector<const Renderable*>& renderables) = 0;
    virtual void FreeResources(const std::vector<const Renderable*>& renderables) = 0;
    virtual void UpdateResources(entt::registry& ecs, const RenderQueue& renderQueue) = 0;
    virtual void ClearResources() = 0;

    virtual inline std::unique_ptr<ResourceManager> Clone() const = 0;

    static std::unique_ptr<ResourceManager> Create();
  };
}