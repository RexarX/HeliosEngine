#include "VulkanResourceManager.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"

namespace Helios
{
  VulkanResourceManager::VulkanResourceManager()
    : m_Context(VulkanContext::Get())
  {
  }

  VulkanResourceManager::~VulkanResourceManager()
  {
    ClearResources();
  }

  void VulkanResourceManager::InitializeResources(const std::vector<Renderable>& renderables)
  {
    for (const auto& renderable : renderables) {
      CreatePipeline(renderable);
    }
  }

  void VulkanResourceManager::UpdateResources(entt::registry& ecs, const RenderQueue& renderQueue)
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