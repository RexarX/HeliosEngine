#pragma once

#include <helios/app/plugin.hpp>
#include <helios/utils/dynamic_library.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace helios::app {

// Forward declaration
class App;

/**
 * @brief Function signature for plugin creation.
 * @details Dynamic plugins must export a function with this signature.
 * The function should create and return a new Plugin instance.
 */
using CreatePluginFn = Plugin* (*)();

/**
 * @brief C-compatible plugin identity exported by dynamic libraries.
 * @details Returned from `helios_plugin_type_id`. `qualified_name` may be null;
 * when set, storage must outlive the plugin image (typically static data).
 */
struct PluginTypeExport {
  uint64_t hash = 0;  ///< `TypeIndex::Hash()` for the plugin type
  const char* qualified_name = nullptr;  ///< Optional qualified type name

  /**
   * @brief Builds an export descriptor from a `PluginTypeId`.
   * @param type_id Plugin type identity
   * @return Export descriptor suitable for `helios_plugin_type_id`
   */
  [[nodiscard]] static constexpr PluginTypeExport From(
      const PluginTypeId& type_id) noexcept {
    const std::string_view qualified = type_id.QualifiedName();
    const uint64_t hash = type_id.Index().Hash();
    return {.hash = hash,
            .qualified_name = qualified.empty() ? nullptr : qualified.data()};
  }

  /**
   * @brief Builds an export descriptor from a plugin type.
   * @tparam T Plugin type
   * @return Export descriptor suitable for `helios_plugin_type_id`
   */
  template <typename T>
  [[nodiscard]] static constexpr PluginTypeExport From() noexcept {
    constexpr auto type_id = PluginTypeId::From<T>();
    return {.hash = type_id.Index().Hash(),
            .qualified_name = utils::details::GetFullTypeNameCString<T>()};
  }

  /**
   * @brief Builds an export descriptor from a plugin type.
   * @tparam T Plugin type
   * @param Plugin instance
   * @return Export descriptor suitable for `helios_plugin_type_id`
   */
  template <typename T>
  [[nodiscard]] static constexpr PluginTypeExport From(
      const T& /*plugin*/) noexcept {
    return From<std::remove_cvref_t<T>>();
  }
};

/**
 * @brief Function signature for the plugin type export symbol.
 * @details Dynamic plugins must export this with C linkage. The returned
 * pointer must remain valid for the lifetime of the library image.
 */
using PluginTypeExportFn = const PluginTypeExport* (*)();

/// @brief Default symbol name for the plugin creation function.
inline constexpr std::string_view kDefaultCreateSymbol = "helios_create_plugin";

/// @brief Default symbol name for the plugin ID function.
inline constexpr std::string_view kDefaultPluginIdSymbol =
    "helios_plugin_type_id";

/// @brief Error codes for dynamic plugin operations.
enum class DynamicPluginError : uint8_t {
  kLibraryLoadFailed,     ///< Failed to load the dynamic library
  kCreateSymbolNotFound,  ///< Plugin creation function not found
  kIdSymbolNotFound,      ///< Plugin ID function not found
  kNameSymbolNotFound,    ///< Plugin name function not found
  kCreateFailed,          ///< Plugin creation function returned nullptr
  kReloadFailed,          ///< Failed to reload plugin
};

template <typename T>
using DynamicPluginResult = std::expected<T, DynamicPluginError>;

/**
 * @brief Gets a human-readable description for a `DynamicPluginError`.
 * @param error The error code
 * @return String description of the error
 */
[[nodiscard]] constexpr std::string_view ToString(
    DynamicPluginError error) noexcept {
  switch (error) {
    using enum DynamicPluginError;
    case kLibraryLoadFailed:
      return "Failed to load dynamic library";
    case kCreateSymbolNotFound:
      return "Plugin creation function not found";
    case kIdSymbolNotFound:
      return "Plugin ID function not found";
    case kNameSymbolNotFound:
      return "Plugin name function not found";
    case kCreateFailed:
      return "Plugin creation function returned nullptr";
    case kReloadFailed:
      return "Failed to reload plugin";
    default:
      return "Unknown error";
  }
}

