#include "Renderer/GraphicsContext.h"

namespace Helios
{
  std::shared_ptr<GraphicsContext> GraphicsContext::m_Instance = nullptr;
  GraphicsContext::GraphicsContext(void* window)
    : m_Window(window), m_RendererAPI(RendererAPI::Create(window))
  {
  }

  GraphicsContext::~GraphicsContext() = default;

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

  void GraphicsContext::SetViewport(const uint32_t width, const uint32_t height,
                                    const uint32_t x, const uint32_t y)
  {
    m_RendererAPI->SetViewport(width, height, x, y);
  }

  void GraphicsContext::SetVSync(const bool enabled)
  {
    m_RendererAPI->SetVSync(enabled);
  }

  void GraphicsContext::SetResized(const bool resized)
  {
    m_RendererAPI->SetResized(resized);
  }

  void GraphicsContext::SetImGuiState(const bool enabled)
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
    CORE_ASSERT(m_Instance == nullptr, "GraphicsContext already created!");

    m_Instance = std::make_shared<GraphicsContext>(window);
    return m_Instance;
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Get()
  {
    CORE_ASSERT(m_Instance != nullptr, "GraphicsContext not created!");
    return m_Instance;
  }
}