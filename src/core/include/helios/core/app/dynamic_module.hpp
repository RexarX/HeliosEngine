#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/module.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/dynamic_library.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace helios::app {

// Forward declaration
class App;

/**
 * @brief Function signature for module creation.
 * @details Dynamic modules must export a function with this signature.
 * The function should create and return a new Module instance.
 */
using CreateModuleFn = Module* (*)();

/**
 * @brief Function signature for getting module type ID.
 * @details Dynamic modules must export a function with this signature.
 * The function should return the unique type ID of the module.
 */
using ModuleIdFn = ModuleTypeId (*)();

/**
 * @brief Function signature for getting module name.
 * @details Dynamic modules must export a function with this signature.
 * The function should return the name of the module as a null-terminated string.
 */
using ModuleNameFn = const char* (*)();

/**
 * @brief Default symbol name for the module creation function.
 */
inline constexpr std::string_view kDefaultCreateSymbol = "helios_create_module";

/**
 * @brief Default symbol name for the module ID function.
 */
inline constexpr std::string_view kDefaultModuleIdSymbol = "helios_module_id";

/**
 * @brief Default symbol name for the module name function.
 */
inline constexpr std::string_view kDefaultModuleNameSymbol = "helios_module_name";

/**
 * @brief Error codes for dynamic module operations.
 */
enum class DynamicModuleError : uint8_t {
  LibraryLoadFailed,     ///< Failed to load the dynamic library
  CreateSymbolNotFound,  ///< Module creation function not found
  IdSymbolNotFound,      ///< Module ID function not found
  NameSymbolNotFound,    ///< Module name function not found
  CreateFailed,          ///< Module creation function returned nullptr
  NotLoaded,             ///< Module is not loaded
  ReloadFailed,          ///< Failed to reload module
  FileNotChanged,        ///< File has not been modified
};

/**
 * @brief Gets a human-readable description for a DynamicModuleError.
 * @param error The error code
 * @return String description of the error
 */
[[nodiscard]] constexpr std::string_view DynamicModuleErrorToString(DynamicModuleError error) noexcept {
  switch (error) {
    case DynamicModuleError::LibraryLoadFailed:
      return "Failed to load dynamic library";
    case DynamicModuleError::CreateSymbolNotFound:
      return "Module creation function not found";
    case DynamicModuleError::IdSymbolNotFound:
      return "Module ID function not found";
    case DynamicModuleError::NameSymbolNotFound:
      return "Module name function not found";
    case DynamicModuleError::CreateFailed:
      return "Module creation function returned nullptr";
    case DynamicModuleError::NotLoaded:
      return "Module is not loaded";
    case DynamicModuleError::ReloadFailed:
      return "Failed to reload module";
    case DynamicModuleError::FileNotChanged:
      return "File has not been modified";
    default:
      return "Unknown error";
  }
}

/**
 * @brief Configuration for dynamic module loading.
 */
struct DynamicModuleConfig {
  std::string_view create_symbol = kDefaultCreateSymbol;           ///< Name of the creation function
  std::string_view module_id_symbol = kDefaultModuleIdSymbol;      ///< Name of the module ID function
  std::string_view module_name_symbol = kDefaultModuleNameSymbol;  ///< Name of the module name function
  bool auto_reload = false;                                        ///< Enable automatic reload on file change
};

