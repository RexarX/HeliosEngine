#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/logger.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>

namespace helios::utils {

/**
 * @brief Error codes for dynamic library operations.
 */
enum class DynamicLibraryError : uint8_t {
  FileNotFound,    ///< Library file not found
  LoadFailed,      ///< Failed to load library
  SymbolNotFound,  ///< Symbol not found in library
  InvalidHandle,   ///< Invalid library handle
  AlreadyLoaded,   ///< Library is already loaded
  NotLoaded,       ///< Library is not loaded
  PlatformError,   ///< Platform-specific error
};

/**
 * @brief Gets a human-readable description for a DynamicLibraryError.
 * @param error The error code
 * @return String description of the error
 */
[[nodiscard]] constexpr std::string_view DynamicLibraryErrorToString(DynamicLibraryError error) noexcept {
  switch (error) {
    case DynamicLibraryError::FileNotFound:
      return "Library file not found";
    case DynamicLibraryError::LoadFailed:
      return "Failed to load library";
    case DynamicLibraryError::SymbolNotFound:
      return "Symbol not found in library";
    case DynamicLibraryError::InvalidHandle:
      return "Invalid library handle";
    case DynamicLibraryError::AlreadyLoaded:
      return "Library is already loaded";
    case DynamicLibraryError::NotLoaded:
      return "Library is not loaded";
    case DynamicLibraryError::PlatformError:
      return "Platform-specific error";
    default:
      return "Unknown error";
  }
}

/**
 * @brief Cross-platform dynamic library loader.
 * @details Provides a unified interface for loading dynamic libraries (DLLs on Windows,
 * .so on Linux, .dylib on macOS) and retrieving function symbols.
 *
 * @note Not thread-safe. External synchronization required for concurrent access.
 *
 * @example
 * @code
 * DynamicLibrary lib;
 * if (auto result = lib.Load("my_module.so"); result) {
 *   using CreateModuleFn = Module* (*)();
 *   if (auto fn = lib.GetSymbol<CreateModuleFn>("create_module"); fn) {
 *     Module* module = (*fn)();
 *     // Use module...
 *   }
 * }
 * @endcode
 */
class DynamicLibrary {
public:
  /// Native handle type (void* on all platforms for ABI stability)
  using HandleType = void*;
  static constexpr HandleType kInvalidHandle = nullptr;

  DynamicLibrary() = default;

  /**
   * @brief Constructs and loads a library from the specified path.
   * @param path Path to the dynamic library
   */
  explicit DynamicLibrary(const std::filesystem::path& path);
  DynamicLibrary(const DynamicLibrary&) = delete;
  DynamicLibrary(DynamicLibrary&& other) noexcept;

  /**
   * @brief Destructor that unloads the library if loaded.
   */
  ~DynamicLibrary() noexcept;

  DynamicLibrary& operator=(const DynamicLibrary&) = delete;
  DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

  [[nodiscard]] static auto FromPath(const std::filesystem::path& path)
      -> std::expected<DynamicLibrary, DynamicLibraryError>;

  /**
   * @brief Loads a dynamic library from the specified path.
   * @param path Path to the dynamic library
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto Load(const std::filesystem::path& path) -> std::expected<void, DynamicLibraryError>;

  /**
   * @brief Unloads the currently loaded library.
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto Unload() -> std::expected<void, DynamicLibraryError>;

  /**
   * @brief Reloads the library from the same path.
   * @details Unloads the current library and loads it again.
   * @return Expected with void on success, or error on failure
   */
  [[nodiscard]] auto Reload() -> std::expected<void, DynamicLibraryError>;

  /**
   * @brief Gets a raw symbol address from the library.
   * @param name Name of the symbol to retrieve
   * @return Expected with void pointer on success, or error on failure
   */
  [[nodiscard]] auto GetSymbolAddress(std::string_view name) const -> std::expected<void*, DynamicLibraryError>;

