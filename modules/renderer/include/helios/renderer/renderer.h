#pragma once

#include <memory>
#include <functional>
#include <future>
#include <vector>

namespace helios {
namespace renderer {

class Device;
class CommandContext;

/**
 * @brief Main renderer interface that provides high-level rendering operations
 * 
 * This class serves as the primary entry point for the renderer module,
 * providing thread-safe access to rendering resources and operations.
 * It hides NVRHI implementation details behind a clean C++ API.
 */
class Renderer {
public:
    /**
     * @brief Initialize the renderer with the given parameters
     * @param window_handle Native window handle (GLFW window)
     * @param enable_validation Enable Vulkan validation layers
     * @param num_worker_threads Number of worker threads for parallel command recording
     */
    static bool Initialize(void* window_handle, bool enable_validation = false, uint32_t num_worker_threads = 4);
    
    /**
     * @brief Shutdown the renderer and clean up all resources
     */
    static void Shutdown();
    
    /**
     * @brief Get the singleton renderer instance
     */
    static Renderer& GetInstance();
    
    /**
     * @brief Get the main device interface
     */
    Device& GetDevice() const { return *device_; }
    
    /**
     * @brief Create a new command context for a specific thread
     * @param thread_id Unique identifier for the calling thread
     * @return Command context for thread-safe command recording
     */
    std::unique_ptr<CommandContext> CreateCommandContext(uint32_t thread_id);
    
    /**
     * @brief Submit a rendering task to be executed asynchronously
     * @param task Function to execute on a worker thread
     * @return Future that can be used to wait for task completion
     */
    template<typename F, typename... Args>
    auto SubmitTask(F&& task, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
    
    /**
     * @brief Begin a new frame
     */
    void BeginFrame();
    
    /**
     * @brief End the current frame and present
     */
    void EndFrame();
    
    /**
     * @brief Wait for all pending GPU operations to complete
     */
    void WaitIdle();

private:
    Renderer() = default;
    ~Renderer() = default;
    
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    std::unique_ptr<Device> device_;
    static std::unique_ptr<Renderer> instance_;
};

// Template implementation
template<typename F, typename... Args>
auto Renderer::SubmitTask(F&& task, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    // Implementation will delegate to internal thread pool
    // This will be implemented in the .cpp file using the existing ThreadPool
    static_assert(sizeof...(args) >= 0, "Template implementation in header for now");
    return {};
}

} // namespace renderer
} // namespace helios