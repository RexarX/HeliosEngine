#pragma once

#include "Renderer/ResourceManager.h"

namespace Helios
{
  class VulkanContext;
  class VulkanShader;

  enum class PipelineType
  {
    Regular,
    Wireframe
  };

  struct Effect
  {
    std::shared_ptr<VulkanShader> shader = nullptr;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkPushConstantRange pushConstant;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool;
  };

  struct RenderableHash
  {
    size_t operator()(const Renderable& renderable) const;

  private:
    template <class T>
    void CombineHash(size_t& seed, const T& v) const
    {
      std::hash<T> hasher;
      seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
  };

  class VulkanResourceManager final : public ResourceManager
  {
  public:
    VulkanResourceManager();
    virtual ~VulkanResourceManager();

    void InitializeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) override;
    void FreeResources(const entt::registry& registry, const std::vector<entt::entity>& renderables) override;
    void UpdateResources(const RenderQueue& renderables) override;
    void ClearResources() override;

    inline std::unique_ptr<ResourceManager> Clone() const override {
      return std::make_unique<VulkanResourceManager>(*this);
    }

    inline const Effect& GetEffect(const Renderable& renderable, PipelineType type = PipelineType::Regular) const {
      return m_Effects.at(renderable).at(type);
    }

  private:
    void CreatePipeline(const Renderable& renderable);

  private:
    VulkanContext& m_Context;

    std::unordered_map<Renderable, std::map<PipelineType, Effect>, RenderableHash> m_Effects;
  };
}