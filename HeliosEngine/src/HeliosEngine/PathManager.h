#pragma once

#include "Core.h"

#include <filesystem>

namespace Helios {
  class HELIOSENGINE_API PathManager {
  public:
    static inline std::filesystem::path GetUserConfigDirectory() {
      static const std::filesystem::path userDirectoryPath = InitializeUserConfigDirectory();
      return userDirectoryPath;
    }

  private:
    static std::filesystem::path InitializeUserConfigDirectory();
  };
}