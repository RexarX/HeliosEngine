#include <doctest/doctest.h>

#include <helios/core/utils/dynamic_library.hpp>

#include <concepts>
#include <filesystem>
#include <string>
#include <string_view>

using namespace helios::utils;

TEST_SUITE("DynamicLibrary") {
  TEST_CASE("Default construction") {
    DynamicLibrary lib;

    CHECK_FALSE(lib.Loaded());
    CHECK_EQ(lib.Handle(), DynamicLibrary::kInvalidHandle);
    CHECK(lib.Path().empty());
  }

  TEST_CASE("Platform extension") {
    auto ext = DynamicLibrary::GetPlatformExtension();

#if defined(HELIOS_PLATFORM_WINDOWS)
    CHECK_EQ(ext, ".dll");
#elif defined(HELIOS_PLATFORM_MACOS)
    CHECK_EQ(ext, ".dylib");
#else
    CHECK_EQ(ext, ".so");
#endif
  }

  TEST_CASE("Platform prefix") {
    auto prefix = DynamicLibrary::GetPlatformPrefix();

#if defined(HELIOS_PLATFORM_WINDOWS)
    CHECK(prefix.empty());
#else
    CHECK_EQ(prefix, "lib");
#endif
  }

  TEST_CASE("Load non-existent library") {
    DynamicLibrary lib;
    auto result = lib.Load("/nonexistent/path/to/library.so");

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicLibraryError::FileNotFound);
    CHECK_FALSE(lib.Loaded());
  }

  TEST_CASE("Unload when not loaded") {
    DynamicLibrary lib;
    auto result = lib.Unload();

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicLibraryError::NotLoaded);
  }

  TEST_CASE("Reload when not loaded") {
    DynamicLibrary lib;
    auto result = lib.Reload();

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicLibraryError::NotLoaded);
  }

  TEST_CASE("Get symbol when not loaded") {
    DynamicLibrary lib;
    auto result = lib.GetSymbolAddress("some_symbol");

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicLibraryError::NotLoaded);
  }

  TEST_CASE("GetSymbol typed version") {
    DynamicLibrary lib;
    using FnType = void (*)();
    auto result = lib.GetSymbol<FnType>("some_function");

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicLibraryError::NotLoaded);
  }

  TEST_CASE("Move construction") {
    DynamicLibrary lib1;
    // Can't test with actual library, but can test the move mechanics
    DynamicLibrary lib2(std::move(lib1));

    CHECK_FALSE(lib2.Loaded());
    CHECK_FALSE(lib1.Loaded());  // NOLINT(bugprone-use-after-move)
  }

  TEST_CASE("Move assignment") {
    DynamicLibrary lib1;
    DynamicLibrary lib2;

    lib2 = std::move(lib1);

    CHECK_FALSE(lib2.Loaded());
  }

  TEST_CASE("Error to string") {
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::FileNotFound), "Library file not found");
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::LoadFailed), "Failed to load library");
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::SymbolNotFound), "Symbol not found in library");
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::InvalidHandle), "Invalid library handle");
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::AlreadyLoaded), "Library is already loaded");
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::NotLoaded), "Library is not loaded");
    CHECK_EQ(DynamicLibraryErrorToString(DynamicLibraryError::PlatformError), "Platform-specific error");
  }

  TEST_CASE("GetLastErrorMessage returns string") {
    // Just verify it doesn't crash and returns something
    std::string msg = DynamicLibrary::GetLastErrorMessage();
    // Message content varies by platform, just check it's callable
    CHECK_GE(msg.length(), 0);
  }

  TEST_CASE("kInvalidHandle is nullptr") {
    CHECK_EQ(DynamicLibrary::kInvalidHandle, nullptr);
  }

  TEST_CASE("HandleType is void*") {
    static_assert(std::same_as<DynamicLibrary::HandleType, void*>);
  }

  TEST_CASE("Path is empty when not loaded") {
    DynamicLibrary lib;
    CHECK(lib.Path().empty());
  }

  TEST_CASE("Self move assignment is safe") {
    DynamicLibrary lib;
    lib = std::move(lib);  // NOLINT(clang-diagnostic-self-move)
    CHECK_FALSE(lib.Loaded());
  }
}
