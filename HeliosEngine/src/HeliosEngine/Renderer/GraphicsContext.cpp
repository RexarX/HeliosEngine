#include "GraphicsContext.h"

namespace Helios
{
  std::shared_ptr<GraphicsContext> GraphicsContext::m_Instance = nullptr;

  GraphicsContext::GraphicsContext(void* window)
    : m_Window(window), m_RendererAPI(RendererAPI::Create(window))
  {
  }

  void GraphicsContext::Init()
  {
    m_RendererAPI->Init();
  }

  void GraphicsContext::Shutdown()
  {
    m_RendererAPI->Shutdown();
  }

  void GraphicsContext::Update()
  {
    m_RendererAPI->Update();
  }

  void GraphicsContext::BeginFrame()
  {
    m_RendererAPI->BeginFrame();
  }

  void GraphicsContext::EndFrame()
  {
    m_RendererAPI->EndFrame();
  }

  void GraphicsContext::Record(const RenderQueue& queue, const ResourceManager& manager)
  {
    m_RendererAPI->Record(queue, manager);
  }

  void GraphicsContext::SetViewport(uint32_t width, uint32_t height, uint32_t x, uint32_t y)
  {
    m_RendererAPI->SetViewport(width, height, x, y);
  }

  void GraphicsContext::SetVSync(bool enabled)
  {
    m_RendererAPI->SetVSync(enabled);
  }

  void GraphicsContext::SetResized(bool resized)
  {
    m_RendererAPI->SetResized(resized);
  }

  void GraphicsContext::SetImGuiState(bool enabled)
  {
    m_RendererAPI->SetImGuiState(enabled);
  }

  void GraphicsContext::InitImGui()
  {
    m_RendererAPI->InitImGui();
  }

  void GraphicsContext::ShutdownImGui()
  {
    m_RendererAPI->ShutdownImGui();
  }

  void GraphicsContext::BeginFrameImGui()
  {
    m_RendererAPI->BeginFrameImGui();
  }

  void GraphicsContext::EndFrameImGui()
  {
    m_RendererAPI->EndFrameImGui();
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Create(void* window)
  {
    CORE_ASSERT(m_Instance == nullptr, "GraphicsContext is already created!");

    m_Instance = std::make_shared<GraphicsContext>(window);
    return m_Instance;
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Get()
  {
    CORE_ASSERT(m_Instance != nullptr, "GraphicsContext is not created!");
    return m_Instance;
  }
}