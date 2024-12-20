#pragma once

#include "Renderer/Shader.h"

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

namespace Helios {
  class VulkanShader final : public Shader {
  public:
    struct ReflectionData {
      std::vector<VkDescriptorSetLayoutBinding> bindings;
      std::vector<VkVertexInputAttributeDescription> attributes;
      std::vector<VkVertexInputBindingDescription> inputBindings;
      VkPushConstantRange pushConstantRange;
    };

    struct VulkanInfo {
      std::string path;
      VkShaderStageFlagBits stage;
      VkShaderModule shaderModule = VK_NULL_HANDLE;
      ReflectionData reflectionData;
    };

    VulkanShader(const std::initializer_list<Info>& infos);
    virtual ~VulkanShader();

    inline const std::vector<VulkanInfo>& GetVulkanInfos() const { return m_VulkanInfos; }

  private:
    void Load();

    static VkShaderStageFlagBits TranslateStageToVulkan(Stage stage);
    static shaderc_shader_kind TranslateStageToShaderc(VkShaderStageFlagBits stage);

	  static std::expected<std::vector<uint32_t>, std::string> GLSLtoSPV(VkShaderStageFlagBits shaderType,
                                                                       const std::string& glslShader,
                                                                       std::string_view fileName);

  private:
    std::vector<VulkanInfo> m_VulkanInfos;
  };
}