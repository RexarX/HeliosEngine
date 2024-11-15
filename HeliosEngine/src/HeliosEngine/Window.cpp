#include "Window.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Helios {
  std::unique_ptr<Window> Window::Create(std::string_view title, uint32_t width, uint32_t height) {
#ifdef PLATFORM_WINDOWS
    return std::make_unique<WindowsWindow>(title, width, height);
#elif defined(PLATFORM_LINUX)
    return nullptr;
#else
    CORE_ASSERT_CRITICAL(false, "Unknown platform!");
    return nullptr;
#endif
  }
}