/// @brief Configuration for dynamic plugin loading.
struct DynamicPluginConfig {
  std::string_view create_symbol =
      kDefaultCreateSymbol;  ///< Name of the creation function
  std::string_view plugin_type_id_symbol =
      kDefaultPluginIdSymbol;  ///< Name of the plugin ID function
  bool auto_reload = false;    ///< Enable automatic reload on file change
};

/**
 * @brief Wrapper for dynamically loaded plugins.
 * @details Loads a plugin from a shared library and manages its lifecycle.
 * Supports hot-reloading: when the library file changes, the plugin can be
 * unloaded and reloaded without restarting the application.
 *
 * The dynamic library must export:
 * - A creation function (default: "helios_create_plugin") that returns
 * `Plugin*`
 * - A plugin type export function (default: "helios_plugin_type_id") that
 * returns a `PluginTypeExport` via `PluginTypeExportFn`
 *
 * @note Not thread-safe.
 * @code
 * // In my_plugin.cpp (compiled as shared library)
 * class MyPlugin : public helios::app::Plugin {
 * public:
 *   static constexpr std::string_view kName = "MyPlugin";
 *
 *   void Build(App& app) override { ... }
 *   void Destroy(App& app) override { ... }
 * };
 *
 * extern "C" {
 *
 * HELIOS_EXPORT helios::app::Plugin* helios_create_plugin() {
 *   return new MyPlugin();
 * }
 *
 * HELIOS_EXPORT auto helios_plugin_type_id()
 *     -> const helios::app::PluginTypeExport* {
 *   static const auto kExport =
 *       helios::app::PluginTypeExport::From<MyPlugin>();
 *   return &kExport;
 * }
 *
 * }
 * @endcode
 *
 * @code
 * DynamicPlugin dyn_plugin;
 * if (auto result = dyn_plugin.Load("my_plugin.so"); result) {
 *   app.AddDynamicPlugin(std::move(dyn_plugin));
 * }
 * @endcode
 */
class DynamicPlugin {
public:
  using FileTime = std::filesystem::file_time_type;

  DynamicPlugin() = default;

  /**
   * @brief Constructs and loads a plugin from the specified path.
   * @param path Path to the dynamic library
   * @param config Configuration options
   */
  explicit DynamicPlugin(const std::filesystem::path& path,
                         DynamicPluginConfig config = {});
  DynamicPlugin(const DynamicPlugin&) = delete;
  DynamicPlugin(DynamicPlugin&& other) noexcept;
  ~DynamicPlugin() noexcept = default;

  DynamicPlugin& operator=(const DynamicPlugin&) = delete;
  DynamicPlugin& operator=(DynamicPlugin&& other) noexcept;

  /**
   * @brief Loads a plugin from the specified path.
   * @param path Path to the dynamic library
   * @param config Configuration options
   * @return `DynamicPluginResult` that holds void on success, or
   * `DynamicPluginError` on failure
   */
  [[nodiscard]] auto Load(const std::filesystem::path& path,
                          DynamicPluginConfig config = {})
      -> DynamicPluginResult<void>;

  /**
   * @brief Unloads the current plugin.
   * @details Releases the plugin and unloads the library.
   * @warning Triggers assertion if the plugin is not loaded.
   * @return `DynamicPluginResult` that holds void on success, or
   * `DynamicPluginError` on failure
   */
  [[nodiscard]] auto Unload() -> DynamicPluginResult<void>;

  /**
   * @brief Reloads the plugin from the same path.
   * @details Calls Destroy on the old plugin, unloads the library,
   * loads it again, and calls Build on the new plugin.
   * @warning Triggers assertion if the plugin is not loaded.
   * @param app Reference to the app for Build/Destroy calls
   * @return `DynamicPluginResult` that holds void on success, or
   * `DynamicPluginError` on failure
   */
  [[nodiscard]] auto Reload(App& app) -> DynamicPluginResult<void>;

