#pragma once

#include "Renderer/Shader.h"

#include <vulkan/vulkan.h>

namespace Helios
{
  struct VulkanShaderInfo
  {
    std::string path;
    std::vector<uint32_t> code;
    VkShaderStageFlagBits stage;
    VkShaderModule shaderModule;
  };

  class VulkanShader : public Shader
  {
  public:
    VulkanShader(const std::initializer_list<ShaderInfo>& infos);
    virtual ~VulkanShader() = default;

    void Load() override;
    void Unload() override;

  private:
    bool m_Loaded = false;

    std::vector<VulkanShaderInfo> m_ShaderInfos;
  };
}