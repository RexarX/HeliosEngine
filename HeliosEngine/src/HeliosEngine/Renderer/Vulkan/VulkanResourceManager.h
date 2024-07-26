#pragma once

#include "Renderer/ResourceManager.h"

#include <vulkan/vulkan.h>

namespace Helios
{
  class VulkanResourceManager : public ResourceManager
  {
  public:
    VulkanResourceManager() = default;
    virtual ~VulkanResourceManager() = default;

    void InitializeResources(const std::vector<Renderable>& renderables) override;
    void UpdateResources(const RenderQueue& renderQueue) override;

    inline std::unique_ptr<ResourceManager> Clone() const override {
      return std::make_unique<VulkanResourceManager>(*this);
    }

  private:
    // Specific methods

  private:
    std::unordered_map<uint32_t, VkShaderModule> m_ShaderModules;

    std::unordered_map<uint32_t, VkBuffer> m_VertexBuffers;
    std::unordered_map<uint32_t, VkBuffer> m_IndexBuffers;

    std::unordered_map<uint32_t, VkImage> m_TextureImages;
    std::unordered_map<uint32_t, VkImageView> m_TextureImageViews;
    std::unordered_map<uint32_t, VkSampler> m_TextureSamplers;

    std::unordered_map<uint32_t, VkDescriptorSetLayout> m_DescriptorSetLayouts;
    std::unordered_map<uint32_t, VkDescriptorSet> m_DescriptorSets;

    std::unordered_map<uint32_t, VkPipelineLayout> m_PipelineLayouts;
    std::unordered_map<uint32_t, VkPipelineLayout> m_Pipelines;
  };
}