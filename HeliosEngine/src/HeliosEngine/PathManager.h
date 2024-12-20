#pragma once

#include "Core.h"

#include <filesystem>
#include <map>

namespace Helios {
  class HELIOSENGINE_API PathManager {
  public:
    enum class Directory {
      UserConfig
    };

    static void LoadDefaults();

    static void SetPath(Directory dirType, std::filesystem::path path);
    static void SetPath(Directory dirType, std::filesystem::path&& path);

    static inline const std::filesystem::path& GetPath(Directory dirType) { return m_Paths[dirType]; }

  private:
    static std::filesystem::path InitializeUserConfigDirectory();

  private:
    static std::map<Directory, std::filesystem::path> m_Paths;
  };
}