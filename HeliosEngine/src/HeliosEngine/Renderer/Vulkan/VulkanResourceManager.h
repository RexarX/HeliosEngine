#pragma once

#include "Renderer/ResourceManager.h"

#include <vulkan/vulkan.h>

namespace Helios {
  class VulkanContext;
  class VulkanShader;

  class VulkanResourceManager final : public ResourceManager {
  public:
    struct VulkanEffect {
      std::shared_ptr<VulkanShader> shader = nullptr;

      VkPipeline pipeline;
      VkPipelineLayout pipelineLayout;
      VkPushConstantRange pushConstant;
      std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
      std::vector<VkDescriptorSet> descriptorSets;
      VkDescriptorPool descriptorPool;
    };

    VulkanResourceManager();
    virtual ~VulkanResourceManager();

    void InitializeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) override;
    void FreeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) override;
    void UpdateResources(const RenderQueue& renderables) override;
    void ClearResources() override;

    inline std::unique_ptr<ResourceManager> Clone() const override {
      return std::make_unique<VulkanResourceManager>(*this);
    }

    inline const VulkanEffect& GetEffect(const Renderable& renderable, PipelineType type = PipelineType::Regular) const {
      return m_Effects.at(renderable).at(type);
    }

  private:
    void CreatePipeline(const Renderable& renderable);

  private:
    VulkanContext& m_Context;

    std::unordered_map<Renderable, std::map<PipelineType, VulkanEffect>, RenderableHash> m_Effects;
  };
}