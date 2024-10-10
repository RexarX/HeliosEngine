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
      CombineHash(seed, renderable.material->albedo != nullptr);
      CombineHash(seed, renderable.material->normalMap != nullptr ||
                        renderable.material->specularMap != nullptr ||
                        renderable.material->roughnessMap != nullptr ||
                        renderable.material->metallicMap != nullptr ||
                        renderable.material->aoMap != nullptr);

      CombineHash(seed, renderable.material->color.x >= 0.0f);
      CombineHash(seed, renderable.material->specular >= 0.0f ||
                        renderable.material->metallic >= 0.0f ||
                        renderable.material->roughness >= 0.0f);
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

  void VulkanResourceManager::InitializeResources(const std::vector<const Renderable*>& renderables)
  {
    for (const Renderable* renderable : renderables) {
      CreatePipeline(*renderable);
    }
  }

  void VulkanResourceManager::FreeResources(const std::vector<const Renderable*>& renderables)
  {
  }

  void VulkanResourceManager::UpdateResources(entt::registry& registry, const RenderQueue& renderQueue)
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