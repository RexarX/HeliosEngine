#include <helios/core/utils/dynamic_library.hpp>

#include <helios/core/logger.hpp>

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

#ifdef HELIOS_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(HELIOS_PLATFORM_LINUX) || defined(HELIOS_PLATFORM_MACOS)
#include <dlfcn.h>
#else
#error "Unsupported platform for dynamic library loading"
#endif

namespace helios::utils {

auto DynamicLibrary::Load(const std::filesystem::path& path) -> std::expected<void, DynamicLibraryError> {
  if (Loaded()) {
    return std::unexpected(DynamicLibraryError::AlreadyLoaded);
  }

  if (!std::filesystem::exists(path)) {
    return std::unexpected(DynamicLibraryError::FileNotFound);
  }

#ifdef HELIOS_PLATFORM_WINDOWS
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  handle_ = reinterpret_cast<HandleType>(LoadLibraryW(path.wstring().c_str()));
#else
  handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif

  if (handle_ == kInvalidHandle) {
    HELIOS_ERROR("Failed to load library '{}': {}!", path.string(), GetLastErrorMessage());
    return std::unexpected(DynamicLibraryError::LoadFailed);
  }

  path_ = path;
  HELIOS_DEBUG("Loaded dynamic library: {}", path_.string());
  return {};
}

auto DynamicLibrary::Unload() -> std::expected<void, DynamicLibraryError> {
  if (!Loaded()) {
    return std::unexpected(DynamicLibraryError::NotLoaded);
  }

  bool success = false;
#ifdef HELIOS_PLATFORM_WINDOWS
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  success = FreeLibrary(reinterpret_cast<HMODULE>(handle_)) != 0;
#else
  success = dlclose(handle_) == 0;
#endif

  if (!success) {
    HELIOS_ERROR("Failed to unload library '{}': {}!", path_.string(), GetLastErrorMessage());
    return std::unexpected(DynamicLibraryError::PlatformError);
  }

  HELIOS_DEBUG("Unloaded dynamic library: {}", path_.string());
  handle_ = kInvalidHandle;
  path_.clear();
  return {};
}

auto DynamicLibrary::GetSymbolAddress(std::string_view name) const -> std::expected<void*, DynamicLibraryError> {
  if (!Loaded()) {
    return std::unexpected(DynamicLibraryError::NotLoaded);
  }

  // Need null-terminated string for platform APIs
  std::string name_str(name);

  void* symbol = nullptr;
#ifdef HELIOS_PLATFORM_WINDOWS
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  symbol = reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle_), name_str.c_str()));
#else
  symbol = dlsym(handle_, name_str.c_str());
#endif

  if (symbol == nullptr) {
    HELIOS_WARN("Symbol '{}' not found in library '{}': {}!", name, path_.string(), GetLastErrorMessage());
    return std::unexpected(DynamicLibraryError::SymbolNotFound);
  }

  return symbol;
}

std::string DynamicLibrary::GetLastErrorMessage() noexcept {
#ifdef HELIOS_PLATFORM_WINDOWS
  DWORD error_code = GetLastError();
  if (error_code == 0) {
    return "No error";
  }

  LPSTR message_buffer = nullptr;
  const size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);

  std::string message(message_buffer, size);
  LocalFree(message_buffer);

  // Remove trailing newline
  while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
    message.pop_back();
  }

  return message;
#else
  const char* error = dlerror();
  return error != nullptr ? std::string(error) : "No error";
#endif
}

}  // namespace helios::utils
