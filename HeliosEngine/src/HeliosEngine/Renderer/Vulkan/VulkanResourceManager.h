#pragma once

#include "Renderer/ResourceManager.h"

#include "VulkanContext.h"
#include "VulkanShader.h"

namespace Helios
{
  enum class PipelineType
  {
    Regular,
    Wireframe
  };

  struct Effect
  {
    std::shared_ptr<VulkanShader> shader;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkPushConstantRange pushConstant;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool;
  };

  struct RenderableHash
  {
    size_t operator()(const Renderable& renderable) const
    {
      size_t seed = 0;

      if (renderable.mesh != nullptr) {
        const auto& layout = renderable.mesh->GetVertexLayout();
        for (const auto& element : layout.GetElements()) {
          CombineHash(seed, static_cast<uint32_t>(element.type));
        }
      }

      if (renderable.material != nullptr) {
        CombineHash(seed, renderable.material->albedo != nullptr);
        CombineHash(seed, renderable.material->normalMap != nullptr);
        CombineHash(seed, renderable.material->specularMap != nullptr);
        CombineHash(seed, renderable.material->roughnessMap != nullptr);
        CombineHash(seed, renderable.material->metallicMap != nullptr);
        CombineHash(seed, renderable.material->aoMap != nullptr);

        CombineHash(seed, renderable.material->color.x >= 0.0f);
        CombineHash(seed, renderable.material->specular >= 0.0f ||
                          renderable.material->metallic >= 0.0f ||
                          renderable.material->roughness >= 0.0f);
      }

      return seed;
    }

  private:
    template <class T>
    void CombineHash(size_t& seed, const T& v) const
    {
      std::hash<T> hasher;
      seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
  };

  class VulkanResourceManager : public ResourceManager
  {
  public:
    VulkanResourceManager();
    virtual ~VulkanResourceManager();

    void InitializeResources(const std::vector<Renderable>& renderables) override;
    void UpdateResources(entt::registry& ecs, const RenderQueue& renderQueue) override;
    void ClearResources() override;

    inline std::unique_ptr<ResourceManager> Clone() const override {
      return std::make_unique<VulkanResourceManager>(*this);
    }

    inline const Effect& GetEffect(const Renderable& renderable, PipelineType type) const {
      return m_Effects.at(renderable).at(type);
    }

  private:
    void CreatePipeline(const Renderable& renderable);

  private:
    VulkanContext& m_Context;

    std::unordered_map<Renderable, std::map<PipelineType, Effect>, RenderableHash> m_Effects;
  };
}