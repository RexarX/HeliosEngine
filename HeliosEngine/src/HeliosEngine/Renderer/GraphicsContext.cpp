#include "GraphicsContext.h"

namespace Helios {
  GraphicsContext::GraphicsContext(RendererAPI::API api, void* window)
    : m_Window(window), m_RendererAPI(RendererAPI::Create(api, window)) {
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Create(RendererAPI::API api, void* window) {
    if (m_Instance == nullptr) {
      m_Instance = std::make_shared<GraphicsContext>(api, window);
    }
    
    return m_Instance;
  }

  std::shared_ptr<GraphicsContext>& GraphicsContext::Get() {
    CORE_ASSERT(m_Instance != nullptr, "GraphicsContext is not created!");
    return m_Instance;
  }
}