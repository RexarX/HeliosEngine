#pragma once

#include "RendererAPI.h"

#include "pch.h"

namespace Helios
{
  class GraphicsContext
  {
  public:
    GraphicsContext(void* window);
    ~GraphicsContext() = default;

    void Init();
    void Shutdown();
    void Update();

    void BeginFrame();
    void EndFrame();
    void Record(const RenderQueue& queue, const ResourceManager& manager);

    void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0);

    void InitImGui();
    void ShutdownImGui();
    void BeginFrameImGui();
    void EndFrameImGui();

    void SetVSync(bool enabled);
    void SetResized(bool resized);
    void SetImGuiState(bool enabled);

    static std::shared_ptr<GraphicsContext>& Create(void* window);
    static std::shared_ptr<GraphicsContext>& Get();

  private:
    static std::shared_ptr<GraphicsContext> m_Instance;

    void* m_Window = nullptr;
    std::unique_ptr<RendererAPI> m_RendererAPI = nullptr;
  };
}