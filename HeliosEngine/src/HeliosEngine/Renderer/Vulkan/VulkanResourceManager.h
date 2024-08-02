#pragma once

#include "Renderer/ResourceManager.h"

#include "Renderer/Vulkan/VulkanContext.h"

namespace Helios
{
  class VulkanResourceManager : public ResourceManager
  {
  public:
    VulkanResourceManager();
    virtual ~VulkanResourceManager();

    void InitializeResources(const std::vector<Renderable>& renderables) override;
    void UpdateResources(const RenderQueue& renderQueue) override;
    void ClearResources() override;

    inline std::unique_ptr<ResourceManager> Clone() const override {
      return std::make_unique<VulkanResourceManager>(*this);
    }

  private:
    // Methods

  private:
    VulkanContext& m_Context;
  };
}