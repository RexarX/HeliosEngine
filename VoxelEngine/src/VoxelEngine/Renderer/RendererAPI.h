#pragma once

#include "pch.h"

namespace Engine
{
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

    virtual void SetViewport(const uint32_t width, const uint32_t height,
                             const uint32_t x = 0, const uint32_t y = 0) = 0;

    virtual void InitImGui() = 0;
    virtual void ShutdownImGui() = 0;
    virtual void BeginFrameImGui() = 0;
    virtual void EndFrameImGui() = 0;

    virtual void SetVSync(const bool enabled) = 0;
    virtual void SetResized(const bool resized) = 0;
    virtual void SetImGuiState(const bool enabled) = 0;

    static void SetAPI(const API api) { m_API = api; }
    static inline const API GetAPI() { return m_API; }

    static std::unique_ptr<RendererAPI> Create();

  private:
    static API m_API;
  };
}