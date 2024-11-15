#include "GraphicsContext.h"

namespace Helios {
  std::shared_ptr<GraphicsContext> GraphicsContext::m_Instance = nullptr;

  GraphicsContext::GraphicsContext(void* window)
    : m_Window(window), m_RendererAPI(RendererAPI::Create(window)) {
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Create(void* window) {
    CORE_ASSERT(m_Instance == nullptr, "GraphicsContext is already created!");

    m_Instance = std::make_shared<GraphicsContext>(window);
    return m_Instance;
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Get() {
    CORE_ASSERT(m_Instance != nullptr, "GraphicsContext is not created!");
    return m_Instance;
  }
}