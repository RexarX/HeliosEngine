#pragma once

#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class IShader;
}

namespace helios {
namespace renderer {

/**
 * @brief Shader wrapper that encapsulates NVRHI shader functionality
 * 
 * This class provides a RAII-based interface for GPU shaders including
 * vertex, fragment, compute, geometry, and tessellation shaders. It ensures
 * proper resource lifetime management and supports hot-reloading.
 */
class Shader {
public:
    /**
     * @brief Shader stages
     */
    enum Stage : uint32_t {
        VERTEX   = 1 << 0,
        FRAGMENT = 1 << 1,
        COMPUTE  = 1 << 2,
        GEOMETRY = 1 << 3,
        TESS_CONTROL = 1 << 4,
        TESS_EVAL = 1 << 5
    };
    
    virtual ~Shader() = default;
    
    /**
     * @brief Get the shader stage
     */
    virtual Stage GetStage() const = 0;
    
    /**
     * @brief Get the SPIR-V bytecode
     */
    virtual const std::vector<uint8_t>& GetBytecode() const = 0;
    
    /**
     * @brief Get shader reflection information
     */
    virtual const class ShaderReflection& GetReflection() const = 0;
    
    /**
     * @brief Reload shader from source (for hot-reloading)
     * @param spirv_code New SPIR-V bytecode
     * @return True if reload was successful
     */
    virtual bool Reload(const std::vector<uint8_t>& spirv_code) = 0;
    
    /**
     * @brief Get the underlying NVRHI shader (for internal use only)
     */
    virtual nvrhi::IShader* GetNVRHIShader() const = 0;

protected:
    Shader() = default;
};

/**
 * @brief Shader reflection information
 * 
 * Contains information about shader inputs, outputs, and resources
 * extracted from SPIR-V bytecode using SPIRV-Reflect.
 */
class ShaderReflection {
public:
    /**
     * @brief Vertex input attribute
     */
    struct VertexAttribute {
        uint32_t location;
        uint32_t format;
        uint32_t offset;
        const char* semantic_name;
    };
    
    /**
     * @brief Descriptor set binding
     */
    struct DescriptorBinding {
        uint32_t set;
        uint32_t binding;
        uint32_t descriptor_type;
        uint32_t count;
        const char* name;
    };
    
    /**
     * @brief Push constant range
     */
    struct PushConstantRange {
        uint32_t stage_flags;
        uint32_t offset;
        uint32_t size;
    };
    
    virtual ~ShaderReflection() = default;
    
    /**
     * @brief Get vertex input attributes (for vertex shaders)
     */
    virtual const std::vector<VertexAttribute>& GetVertexAttributes() const = 0;
    
    /**
     * @brief Get descriptor set bindings
     */
    virtual const std::vector<DescriptorBinding>& GetDescriptorBindings() const = 0;
    
    /**
     * @brief Get push constant ranges
     */
    virtual const std::vector<PushConstantRange>& GetPushConstantRanges() const = 0;

protected:
    ShaderReflection() = default;
};

/**
 * @brief Shader creation parameters
 */
struct ShaderDesc {
    Shader::Stage stage;
    std::vector<uint8_t> spirv_code;
    const char* debug_name = nullptr;
    const char* entry_point = "main";
};

} // namespace renderer
} // namespace helios