#include "VulkanResourceManager.h"
#include "VulkanContext.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"

#include "Renderer/Material.h"

namespace Helios {
  VulkanResourceManager::VulkanResourceManager()
    : m_Context(VulkanContext::Get()) {
  }

  VulkanResourceManager::~VulkanResourceManager() {
    ClearResources();
  }

  void VulkanResourceManager::InitializeResources(const entt::registry& registry,
                                                  const std::vector<entt::entity>& renderables) {
    std::for_each(std::execution::par, renderables.begin(), renderables.end(),
      [this, &registry](entt::entity entity) {
        CreatePipeline(registry.get<Renderable>(entity));
      }
    );
  }

  void VulkanResourceManager::FreeResources(const entt::registry& registry,
                                            const std::vector<entt::entity>& renderables) {}

  void VulkanResourceManager::UpdateResources(const RenderQueue& renderables) {}

  void VulkanResourceManager::ClearResources() {
    if (m_Effects.empty()) { return; }

    VkDevice device = m_Context.GetDevice();

    // Clear
  }

  void VulkanResourceManager::CreatePipeline(const Renderable& renderable) {
    VkDevice device = m_Context.GetDevice();
    VkRenderPass renderPass = m_Context.GetRenderPass();
    
  }
}