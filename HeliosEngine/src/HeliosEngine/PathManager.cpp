#include "PathManager.h"
#include "Application.h"

namespace Helios {
  std::map<PathManager::Directory, std::filesystem::path> PathManager::m_Paths;

  void PathManager::LoadDefaults() {
    m_Paths.emplace(Directory::UserConfig, InitializeUserConfigDirectory());
  }

  void PathManager::SetPath(Directory dirType, std::filesystem::path path) {
    if (path.empty()) {
      CORE_ASSERT(false, "Failed to set path: path is empty!");
      return;
    }

    if (!std::filesystem::is_directory(path)) {
      CORE_ASSERT(false, "Failed to set path: path '{}' is not a directory!", path.string());
      return;
    }

    std::filesystem::create_directories(path);
    
    m_Paths[dirType] = std::move(path);
  }

  void PathManager::SetPath(Directory dirType, std::filesystem::path&& path) {
    if (path.empty()) {
      CORE_ASSERT(false, "Failed to set path: path is empty!");
      return;
    }

    if (!std::filesystem::is_directory(path)) {
      CORE_ASSERT(false, "Failed to set path: path '{}' is not a directory!", path.string());
      return;
    }

    std::filesystem::create_directories(path);

    m_Paths[dirType] = std::move(path);
  }

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

    std::filesystem::create_directories(path);
  }
}