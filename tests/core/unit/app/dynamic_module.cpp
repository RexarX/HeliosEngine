#include <doctest/doctest.h>

#include <helios/core/app/dynamic_module.hpp>
#include <helios/core/utils/dynamic_library.hpp>

#include <filesystem>
#include <string>
#include <string_view>

using namespace helios::app;

TEST_SUITE("DynamicModule") {
  TEST_CASE("Default construction") {
    DynamicModule module;

    CHECK_FALSE(module.Loaded());
    CHECK_EQ(module.GetModulePtr(), nullptr);
    CHECK(module.Path().empty());
    CHECK_EQ(module.GetModuleId(), 0);
    CHECK(module.GetModuleName().empty());
  }

  TEST_CASE("Load non-existent library") {
    DynamicModule module;
    auto result = module.Load("/nonexistent/path/to/module.so");

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::LibraryLoadFailed);
    CHECK_FALSE(module.Loaded());
  }

  TEST_CASE("Unload when not loaded") {
    DynamicModule module;
    auto result = module.Unload();

    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::NotLoaded);
  }

  TEST_CASE("HasFileChanged when not loaded") {
    DynamicModule module;
    CHECK_FALSE(module.HasFileChanged());
  }

  TEST_CASE("Move construction") {
    DynamicModule module1;
    DynamicModule module2(std::move(module1));

    CHECK_FALSE(module2.Loaded());
    CHECK_FALSE(module1.Loaded());       // NOLINT(bugprone-use-after-move)
    CHECK_EQ(module1.GetModuleId(), 0);  // NOLINT(bugprone-use-after-move)
  }

  TEST_CASE("Move assignment") {
    DynamicModule module1;
    DynamicModule module2;

    module2 = std::move(module1);

    CHECK_FALSE(module2.Loaded());
  }

  TEST_CASE("Default config values") {
    DynamicModuleConfig config;

    CHECK_EQ(config.create_symbol, kDefaultCreateSymbol);
    CHECK_EQ(config.module_id_symbol, kDefaultModuleIdSymbol);
    CHECK_EQ(config.module_name_symbol, kDefaultModuleNameSymbol);
    CHECK_FALSE(config.auto_reload);
  }

  TEST_CASE("Error to string") {
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::LibraryLoadFailed), "Failed to load dynamic library");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::CreateSymbolNotFound),
             "Module creation function not found");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::IdSymbolNotFound), "Module ID function not found");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::NameSymbolNotFound), "Module name function not found");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::CreateFailed), "Module creation function returned nullptr");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::NotLoaded), "Module is not loaded");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::ReloadFailed), "Failed to reload module");
    CHECK_EQ(DynamicModuleErrorToString(DynamicModuleError::FileNotChanged), "File has not been modified");
  }

  TEST_CASE("Config accessors") {
    DynamicModule module;

    const auto& config = module.Config();
    CHECK_EQ(config.create_symbol, kDefaultCreateSymbol);
    CHECK_EQ(config.module_id_symbol, kDefaultModuleIdSymbol);
    CHECK_EQ(config.module_name_symbol, kDefaultModuleNameSymbol);
  }

  TEST_CASE("Library accessor") {
    DynamicModule module;

    // Check we can access the library
    const helios::utils::DynamicLibrary& lib = module.Library();
    CHECK_FALSE(lib.Loaded());

    helios::utils::DynamicLibrary& mut_lib = module.Library();
    CHECK_FALSE(mut_lib.Loaded());
  }

  TEST_CASE("Symbol names") {
    CHECK_EQ(kDefaultCreateSymbol, "helios_create_module");
    CHECK_EQ(kDefaultModuleIdSymbol, "helios_module_id");
    CHECK_EQ(kDefaultModuleNameSymbol, "helios_module_name");
  }

  TEST_CASE("Custom config") {
    DynamicModuleConfig config{
        .create_symbol = "custom_create",
        .module_id_symbol = "custom_module_id",
        .module_name_symbol = "custom_module_name",
        .auto_reload = true,
    };

    CHECK_EQ(config.create_symbol, "custom_create");
    CHECK_EQ(config.module_id_symbol, "custom_module_id");
    CHECK_EQ(config.module_name_symbol, "custom_module_name");
    CHECK(config.auto_reload);
  }

  TEST_CASE("ReleaseModule when not loaded") {
    DynamicModule module;
    auto released = module.ReleaseModule();
    CHECK_EQ(released, nullptr);
  }

  TEST_CASE("GetModuleId when not loaded") {
    DynamicModule module;
    CHECK_EQ(module.GetModuleId(), 0);
  }

  TEST_CASE("GetModuleName when not loaded") {
    DynamicModule module;
    CHECK(module.GetModuleName().empty());
  }
}

// Note: Full integration tests for actual library loading would require
// building a test shared library. These tests cover the API surface and
// error handling without requiring actual dynamic library files.
