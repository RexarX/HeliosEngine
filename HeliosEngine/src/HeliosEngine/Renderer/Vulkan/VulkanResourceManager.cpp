#include "VulkanResourceManager.h"
#include "VulkanContext.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

namespace Helios
{
  size_t RenderableHash::operator()(const Renderable& renderable) const
  {
    size_t seed = 0;

    if (renderable.mesh != nullptr) {
      const VertexLayout& layout = renderable.mesh->GetVertexLayout();
      for (const VertexElement& element : layout.GetElements()) {
        CombineHash(seed, static_cast<uint32_t>(element.type));
      }
    }

    if (renderable.material != nullptr) {
      CombineHash(seed, renderable.material->GetAlbedo() != nullptr);
      CombineHash(seed, renderable.material->GetNormalMap() != nullptr ||
                        renderable.material->GetSpecularMap() != nullptr ||
                        renderable.material->GetRoughnessMap() != nullptr ||
                        renderable.material->GetMetallicMap() != nullptr ||
                        renderable.material->GetAoMap() != nullptr);

      CombineHash(seed, renderable.material->GetColor().x >= 0.0f);
      CombineHash(seed, renderable.material->GetSpecular() >= 0.0f ||
                        renderable.material->GetMetallic() >= 0.0f ||
                        renderable.material->GetRoughness() >= 0.0f);
    }

    return seed;
  }

  VulkanResourceManager::VulkanResourceManager()
    : m_Context(VulkanContext::Get())
  {
  }

  VulkanResourceManager::~VulkanResourceManager()
  {
    ClearResources();
  }

  void VulkanResourceManager::InitializeResources(const entt::registry& registry,
                                                  const std::vector<entt::entity>& renderables)
  {
    std::for_each(renderables.begin(), renderables.end(), [this, &registry](entt::entity entity) {
      CreatePipeline(registry.get<Renderable>(entity));
    });
  }

  void VulkanResourceManager::FreeResources(const entt::registry& registry,
                                            const std::vector<entt::entity>& renderables)
  {
  }

  void VulkanResourceManager::UpdateResources(const RenderQueue& renderables)
  {
  }

  void VulkanResourceManager::ClearResources()
  { 
    if (m_Effects.empty()) { return; }

    VkDevice device = m_Context.GetDevice();

    // Clear
  }

  void VulkanResourceManager::CreatePipeline(const Renderable& renderable)
  {
    VkDevice device = m_Context.GetDevice();
    VkRenderPass renderPass = m_Context.GetRenderPass();
    
  }
}