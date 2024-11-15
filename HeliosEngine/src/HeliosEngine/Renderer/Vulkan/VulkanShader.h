#pragma once

#include "Renderer/Shader.h"

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

namespace Helios {
  class VulkanShader final : public Shader {
  public:
    struct ShaderInfo {
      std::string path;
      VkShaderStageFlagBits stage;
      VkShaderModule shaderModule;
    };

    VulkanShader(const std::initializer_list<Info>& infos);
    virtual ~VulkanShader();

    inline const std::vector<ShaderInfo>& GetShaderInfos() const { return m_ShaderInfos; }

  private:
    void Load();

    VkShaderStageFlagBits TranslateStageToVulkan(Stage stage) const;
    shaderc_shader_kind TranslateStageToShaderc(VkShaderStageFlagBits stage) const;

	  bool GLSLtoSPV(VkShaderStageFlagBits shaderType, const std::string& glslShader,
									 const std::string& fileName, std::vector<uint32_t>& spvShader) const;

  private:
    std::vector<ShaderInfo> m_ShaderInfos;
  };
}