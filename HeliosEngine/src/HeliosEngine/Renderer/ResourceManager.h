#pragma once

#include "RenderQueue.h"

namespace Helios {
  class ResourceManager {
  public:
    enum class PipelineType {
      Regular,
      Wireframe
    };

    virtual ~ResourceManager() = default;

    virtual void InitializeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) = 0;
    virtual void FreeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) = 0;
    virtual void UpdateResources(const RenderQueue& renderables) = 0;
    virtual void ClearResources() = 0;

    virtual inline std::unique_ptr<ResourceManager> Clone() const = 0;

    static std::unique_ptr<ResourceManager> Create();
  };
}