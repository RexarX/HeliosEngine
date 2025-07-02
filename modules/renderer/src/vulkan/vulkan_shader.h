#pragma once

#include "helios/renderer/resources/shader.h"

namespace nvrhi {
    class IShader;
}

namespace helios {
namespace renderer {

/**
 * @brief Vulkan implementation of Shader using NVRHI
 */
class VulkanShader final : public Shader {
public:
    VulkanShader(std::shared_ptr<nvrhi::IShader> nvrhi_shader, const ShaderDesc& desc);
    ~VulkanShader() override = default;
    
    // Shader interface implementation
    Stage GetStage() const override { return stage_; }
    const std::vector<uint8_t>& GetBytecode() const override { return bytecode_; }
    const ShaderReflection& GetReflection() const override { return *reflection_; }
    bool Reload(const std::vector<uint8_t>& spirv_code) override;
    nvrhi::IShader* GetNVRHIShader() const override { return nvrhi_shader_.get(); }

private:
    std::shared_ptr<nvrhi::IShader> nvrhi_shader_;
    Stage stage_;
    std::vector<uint8_t> bytecode_;
    std::unique_ptr<ShaderReflection> reflection_;
};

/**
 * @brief Vulkan implementation of ShaderReflection
 */
class VulkanShaderReflection final : public ShaderReflection {
public:
    VulkanShaderReflection(const std::vector<uint8_t>& spirv_code);
    ~VulkanShaderReflection() override = default;
    
    // ShaderReflection interface implementation
    const std::vector<VertexAttribute>& GetVertexAttributes() const override { return vertex_attributes_; }
    const std::vector<DescriptorBinding>& GetDescriptorBindings() const override { return descriptor_bindings_; }
    const std::vector<PushConstantRange>& GetPushConstantRanges() const override { return push_constant_ranges_; }

private:
    std::vector<VertexAttribute> vertex_attributes_;
    std::vector<DescriptorBinding> descriptor_bindings_;
    std::vector<PushConstantRange> push_constant_ranges_;
    
    void ReflectSpirv(const std::vector<uint8_t>& spirv_code);
};

} // namespace renderer
} // namespace helios