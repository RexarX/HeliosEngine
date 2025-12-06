#include <doctest/doctest.h>

#include <helios/core/logger.hpp>

#include <source_location>
#include <string>

// Test logger types
struct TestLogger {
  static constexpr std::string_view Name() noexcept { return "test_logger"; }
};

struct TestLoggerWithConfig {
  static constexpr std::string_view Name() noexcept { return "test_logger_with_config"; }
  static helios::LoggerConfig Config() noexcept { return helios::LoggerConfig::ConsoleOnly(); }
};

TEST_SUITE("helios::Logger") {
  TEST_CASE("Logger::GetInstance: Default logger basic usage") {
    [[maybe_unused]] auto& logger = helios::Logger::GetInstance();

    CHECK_NOTHROW(HELIOS_TRACE("Trace message"));
    CHECK_NOTHROW(HELIOS_DEBUG("Debug message"));
    CHECK_NOTHROW(HELIOS_INFO("Info message"));
    CHECK_NOTHROW(HELIOS_WARN("Warn message"));
    CHECK_NOTHROW(HELIOS_ERROR("Error message"));
    CHECK_NOTHROW(HELIOS_CRITICAL("Critical message"));

    // Formatted logging
    CHECK_NOTHROW(HELIOS_INFO("Formatted {}: {}", "number", 42));
  }

  TEST_CASE("Logger::AddLogger: Typed logger and level control") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger test_logger{};

    auto config = helios::LoggerConfig::ConsoleOnly();
    CHECK_NOTHROW(logger.AddLogger(test_logger, config));

#if defined(HELIOS_RELEASE_MODE)
    // In Release mode, console-only logger might not be created
    if (logger.HasLogger(test_logger)) {
      CHECK_NOTHROW(logger.SetLevel(test_logger, helios::LogLevel::kWarn));
    }
#else
    CHECK(logger.HasLogger(test_logger));
    CHECK_NOTHROW(logger.SetLevel(test_logger, helios::LogLevel::kWarn));

    CHECK_NOTHROW(HELIOS_TRACE_LOGGER(test_logger, "Trace message"));
    CHECK_NOTHROW(HELIOS_DEBUG_LOGGER(test_logger, "Debug message"));
    CHECK_NOTHROW(HELIOS_INFO_LOGGER(test_logger, "Info message"));
    CHECK_NOTHROW(HELIOS_WARN_LOGGER(test_logger, "Warn message"));
    CHECK_NOTHROW(HELIOS_ERROR_LOGGER(test_logger, "Error message"));
    CHECK_NOTHROW(HELIOS_CRITICAL_LOGGER(test_logger, "Critical message"));
#endif
  }

  TEST_CASE("Logger::SetLevel: Flush and level setters") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger test_logger{};

    auto config = helios::LoggerConfig::Default();
    CHECK_NOTHROW(logger.AddLogger(test_logger, config));

    CHECK_NOTHROW(logger.FlushAll());
    CHECK_NOTHROW(logger.Flush(test_logger));
    CHECK_NOTHROW(logger.SetLevel(helios::LogLevel::kDebug));
    CHECK_NOTHROW(logger.SetLevel(test_logger, helios::LogLevel::kError));

    // Test level getters
    CHECK_EQ(logger.GetLevel(), helios::LogLevel::kDebug);
    if (logger.HasLogger(test_logger)) {
      CHECK_EQ(logger.GetLevel(test_logger), helios::LogLevel::kError);
    }
  }

  TEST_CASE("LoggerConfig::ConsoleOnly: Configuration options") {
    auto config = helios::LoggerConfig::ConsoleOnly();
    CHECK(config.enable_console);
    CHECK_FALSE(config.enable_file);
  }

  TEST_CASE("LoggerConfig::FileOnly: Configuration options") {
    auto config = helios::LoggerConfig::FileOnly();
    CHECK_FALSE(config.enable_console);
    CHECK(config.enable_file);
  }

  TEST_CASE("LoggerConfig::Release: Configuration options") {
    auto config = helios::LoggerConfig::Release();
    CHECK_FALSE(config.enable_console);
    CHECK(config.enable_file);
    CHECK(config.async_logging);
  }

  TEST_CASE("LoggerConfig::Debug: Configuration options") {
    auto config = helios::LoggerConfig::Debug();
    CHECK(config.enable_console);
    CHECK(config.enable_file);
    CHECK_FALSE(config.async_logging);
  }

  TEST_CASE("LoggerConfig: Custom configuration") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger custom_logger{};
    helios::LoggerConfig config;
    config.log_directory = "CustomLogs";
    config.file_name_pattern = "custom_{name}_{timestamp}.log";
    config.enable_console = false;
    config.enable_file = true;

    CHECK_NOTHROW(logger.AddLogger(custom_logger, config));
  }

  TEST_CASE("Logger::LogAssertionFailure: Direct function calls") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger test_logger{};

    auto config = helios::LoggerConfig::ConsoleOnly();
    CHECK_NOTHROW(logger.AddLogger(test_logger, config));

    // Typed logger, string message
    CHECK_NOTHROW(logger.LogAssertionFailure(test_logger, "x > 0", std::source_location::current(),
                                             "x was not greater than zero"));

    // Typed logger, formatted message
    CHECK_NOTHROW(logger.LogAssertionFailure(test_logger, "y == 42", std::source_location::current(),
                                             "y was {}, expected {}", 41, 42));

    // Default logger, string message
    CHECK_NOTHROW(logger.LogAssertionFailure("z != nullptr", std::source_location::current(), "z was nullptr"));

    // Default logger, formatted message
    CHECK_NOTHROW(logger.LogAssertionFailure("ptr != nullptr", std::source_location::current(), "ptr was at address {}",
                                             static_cast<void*>(nullptr)));
  }

  TEST_CASE("Logger::ShouldLog: Level checks") {
    auto& logger = helios::Logger::GetInstance();

    // Set default logger to warn level
    logger.SetLevel(helios::LogLevel::kWarn);

    CHECK_FALSE(logger.ShouldLog(helios::LogLevel::kTrace));
    CHECK_FALSE(logger.ShouldLog(helios::LogLevel::kDebug));
    CHECK_FALSE(logger.ShouldLog(helios::LogLevel::kInfo));
    CHECK(logger.ShouldLog(helios::LogLevel::kWarn));
    CHECK(logger.ShouldLog(helios::LogLevel::kError));
    CHECK(logger.ShouldLog(helios::LogLevel::kCritical));

    // Reset to trace
    logger.SetLevel(helios::LogLevel::kTrace);
  }

  TEST_CASE("Logger::RemoveLogger") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger temp_logger{};

    auto config = helios::LoggerConfig::ConsoleOnly();
    logger.AddLogger(temp_logger, config);

    if (logger.HasLogger(temp_logger)) {
      CHECK(logger.HasLogger(temp_logger));
      logger.RemoveLogger(temp_logger);
      CHECK_FALSE(logger.HasLogger(temp_logger));
    }

    // Cannot remove default logger
    CHECK(logger.HasLogger(helios::kDefaultLogger));
    logger.RemoveLogger(helios::kDefaultLogger);
    CHECK(logger.HasLogger(helios::kDefaultLogger));
  }

  TEST_CASE("Logger::SetDefaultConfig: Config management") {
    auto& logger = helios::Logger::GetInstance();

    // Get current default config
    const auto& default_config = logger.GetDefaultConfig();
    CHECK_FALSE(default_config.log_directory.empty());

    // Set new default config
    helios::LoggerConfig new_config;
    new_config.log_directory = "NewDefaultLogs";
    new_config.file_name_pattern = "new_{name}_{timestamp}.log";

    logger.SetDefaultConfig(new_config);
    const auto& updated_config = logger.GetDefaultConfig();
    CHECK_EQ(updated_config.log_directory, "NewDefaultLogs");
    CHECK_EQ(updated_config.file_name_pattern, "new_{name}_{timestamp}.log");
  }

  TEST_CASE("LoggerTrait: Concept") {
    CHECK(helios::LoggerTrait<TestLogger>);
    CHECK(helios::LoggerTrait<TestLoggerWithConfig>);
    CHECK(helios::LoggerTrait<helios::DefaultLogger>);
  }

  TEST_CASE("LoggerWithConfigTrait: Concept") {
    CHECK_FALSE(helios::LoggerWithConfigTrait<TestLogger>);
    CHECK(helios::LoggerWithConfigTrait<TestLoggerWithConfig>);
    CHECK(helios::LoggerWithConfigTrait<helios::DefaultLogger>);
  }

  TEST_CASE("LoggerIdOf: Unique IDs") {
    constexpr auto id1 = helios::LoggerIdOf<TestLogger>();
    constexpr auto id2 = helios::LoggerIdOf<TestLoggerWithConfig>();
    constexpr auto id3 = helios::LoggerIdOf<helios::DefaultLogger>();

    // IDs should be unique
    CHECK_NE(id1, id2);
    CHECK_NE(id1, id3);
    CHECK_NE(id2, id3);
  }

  TEST_CASE("LoggerNameOf: Correct names") {
    constexpr auto name1 = helios::LoggerNameOf<TestLogger>();
    constexpr auto name2 = helios::LoggerNameOf<TestLoggerWithConfig>();
    constexpr auto name3 = helios::LoggerNameOf<helios::DefaultLogger>();

    CHECK_EQ(name1, "test_logger");
    CHECK_EQ(name2, "test_logger_with_config");
    CHECK_EQ(name3, "HELIOS");
  }

  TEST_CASE("LoggerConfigOf: Returns correct config") {
    auto config1 = helios::LoggerConfigOf<TestLogger>();
    auto config2 = helios::LoggerConfigOf<TestLoggerWithConfig>();

    // TestLogger doesn't have Config(), so it should return Default()
    CHECK(config1.enable_console);
    CHECK(config1.enable_file);

    // TestLoggerWithConfig has ConsoleOnly config
    CHECK(config2.enable_console);
    CHECK_FALSE(config2.enable_file);
  }

  TEST_CASE("Logger::AddLogger: Custom source location level") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger test_logger{};
    helios::LoggerConfig config;
    config.enable_console = true;
    config.enable_file = false;
    config.source_location_level = helios::LogLevel::kWarn;  // Only show source location for warnings and above

    CHECK_NOTHROW(logger.AddLogger(test_logger, config));

    if (logger.HasLogger(test_logger)) {
      // These should not include source location (below kWarn)
      CHECK_NOTHROW(HELIOS_INFO_LOGGER(test_logger, "Info without source location"));
      CHECK_NOTHROW(HELIOS_DEBUG_LOGGER(test_logger, "Debug without source location"));

      // These should include source location (kWarn and above)
      CHECK_NOTHROW(HELIOS_WARN_LOGGER(test_logger, "Warn with source location"));
      CHECK_NOTHROW(HELIOS_ERROR_LOGGER(test_logger, "Error with source location"));
    }
  }

  TEST_CASE("Logger::AddLogger: Custom stack trace level") {
    auto& logger = helios::Logger::GetInstance();
    constexpr TestLogger test_logger{};
    helios::LoggerConfig config;
    config.enable_console = true;
    config.enable_file = false;
    config.stack_trace_level = helios::LogLevel::kError;  // Show stack trace for errors and critical

    CHECK_NOTHROW(logger.AddLogger(test_logger, config));

    if (logger.HasLogger(test_logger)) {
      // This should not include stack trace
      CHECK_NOTHROW(HELIOS_WARN_LOGGER(test_logger, "Warn without stack trace"));

      // These should include stack trace
      CHECK_NOTHROW(HELIOS_ERROR_LOGGER(test_logger, "Error with stack trace"));
      CHECK_NOTHROW(HELIOS_CRITICAL_LOGGER(test_logger, "Critical with stack trace"));
    }
  }

  TEST_CASE("kDefaultLogger: Constexpr") {
    constexpr auto default_logger = helios::kDefaultLogger;
    constexpr auto name = helios::LoggerNameOf<helios::DefaultLogger>();

    CHECK_EQ(name, "HELIOS");

    // Can use in constexpr contexts
    [[maybe_unused]] constexpr auto id = helios::LoggerIdOf<decltype(default_logger)>();
  }
}  // TEST_SUITE
