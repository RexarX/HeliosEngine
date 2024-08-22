#pragma once

#include "Renderer/Shader.h"

#include <vulkan/vulkan.h>

namespace Helios
{
  struct VulkanShaderInfo
  {
    std::string path;
    VkShaderStageFlagBits stage;
    VkShaderModule shaderModule;
  };

  class VulkanShader : public Shader
  {
  public:
    VulkanShader(const std::initializer_list<ShaderInfo>& infos);
    virtual ~VulkanShader();

    inline const std::vector<VulkanShaderInfo>& GetShaderInfos() const { return m_ShaderInfos; }

  private:
    void Load();

  private:
    std::vector<VulkanShaderInfo> m_ShaderInfos;
  };
}