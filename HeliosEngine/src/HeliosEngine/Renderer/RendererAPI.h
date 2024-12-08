#pragma once

#include "pch.h"

namespace Helios {
  class PipelineManager;
  class RenderQueue;

  class RendererAPI {
  public:
    enum class API {
      None = 0,
      Vulkan,
      OpenGL
    };

    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Record(const RenderQueue& queue, const PipelineManager& manager) = 0;

    virtual void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0) = 0;

    virtual void InitImGui() = 0;
    virtual void ShutdownImGui() = 0;
    virtual void BeginFrameImGui() = 0;
    virtual void EndFrameImGui() = 0;

    virtual void SetVSync(bool enabled) = 0;
    virtual void SetResized(bool resized) = 0;

    static inline API GetAPI() { return m_API; }

    static std::unique_ptr<RendererAPI> Create(API api, void* window);

  private:
    static inline API m_API = API::None;
  };
}