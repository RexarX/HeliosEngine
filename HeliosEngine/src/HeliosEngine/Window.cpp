#include "Window.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Linux/LinuxWindow.h"

namespace Helios {
  std::unique_ptr<Window> Window::Create() {
#ifdef PLATFORM_WINDOWS
    return std::make_unique<WindowsWindow>();
#elif defined(PLATFORM_LINUX)
    return std::make_unique<LinuxWindow>();
#else
    CORE_ASSERT_CRITICAL(false, "Unknown platform!");
    return nullptr;
#endif
  }
}