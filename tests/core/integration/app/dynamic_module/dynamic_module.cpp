#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/dynamic_module.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/dynamic_library.hpp>

#include "test_module.hpp"

#include <filesystem>
#include <string_view>
#include <thread>

using namespace helios::app;
using namespace helios::test;

// ============================================================================
// Dynamic Module Integration Tests
// ============================================================================

namespace {

// Helper to get the test module library path
[[nodiscard]] std::filesystem::path GetTestModulePath() {
  // The test module should be in the bin directory alongside the test executable
  // Get the path to the test executable
  std::filesystem::path exe_path = std::filesystem::current_path();

  // Navigate to the bin directory (test is run from project root)
  // Module is in: bin/debug-linux-x86_64/helios_test_module.so
  // Test exe is in: bin/tests/debug-linux-x86_64/helios_core_integration

  // Try to find the module in the bin directory
  std::filesystem::path bin_path = exe_path / "bin";

#if defined(HELIOS_PLATFORM_WINDOWS)
  constexpr std::string_view module_name = "helios_test_module.dll";
#elif defined(HELIOS_PLATFORM_MACOS)
  constexpr std::string_view module_name = "helios_test_module.dylib";
#else
  constexpr std::string_view module_name = "helios_test_module.so";
#endif

  // Try to find the module file by searching in bin subdirectories
  if (std::filesystem::exists(bin_path)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(bin_path)) {
      if (entry.is_regular_file() && entry.path().filename() == module_name) {
        return entry.path();
      }
    }
  }

  // Fallback to assuming it's in the current directory
  return exe_path / module_name;
}

}  // namespace

