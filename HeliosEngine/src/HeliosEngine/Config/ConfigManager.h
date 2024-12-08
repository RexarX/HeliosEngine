#pragma once

#include "UserConfig.h"

namespace Helios {
  class HELIOSENGINE_API ConfigManager {
  public:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ~ConfigManager() = default;

    template <ConfigTrait T>
    void LoadConfiguration(const std::filesystem::path& configPath);

    template <ConfigTrait T>
    void SaveConfiguration(const std::filesystem::path& configPath);

    template <ConfigTrait T>
    T& RegisterConfig();

    void ParseCommandLineArgs(int argc, char** argv);

    template <ConfigTrait T>
    T& GetConfig();

    static inline ConfigManager& Get() {
      static ConfigManager instance;
      return instance;
    }

    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

  private:
    ConfigManager() {
      RegisterConfig<UserConfig>();
    }

    static void WriteConfigToFile(const Config& config, const std::filesystem::path& configPath);

  private:
    std::map<std::type_index, std::unique_ptr<Config>> m_RegisteredConfigs;
  };

  template <ConfigTrait T>
  void ConfigManager::LoadConfiguration(const std::filesystem::path& configPath) {
    std::type_index type(typeid(T));
    auto it = m_RegisteredConfigs.find(type);
    if (it == m_RegisteredConfigs.end()) {
      CORE_ASSERT(false, "Config '{}' is not registred!", type.name());
      return;
    }

    if (configPath.empty()) {
      CORE_ASSERT(false, "Config path is empty valid!");
      return;
    } else if (!std::filesystem::exists(configPath)) {
      std::filesystem::create_directories(configPath.parent_path());
      return;
    }

    std::string path = configPath.string();

    if (configPath.extension() != ".toml") {
      CORE_ASSERT(false, "The only currently supported format is '.toml', '{}' is not supported!",
                  Utils::getFileExtension(path));
      return;
    }

    CORE_INFO("Loading '{}' config file.", path);

    Config& config = *it->second;
    toml::table table;
    try {
      table = toml::parse_file(path);
    }
    catch (const toml::parse_error& error) {
      CORE_ASSERT(false, "Failed to load config file '{}': {} ({} line, {} column)!", path,
                  error.description(), error.source().begin.line, error.source().begin.column);
      CORE_WARN("Using default configuration!");
      config.LoadDefaults();
      return;
    }

    config.MarkDirty();
    config.Deserialize(table);
    config.ClearDirty();
  }

  template <ConfigTrait T>
  void ConfigManager::SaveConfiguration(const std::filesystem::path& configPath) {
    std::type_index type(typeid(T));
    auto it = m_RegisteredConfigs.find(type);
    if (it == m_RegisteredConfigs.end()) {
      CORE_ASSERT(false, "Config '{}' is not registred!", type.name());
      return;
    }

    if (configPath.empty()) {
      CORE_ASSERT(false, "Config path is empty!");
      return;
    }

    if (configPath.extension() != ".toml") {
      CORE_ASSERT(false, "The only currently supported format is '.toml', '{}' is not supported!",
                  configPath.extension().string());
      return;
    }

    CORE_INFO("Saving config into '{}' file.", configPath.string());

    std::filesystem::create_directories(configPath.parent_path());

    Config& config = *it->second;
    if (config.IsDirty() || !std::filesystem::exists(configPath)) {
      WriteConfigToFile(config, configPath);
      config.ClearDirty();
    }
  }

  template <ConfigTrait T>
  T& ConfigManager::RegisterConfig() {
    std::type_index type(typeid(T));
    auto it = m_RegisteredConfigs.find(type);
    if (it == m_RegisteredConfigs.end()) {
      auto [iterator, inserted] = m_RegisteredConfigs.emplace(
        type, std::make_unique<T>()
      );
      return static_cast<T&>(*iterator->second);
    }

    return static_cast<T&>(*it->second);
  }

  template <ConfigTrait T>
  T& ConfigManager::GetConfig() {
    auto it = m_RegisteredConfigs.find(typeid(T));
    if (it == m_RegisteredConfigs.end()) {
      return RegisterConfig<T>();
    }
    return static_cast<T&>(*it->second);
  }
}