  /**
   * @brief Reloads the plugin only if the file has changed.
   * @warning Triggers assertion if the plugin is not loaded.
   * @param app Reference to the app for Build/Destroy calls
   * @return `DynamicPluginResult` that holds void on success, or
   * `DynamicPluginError` on failure
   */
  [[nodiscard]] auto ReloadIfChanged(App& app) -> DynamicPluginResult<void>;

  /**
   * @brief Updates the cached file modification time.
   * @details Call this after detecting a change to reset the tracking.
   */
  void UpdateFileTime() noexcept;

  /**
   * @brief Checks if the library file has been modified since last load.
   * @return True if the file modification time has changed
   */
  [[nodiscard]] bool HasFileChanged() const noexcept;

  /**
   * @brief Checks if a plugin is currently loaded.
   * @return True if a plugin is loaded
   */
  [[nodiscard]] bool Loaded() const noexcept {
    return plugin_ != nullptr && library_.Loaded();
  }

  /**
   * @brief Gets reference to the loaded plugin.
   * @warning Triggers assertion if plugin is not loaded.
   * @return Reference to the plugin
   */
  [[nodiscard]] Plugin& GetPlugin() noexcept;

  /**
   * @brief Gets const reference to the loaded plugin.
   * @warning Triggers assertion if plugin is not loaded.
   * @return Const reference to the plugin
   */
  [[nodiscard]] const Plugin& GetPlugin() const noexcept;

  /**
   * @brief Tries to get the loaded plugin.
   * @return Pointer to plugin, or `nullptr` if not loaded
   */
  [[nodiscard]] Plugin* TryGetPlugin() noexcept { return plugin_.get(); }

  /**
   * @brief Tries to get the loaded plugin.
   * @return Const pointer to plugin, or `nullptr` if not loaded
   */
  [[nodiscard]] const Plugin* TryGetPlugin() const noexcept {
    return plugin_.get();
  }

  /**
   * @brief Releases ownership of the plugin.
   * @details After this call, the `DynamicPlugin` no longer owns the plugin.
   * The caller is responsible for the plugin's lifetime.
   * @return Unique pointer to the plugin
   */
  [[nodiscard]] auto ReleasePlugin() noexcept -> std::unique_ptr<Plugin> {
    return std::move(plugin_);
  }

  /**
   * @brief Gets the plugin type ID.
   * @return Plugin type ID, or 0 if not loaded
   */
  [[nodiscard]] PluginTypeId GetPluginTypeId() const noexcept {
    return plugin_type_id_;
  }

  /**
   * @brief Gets the plugin name.
   * @return Plugin name, or empty string if not loaded
   */
  [[nodiscard]] std::string_view GetPluginName() const noexcept {
    return plugin_type_id_.Name();
  }

  /**
   * @brief Gets the path of the loaded library.
   * @return Path to the library, or empty if not loaded
   */
  [[nodiscard]] const std::filesystem::path& Path() const noexcept {
    return library_.Path();
  }

  /**
   * @brief Gets reference to the underlying dynamic library.
   * @return Reference to `DynamicLibrary`
   */
  [[nodiscard]] utils::DynamicLibrary& Library() noexcept { return library_; }

  /**
   * @brief Gets const reference to the underlying dynamic library.
   * @return Const reference to `DynamicLibrary`
   */
  [[nodiscard]] const utils::DynamicLibrary& Library() const noexcept {
    return library_;
  }

  /**
   * @brief Gets the configuration used for this plugin.
   * @return Configuration struct
   */
  [[nodiscard]] const DynamicPluginConfig& Config() const noexcept {
    return config_;
  }

private:
  /**
   * @brief Loads plugin symbols and creates the plugin instance.
   * @return `DynamicPluginResult` that holds void on success, or
   * `DynamicPluginError` on failure
   */
  [[nodiscard]] auto LoadPluginInstance() -> DynamicPluginResult<void>;

  utils::DynamicLibrary library_;  ///< The dynamic library

  std::unique_ptr<Plugin> plugin_;  ///< The created plugin instance (owned)
  PluginTypeId plugin_type_id_;     ///< Cached plugin type ID

  DynamicPluginConfig config_;  ///< Plugin configuration
  FileTime last_write_time_;    ///< Last known file modification time
};

}  // namespace helios::app
