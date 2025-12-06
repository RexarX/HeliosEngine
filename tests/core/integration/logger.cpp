#include <doctest/doctest.h>

#include <helios/core/logger.hpp>

// Test logger types for integration testing
struct IntegrationTestLogger1 {
  static constexpr std::string_view Name() noexcept { return "integration_test1"; }
};

struct IntegrationTestLogger2 {
  static constexpr std::string_view Name() noexcept { return "integration_test2"; }
};

struct ConsoleOnlyLogger {
  static constexpr std::string_view Name() noexcept { return "console_only"; }
  static helios::LoggerConfig Config() noexcept { return helios::LoggerConfig::ConsoleOnly(); }
};

struct FileOnlyLogger {
  static constexpr std::string_view Name() noexcept { return "file_only"; }
  static helios::LoggerConfig Config() noexcept { return helios::LoggerConfig::FileOnly(); }
};

TEST_SUITE("helios::LoggerIntegration") {
  TEST_CASE("Logger Integration: Multiple loggers interaction") {
    auto& logger = helios::Logger::GetInstance();

    // Test multiple loggers working together with new config API
    constexpr IntegrationTestLogger1 test1{};
    constexpr IntegrationTestLogger2 test2{};

    auto config1 = helios::LoggerConfig::ConsoleOnly();
    auto config2 = helios::LoggerConfig::ConsoleOnly();

    logger.AddLogger(test1, config1);
    logger.AddLogger(test2, config2);

    CHECK_NOTHROW(HELIOS_INFO_LOGGER(test1, "Message from test1"));
    CHECK_NOTHROW(HELIOS_INFO_LOGGER(test2, "Message from test2"));
  }

  TEST_CASE("Logger Integration: Configuration variants") {
    auto& logger = helios::Logger::GetInstance();

    SUBCASE("Console and File logger") {
      constexpr IntegrationTestLogger1 test_logger{};
      auto config = helios::LoggerConfig::Default();
      logger.AddLogger(test_logger, config);

      if (logger.HasLogger(test_logger)) {
        CHECK_NOTHROW(HELIOS_INFO_LOGGER(test_logger, "Testing console and file output"));
      }
    }

    SUBCASE("File only logger") {
      constexpr FileOnlyLogger file_logger{};
      helios::LoggerConfig config = helios::LoggerConfigOf<FileOnlyLogger>();
      config.log_directory = "TestLogs";
      logger.AddLogger(file_logger, config);

      if (logger.HasLogger(file_logger)) {
        CHECK_NOTHROW(HELIOS_INFO_LOGGER(file_logger, "Testing file-only output"));
      }
    }

    SUBCASE("Custom log directory") {
      constexpr IntegrationTestLogger1 custom_logger{};
      helios::LoggerConfig config;
      config.log_directory = "CustomTestLogs";
      config.file_name_pattern = "integration_{name}_{timestamp}.log";
      config.enable_console = true;
      config.enable_file = true;

      logger.AddLogger(custom_logger, config);

      if (logger.HasLogger(custom_logger)) {
        CHECK_NOTHROW(HELIOS_INFO_LOGGER(custom_logger, "Testing custom directory"));
      }
    }
  }

  TEST_CASE("Logger Integration: Level control across multiple loggers") {
    auto& logger = helios::Logger::GetInstance();

    constexpr IntegrationTestLogger1 test1{};
    constexpr IntegrationTestLogger2 test2{};

    auto config = helios::LoggerConfig::ConsoleOnly();
    logger.AddLogger(test1, config);
    logger.AddLogger(test2, config);

    if (logger.HasLogger(test1) && logger.HasLogger(test2)) {
      // Set different levels for different loggers
      logger.SetLevel(test1, helios::LogLevel::kWarn);
      logger.SetLevel(test2, helios::LogLevel::kTrace);

      // Verify levels
      CHECK_EQ(logger.GetLevel(test1), helios::LogLevel::kWarn);
      CHECK_EQ(logger.GetLevel(test2), helios::LogLevel::kTrace);

      // Test logging at different levels
      CHECK_NOTHROW(HELIOS_TRACE_LOGGER(test2, "This should appear for test2"));
      CHECK_NOTHROW(HELIOS_WARN_LOGGER(test1, "This should appear for test1"));
    }
  }

  TEST_CASE("Logger Integration: Flush operations") {
    auto& logger = helios::Logger::GetInstance();

    constexpr IntegrationTestLogger1 test1{};
    constexpr IntegrationTestLogger2 test2{};

    auto config = helios::LoggerConfig::Default();
    logger.AddLogger(test1, config);
    logger.AddLogger(test2, config);

    // Write some messages
    CHECK_NOTHROW(HELIOS_INFO_LOGGER(test1, "Message 1"));
    CHECK_NOTHROW(HELIOS_INFO_LOGGER(test2, "Message 2"));

    // Test individual flush
    CHECK_NOTHROW(logger.Flush(test1));
    CHECK_NOTHROW(logger.Flush(test2));

    // Test flush all
    CHECK_NOTHROW(logger.FlushAll());
  }

  TEST_CASE("Logger Integration: Assertion logging") {
    auto& logger = helios::Logger::GetInstance();

    constexpr IntegrationTestLogger1 test_logger{};
    auto config = helios::LoggerConfig::ConsoleOnly();
    logger.AddLogger(test_logger, config);

    // Test assertion logging through logger API
    CHECK_NOTHROW(
        logger.LogAssertionFailure(test_logger, "x > 0", std::source_location::current(), "Value was negative"));

    CHECK_NOTHROW(logger.LogAssertionFailure(test_logger, "ptr != nullptr", std::source_location::current(),
                                             "Pointer was null at address {}", static_cast<void*>(nullptr)));
  }

  TEST_CASE("Logger Integration: Default configuration management") {
    auto& logger = helios::Logger::GetInstance();

    // Get current default config
    const auto& current_default = logger.GetDefaultConfig();
    CHECK_FALSE(current_default.log_directory.empty());

    // Create custom default config
    helios::LoggerConfig new_default;
    new_default.log_directory = "IntegrationTestLogs";
    new_default.file_name_pattern = "test_{name}_{timestamp}.log";
    new_default.enable_console = true;
    new_default.enable_file = true;

    // Set as new default
    logger.SetDefaultConfig(new_default);

    // Verify it was set
    const auto& updated_default = logger.GetDefaultConfig();
    CHECK_EQ(updated_default.log_directory, "IntegrationTestLogs");
    CHECK_EQ(updated_default.file_name_pattern, "test_{name}_{timestamp}.log");

    // New loggers should use this config by default
    constexpr IntegrationTestLogger1 uses_default{};
    logger.AddLogger(uses_default);

    if (logger.HasLogger(uses_default)) {
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(uses_default, "Using default configuration"));
    }
  }

  TEST_CASE("Logger Integration: Configurable source location across loggers") {
    auto& logger = helios::Logger::GetInstance();

    constexpr IntegrationTestLogger1 logger_with_source_loc{};
    constexpr IntegrationTestLogger2 logger_without_source_loc{};

    // Logger 1: source location from Info level
    helios::LoggerConfig config1;
    config1.enable_console = true;
    config1.enable_file = false;
    config1.source_location_level = helios::LogLevel::kInfo;

    // Logger 2: source location only from Critical level
    helios::LoggerConfig config2;
    config2.enable_console = true;
    config2.enable_file = false;
    config2.source_location_level = helios::LogLevel::kCritical;

    logger.AddLogger(logger_with_source_loc, config1);
    logger.AddLogger(logger_without_source_loc, config2);

    if (logger.HasLogger(logger_with_source_loc) && logger.HasLogger(logger_without_source_loc)) {
      // Logger 1 should include source location for info and above
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(logger_with_source_loc, "Info with location"));
      CHECK_NOTHROW(HELIOS_ERROR_LOGGER(logger_with_source_loc, "Error with location"));

      // Logger 2 should not include source location for these levels
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(logger_without_source_loc, "Info without location"));
      CHECK_NOTHROW(HELIOS_ERROR_LOGGER(logger_without_source_loc, "Error without location"));

      // Logger 2 should include source location only for critical
      CHECK_NOTHROW(HELIOS_CRITICAL_LOGGER(logger_without_source_loc, "Critical with location"));
    }
  }

  TEST_CASE("Logger Integration: Logger with predefined config") {
    auto& logger = helios::Logger::GetInstance();

    // Test logger types with built-in configuration
    constexpr ConsoleOnlyLogger console_logger{};
    constexpr FileOnlyLogger file_logger{};

    // These should use their predefined configs
    logger.AddLogger(console_logger);  // Uses ConsoleOnly config
    logger.AddLogger(file_logger);     // Uses FileOnly config

    if (logger.HasLogger(console_logger)) {
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(console_logger, "Console-only logger message"));
    }

    if (logger.HasLogger(file_logger)) {
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(file_logger, "File-only logger message"));
    }
  }

  TEST_CASE("Logger Integration: ShouldLog checks with multiple loggers") {
    auto& logger = helios::Logger::GetInstance();

    constexpr IntegrationTestLogger1 restrictive_logger{};
    constexpr IntegrationTestLogger2 permissive_logger{};

    logger.AddLogger(restrictive_logger, helios::LoggerConfig::ConsoleOnly());
    logger.AddLogger(permissive_logger, helios::LoggerConfig::ConsoleOnly());

    // Set different levels
    logger.SetLevel(restrictive_logger, helios::LogLevel::kError);
    logger.SetLevel(permissive_logger, helios::LogLevel::kTrace);

    if (logger.HasLogger(restrictive_logger) && logger.HasLogger(permissive_logger)) {
      // Restrictive logger
      CHECK_FALSE(logger.ShouldLog(restrictive_logger, helios::LogLevel::kTrace));
      CHECK_FALSE(logger.ShouldLog(restrictive_logger, helios::LogLevel::kDebug));
      CHECK_FALSE(logger.ShouldLog(restrictive_logger, helios::LogLevel::kInfo));
      CHECK_FALSE(logger.ShouldLog(restrictive_logger, helios::LogLevel::kWarn));
      CHECK(logger.ShouldLog(restrictive_logger, helios::LogLevel::kError));
      CHECK(logger.ShouldLog(restrictive_logger, helios::LogLevel::kCritical));

      // Permissive logger
      CHECK(logger.ShouldLog(permissive_logger, helios::LogLevel::kTrace));
      CHECK(logger.ShouldLog(permissive_logger, helios::LogLevel::kDebug));
      CHECK(logger.ShouldLog(permissive_logger, helios::LogLevel::kInfo));
      CHECK(logger.ShouldLog(permissive_logger, helios::LogLevel::kWarn));
      CHECK(logger.ShouldLog(permissive_logger, helios::LogLevel::kError));
      CHECK(logger.ShouldLog(permissive_logger, helios::LogLevel::kCritical));
    }
  }

  TEST_CASE("Logger Integration: Logger removal and re-addition") {
    auto& logger = helios::Logger::GetInstance();

    constexpr IntegrationTestLogger1 temp_logger{};

    // Add logger
    auto config = helios::LoggerConfig::ConsoleOnly();
    logger.AddLogger(temp_logger, config);

    if (logger.HasLogger(temp_logger)) {
      CHECK(logger.HasLogger(temp_logger));
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(temp_logger, "Message before removal"));

      // Remove logger
      logger.RemoveLogger(temp_logger);
      CHECK_FALSE(logger.HasLogger(temp_logger));

      // Re-add logger with different config
      auto new_config = helios::LoggerConfig::Default();
      logger.AddLogger(temp_logger, new_config);

      if (logger.HasLogger(temp_logger)) {
        CHECK(logger.HasLogger(temp_logger));
        CHECK_NOTHROW(HELIOS_INFO_LOGGER(temp_logger, "Message after re-addition"));
      }
    }
  }
}  // TEST_SUITE