  /**
   * @brief Gets a typed function pointer from the library.
   * @tparam T Function pointer type
   * @param name Name of the symbol to retrieve
   * @return Expected with function pointer on success, or error on failure
   */
  template <typename T>
    requires std::is_pointer_v<T>
  [[nodiscard]] auto GetSymbol(std::string_view name) const -> std::expected<T, DynamicLibraryError>;

  /**
   * @brief Checks if a library is currently loaded.
   * @return True if a library is loaded
   */
  [[nodiscard]] bool Loaded() const noexcept { return handle_ != kInvalidHandle; }

  /**
   * @brief Gets the path of the loaded library.
   * @return Path to the library, or empty if not loaded
   */
  [[nodiscard]] const std::filesystem::path& Path() const noexcept { return path_; }

  /**
   * @brief Gets the native handle of the loaded library.
   * @return Native handle, or kInvalidHandle if not loaded
   */
  [[nodiscard]] HandleType Handle() const noexcept { return handle_; }

  /**
   * @brief Gets the last platform-specific error message.
   * @return Error message string
   */
  [[nodiscard]] static std::string GetLastErrorMessage() noexcept;

  /**
   * @brief Gets the platform-specific file extension for dynamic libraries.
   * @return ".dll" on Windows, ".so" on Linux, ".dylib" on macOS
   */
  [[nodiscard]] static constexpr std::string_view GetPlatformExtension() noexcept {
#ifdef HELIOS_PLATFORM_WINDOWS
    return ".dll";
#elifdef HELIOS_PLATFORM_MACOS
    return ".dylib";
#else
    return ".so";
#endif
  }

  /**
   * @brief Gets the platform-specific library prefix.
   * @return Empty on Windows, "lib" on Unix-like systems
   */
  [[nodiscard]] static constexpr std::string_view GetPlatformPrefix() noexcept {
#ifdef HELIOS_PLATFORM_WINDOWS
    return "";
#else
    return "lib";
#endif
  }

private:
  HandleType handle_ = kInvalidHandle;  ///< Native library handle
  std::filesystem::path path_;          ///< Path to the loaded library
};

inline DynamicLibrary::DynamicLibrary(const std::filesystem::path& path) {
  auto result = Load(path);
  if (!result) {
    HELIOS_ERROR("Failed to load dynamic library '{}': {}!", path.string(),
                 DynamicLibraryErrorToString(result.error()));
  }
}

inline DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : handle_(other.handle_), path_(std::move(other.path_)) {
  other.handle_ = kInvalidHandle;
}

inline DynamicLibrary::~DynamicLibrary() noexcept {
  if (Loaded()) {
    auto result = Unload();
    if (!result) {
      HELIOS_WARN("Failed to unload dynamic library '{}': {}!", path_.string(),
                  DynamicLibraryErrorToString(result.error()));
    }
  }
}

inline DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
  if (this != &other) {
    if (Loaded()) {
      [[maybe_unused]] auto _ = Unload();
    }
    handle_ = other.handle_;
    path_ = std::move(other.path_);
    other.handle_ = kInvalidHandle;
  }
  return *this;
}

inline auto FromPath(const std::filesystem::path& path) -> std::expected<DynamicLibrary, DynamicLibraryError> {
  DynamicLibrary lib;
  if (const auto result = lib.Load(path); !result) {
    return std::unexpected(result.error());
  }
  return lib;
}

inline auto DynamicLibrary::Reload() -> std::expected<void, DynamicLibraryError> {
  if (!Loaded()) {
    return std::unexpected(DynamicLibraryError::NotLoaded);
  }

  const auto saved_path = path_;

  auto unload_result = Unload();
  if (!unload_result) {
    return unload_result;
  }

  return Load(saved_path);
}

template <typename T>
  requires std::is_pointer_v<T>
inline auto DynamicLibrary::GetSymbol(std::string_view name) const -> std::expected<T, DynamicLibraryError> {
  auto result = GetSymbolAddress(name);
  if (!result) {
    return std::unexpected(result.error());
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<T>(*result);
}

}  // namespace helios::utils
