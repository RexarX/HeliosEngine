#include "GraphicsContext.h"

namespace Helios {
  GraphicsContext::GraphicsContext(RendererAPI::API api, void* window)
    : m_RendererAPI(RendererAPI::Create(api, window)) {
  }

  std::shared_ptr<GraphicsContext> GraphicsContext::Create(RendererAPI::API api, void* window) {
    if (m_Instance == nullptr) {
      m_Instance = std::make_shared<GraphicsContext>(api, window);
    }
    return m_Instance;
  }

  std::shared_ptr<GraphicsContext> GraphicsContext::Get() {
    if (m_Instance == nullptr) {
      CORE_ASSERT(false, "Failed to get GraphicsContext: GraphicsContext is not created!");
      return nullptr;
    }
    return m_Instance;
  }
}