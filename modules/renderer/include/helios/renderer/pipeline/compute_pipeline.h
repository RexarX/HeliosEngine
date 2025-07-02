#pragma once

#include <memory>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class IComputePipeline;
}

namespace helios {
namespace renderer {

class Shader;

/**
 * @brief Compute pipeline state object
 * 
 * This class encapsulates compute pipeline state including the compute shader
 * and provides efficient dispatch operations for GPU compute workloads.
 */
class ComputePipeline {
public:
    virtual ~ComputePipeline() = default;
    
    /**
     * @brief Get the compute shader
     */
    virtual std::shared_ptr<Shader> GetComputeShader() const = 0;
    
    /**
     * @brief Get the local work group size
     */
    virtual void GetLocalWorkGroupSize(uint32_t& x, uint32_t& y, uint32_t& z) const = 0;
    
    /**
     * @brief Get the pipeline hash for caching
     */
    virtual uint64_t GetHash() const = 0;
    
    /**
     * @brief Get the underlying NVRHI pipeline (for internal use only)
     */
    virtual nvrhi::IComputePipeline* GetNVRHIPipeline() const = 0;

protected:
    ComputePipeline() = default;
};

/**
 * @brief Compute pipeline creation parameters
 */
struct ComputePipelineDesc {
    std::shared_ptr<Shader> compute_shader;
    const char* debug_name = nullptr;
};

} // namespace renderer
} // namespace helios