/**
 * @brief Wrapper for dynamically loaded modules.
 * @details Loads a module from a shared library and manages its lifecycle.
 * Supports hot-reloading: when the library file changes, the module can be
 * unloaded and reloaded without restarting the application.
 *
 * The dynamic library must export:
 * - A creation function (default: "helios_create_module") that returns Module*
 * - A module ID function (default: "helios_module_id") that returns ModuleTypeId
 * - A module name function (default: "helios_module_name") that returns const char*
 *
 * @note Not thread-safe. External synchronization required for concurrent access.
 *
 * @example Library implementation:
 * @code
 * // In my_module.cpp (compiled as shared library)
 * class MyModule : public helios::app::Module {
 * public:
 *   static constexpr std::string_view GetName() noexcept { return "MyModule"; }
 *   void Build(App& app) override { ... }
 *   void Destroy(App& app) override { ... }
 * };
 *
 * extern "C" {
 *   HELIOS_EXPORT helios::app::Module* helios_create_module() {
 *     return new MyModule();
 *   }
 *   HELIOS_EXPORT helios::app::ModuleTypeId helios_module_id() {
 *     return helios::app::ModuleTypeIdOf<MyModule>();
 *   }
 *   HELIOS_EXPORT const char* helios_module_name() {
 *     return "MyModule";
 *   }
 * }
 * @endcode
 *
 * @example Usage:
 * @code
 * DynamicModule dyn_module;
 * if (auto result = dyn_module.Load("my_module.so"); result) {
 *   app.AddDynamicModule(std::move(dyn_module));
 * }
 * @endcode
 */
class DynamicModule {
public:
  using FileTime = std::filesystem::file_time_type;

  DynamicModule() = default;

  /**
   * @brief Constructs and loads a module from the specified path.
   * @param path Path to the dynamic library
   * @param config Configuration options
   */
  explicit DynamicModule(const std::filesystem::path& path, DynamicModuleConfig config = {});
  DynamicModule(const DynamicModule&) = delete;
  DynamicModule(DynamicModule&& other) noexcept;

  /**
   * @brief Destructor that destroys the module if loaded.
   */
  ~DynamicModule() noexcept = default;

  DynamicModule& operator=(const DynamicModule&) = delete;
  DynamicModule& operator=(DynamicModule&& other) noexcept;

  /**
   * @brief Loads a module from the specified path.
   * @param path Path to the dynamic library
   * @param config Configuration options
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto Load(const std::filesystem::path& path, DynamicModuleConfig config = {})
      -> std::expected<void, DynamicModuleError>;

  /**
   * @brief Unloads the current module.
   * @details Releases the module and unloads the library.
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto Unload() -> std::expected<void, DynamicModuleError>;

  /**
   * @brief Reloads the module from the same path.
   * @details Calls Destroy on the old module, unloads the library,
   * loads it again, and calls Build on the new module.
   * @param app Reference to the app for Build/Destroy calls
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto Reload(App& app) -> std::expected<void, DynamicModuleError>;

  /**
   * @brief Reloads the module only if the file has changed.
   * @param app Reference to the app for Build/Destroy calls
   * @return Expected with void on success, FileNotChanged if not modified, or error on failure
   */
  [[nodiscard]] auto ReloadIfChanged(App& app) -> std::expected<void, DynamicModuleError>;

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
   * @brief Checks if a module is currently loaded.
   * @return True if a module is loaded
   */
  [[nodiscard]] bool Loaded() const noexcept { return module_ != nullptr && library_.Loaded(); }

  /**
   * @brief Gets reference to the loaded module.
   * @warning Triggers assertion if module is not loaded.
   * @return Reference to the module
   */
  [[nodiscard]] Module& GetModule() noexcept;

  /**
   * @brief Gets const reference to the loaded module.
   * @warning Triggers assertion if module is not loaded.
   * @return Const reference to the module
   */
  [[nodiscard]] const Module& GetModule() const noexcept;

  /**
   * @brief Gets pointer to the loaded module.
   * @return Pointer to module, or nullptr if not loaded
   */
  [[nodiscard]] Module* GetModulePtr() noexcept { return module_.get(); }

  /**
   * @brief Gets const pointer to the loaded module.
   * @return Const pointer to module, or nullptr if not loaded
   */
  [[nodiscard]] const Module* GetModulePtr() const noexcept { return module_.get(); }

  /**
   * @brief Releases ownership of the module.
   * @details After this call, the DynamicModule no longer owns the module.
   * The caller is responsible for the module's lifetime.
   * @return Unique pointer to the module
   */
  [[nodiscard]] std::unique_ptr<Module> ReleaseModule() noexcept { return std::move(module_); }

  /**
   * @brief Gets the module type ID.
   * @return Module type ID, or 0 if not loaded
   */
  [[nodiscard]] ModuleTypeId GetModuleId() const noexcept { return module_id_; }

