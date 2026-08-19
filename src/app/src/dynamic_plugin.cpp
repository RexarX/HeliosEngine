#include <pch.hpp>

#include <helios/app/dynamic_plugin.hpp>

#include <helios/app/details/profile.hpp>
#include <helios/app/plugin.hpp>
#include <helios/assert.hpp>
#include <helios/log/logger.hpp>
#include <helios/utils/dynamic_library.hpp>

#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

namespace helios::app {

DynamicPlugin::DynamicPlugin(const std::filesystem::path& path,
                             DynamicPluginConfig config) {
  const auto result = Load(path, config);
  if (!result) [[unlikely]] {
    log::Error("Failed to load dynamic plugin '{}': {}!", path.string(),
               DynamicPluginErrorToString(result.error()));
  }
}

DynamicPlugin::DynamicPlugin(DynamicPlugin&& other) noexcept
    : library_(std::move(other.library_)),
      plugin_(std::move(other.plugin_)),
      plugin_type_id_(other.plugin_type_id_),
      config_(other.config_),
      last_write_time_(other.last_write_time_) {}

DynamicPlugin& DynamicPlugin::operator=(DynamicPlugin&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  plugin_.reset();
  library_ = std::move(other.library_);
  plugin_ = std::move(other.plugin_);
  plugin_type_id_ = other.plugin_type_id_;
  config_ = other.config_;
  last_write_time_ = other.last_write_time_;

  return *this;
}

auto DynamicPlugin::Load(const std::filesystem::path& path,
                         DynamicPluginConfig config)
    -> DynamicPluginResult<void> {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_TEXT(
      std::format("{} ({})", plugin_type_id_.QualifiedName(), path.string()));

  config_ = config;

  // Load the library
  const auto load_result = library_.Load(path);
  if (!load_result) [[unlikely]] {
    return std::unexpected(DynamicPluginError::kLibraryLoadFailed);
  }

  // Load plugin symbols and create instance
  auto plugin_result = LoadPluginInstance();
  if (!plugin_result) {
    [[maybe_unused]] const auto result = library_.Unload();
    return plugin_result;
  }

  // Cache the file modification time
  UpdateFileTime();

  log::Info("Loaded dynamic plugin '{}' from: {}", GetPluginName(),
            path.string());
  return {};
}

auto DynamicPlugin::Unload() -> DynamicPluginResult<void> {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_TEXT(std::format(
      "{} ({})", plugin_type_id_.QualifiedName(), library_.Path().string()));

  HELIOS_ASSERT(Loaded(), "Plugin is not loaded!");

  // Release plugin before unloading library
  plugin_.reset();

  const auto unload_result = library_.Unload();
  if (!unload_result) [[unlikely]] {
    return std::unexpected(DynamicPluginError::kLibraryLoadFailed);
  }

  return {};
}

auto DynamicPlugin::Reload(App& app) -> DynamicPluginResult<void> {
  HELIOS_APP_PROFILE_SCOPE();
  HELIOS_APP_PROFILE_ZONE_TEXT(std::format(
      "{} ({})", plugin_type_id_.QualifiedName(), library_.Path().string()));

  HELIOS_ASSERT(Loaded(), "Plugin is not loaded!");

  // Call Destroy on the old plugin
  log::Info("Reloading dynamic plugin '{}' from '{}'", GetPluginName(),
            library_.Path().string());
  plugin_->Destroy(app);

  // Save the path and config before unloading
  const auto saved_path = library_.Path();
  auto saved_config = config_;

  // Release plugin and unload library
  plugin_.reset();

  const auto unload_result = library_.Unload();
  if (!unload_result) [[unlikely]] {
    return std::unexpected(DynamicPluginError::kReloadFailed);
  }

  // Reload the library
  const auto load_result = library_.Load(saved_path);
  if (!load_result) [[unlikely]] {
    return std::unexpected(DynamicPluginError::kReloadFailed);
  }

  // Load plugin symbols and create new instance
  config_ = saved_config;
  const auto plugin_result = LoadPluginInstance();
  if (!plugin_result) [[unlikely]] {
    [[maybe_unused]] const auto result = library_.Unload();
    return std::unexpected(DynamicPluginError::kReloadFailed);
  }

  // Call Build on the new plugin
  plugin_->Build(app);

  // Update file time
  UpdateFileTime();

  log::Info("Successfully reloaded dynamic plugin '{}' from '{}'",
            GetPluginName(), saved_path.string());
  return {};
}

auto DynamicPlugin::ReloadIfChanged(App& app) -> DynamicPluginResult<void> {
  HELIOS_ASSERT(Loaded(), "Plugin is not loaded!");

  if (!HasFileChanged()) {
    return {};
  }

  return Reload(app);
}

void DynamicPlugin::UpdateFileTime() noexcept {
  if (!library_.Loaded()) [[unlikely]] {
    return;
  }

  std::error_code ec;
  last_write_time_ = std::filesystem::last_write_time(library_.Path(), ec);
}

bool DynamicPlugin::HasFileChanged() const noexcept {
  if (!library_.Loaded()) {
    return false;
  }

  std::error_code ec;
  const auto current_time =
      std::filesystem::last_write_time(library_.Path(), ec);
  if (ec) [[unlikely]] {
    return false;
  }

  return current_time != last_write_time_;
}

Plugin& DynamicPlugin::GetPlugin() noexcept {
  HELIOS_ASSERT(Loaded(), "Plugin is not loaded!");
  return *plugin_;
}

const Plugin& DynamicPlugin::GetPlugin() const noexcept {
  HELIOS_ASSERT(Loaded(), "Plugin is not loaded!");
  return *plugin_;
}

auto DynamicPlugin::LoadPluginInstance() -> DynamicPluginResult<void> {
  // Get create function
  const auto create_result =
      library_.GetSymbol<CreatePluginFn>(config_.create_symbol);
  if (!create_result) [[unlikely]] {
    log::Error("Create function '{}' not found in library!",
               config_.create_symbol);
    return std::unexpected(DynamicPluginError::kCreateSymbolNotFound);
  }

  // Get plugin type export function
  const auto id_result =
      library_.GetSymbol<PluginTypeExportFn>(config_.plugin_type_id_symbol);
  if (!id_result) [[unlikely]] {
    log::Error("Plugin ID function '{}' not found in library!",
               config_.plugin_type_id_symbol);
    return std::unexpected(DynamicPluginError::kIdSymbolNotFound);
  }

  const PluginTypeExportFn export_fn = *id_result;
  const PluginTypeExport* export_info = export_fn();
  if (export_info == nullptr) [[unlikely]] {
    log::Error("Plugin type export function returned nullptr!");
    return std::unexpected(DynamicPluginError::kCreateFailed);
  }

  const std::string_view qualified_name =
      export_info->qualified_name != nullptr
          ? std::string_view{export_info->qualified_name}
          : std::string_view{};
  plugin_type_id_ =
      PluginTypeId::FromExported(export_info->hash, qualified_name);

  // Create the plugin
  CreatePluginFn create_fn = *create_result;
  Plugin* raw_plugin = create_fn();

  if (raw_plugin == nullptr) [[unlikely]] {
    log::Error("Plugin creation function returned nullptr!");
    return std::unexpected(DynamicPluginError::kCreateFailed);
  }

  plugin_.reset(raw_plugin);
  return {};
}

}  // namespace helios::app