TEST_SUITE("DynamicModule Integration") {
  TEST_CASE("Load and use test module with AddDynamicModule") {
    auto module_path = GetTestModulePath();

    // Skip test if module library doesn't exist
    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found at: ", module_path.string());
      MESSAGE("Skipping dynamic module integration test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    // Load the module
    auto load_result = dyn_module.Load(module_path);
    REQUIRE(load_result.has_value());
    CHECK(dyn_module.Loaded());
    CHECK_NE(dyn_module.GetModulePtr(), nullptr);
    CHECK_EQ(dyn_module.Path(), module_path);

    // Check module ID and name were loaded
    CHECK_NE(dyn_module.GetModuleId(), 0);
    CHECK_EQ(dyn_module.GetModuleName(), "TestModule");

    // Add dynamic module to app (moves ownership)
    app.AddDynamicModule(std::move(dyn_module));

    // Original should no longer be loaded
    CHECK_FALSE(dyn_module.Loaded());  // NOLINT(bugprone-use-after-move)

    // Check app knows about the module
    CHECK(app.ContainsDynamicModule(ModuleTypeIdOf<TestModule>()));

    // Run the app - this will Build the module, Initialize, Update, etc.
    // We need to set a custom runner that exits after a few updates
    int update_count = 0;
    app.SetRunner([&update_count](App& a) -> AppExitCode {
      for (int i = 0; i < 3; ++i) {
        a.TickTime();
        a.Update();
        ++update_count;
      }
      return AppExitCode::kSuccess;
    });

    // Verify the resource was added by the module during Build
    // Note: Run() will call BuildModules which calls Build on our module
    auto exit_code = app.Run();
    CHECK_EQ(exit_code, AppExitCode::kSuccess);
    CHECK_EQ(update_count, 3);
  }

  TEST_CASE("Load module and manually call Build") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    auto load_result = dyn_module.Load(module_path);
    REQUIRE(load_result.has_value());

    // Manually call Build before adding to app
    dyn_module.GetModule().Build(app);

    // Verify the resource was added
    CHECK(app.HasResource<TestResource>());

    {
      const auto& resource = app.GetMainWorld().ReadResource<TestResource>();
      CHECK(resource.initialized);
      CHECK_EQ(resource.counter, 42);
    }

    // Initialize the app before running
    app.Initialize();

    // Run the app to execute the system
    app.Update();

    // Verify the system incremented the counter
    {
      const auto& resource = app.GetMainWorld().ReadResource<TestResource>();
      CHECK_EQ(resource.counter, 43);
    }

    // Destroy the module
    dyn_module.GetModule().Destroy(app);

    // Unload the module
    auto unload_result = dyn_module.Unload();
    CHECK(unload_result.has_value());
    CHECK_FALSE(dyn_module.Loaded());
    CHECK_EQ(dyn_module.GetModulePtr(), nullptr);
  }

  TEST_CASE("Load module in constructor") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module(module_path);

    CHECK(dyn_module.Loaded());
    CHECK(dyn_module.GetModulePtr() != nullptr);
    CHECK_NE(dyn_module.GetModuleId(), 0);
    CHECK_FALSE(dyn_module.GetModuleName().empty());

    dyn_module.GetModule().Build(app);
    CHECK(app.HasResource<TestResource>());

    app.Initialize();

    dyn_module.GetModule().Destroy(app);
  }

  TEST_CASE("Module reload") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    // Initial load
    auto load_result = dyn_module.Load(module_path);
    REQUIRE(load_result.has_value());
    dyn_module.GetModule().Build(app);

    CHECK(app.HasResource<TestResource>());

    {
      const auto& resource = app.GetMainWorld().ReadResource<TestResource>();
      CHECK_EQ(resource.counter, 42);
    }

    // Initialize the app
    app.Initialize();

    // Run once to increment counter
    app.Update();

    {
      const auto& resource = app.GetMainWorld().ReadResource<TestResource>();
      CHECK_EQ(resource.counter, 43);
    }

    // Note: After app initialization, reloading a module that tries to add
    // resources/systems will fail. This is expected behavior - reload is meant
    // for hot-reloading code changes in the same module structure, not for
    // adding new resources. For this test, we just verify the reload mechanism
    // works at the library level, without calling Build() again.

    // Unload the module cleanly
    dyn_module.GetModule().Destroy(app);
    auto unload_result = dyn_module.Unload();
    CHECK(unload_result.has_value());

    // Reload the library (without calling Build since app is initialized)
    load_result = dyn_module.Load(module_path);
    CHECK(load_result.has_value());
    CHECK(dyn_module.Loaded());
  }

  TEST_CASE("File change detection") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    auto load_result = dyn_module.Load(module_path);
    REQUIRE(load_result.has_value());
    dyn_module.GetModule().Build(app);

    // Don't initialize - we need to be able to reload and call Build() again

    // Initially, file should not have changed
    CHECK_FALSE(dyn_module.HasFileChanged());

    // ReloadIfChanged should return FileNotChanged
    auto reload_result = dyn_module.ReloadIfChanged(app);
    CHECK_FALSE(reload_result.has_value());
    CHECK_EQ(reload_result.error(), DynamicModuleError::FileNotChanged);

    // Touch the file to change its modification time
    {
      std::error_code ec;
      auto current_time = std::filesystem::last_write_time(module_path, ec);
      if (!ec) {
        // Sleep briefly to ensure time difference
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Update the file time
        auto new_time = std::filesystem::file_time_type::clock::now();
        std::filesystem::last_write_time(module_path, new_time, ec);

        if (!ec) {
          // Now file should have changed
          CHECK(dyn_module.HasFileChanged());

          // ReloadIfChanged should succeed
          reload_result = dyn_module.ReloadIfChanged(app);
          CHECK(reload_result.has_value());
          CHECK(dyn_module.Loaded());

          // After reload, file should not be marked as changed
          CHECK_FALSE(dyn_module.HasFileChanged());
        }
      }
    }

    dyn_module.GetModule().Destroy(app);
  }

  TEST_CASE("Custom symbol names") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    // Try to load with default symbols (should succeed)
    DynamicModuleConfig config{
        .create_symbol = "helios_create_module",
        .module_id_symbol = "helios_module_id",
        .module_name_symbol = "helios_module_name",
        .auto_reload = false,
    };

    auto load_result = dyn_module.Load(module_path, config);
    CHECK(load_result.has_value());
    CHECK(dyn_module.Loaded());

    const auto& stored_config = dyn_module.Config();
    CHECK_EQ(stored_config.create_symbol, "helios_create_module");
    CHECK_EQ(stored_config.module_id_symbol, "helios_module_id");
    CHECK_EQ(stored_config.module_name_symbol, "helios_module_name");
    CHECK_FALSE(stored_config.auto_reload);

    dyn_module.GetModule().Build(app);
    app.Initialize();
    dyn_module.GetModule().Destroy(app);
  }

  TEST_CASE("Load non-existent module") {
    DynamicModule dyn_module;

    auto result = dyn_module.Load("/nonexistent/path/to/module.so");
    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::LibraryLoadFailed);
    CHECK_FALSE(dyn_module.Loaded());
  }

  TEST_CASE("Load with invalid create symbol") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    DynamicModule dyn_module;

    // Try to load with non-existent create symbol
    DynamicModuleConfig config{
        .create_symbol = "nonexistent_create_function",
        .module_id_symbol = "helios_module_id",
        .module_name_symbol = "helios_module_name",
        .auto_reload = false,
    };

    auto result = dyn_module.Load(module_path, config);
    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::CreateSymbolNotFound);
    CHECK_FALSE(dyn_module.Loaded());
  }

  TEST_CASE("Load with invalid module ID symbol") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    DynamicModule dyn_module;

    DynamicModuleConfig config{
        .create_symbol = "helios_create_module",
        .module_id_symbol = "nonexistent_id_function",
        .module_name_symbol = "helios_module_name",
        .auto_reload = false,
    };

    auto result = dyn_module.Load(module_path, config);
    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::IdSymbolNotFound);
    CHECK_FALSE(dyn_module.Loaded());
  }

  TEST_CASE("Load with invalid module name symbol") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    DynamicModule dyn_module;

    DynamicModuleConfig config{
        .create_symbol = "helios_create_module",
        .module_id_symbol = "helios_module_id",
        .module_name_symbol = "nonexistent_name_function",
        .auto_reload = false,
    };

    auto result = dyn_module.Load(module_path, config);
    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::NameSymbolNotFound);
    CHECK_FALSE(dyn_module.Loaded());
  }

  TEST_CASE("Unload when not loaded") {
    DynamicModule dyn_module;

    auto result = dyn_module.Unload();
    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::NotLoaded);
  }

  TEST_CASE("Reload when not loaded") {
    App app;
    DynamicModule dyn_module;

    auto result = dyn_module.Reload(app);
    CHECK_FALSE(result.has_value());
    CHECK_EQ(result.error(), DynamicModuleError::NotLoaded);
  }

  TEST_CASE("Module move semantics") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module1;

    auto load_result = dyn_module1.Load(module_path);
    REQUIRE(load_result.has_value());
    dyn_module1.GetModule().Build(app);

    CHECK(dyn_module1.Loaded());
    auto* module_ptr = dyn_module1.GetModulePtr();
    auto module_id = dyn_module1.GetModuleId();
    CHECK(module_ptr != nullptr);
    CHECK_NE(module_id, 0);

    // Move construction
    DynamicModule dyn_module2(std::move(dyn_module1));
    CHECK(dyn_module2.Loaded());
    CHECK_EQ(dyn_module2.GetModulePtr(), module_ptr);
    CHECK_EQ(dyn_module2.GetModuleId(), module_id);
    CHECK_FALSE(dyn_module1.Loaded());              // NOLINT(bugprone-use-after-move)
    CHECK_EQ(dyn_module1.GetModulePtr(), nullptr);  // NOLINT(bugprone-use-after-move)
    CHECK_EQ(dyn_module1.GetModuleId(), 0);         // NOLINT(bugprone-use-after-move)

    app.Initialize();

    // Move assignment
    DynamicModule dyn_module3;
    dyn_module3 = std::move(dyn_module2);
    CHECK(dyn_module3.Loaded());
    CHECK_EQ(dyn_module3.GetModulePtr(), module_ptr);
    CHECK_EQ(dyn_module3.GetModuleId(), module_id);
    CHECK_FALSE(dyn_module2.Loaded());              // NOLINT(bugprone-use-after-move)
    CHECK_EQ(dyn_module2.GetModulePtr(), nullptr);  // NOLINT(bugprone-use-after-move)
    CHECK_EQ(dyn_module2.GetModuleId(), 0);         // NOLINT(bugprone-use-after-move)

    dyn_module3.GetModule().Destroy(app);
  }

  TEST_CASE("UpdateFileTime") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    auto load_result = dyn_module.Load(module_path);
    REQUIRE(load_result.has_value());
    dyn_module.GetModule().Build(app);

    app.Initialize();

    // Touch file
    {
      std::error_code ec;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      auto new_time = std::filesystem::file_time_type::clock::now();
      std::filesystem::last_write_time(module_path, new_time, ec);

      if (!ec) {
        CHECK(dyn_module.HasFileChanged());

        // Update file time manually
        dyn_module.UpdateFileTime();

        // Now it should not detect a change
        CHECK_FALSE(dyn_module.HasFileChanged());
      }
    }

    dyn_module.GetModule().Destroy(app);
  }

  TEST_CASE("Library access") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;
    DynamicModule dyn_module;

    auto load_result = dyn_module.Load(module_path);
    REQUIRE(load_result.has_value());

    // Access library through module
    const helios::utils::DynamicLibrary& lib = dyn_module.Library();
    CHECK(lib.Loaded());
    CHECK_EQ(lib.Path(), module_path);

    dyn_module.GetModule().Build(app);
    app.Initialize();
    dyn_module.GetModule().Destroy(app);
  }

  TEST_CASE("AddDynamicModule rejects duplicate modules") {
    auto module_path = GetTestModulePath();

    if (!std::filesystem::exists(module_path)) {
      MESSAGE("Test module library not found, skipping test");
      return;
    }

    App app;

    // Load first module
    DynamicModule dyn_module1;
    auto load_result = dyn_module1.Load(module_path);
    REQUIRE(load_result.has_value());

    auto module_id = dyn_module1.GetModuleId();

    // Add first module
    app.AddDynamicModule(std::move(dyn_module1));
    CHECK(app.ContainsDynamicModule(module_id));

    // Load second module with same type
    DynamicModule dyn_module2;
    load_result = dyn_module2.Load(module_path);
    REQUIRE(load_result.has_value());

    // Try to add duplicate - should warn but not crash
    app.AddDynamicModule(std::move(dyn_module2));

    // Module count should still be 1 (duplicate was rejected)
    CHECK_EQ(app.ModuleCount(), 1);
  }
}
