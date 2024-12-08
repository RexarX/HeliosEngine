#include "VulkanPipelineManager.h"
#include "VulkanContext.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"

#include "Renderer/Material.h"

namespace Helios {
  VulkanPipelineManager::VulkanPipelineManager()
    : m_Context(VulkanContext::Get()) {
  }

  VulkanPipelineManager::~VulkanPipelineManager() {
    ClearResources();
  }

  void VulkanPipelineManager::InitializeResources(const entt::registry& registry,
                                                  const std::vector<entt::entity>& renderables) {
    for (entt::entity entity : renderables) {
      CreatePipeline(registry.get<Renderable>(entity));
    }
  }

  void VulkanPipelineManager::FreeResources(const entt::registry& registry,
                                            const std::vector<entt::entity>& renderables) {}

  void VulkanPipelineManager::UpdateResources(const RenderQueue& renderables) {}

  void VulkanPipelineManager::ClearResources() {
    if (m_Pipelines.empty()) { return; }

    VkDevice device = m_Context.GetDevice();

    // Clear
  }

  void VulkanPipelineManager::CreatePipeline(const Renderable& renderable) {
    VkDevice device = m_Context.GetDevice();
    VkRenderPass renderPass = m_Context.GetRenderPass();
    
  }
}