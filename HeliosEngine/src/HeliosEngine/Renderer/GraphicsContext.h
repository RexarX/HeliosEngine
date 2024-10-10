#pragma once

#include "RendererAPI.h"

namespace Helios
{
  class GraphicsContext
  {
  public:
    GraphicsContext(void* window);
    ~GraphicsContext() = default;

    void Init() { m_RendererAPI->Init(); }
    void Shutdown() { m_RendererAPI->Shutdown(); }
    void Update() { m_RendererAPI->Update(); }

    void BeginFrame() { m_RendererAPI->BeginFrame(); }
    void EndFrame() { m_RendererAPI->EndFrame(); }
    void Record(const RenderQueue& queue, const ResourceManager& manager) {
      m_RendererAPI->Record(queue, manager);
    }

    void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0) {
      m_RendererAPI->SetViewport(width, height, x, y);
    }

    void InitImGui() { m_RendererAPI->InitImGui(); }
    void ShutdownImGui() { m_RendererAPI->ShutdownImGui(); }
    void BeginFrameImGui() { m_RendererAPI->BeginFrameImGui(); }
    void EndFrameImGui() { m_RendererAPI->EndFrameImGui(); }

    void SetVSync(bool enabled) { m_RendererAPI->SetVSync(enabled); }
    void SetResized(bool resized) { m_RendererAPI->SetResized(resized); }
    void SetImGuiState(bool enabled) { m_RendererAPI->SetImGuiState(enabled); }

    static std::shared_ptr<GraphicsContext>& Create(void* window);
    static std::shared_ptr<GraphicsContext>& Get();

  private:
    static std::shared_ptr<GraphicsContext> m_Instance;

    void* m_Window = nullptr;
    std::unique_ptr<RendererAPI> m_RendererAPI = nullptr;
  };
}