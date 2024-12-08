#pragma once

#include "Renderer/PipelineManager.h"

#include <vulkan/vulkan.h>

namespace Helios {
  class VulkanContext;
  class VulkanShader;

  class VulkanPipelineManager final : public PipelineManager {
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

    VulkanPipelineManager();
    virtual ~VulkanPipelineManager();

    void InitializeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) override;
    void FreeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) override;
    void UpdateResources(const RenderQueue& renderables) override;
    void ClearResources() override;

    inline std::unique_ptr<PipelineManager> Clone() const override {
      return std::make_unique<VulkanPipelineManager>(*this);
    }

    inline const VulkanEffect& GetPipeline(const Renderable& renderable, PipelineType type = PipelineType::Regular) const {
      return m_Pipelines.at(renderable).at(type);
    }

  private:
    void CreatePipeline(const Renderable& renderable);

  private:
    VulkanContext& m_Context;

    std::unordered_map<Renderable, std::map<PipelineType, VulkanEffect>, RenderableHash> m_Pipelines;
  };
}