  /**
   * @brief Gets the module name.
   * @return Module name, or empty string if not loaded
   */
  [[nodiscard]] std::string_view GetModuleName() const noexcept { return module_name_; }

  /**
   * @brief Gets the path of the loaded library.
   * @return Path to the library, or empty if not loaded
   */
  [[nodiscard]] const std::filesystem::path& Path() const noexcept { return library_.Path(); }

  /**
   * @brief Gets reference to the underlying dynamic library.
   * @return Reference to helios::utils::DynamicLibrary
   */
  [[nodiscard]] helios::utils::DynamicLibrary& Library() noexcept { return library_; }

  /**
   * @brief Gets const reference to the underlying dynamic library.
   * @return Const reference to helios::utils::DynamicLibrary
   */
  [[nodiscard]] const helios::utils::DynamicLibrary& Library() const noexcept { return library_; }

  /**
   * @brief Gets the configuration used for this module.
   * @return Configuration struct
   */
  [[nodiscard]] const DynamicModuleConfig& Config() const noexcept { return config_; }

private:
  /**
   * @brief Loads module symbols and creates the module instance.
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto LoadModuleInstance() -> std::expected<void, DynamicModuleError>;

  helios::utils::DynamicLibrary library_;  ///< The dynamic library

  std::unique_ptr<Module> module_;  ///< The created module instance (owned)
  ModuleTypeId module_id_ = 0;      ///< Cached module type ID
  std::string module_name_;         ///< Cached module name

  DynamicModuleConfig config_;  ///< Module configuration
  FileTime last_write_time_{};  ///< Last known file modification time
};

inline DynamicModule::DynamicModule(const std::filesystem::path& path, DynamicModuleConfig config) {
  auto result = Load(path, config);
  if (!result) {
    HELIOS_ERROR("Failed to load dynamic module '{}': {}", path.string(), DynamicModuleErrorToString(result.error()));
  }
}

inline DynamicModule::DynamicModule(DynamicModule&& other) noexcept
    : library_(std::move(other.library_)),
      module_(std::move(other.module_)),
      module_id_(other.module_id_),
      module_name_(std::move(other.module_name_)),
      config_(other.config_),
      last_write_time_(other.last_write_time_) {
  other.module_id_ = 0;
}

inline DynamicModule& DynamicModule::operator=(DynamicModule&& other) noexcept {
  if (this != &other) {
    module_.reset();
    library_ = std::move(other.library_);
    module_ = std::move(other.module_);
    module_id_ = other.module_id_;
    module_name_ = std::move(other.module_name_);
    config_ = other.config_;
    last_write_time_ = other.last_write_time_;

    other.module_id_ = 0;
  }
  return *this;
}

inline auto DynamicModule::Load(const std::filesystem::path& path, DynamicModuleConfig config)
    -> std::expected<void, DynamicModuleError> {
  config_ = config;

  // Load the library
  const auto load_result = library_.Load(path);
  if (!load_result) {
    return std::unexpected(DynamicModuleError::LibraryLoadFailed);
  }

  // Load module symbols and create instance
  auto module_result = LoadModuleInstance();
  if (!module_result) {
    [[maybe_unused]] auto _ = library_.Unload();
    return module_result;
  }

  // Cache the file modification time
  UpdateFileTime();

  HELIOS_INFO("Loaded dynamic module '{}' from: {}", module_name_, path.string());
  return {};
}

inline auto DynamicModule::Unload() -> std::expected<void, DynamicModuleError> {
  if (!Loaded()) {
    return std::unexpected(DynamicModuleError::NotLoaded);
  }

  // Release module before unloading library
  module_.reset();
  module_id_ = 0;
  module_name_.clear();

  auto unload_result = library_.Unload();
  if (!unload_result) {
    return std::unexpected(DynamicModuleError::LibraryLoadFailed);
  }

  return {};
}

inline auto DynamicModule::Reload(App& app) -> std::expected<void, DynamicModuleError> {
  if (!Loaded()) {
    return std::unexpected(DynamicModuleError::NotLoaded);
  }

  // Call Destroy on the old module
  HELIOS_INFO("Reloading dynamic module '{}': {}", module_name_, library_.Path().string());
  module_->Destroy(app);

  // Save the path and config before unloading
  auto saved_path = library_.Path();
  auto saved_config = config_;

  // Release module and unload library
  module_.reset();
  module_id_ = 0;
  module_name_.clear();

  auto unload_result = library_.Unload();
  if (!unload_result) {
    return std::unexpected(DynamicModuleError::ReloadFailed);
  }

  // Reload the library
  auto load_result = library_.Load(saved_path);
  if (!load_result) {
    return std::unexpected(DynamicModuleError::ReloadFailed);
  }

  // Load module symbols and create new instance
  config_ = saved_config;
  auto module_result = LoadModuleInstance();
  if (!module_result) {
    [[maybe_unused]] auto _ = library_.Unload();
    return std::unexpected(DynamicModuleError::ReloadFailed);
  }

  // Call Build on the new module
  module_->Build(app);

  // Update file time
  UpdateFileTime();

  HELIOS_INFO("Successfully reloaded dynamic module '{}': {}", module_name_, saved_path.string());
  return {};
}

inline auto DynamicModule::ReloadIfChanged(App& app) -> std::expected<void, DynamicModuleError> {
  if (!HasFileChanged()) {
    return std::unexpected(DynamicModuleError::FileNotChanged);
  }

  return Reload(app);
}

inline void DynamicModule::UpdateFileTime() noexcept {
  if (!library_.Loaded()) {
    return;
  }

  std::error_code ec;
  last_write_time_ = std::filesystem::last_write_time(library_.Path(), ec);
}

inline bool DynamicModule::HasFileChanged() const noexcept {
  if (!library_.Loaded()) {
    return false;
  }

  std::error_code ec;
  auto current_time = std::filesystem::last_write_time(library_.Path(), ec);
  if (ec) {
    return false;
  }

  return current_time != last_write_time_;
}

inline Module& DynamicModule::GetModule() noexcept {
  HELIOS_ASSERT(Loaded(), "Failed to get module: Module is not loaded!");
  return *module_;
}

inline const Module& DynamicModule::GetModule() const noexcept {
  HELIOS_ASSERT(Loaded(), "Failed to get module: Module is not loaded!");
  return *module_;
}

inline auto DynamicModule::LoadModuleInstance() -> std::expected<void, DynamicModuleError> {
  // Get create function
  auto create_result = library_.GetSymbol<CreateModuleFn>(config_.create_symbol);
  if (!create_result) {
    HELIOS_ERROR("Create function '{}' not found in library", config_.create_symbol);
    return std::unexpected(DynamicModuleError::CreateSymbolNotFound);
  }

  // Get module ID function
  auto id_result = library_.GetSymbol<ModuleIdFn>(config_.module_id_symbol);
  if (!id_result) {
    HELIOS_ERROR("Module ID function '{}' not found in library", config_.module_id_symbol);
    return std::unexpected(DynamicModuleError::IdSymbolNotFound);
  }

  // Get module name function
  auto name_result = library_.GetSymbol<ModuleNameFn>(config_.module_name_symbol);
  if (!name_result) {
    HELIOS_ERROR("Module name function '{}' not found in library", config_.module_name_symbol);
    return std::unexpected(DynamicModuleError::NameSymbolNotFound);
  }

  // Get module ID and name
  ModuleIdFn id_fn = *id_result;
  ModuleNameFn name_fn = *name_result;
  module_id_ = id_fn();
  module_name_ = name_fn();

  // Create the module
  CreateModuleFn create_fn = *create_result;
  Module* raw_module = create_fn();

  if (raw_module == nullptr) {
    HELIOS_ERROR("Module creation function returned nullptr");
    module_id_ = 0;
    module_name_.clear();
    return std::unexpected(DynamicModuleError::CreateFailed);
  }

  module_.reset(raw_module);
  return {};
}

}  // namespace helios::app
