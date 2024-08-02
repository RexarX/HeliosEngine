#include "Renderer/Vulkan/VulkanResourceManager.h"
#include "Renderer/Vulkan/VulkanMesh.h"
#include "Renderer/Vulkan/VulkanShader.h"
#include "Renderer/Vulkan/VulkanTexture.h"

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
  }

  void VulkanResourceManager::UpdateResources(const RenderQueue& renderQueue)
  {
  }

  void VulkanResourceManager::ClearResources()
  {
    VkDevice device = m_Context.GetDevice();

    // Clear
  }
}