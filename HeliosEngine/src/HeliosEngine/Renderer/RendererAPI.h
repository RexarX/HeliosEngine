#pragma once

namespace Helios
{
  class ResourceManager;
  class RenderQueue;

  class RendererAPI
  {
  public:
    enum class API
    {
      None = 0,
      Vulkan = 1
    };

    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Record(const RenderQueue& queue, const ResourceManager& manager) = 0;

    virtual void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0) = 0;

    virtual void InitImGui() = 0;
    virtual void ShutdownImGui() = 0;
    virtual void BeginFrameImGui() = 0;
    virtual void EndFrameImGui() = 0;

    virtual void SetVSync(bool enabled) = 0;
    virtual void SetResized(bool resized) = 0;
    virtual void SetImGuiState(bool enabled) = 0;

    static void SetAPI(API api) { m_API = api; }
    static inline API GetAPI() { return m_API; }

    static std::unique_ptr<RendererAPI> Create(void* window);

  private:
    static API m_API;
  };
}