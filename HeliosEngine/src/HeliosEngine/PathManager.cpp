#include "PathManager.h"
#include "Application.h"

namespace Helios {
  std::filesystem::path PathManager::InitializeUserConfigDirectory() {
#ifdef PLATFORM_WINDOWS
    std::filesystem::path path(std::getenv("USERPROFILE"));
    path /= "Documents";
    path /= Application::Get().GetName();
    return path;
#elif defined(PLATFORM_LINUX)
    std::filesystem::path path(std::getenv("HOME"));
    path /= ".config";
    path /= Application::Get().GetName();
    return path;
#else
    CORE_ASSERT_CRITICAL(false, "Unknown platform!");
    return std::filesystem::path();
#endif
  }
}