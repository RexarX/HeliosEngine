#include "Input.h"

#include "Platform/Windows/WindowsInput.h"
#include "Platform/Linux/LinuxInput.h"

namespace Helios {
#ifdef PLATFORM_WINDOWS
  std::unique_ptr<Input> Input::m_Instance = std::make_unique<WindowsInput>();
#elif defined(PLATFORM_LINUX)
  std::unique_ptr<Input> Input::m_Instance = std::make_unique<LinuxInput>();
#else
  CORE_ASSERT_CRITICAL(false, "Unknown platform!");
  std::unique_ptr<Input> Input::m_Instance = nullptr;
#endif
}